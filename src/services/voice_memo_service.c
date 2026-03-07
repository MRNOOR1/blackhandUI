#include "voice_memo_service.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define INITIAL_MEMO_CAPACITY 16
#define TICK_STEP_MS 33

static VoiceMemo **memos_index = NULL;
static size_t memos_count = 0;
static size_t memos_capacity = 0;

static VMState current_state = VM_IDLE;
static VoiceMemo *current_memo = NULL;
static int current_elapsed_ms = 0;
static unsigned memo_serial = 0;

static const char *VOICE_MEMO_PATH = "./VoiceMemos";

static int ensure_capacity(void)
{
	if (memos_count < memos_capacity)
		return 0;
	size_t new_capacity = (memos_capacity == 0) ? INITIAL_MEMO_CAPACITY : memos_capacity * 2;
	VoiceMemo **new_array = realloc(memos_index, new_capacity * sizeof(VoiceMemo *));
	if (!new_array)
		return -1;
	memos_index = new_array;
	memos_capacity = new_capacity;
	return 0;
}

static void insert_at_front(VoiceMemo *memo)
{
	for (size_t i = memos_count; i > 0; i--)
	{
		memos_index[i] = memos_index[i - 1];
	}
	memos_index[0] = memo;
	memos_count++;
}

static char *make_timestamp_filename(void)
{
	time_t now = time(NULL);
	struct tm tmv;
	localtime_r(&now, &tmv);

	char name[64];
	strftime(name, sizeof(name), "memo_%Y%m%d_%H%M%S", &tmv);
	char final_name[80];
	snprintf(final_name, sizeof(final_name), "%s_%u.vmemo", name, memo_serial++);
	return strdup(final_name);
}

static int write_memo_file(const VoiceMemo *memo)
{
	if (!memo || !memo->filename)
		return -1;

	char path[1024];
	snprintf(path, sizeof(path), "%s/%s", VOICE_MEMO_PATH, memo->filename);
	FILE *f = fopen(path, "w");
	if (!f)
		return -1;

	fprintf(f, "duration_ms=%d\n", memo->duration_ms);
	fclose(f);
	return 0;
}

void voice_memo_service_init(void)
{
	memos_index = malloc(INITIAL_MEMO_CAPACITY * sizeof(VoiceMemo *));
	if (!memos_index)
		return;

	memos_count = 0;
	memos_capacity = INITIAL_MEMO_CAPACITY;
	current_state = VM_IDLE;
	current_memo = NULL;
	current_elapsed_ms = 0;

	struct stat st = {0};
	if (stat(VOICE_MEMO_PATH, &st) == -1)
	{
		if (mkdir(VOICE_MEMO_PATH, 0755) != 0)
		{
			free(memos_index);
			memos_index = NULL;
			memos_capacity = 0;
			memos_count = 0;
			return;
		}
	}

	DIR *dir = opendir(VOICE_MEMO_PATH);
	if (!dir)
	{
		free(memos_index);
		memos_index = NULL;
		memos_capacity = 0;
		memos_count = 0;
		return;
	}

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		if (entry->d_name[0] == '.')
			continue;
		const char *ext = strrchr(entry->d_name, '.');
		if (!ext || strcmp(ext, ".vmemo") != 0)
			continue;

		if (ensure_capacity() != 0)
			continue;

		char path[1024];
		snprintf(path, sizeof(path), "%s/%s", VOICE_MEMO_PATH, entry->d_name);
		FILE *f = fopen(path, "r");
		if (!f)
			continue;

		VoiceMemo *memo = malloc(sizeof(VoiceMemo));
		if (!memo)
		{
			fclose(f);
			continue;
		}

		memo->filename = strdup(entry->d_name);
		memo->duration_ms = 0;

		char line[128];
		if (fgets(line, sizeof(line), f))
		{
			int d = 0;
			if (sscanf(line, "duration_ms=%d", &d) == 1)
			{
				memo->duration_ms = d;
			}
		}
		fclose(f);

		if (!memo->filename)
		{
			free(memo);
			continue;
		}
		insert_at_front(memo);
	}

	closedir(dir);
}

VMState voice_memo_service_state(void)
{
	return current_state;
}

const VoiceMemo *voice_memo_service_current(void)
{
	return current_memo;
}

