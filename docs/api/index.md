# API reference

Generated from the `///` comments in `src/` by Breathe, with Doxygen acting
purely as the C++ parser. One page per module, mirroring the layering
described in [architecture](../architecture.md):

- [`stats/`](stats.md) — stat ids, modifiers, the three-bucket pipeline
- [`sim/`](sim.md) — integral simulation time
- [`effects/`](effects.md) — buffs, debuffs, auras, item passives
- [`champions/`](champions.md) — resolving a unit
- [`items/`](items.md) — named bags of modifiers
- [`events/`](events.md) — the recursive event variant
- [`view/`](view.md) — the SDL3 layer
