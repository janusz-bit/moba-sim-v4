# moba_sim

A headless C++23 MOBA simulation core with an optional SDL3 view on top:
champions, items, and a stat system with full provenance tracking.

- [**README**](README.md) — what's inside, binaries, building and running.
- [**Architecture**](docs/architecture.md) — the layers, the invariants, and
  the reasoning behind the non-obvious decisions
  ([Polish version](docs/architecture.pl.md)).
- [**API reference**](docs/api/index.md) — the `///`-documented public API,
  one page per module.
- [**AGENTS.md**](AGENTS.md) — the development workflow and tooling.

## What's inside

- [`stats/`](docs/api/stats.md) — every stat is
  `sum(Base) * (1 + sum(Inc)) * product(1 + More)`, with a provenance label on
  every modifier so any final number can be explained.
- [`sim/`](docs/api/sim.md) — integral ticks; buff expiry is exact, replay is
  reproducible, tests need no epsilons.
- [`effects/`](docs/api/effects.md) — buffs, debuffs, auras and item passives;
  identity, dependencies, lifetime and stacking are data, evaluated in one
  dependency-ordered pass.
- [`champions/`](docs/api/champions.md) — wiki-format base stats, items and
  effects resolved into one stat table.
- [`items/`](docs/api/items.md) — named collections of stat modifiers.
- [`events/`](docs/api/events.md) — variant-based events with recursive
  nesting via `EventSequence`.
- [`view/`](docs/api/view.md) — the thin SDL3 layer: RAII window, 2D renderer
  and a fixed-timestep game loop.

```{toctree}
:hidden:
:caption: Guides

README.md
docs/architecture.md
docs/architecture.pl.md
AGENTS.md
```

```{toctree}
:hidden:
:caption: API reference

docs/api/index
docs/api/stats
docs/api/sim
docs/api/effects
docs/api/champions
docs/api/items
docs/api/events
docs/api/view
```
