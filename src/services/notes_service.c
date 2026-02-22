#include "notes_service.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#define INITIAL_NOTES_CAPACITY 16
#define MAX_LINE_LENGTH 256

Note **notes_index = NULL;
size_t notes_count = 0;
size_t notes_capacity = 0;
static const char *NOTES_PATH = "./Notes";

void notes_service_init(void)
{
	// Allocate initial array
	notes_index = malloc(INITIAL_NOTES_CAPACITY * sizeof(Note *));
	if (!notes_index)
	{
		fprintf(stderr, "Failed to allocate notes index\n");
		exit(EXIT_FAILURE);
	}
	notes_count = 0;
	notes_capacity = INITIAL_NOTES_CAPACITY;

	// Ensure notes directory exists
	struct stat st = {0};
	if (stat(NOTES_PATH, &st) == -1)
	{
		if (mkdir(NOTES_PATH, 0755) != 0)
		{
			perror("Failed to create notes directory");
			return;
		}
	}

	// Open directory
	DIR *dir = opendir(NOTES_PATH);
	if (!dir)
	{
		perror("Could not open notes directory");
		return;
	}

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		if (entry->d_name[0] == '.')
			continue; // skip "." and ".."

		// Check extension
		const char *ext = strrchr(entry->d_name, '.');
		if (!ext || strcmp(ext, ".md") != 0)
			continue;

		// Build full path
		char filepath[1024];
		snprintf(filepath, sizeof(filepath), "%s/%s", NOTES_PATH, entry->d_name);

		// Open file
		FILE *f = fopen(filepath, "r");
		if (!f)
		{
			perror("Failed to open note file");
			continue;
		}

		Note *new_note = malloc(sizeof(Note));
		if (!new_note)
		{
			fclose(f);
			continue;
		}

		new_note->filename = strdup(entry->d_name);
		new_note->title = strdup("Untitled");
		new_note->created_at = strdup("Unknown");
		new_note->content = strdup("");

		char line[MAX_LINE_LENGTH];

		// Read title line
		if (fgets(line, sizeof(line), f))
		{
			line[strcspn(line, "\r\n")] = '\0';
			if (strncmp(line, "Title: ", 7) == 0)
			{
				free(new_note->title);
				new_note->title = strdup(line + 7);
			}
		}

		// Read created_at line
		if (fgets(line, sizeof(line), f))
		{
			line[strcspn(line, "\r\n")] = '\0';
			if (strncmp(line, "Created: ", 9) == 0)
			{
				free(new_note->created_at);
				new_note->created_at = strdup(line + 9);
			}
		}

		// Read remaining content
		long current_pos = ftell(f);
		fseek(f, 0, SEEK_END);
		long total_size = ftell(f);
		long content_len = total_size - current_pos;
		if (content_len < 0)
			content_len = 0;
		fseek(f, current_pos, SEEK_SET);

		free(new_note->content);
		new_note->content = malloc(content_len + 1);
		if (new_note->content)
		{
			size_t read_bytes = fread(new_note->content, 1, content_len, f);
			new_note->content[read_bytes] = '\0';
		}
		else
		{
			new_note->content = strdup("");
		}

		fclose(f);

		// Resize array if needed
		if (notes_count >= notes_capacity)
		{
			size_t new_capacity = notes_capacity * 2;
			Note **new_array = realloc(notes_index, new_capacity * sizeof(Note *));
			if (!new_array)
			{
				fprintf(stderr, "Failed to resize notes array\n");
				free(new_note->filename);
				free(new_note->title);
				free(new_note->created_at);
				free(new_note->content);
				free(new_note);
				continue;
			}
			notes_index = new_array;
			notes_capacity = new_capacity;
		}

		// Insert newest-first (index 0)
		for (size_t j = notes_count; j > 0; j--)
		{
			notes_index[j] = notes_index[j - 1];
		}
		notes_index[0] = new_note;
		notes_count++;
	}

	closedir(dir);
}

