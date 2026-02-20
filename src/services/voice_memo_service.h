#ifndef VOICE_MEMO_SERVICE_H
#define VOICE_MEMO_SERVICE_H

/*
 * voice_memo_service.h
 *
 * This service owns voice memo domain logic.
 * Implementations to add:
 * - Start/stop recording
 * - Memo list management
 * - Playback/delete actions
 */

void voice_memo_service_init(void);
void voice_memo_service_shutdown(void);

#endif
