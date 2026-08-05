# moba-sim

A small MOBA (League of Legends style) simulator written in C++23: champions,
items, and a stat system with full provenance tracking, plus an SDL3-based 2D
visualization.

## What's inside

- **Stat pipeline** (`src/stats/`) — every stat is computed as
  `sum(Base) * (1 + sum(Inc)) * product(More)` (Path-of-Exile-style buckets).
  Each modifier carries a source label, so `breakdown()` can explain exactly
  where a final number came from.
- **Simulation time** (`src/sim/`) — integral ticks (`Tick`, `TickSpan`,
  `TickRate`). Durations are exact, so expiry is reproducible and never drifts.
- **Effects** (`src/effects/`) — buffs, debuffs, auras and item passives.
  Identity, dependencies, lifetime and stacking behaviour are **data**, so the
  framework can refresh, extend, stack, dispel and report time remaining, and
  evaluate everything in one exact dependency-ordered pass — no fixed-point
  iteration and no convergence tolerance.
- **Champions** (`src/champions/`) — base stats + per-level growth in the
  [LoL wiki format](https://wiki.leagueoflegends.com/en-us/Ahri), resolved
  together with items and effects into one stat table.
- **Items** (`src/items/`) — named collections of stat modifiers that can be
  equipped and unequipped.
- **Events** (`src/events/`) — `std::variant`-based events with recursive
  nesting via `EventSequence`.
- **View** (`src/view/`) — thin SDL3 wrapper: RAII window, 2D renderer, and a
  fixed-timestep game loop
  (["Fix Your Timestep"](https://gafferongames.com/post/fix_your_timestep/)).

See [`docs/architecture.md`](docs/architecture.md) for the design and the
reasoning behind it.

## Binaries

| Target | Description |
|---|---|
| `moba-sim` | Console demo: a champion with items and effects, stat breakdowns, a buff expiring over simulated time, and the recursive event system. |
| `moba-sim-view` | Interactive SDL3 demo: move a champion with WASD; movement speed comes live from the stat pipeline. SPACE applies a 3-second haste buff that expires on its own, TAB prints a stat breakdown, Escape quits. |
| `moba_sim_tests` | Catch2 test suite. |

## Running

No checkout build needed — `nix run` fetches, builds and runs:

```sh
nix run .                       # console demo
nix run .#moba-sim-view         # interactive SDL3 demo
nix run .#tests                 # the whole Catch2 suite
nix run .#tests -- "[effects]"  # arguments pass through to Catch2
nix run .#tests -- --list-tests
```

## Building

### With Nix (recommended)

```sh
nix build          # builds and runs the tests
nix flake check    # same, as a check
nix develop        # dev shell with cmake, ninja, clang, SDL3, Catch2,
                   # clang-tools and pre-commit hooks (direnv: `use flake`)
```

`nix build` produces two outputs: `out` with both binaries and `tests` with the
Catch2 binary, so `nix run .#tests` reuses that build instead of compiling
everything a second time.

### Manually

Requires CMake >= 3.25, a C++23 compiler, SDL3 and Catch2 v3.

```sh
cmake -B build -G Ninja
cmake --build build
ctest --test-dir build
./build/tests/moba_sim_tests "[effects]"   # filter by tag
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