Note *notes_service_create(const char *title, const char *content)
{
	Note *new_note = malloc(sizeof(Note));
	if (!new_note)
	{
		fprintf(stderr, "Failed to create a new notes\n");
		return NULL;
	}
	time_t now = time(NULL); // seconds since epoch
	struct tm tm_now;
	localtime_r(&now, &tm_now); // thread-safe local time

	char filename[128];
	snprintf(filename, sizeof(filename), "%04d%02d%02d%02d%02d%02d_%zu.md",
			 tm_now.tm_year + 1900,
			 tm_now.tm_mon + 1,
			 tm_now.tm_mday,
			 tm_now.tm_hour,
			 tm_now.tm_min,
			 tm_now.tm_sec,
			 notes_count + 1);
	char created[64];
	strftime(created, sizeof(created), "%Y-%m-%d %H:%M:%S", &tm_now);

	new_note->filename = strdup(filename);
	new_note->created_at = strdup(created);
	new_note->title = strdup(title ? title : "Untitled");
	new_note->content = strdup(content ? content : "");
	if (notes_count >= notes_capacity)
	{
		size_t new_capacity = notes_capacity * 2;
		Note **new_array = realloc(notes_index, new_capacity * sizeof(Note *));
		if (!new_array)
		{
			fprintf(stderr, "Failed to resize notes array\n");
			free(new_note->filename);
			free(new_note->title);
			free(new_note->created_at);
			free(new_note->content);
			free(new_note);
			return NULL;
		}
		notes_index = new_array;
		notes_capacity = new_capacity;
	}

	// Insert newest-first (index 0)
	for (size_t j = notes_count; j > 0; j--)
	{
		notes_index[j] = notes_index[j - 1];
	}
	notes_index[0] = new_note;
	notes_count++;

	char filepath[1024];
	snprintf(filepath, sizeof(filepath), "%s/%s", NOTES_PATH, new_note->filename);

	FILE *f = fopen(filepath, "w");
	if (!f)
	{
		fprintf(stderr, "Failed to open file for writing: %s\n", filepath);
		return new_note; // note still in memory
	}

	fprintf(f, "Title: %s\n", new_note->title);
	fprintf(f, "Created: %s\n\n", new_note->created_at);
	fprintf(f, "%s", new_note->content);
	fclose(f);

	return new_note;
}
void notes_service_print(const Note *n);

const Note *notes_service_get_note_by_filename(const char *filename)
{
	if (notes_count == 0)
		return NULL;
	for (int i = 0; i < notes_count; i++)
	{
		Note *note = notes_index[i];
		if (strcmp(note->filename, filename) == 0)
			return note;
	}
	return NULL;
}

int notes_service_delete_note(const Note *n)
{
	if (notes_count == 0)
		return 1;
	for (int i = 0; i < notes_count; i++)
	{
		Note *note = notes_index[i];
		if (strcmp(note->filename, n->filename) == 0)
		{
			// Free all fields
			free(note->filename);
			free(note->title);
			free(note->created_at);
			free(note->content);
			free(note);

			// Shift remaining notes down
			for (size_t j = i; j < notes_count - 1; j++)
				notes_index[j] = notes_index[j + 1];

			notes_count--;
			return 0;
		}
	}
	return 1;
}
int notes_service_update_note(Note *n)
{
	if (!n || notes_count == 0)
		return 1;

	for (size_t i = 0; i < notes_count; i++)
	{
		Note *note = notes_index[i];
		if (strcmp(note->filename, n->filename) == 0)
		{
			free(note->title);
			note->title = strdup(n->title ? n->title : "Untitled");

			free(note->created_at);
			note->created_at = strdup(n->created_at ? n->created_at : "Unknown");

			free(note->content);
			note->content = strdup(n->content ? n->content : "");

			if (i != 0)
			{
				Note *tmp = note;
				for (size_t j = i; j > 0; j--)
					notes_index[j] = notes_index[j - 1];
				notes_index[0] = tmp;
			}

			return 0;
		}
	}
	return 1;
}
size_t notes_service_note_count(void)
{
	return notes_count;
}
Note **notes_service_list_all(size_t *out_count)
{
	if (out_count)
		*out_count = notes_count;
	return notes_index;
}
void notes_service_shutdown(void)
{
	if (!notes_index)
		return;

	for (size_t i = 0; i < notes_count; i++)
	{
		Note *n = notes_index[i];
		if (!n)
			continue;

		free(n->filename);
		free(n->title);
		free(n->content);
		free(n->created_at);
		free(n);
	}

	free(notes_index);
	notes_index = NULL;
	notes_count = 0;
	notes_capacity = 0;
}
