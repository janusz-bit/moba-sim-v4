# moba-sim

A small MOBA (League of Legends style) simulator written in C++23: champions,
items, and a stat system with full provenance tracking, plus an SDL3-based 2D
visualization.

## What's inside

- **Stat pipeline** (`src/stats/`) — every stat is computed as
  `sum(Base) * (1 + sum(Inc)) * product(More)` (Path-of-Exile-style buckets).
  Each modifier carries a source label, so `breakdown()` can explain exactly
  where a final number came from.
- **Champions** (`src/champions/`) — base stats + per-level growth in the
  [LoL wiki format](https://wiki.leagueoflegends.com/en-us/Ahri); each stat is
  a pipeline seeded at a given level, ready for item/buff modifiers.
- **Items** (`src/items/`) — named collections of stat modifiers that can be
  equipped and unequipped.
- **Events** (`src/events/`) — `std::variant`-based events with recursive
  nesting via `EventSequence`.
- **View** (`src/view/`) — thin SDL3 wrapper: RAII window, 2D renderer, and a
  fixed-timestep game loop
  (["Fix Your Timestep"](https://gafferongames.com/post/fix_your_timestep/)).

## Binaries

| Target | Description |
|---|---|
| `moba-sim` | Console demo of the recursive event system. |
| `moba-sim-view` | Interactive SDL3 demo: move a champion with WASD; movement speed comes live from the stat pipeline. Press TAB to print a stat breakdown, Escape to quit. |
| `moba_sim_tests` | Catch2 test suite. |

## Building

### With Nix (recommended)

```sh
nix build          # builds and runs the tests
nix flake check    # same, as a check
nix develop        # dev shell with cmake, ninja, clang, SDL3, Catch2,
                   # clang-tools and pre-commit hooks (direnv: `use flake`)
```

### Manually

Requires CMake >= 3.25, a C++23 compiler, SDL3 and Catch2 v3.

```sh
cmake -B build -G Ninja
cmake --build build
ctest --test-dir build
```

## Development

- Formatting: `clang-format` (C++) and `nixfmt` (Nix), enforced via
  pre-commit hooks set up by the dev shell.
- Warnings: `-Wall -Wextra -Wpedantic -Wconversion -Wshadow`; enable
  `-Werror` with `-DMOBA_SIM_WARNINGS_AS_ERRORS=ON`.
- Tests live in `tests/` and run headless (`SDL_VIDEO_DRIVER=dummy` where
  needed).

## License

[MIT](LICENSE)
