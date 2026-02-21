# Western RPG Prototype (Phase 1)

This repository now contains a first playable foundation for a no-engine, C++ western RPG prototype using SDL2 and OpenGL.

## Included in Phase 1

- CMake project structure with separated core/game/render libraries.
- Fixed-step simulation loop (60 Hz).
- Isometric tile rendering with 128x64 logical tile assumptions.
- 8-direction normalized movement (WASD + arrows).
- Tile-based collision with wall sliding.
- Basic dynamic lighting model (ambient + player lantern + lamp) with soft falloff and flicker.
- Simple player idle/walk animation (procedural bob + sway).
- 4K window target (3840x2160).

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run

```bash
./build/game
```

## Controls

- Move: `WASD` or arrow keys
- Quit: `Esc`

## Map format

`data/maps/frontier_town.map` is an ASCII map:

- `#` = blocked tile
- `.` = walkable tile

Each line is one row.
