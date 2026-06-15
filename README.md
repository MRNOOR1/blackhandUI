# Black Hand UI

Notcurses-based UI for host development and later Buildroot integration for BLACKHAND.

The UI layer implements the **BlackHand OS UI Design Pack (v1.1)**: one fixed
screen skeleton re-skinned by **12 named themes**. Apps own *layout*; themes own
*color, selection style, status-strip format, divider and footer*. No screen
draws raw colors — every screen reads the active theme through `theme_service`
and the shared skeleton helpers in `bh_skin`.

## Target panel

Built for the **D200N2415V1** — a 2.0" IPS panel, **480×360 (4:3), ST7701S**,
used in **landscape**. The UI is laid out on a **48 col × 18 row** character
grid (a 10×20 px monospace cell → exact 4:3). Geometry lives in
`src/config.h` (`PHONE_COLS`, `PHONE_SCREEN_ROWS`); change those two values to
retarget another panel. Run the host preview in a terminal at least 48×18.

## Build (macOS)

```bash
brew install notcurses mpg123
rm -rf build && cmake -S . -B build
cmake --build build
./build/blackhand-ui
```

## Build (Ubuntu)

```bash
sudo apt install libnotcurses-dev libmpg123-dev libout123-dev
rm -rf build && cmake -S . -B build
cmake --build build
./build/blackhand-ui
```

> A previous `build/` may be cached against the old source list. Delete it
> (`rm -rf build`) so CMake picks up the new `bh_skin.c` translation unit.

## Themes

Switch themes at runtime in **Settings → Appearance → Themes** (Center applies
instantly and persists to `settings.conf`). The 12 themes, in order:

| # | Theme | Selection | Status strip | Divider | Footer |
|---|-------|-----------|--------------|---------|--------|
| 1 | Amber Ledger | invert + `>` cursor | glyph `▂▄▆ · HH:MM · [▓▓░]` | solid | wordmark |
| 2 | Phosphor Index | `0n` index + `■` | `SIM:OK · HH:MM · PWR:NN` | solid | wordmark |
| 3 | Redline Mono | centered `[ ITEM ]` | glyph | labeled `── TAG ──` | `◤ BH-OS ◥` |
| 4 | Vale Signal | invert + cursor | glyph | solid | wordmark |
| 5 | Rushnyk | `◆`/`◇` diamond | glyph (red) | ornament `◆╳◆╳` | `✶ BLACKHAND ✶` |
| 6 | Thermal Index | heat color + `›` | thermal blocks | solid | wordmark |
| 7 | Instrument | invert + `0x0n` reg | two-row UTC telemetry | (in status) | sparkline + `NET:LTE` |
| 8 | Boot Rite | `◆/◇` red, teal values | teal text | ornament band | `INIT OK ✶` |
| 9 | Blueprint | outlined box + `A·n` | glyph | dimension `\| ── TAG ── \|` | wordmark |
| 10 | Dossier (light) | solid ink bar | `SIG:3/4 · HH:MM · PWR:NN` | double rule | `REF: BLACKHAND/0.4` + stamp |
| 11 | One-Bit | full invert, bold | block counts | checker `▀▄▀▄` | inverted tag + wordmark |
| 12 | Polar Night | `✶` prefix, white | glyph + aurora | aurora `▁▂▄▆▄▂▁` | GPS coordinates |

## Controls (6-key model, QWEASD)

The hardware is six keys; Center is mapped to Enter on the dev host.

- `W` / `S` — up / down (list move, value change)
- `A` / `D` — left / right (cursor, prev/next, seek)
- `Q` — soft-left / back, `E` — soft-right / contextual action
- `Enter` — Center (open / select / play-pause / toggle / confirm)
- Arrow keys mirror W/A/S/D; `x` quits the dev host.

## Project Structure

- `src/main.c`: app loop, screen routing, global lifecycle.
- `src/bh_skin.c`: themed skeleton primitives — status strip, divider, footer,
  the generic list item (all 10 selection styles) and semantic action cells.
  Every screen renders through these so the active theme owns the look.
- `src/services/theme_service.c`: the 12-theme token table and accessors.
- `src/frame_renderer.c`: draws the per-frame skeleton (status strip + divider).
- `src/screens/`: UI-only draw/input files (one file per screen).
- `src/services/`: domain logic and persistence APIs (settings, mp3, notes, voice memos).
- `src/platform/`: hardware abstraction layer for battery/cellular integration.

## How theming works

`draw_frame()` paints the themed status strip and divider before each screen
draws. Screens render list rows with `bh_list_item()` (which applies the active
theme's selection style) and call `ghost_softkeys()` for the themed footer.
To add or retune a theme, edit the single struct entry in
`src/services/theme_service.c` — no screen code changes.

## Coupling/Cohesion Rules

- Screen files should only render UI and translate key input into intents.
- Services should own data/state and persistence logic.
- Platform files should own hardware access (I2C/UART/GPIO) only.
- `main.c` should orchestrate, not contain feature logic.

## Current Scaffolds

- Added scaffold files for `contacts`, `mp3`, `voice memo`, and `notes` screens.
- Added scaffold service modules for `mp3`, `notes`, and `voice memos`.
- These files intentionally include TODO guidance comments so you can implement each feature yourself.
