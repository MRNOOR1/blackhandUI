#ifndef NOTES_SERVICE_H
#define NOTES_SERVICE_H

/*
 * notes_service.h
 *
 * This service owns note storage and retrieval.
 * Implementations to add:
 * - Create/read/update/delete notes
 * - Persistent storage (file first, SQLite later)
 */

void notes_service_init(void);
void notes_service_shutdown(void);

#endif
