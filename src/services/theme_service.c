#include "theme_service.h"
#include "settings_service.h"

static bool g_is_dark_mode;

void theme_service_init(void) {
    theme_service_sync_from_settings();
}

void theme_service_sync_from_settings(void){
    bool dark = settings_service_get_bool("night_mode");
    g_is_dark_mode = dark;
}

uint32_t theme_bg(void) {
    return g_is_dark_mode ? 0x0D0D0D : 0xF2F2F2;
}

uint32_t theme_text_primary(void){
    return g_is_dark_mode ? 0xF2F2F2 : 0x0D0D0D;
}
uint32_t theme_text_muted(void){
    return g_is_dark_mode ? 0xADADAD : 0x5C5C5C;
}
uint32_t theme_border(void){
    return g_is_dark_mode ? 0xF2F2F2 : 0x0D0D0D;
}