const VoiceMemo **voice_memo_service_list_all(size_t *out_count)
{
	if (out_count)
		*out_count = memos_count;
	return (const VoiceMemo **)memos_index;
}

const VoiceMemo *voice_memo_service_get_by_filename(const char *filename)
{
	if (!filename)
		return NULL;
	for (size_t i = 0; i < memos_count; i++)
	{
		if (strcmp(memos_index[i]->filename, filename) == 0)
		{
			return memos_index[i];
		}
	}
	return NULL;
}

int voice_memo_service_record_start(void)
{
	if (current_state != VM_IDLE)
		return -1;
	current_state = VM_RECORDING;
	current_memo = NULL;
	current_elapsed_ms = 0;
	return 0;
}

int voice_memo_service_record_stop(const char *title_optional)
{
	(void)title_optional;
	if (current_state != VM_RECORDING)
		return -1;
	if (ensure_capacity() != 0)
		return -1;

	VoiceMemo *memo = malloc(sizeof(VoiceMemo));
	if (!memo)
		return -1;

	memo->filename = make_timestamp_filename();
	memo->duration_ms = current_elapsed_ms;
	if (!memo->filename)
	{
		free(memo);
		return -1;
	}

	if (write_memo_file(memo) != 0)
	{
		free(memo->filename);
		free(memo);
		return -1;
	}

	insert_at_front(memo);
	current_state = VM_IDLE;
	current_memo = NULL;
	current_elapsed_ms = 0;
	return 0;
}

int voice_memo_service_play_start(const char *filename)
{
	if (!filename || current_state == VM_RECORDING)
		return -1;
	const VoiceMemo *found = voice_memo_service_get_by_filename(filename);
	if (!found)
		return -1;

	current_memo = (VoiceMemo *)found;
	current_elapsed_ms = 0;
	current_state = VM_PLAYING;
	return 0;
}

int voice_memo_service_play_pause(void)
{
	if (current_state != VM_PLAYING)
		return -1;
	current_state = VM_PAUSED;
	return 0;
}

int voice_memo_service_play_resume(void)
{
	if (current_state != VM_PAUSED)
		return -1;
	current_state = VM_PLAYING;
	return 0;
}

int voice_memo_service_play_stop(void)
{
	if (current_state != VM_PLAYING && current_state != VM_PAUSED)
		return -1;
	current_state = VM_IDLE;
	current_memo = NULL;
	current_elapsed_ms = 0;
	return 0;
}

int voice_memo_service_delete(const char *filename)
{
	if (!filename)
		return -1;

	for (size_t i = 0; i < memos_count; i++)
	{
		VoiceMemo *memo = memos_index[i];
		if (strcmp(memo->filename, filename) != 0)
			continue;

		if (current_memo == memo)
		{
			current_state = VM_IDLE;
			current_memo = NULL;
			current_elapsed_ms = 0;
		}

		char path[1024];
		snprintf(path, sizeof(path), "%s/%s", VOICE_MEMO_PATH, memo->filename);
		remove(path);

		free(memo->filename);
		free(memo);

		for (size_t j = i; j + 1 < memos_count; j++)
		{
			memos_index[j] = memos_index[j + 1];
		}
		memos_count--;
		return 0;
	}

	return -1;
}

int voice_memo_service_tick(void)
{
	if (current_state == VM_RECORDING)
	{
		current_elapsed_ms += TICK_STEP_MS;
		return 0;
	}

	if (current_state == VM_PLAYING)
	{
		current_elapsed_ms += TICK_STEP_MS;
		if (current_memo && current_memo->duration_ms > 0 && current_elapsed_ms >= current_memo->duration_ms)
		{
			current_state = VM_IDLE;
			current_memo = NULL;
			current_elapsed_ms = 0;
		}
	}

	return 0;
}

int voice_memo_service_elapsed_ms(void)
{
	return current_elapsed_ms;
}

int voice_memo_service_total_ms(void)
{
	if (!current_memo)
		return 0;
	return current_memo->duration_ms;
}

void voice_memo_service_shutdown(void)
{
	if (!memos_index)
		return;

	for (size_t i = 0; i < memos_count; i++)
	{
		free(memos_index[i]->filename);
		free(memos_index[i]);
	}
	free(memos_index);

	memos_index = NULL;
	memos_count = 0;
	memos_capacity = 0;
	current_state = VM_IDLE;
	current_memo = NULL;
	current_elapsed_ms = 0;
}
