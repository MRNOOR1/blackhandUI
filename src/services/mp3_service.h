#ifndef MP3_SERVICE_H
#define MP3_SERVICE_H

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

void mp3_service_init(void);
void mp3_service_shutdown(void);

#endif
