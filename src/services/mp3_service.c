#include "mp3_service.h"

#include <dirent.h>
#include <math.h>
#include <mpg123.h>
#include <out123.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define INITIAL_AUDIO_CAPACITY 16

static AudioFile *library = NULL;
static size_t track_count = 0;
static size_t capacity = 0;

static playback_state state = STOPPED;
static int current_index = -1;
static time_t start_time = 0;
static unsigned pause_offset = 0;

static pthread_t player_thread;
static int thread_running = 0;
static int stop_requested = 0;
static pthread_mutex_t mp3_lock = PTHREAD_MUTEX_INITIALIZER;

static float viz_levels[MP3_VIZ_BINS] = {0};

static int ensure_capacity(void) {
    if (track_count < capacity) return 0;
    size_t new_capacity = (capacity == 0) ? INITIAL_AUDIO_CAPACITY : capacity * 2;
    AudioFile *new_library = realloc(library, new_capacity * sizeof(AudioFile));
    if (!new_library) return -1;
    library = new_library;
    capacity = new_capacity;
    return 0;
}

static char *title_from_filename(const char *filename) {
    char *copy = strdup(filename);
    if (!copy) return strdup("Unknown");

    char *dot = strrchr(copy, '.');
    if (dot) *dot = '\0';

    for (char *p = copy; *p; p++) {
        if (*p == '_') *p = ' ';
    }
    return copy;
}

static void clear_visualizer(void) {
    for (int i = 0; i < MP3_VIZ_BINS; i++) viz_levels[i] = 0.0f;
}

static void update_visualizer_from_pcm16(const int16_t *samples, size_t sample_count, int channels) {
    if (!samples || sample_count == 0) return;
    if (channels < 1) channels = 1;

    size_t frames = sample_count / (size_t)channels;
    if (frames == 0) return;

    size_t frames_per_bin = frames / MP3_VIZ_BINS;
    if (frames_per_bin == 0) frames_per_bin = 1;

    for (int b = 0; b < MP3_VIZ_BINS; b++) {
        size_t start = (size_t)b * frames_per_bin;
        if (start >= frames) {
            viz_levels[b] *= 0.85f;
            continue;
        }

        size_t end = start + frames_per_bin;
        if (end > frames) end = frames;

        double sum_sq = 0.0;
        size_t count = 0;
        for (size_t f = start; f < end; f++) {
            int16_t s = samples[f * (size_t)channels];
            double n = (double)s / 32768.0;
            sum_sq += n * n;
            count++;
        }

        float rms = 0.0f;
        if (count > 0) rms = (float)sqrt(sum_sq / (double)count);

        float smoothed = (viz_levels[b] * 0.7f) + (rms * 0.3f);
        if (smoothed > 1.0f) smoothed = 1.0f;
        viz_levels[b] = smoothed;
    }
}

typedef struct {
    char *path;
} player_args_t;

