cmake_minimum_required(VERSION 3.16)
project(blackhand_ui C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(NOTCURSES REQUIRED IMPORTED_TARGET notcurses)

add_executable(blackhand-ui
    src/main.c
    src/screen_home.c
    src/screen_settings.c
)

target_include_directories(blackhand-ui PRIVATE ${NOTCURSES_INCLUDE_DIRS})
target_link_libraries(blackhand-ui PkgConfig::NOTCURSES)
