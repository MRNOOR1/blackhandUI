#ifndef THEME_SERVICE_H
#define THEME_SERVICE_H
/*
theme_service.h 
instead of having each view have their own theme and colours all that will be implemented in this file
*/
#include <stdint.h>


void theme_service_init(void);
uint32_t theme_bg(void);
uint32_t theme_text_primary(void);
uint32_t theme_text_muted(void);
uint32_t theme_border(void);
void theme_service_sync_from_settings(void);


#endif
