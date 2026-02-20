#ifndef SETTINGS_SERVICE_H
#define SETTINGS_SERVICE_H

#include <stdbool.h>

void settings_service_init(void);
void settings_service_shutdown(void);

int settings_service_count(void);
const char *settings_service_label(int index);
bool settings_service_enabled(int index);
void settings_service_toggle(int index);

#endif
