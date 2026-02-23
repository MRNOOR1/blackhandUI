#include "mp3_service.h"
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <mpg123.h>
/*
 * mp3_service.c
 *
 * Intentionally scaffold-only for learning.
 *
 * Implement next:
 * 1) Local folder scan for *.mp3.
 * 2) In-memory track list.
 * 3) Playback API wrapping your desktop backend now, ALSA later.
 */
static AudioFile *library = NULL;
static size_t track_count = 0;
static size_t capacity = 0;
static playback_state state = STOPPED;
static int current_index = -1;
static pid_t player_pid = -1;
static time_t start_time = 0;
static unsigned paused_offset = 0;
static const char *AUDIO_PATH = "./Audio";

int mp3_service_init(const char *audio_root)
{

	DIR *dir opendir(audio_root);
	if (!dir)
	{
		perror("Could not open notes directory");
		return 1;
	}
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		if (entry->d_name[0] == '.')
		{
			continue;
		}
		const char *ext = strrchr(entry->d_name, '.');
		if (!ext || strcmp(ext, ".mp3") != 0)
		{
			continue;
		}
	}
}

void mp3_service_shutdown(void);
size_t mp3_service_count(void);
const AudioFile *mp3_service_get(size_t index);
int mp3_service_play(size_t index);
void mp3_service_pause(void);
void mp3_service_resume(void);
void mp3_service_stop(void);
playback_state mp3_service_get_state(void);
int mp3_service_get_current_index(void);
unsigned mp3_service_get_elapsed(void);