static void *player_thread_fn(void *arg) {
    player_args_t *args = (player_args_t *)arg;
    if (!args || !args->path) {
        free(args);
        return NULL;
    }

    mpg123_handle *mh = NULL;
    out123_handle *ao = NULL;
    unsigned char *buffer = NULL;
    size_t outblock = 0;
    long rate = 0;
    int channels = 0;
    int encoding = 0;

    int err = 0;
    mh = mpg123_new(NULL, &err);
    if (!mh) goto cleanup;

    if (mpg123_open(mh, args->path) != MPG123_OK) goto cleanup;
    if (mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK) goto cleanup;

    ao = out123_new();
    if (!ao) goto cleanup;
    if (out123_open(ao, NULL, NULL) != 0) goto cleanup;
    if (out123_start(ao, rate, channels, encoding) != 0) goto cleanup;

    outblock = mpg123_outblock(mh);
    if (outblock == 0) goto cleanup;
    buffer = malloc(outblock);
    if (!buffer) goto cleanup;

    int paused_locally = 0;
    while (1) {
        pthread_mutex_lock(&mp3_lock);
        int should_stop = stop_requested;
        playback_state st = state;
        pthread_mutex_unlock(&mp3_lock);

        if (should_stop) break;

        if (st == PAUSED) {
            if (!paused_locally) {
                out123_pause(ao);
                paused_locally = 1;
            }
            usleep(20000);
            continue;
        }

        if (paused_locally) {
            out123_continue(ao);
            paused_locally = 0;
        }

        size_t done = 0;
        int r = mpg123_read(mh, buffer, outblock, &done);
        if (r == MPG123_DONE) break;
        if (r != MPG123_OK && r != MPG123_NEW_FORMAT) break;

        if (r == MPG123_NEW_FORMAT) {
            if (mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK) break;
            out123_stop(ao);
            if (out123_start(ao, rate, channels, encoding) != 0) break;
            continue;
        }

        if (done > 0) {
            out123_play(ao, buffer, done);

            pthread_mutex_lock(&mp3_lock);
            if (out123_encsize(encoding) == 2) {
                update_visualizer_from_pcm16((const int16_t *)buffer, done / sizeof(int16_t), channels);
            }
            pthread_mutex_unlock(&mp3_lock);
        }
    }

cleanup:
    if (ao) {
        out123_drop(ao);
        out123_close(ao);
        out123_del(ao);
    }
    if (mh) {
        mpg123_close(mh);
        mpg123_delete(mh);
    }
    free(buffer);

    pthread_mutex_lock(&mp3_lock);
    thread_running = 0;
    if (!stop_requested) {
        state = STOPPED;
        current_index = -1;
        start_time = 0;
        pause_offset = 0;
        clear_visualizer();
    }
    pthread_mutex_unlock(&mp3_lock);

    free(args->path);
    free(args);
    return NULL;
}

int mp3_service_init(const char *audio_root) {
    library = malloc(sizeof(AudioFile) * INITIAL_AUDIO_CAPACITY);
    if (!library) return -1;
    capacity = INITIAL_AUDIO_CAPACITY;
    track_count = 0;

    struct stat st = {0};
    if (stat(audio_root, &st) == -1) {
        if (mkdir(audio_root, 0755) != 0) {
            free(library);
            library = NULL;
            capacity = 0;
            return -1;
        }
    }

    DIR *root_dir = opendir(audio_root);
    if (!root_dir) return -1;

    struct dirent *genre_entry;
    while ((genre_entry = readdir(root_dir)) != NULL) {
        if (genre_entry->d_name[0] == '.') continue;

        char genre_path[1024];
        snprintf(genre_path, sizeof(genre_path), "%s/%s", audio_root, genre_entry->d_name);

        struct stat genre_st;
        if (stat(genre_path, &genre_st) != 0 || !S_ISDIR(genre_st.st_mode)) continue;

        DIR *genre_dir = opendir(genre_path);
        if (!genre_dir) continue;

        struct dirent *author_entry;
        while ((author_entry = readdir(genre_dir)) != NULL) {
            if (author_entry->d_name[0] == '.') continue;

            char author_path[1024];
            snprintf(author_path, sizeof(author_path), "%s/%s", genre_path, author_entry->d_name);

            struct stat author_st;
            if (stat(author_path, &author_st) != 0 || !S_ISDIR(author_st.st_mode)) continue;

            DIR *author_dir = opendir(author_path);
            if (!author_dir) continue;

            struct dirent *file_entry;
            while ((file_entry = readdir(author_dir)) != NULL) {
                if (file_entry->d_name[0] == '.') continue;
                const char *ext = strrchr(file_entry->d_name, '.');
                if (!ext || strcmp(ext, ".mp3") != 0) continue;

                if (ensure_capacity() != 0) continue;

                AudioFile *track = &library[track_count];
                char full_path[1024];
                snprintf(full_path, sizeof(full_path), "%s/%s", author_path, file_entry->d_name);

                track->path = strdup(full_path);
                track->title = title_from_filename(file_entry->d_name);
                track->author = strdup(author_entry->d_name);
                track->genre = strdup(genre_entry->d_name);
                track->duration = 0;

                if (!track->path || !track->title || !track->author || !track->genre) {
                    free(track->path);
                    free(track->title);
                    free(track->author);
                    free(track->genre);
                    continue;
                }

                track_count++;
            }
            closedir(author_dir);
        }
        closedir(genre_dir);
    }
    closedir(root_dir);

    return 0;
}

