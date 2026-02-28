cmake_minimum_required(VERSION 3.16)
project(blackhand_ui C)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(NOTCURSES REQUIRED IMPORTED_TARGET notcurses)
pkg_check_modules(MPG123 REQUIRED IMPORTED_TARGET libmpg123)
pkg_check_modules(OUT123 REQUIRED IMPORTED_TARGET libout123)

add_executable(blackhand-ui
    src/main.c
    src/draw_utils.c
    src/frame_renderer.c
    src/screens/screen_home.c
    src/screens/screen_settings.c
    src/platform/hardware.c
    src/screens/screen_calls.c
    src/screens/screen_messages.c
    src/screens/screen_contacts.c
    src/screens/screen_mp3.c
    src/screens/screen_voice_memo.c
    src/screens/screen_notes.c
    src/services/settings_service.c
    src/services/mp3_service.c
    src/services/notes_service.c
    src/services/voice_memo_service.c
    src/services/theme_service.c
)

target_include_directories(blackhand-ui PRIVATE
    ${NOTCURSES_INCLUDE_DIRS}
    ${MPG123_INCLUDE_DIRS}
    ${OUT123_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(blackhand-ui
    PkgConfig::NOTCURSES
    PkgConfig::MPG123
    PkgConfig::OUT123
    Threads::Threads
)
