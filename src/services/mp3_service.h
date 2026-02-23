#ifndef MP3_SERVICE_H
#define MP3_SERVICE_H
#include <stddef.h>

/*
 * mp3_service.h
 *
 * This service owns MP3 domain logic.
 * Implementations to add:
 * - Library scanning
 * - Track metadata list
 * - Playback state control
 * - Progress/time reporting
 */

typedef struct
{
	char *path;		   /* Full file path */
	char *title;	   /* Filename without extension */
	char *author;	   /* Folder level 2 */
	char *genre;	   /* Folder level 1 */
	unsigned duration; /* Duration in seconds */
} AudioFile;

typedef enum
{
	STOPPED,
	PLAYING,
	PAUSED
} playback_state;

int mp3_service_init(const char *audio_root);
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

#endif