size_t mp3_service_count(void) {
    return track_count;
}

const AudioFile *mp3_service_get(size_t index) {
    if (index >= track_count) return NULL;
    return &library[index];
}

int mp3_service_play(size_t index) {
    if (index >= track_count) return -1;

    mp3_service_stop();

    player_args_t *args = malloc(sizeof(player_args_t));
    if (!args) return -1;
    args->path = strdup(library[index].path);
    if (!args->path) {
        free(args);
        return -1;
    }

    pthread_mutex_lock(&mp3_lock);
    state = PLAYING;
    current_index = (int)index;
    start_time = time(NULL);
    pause_offset = 0;
    stop_requested = 0;
    clear_visualizer();
    thread_running = 1;
    pthread_mutex_unlock(&mp3_lock);

    if (pthread_create(&player_thread, NULL, player_thread_fn, args) != 0) {
        pthread_mutex_lock(&mp3_lock);
        thread_running = 0;
        state = STOPPED;
        current_index = -1;
        pthread_mutex_unlock(&mp3_lock);
        free(args->path);
        free(args);
        return -1;
    }

    return 0;
}

void mp3_service_pause(void) {
    pthread_mutex_lock(&mp3_lock);
    if (state == PLAYING) {
        pause_offset += (unsigned)(time(NULL) - start_time);
        state = PAUSED;
    }
    pthread_mutex_unlock(&mp3_lock);
}

void mp3_service_resume(void) {
    pthread_mutex_lock(&mp3_lock);
    if (state == PAUSED) {
        start_time = time(NULL);
        state = PLAYING;
    }
    pthread_mutex_unlock(&mp3_lock);
}

void mp3_service_stop(void) {
    pthread_t join_thread;
    int should_join = 0;

    pthread_mutex_lock(&mp3_lock);
    if (thread_running) {
        stop_requested = 1;
        join_thread = player_thread;
        should_join = 1;
    }
    state = STOPPED;
    current_index = -1;
    start_time = 0;
    pause_offset = 0;
    clear_visualizer();
    pthread_mutex_unlock(&mp3_lock);

    if (should_join) {
        pthread_join(join_thread, NULL);
        pthread_mutex_lock(&mp3_lock);
        stop_requested = 0;
        thread_running = 0;
        pthread_mutex_unlock(&mp3_lock);
    }
}

playback_state mp3_service_get_state(void) {
    pthread_mutex_lock(&mp3_lock);
    playback_state s = state;
    pthread_mutex_unlock(&mp3_lock);
    return s;
}

int mp3_service_get_current_index(void) {
    pthread_mutex_lock(&mp3_lock);
    int idx = current_index;
    pthread_mutex_unlock(&mp3_lock);
    return idx;
}

unsigned mp3_service_get_elapsed(void) {
    pthread_mutex_lock(&mp3_lock);
    unsigned elapsed = 0;
    if (state == PLAYING) {
        elapsed = pause_offset + (unsigned)(time(NULL) - start_time);
    } else if (state == PAUSED) {
        elapsed = pause_offset;
    }
    pthread_mutex_unlock(&mp3_lock);
    return elapsed;
}

size_t mp3_service_get_visualizer(unsigned char *out_levels, size_t max_levels) {
    if (!out_levels || max_levels == 0) return 0;

    size_t count = (max_levels < MP3_VIZ_BINS) ? max_levels : MP3_VIZ_BINS;
    pthread_mutex_lock(&mp3_lock);
    for (size_t i = 0; i < count; i++) {
        float v = viz_levels[i];
        if (v < 0.0f) v = 0.0f;
        if (v > 1.0f) v = 1.0f;
        out_levels[i] = (unsigned char)(v * 8.0f);
    }
    pthread_mutex_unlock(&mp3_lock);
    return count;
}

void mp3_service_shutdown(void) {
    mp3_service_stop();

    if (!library) return;
    for (size_t i = 0; i < track_count; i++) {
        free(library[i].author);
        free(library[i].genre);
        free(library[i].path);
        free(library[i].title);
    }
    free(library);
    library = NULL;
    track_count = 0;
    capacity = 0;
}
