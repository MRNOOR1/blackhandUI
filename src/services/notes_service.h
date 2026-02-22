#ifndef NOTES_SERVICE_H
#define NOTES_SERVICE_H
#include <time.h>
#include <stddef.h>
/*
 * notes_service.h
 *
 * This service owns note storage and retrieval.
 * Implementations to add:
 * - Create/read/update/delete notes
 * - Persistent storage (file first, SQLite later)
 */


 typedef struct {
    char *filename;
    char *title;
    char *content;
    char* created_at;

 } Note;
 

void notes_service_init(void);
Note* notes_service_create(const char* title, const char* content);
void notes_service_print(const Note* n);
const Note* notes_service_get_note_by_filename(const char* filename);
int notes_service_delete_note(const Note* n);
int notes_service_update_note(Note* n);
size_t notes_service_note_count(void);
Note** notes_service_list_all(size_t* out_count);
void notes_service_shutdown(void);

#endif
