# Black Hand UI

Notcurses-based UI scaffold for host development and later Buildroot integration for BLACKHAND

## Build (macOS)

```bash
brew install notcurses
mkdir -p build
cmake -S . -B build
cmake --build build
./build/blackhand-ui
```

## Build (Ubuntu)

```bash
sudo apt install libnotcurses-dev
mkdir -p build
cmake -S . -B build
cmake --build build
./build/blackhand-ui
```

## Controls

- `h` Home screen
- `s` Settings screen
- `q` Quit

## Project Structure (Reorganized)

- `src/main.c`: app loop, screen routing, global lifecycle.
- `src/screens/`: UI-only draw/input files (one file per screen).
- `src/services/`: domain logic and persistence APIs (settings, mp3, notes, voice memos).
- `src/platform/`: hardware abstraction layer for battery/cellular integration.

## Coupling/Cohesion Rules

- Screen files should only render UI and translate key input into intents.
- Services should own data/state and persistence logic.
- Platform files should own hardware access (I2C/UART/GPIO) only.
- `main.c` should orchestrate, not contain feature logic.

## Current Scaffolds

- Added scaffold files for `contacts`, `mp3`, `voice memo`, and `notes` screens.
- Added scaffold service modules for `mp3`, `notes`, and `voice memos`.
- These files intentionally include TODO guidance comments so you can implement each feature yourself.
