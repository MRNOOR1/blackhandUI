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
