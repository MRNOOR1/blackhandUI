#include "theme_service.h"
#include "settings_service.h"

#include <stdbool.h>

static bool g_is_dark_mode;
static int g_light_theme;

void theme_service_init(void) {
    theme_service_sync_from_settings();
}

void theme_service_sync_from_settings(void){
    bool dark = settings_service_get_bool("night_mode");
    g_is_dark_mode = dark;
    g_light_theme = settings_service_get_light_theme();
}

uint32_t theme_bg(void) {
    if (g_is_dark_mode) return 0x0b110c;
    if (g_light_theme == 0) return 0xe7dcc5; /* Desert Storm */
    if (g_light_theme == 1) return 0xd8f3ff; /* Neon Grid */
    if (g_light_theme == 2) return 0xd9e2ef; /* Ocean Steel */
    if (g_light_theme == 3) return 0xffe6cf; /* Solar Ember */
    return 0xf0f0f0;                           /* Monochrome Ops */
}

uint32_t theme_text_primary(void){
    if (g_is_dark_mode) return 0xd9dfc8;
    if (g_light_theme == 0) return 0x2f3a1f;
    if (g_light_theme == 1) return 0x0f2a3d;
    if (g_light_theme == 2) return 0x1d2c3f;
    if (g_light_theme == 3) return 0x4a2f19;
    return 0x1f1f1f;
}
uint32_t theme_text_muted(void){
    if (g_is_dark_mode) return 0x8e987d;
    if (g_light_theme == 0) return 0x6c7553;
    if (g_light_theme == 1) return 0x355167;
    if (g_light_theme == 2) return 0x45566e;
    if (g_light_theme == 3) return 0x7a5a3a;
    return 0x5c5c5c;
}
uint32_t theme_border(void){
    if (g_is_dark_mode) return 0x6d775d;
    if (g_light_theme == 0) return 0x596340;
    if (g_light_theme == 1) return 0x00a9d6;
    if (g_light_theme == 2) return 0x3e5f8d;
    if (g_light_theme == 3) return 0xff7a1a;
    return 0x4a4a4a;
}

int theme_service_is_dark(void) {
    return g_is_dark_mode ? 1 : 0;
}
