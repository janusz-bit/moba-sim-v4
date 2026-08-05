# Architecture

A headless MOBA simulation core with an optional SDL3 view on top. This document
describes the layers, the invariants that hold them together, and — where a
decision is not obvious — the reasoning behind it.

For build commands and day-to-day workflow see [`AGENTS.md`](../AGENTS.md).

## Contents

- [Layers](#layers) · [Design principles](#design-principles)
- [`stats/` — the stat pipeline](#stats--the-stat-pipeline)
- [`sim/` — simulation time](#sim--simulation-time)
- [`effects/` — buffs, debuffs, auras](#effects--buffs-debuffs-auras)
- [`champions/` — resolving a unit](#champions--resolving-a-unit)
- [`items/`](#items) · [`events/`](#events) · [`view/`](#view)
- [Data flow of one simulation step](#data-flow-of-one-simulation-step)
- [Invariants](#invariants) · [Extending the system](#extending-the-system)
- [Testing strategy](#testing-strategy) · [Not yet built](#not-yet-built)

## Layers

```
                    ┌─────────────────────────────────────────┐
                    │  moba_sim_view  (SDL3)                  │
                    │  window · renderer2d · game_loop        │
                    └───────────────────┬─────────────────────┘
                                        │ Tick / stat reads
╔═══════════════════════════════════════▼═════════════════════════════════════╗
║  moba_sim_core  (no SDL, no I/O, headless)                                  ║
║                                                                             ║
║   champions/     Champion: owns sources, resolves them into a StatTable     ║
║       │                                                                     ║
║       ├── items/        Item: named bag of Modifiers                        ║
║       │                                                                     ║
║       ├── effects/      Effect · EffectSet · Lifetime · StackPolicy         ║
║       │        │        buffs, debuffs, auras, item passives                ║
║       │        │                                                            ║
║       │        └── sim/     Tick · TickSpan · TickRate (integral time)      ║
║       │                                                                     ║
║       └── stats/        StatId · Modifier · StatPipeline · StatTable        ║
║                         StatMask · StatBreakdown                            ║
║                                                                             ║
║   events/        Event variant, recursive dispatch (standalone today)       ║
╚═════════════════════════════════════════════════════════════════════════════╝
```

Dependencies point downward only. `stats/` knows nothing about effects,
champions or time; `effects/` knows about `stats/` and `sim/` but not about
champions; `champions/` composes everything below it.

`moba_sim_core` must never link SDL or perform I/O. The simulation has to be
runnable headless, in a test, or thousands of times in a row for balance
analysis, and CI runs it without a display.

## Design principles

Four ideas explain most of the code.

**1. Behaviour is data wherever the framework needs to reason about it.**
An effect's identity, dependencies, lifetime and stacking rule are plain
structs, not logic hidden in a callable. Only arithmetic is a function. A
framework that cannot see *what* an effect depends on cannot order effects, so
it is forced into iterating until numbers stop moving; one that cannot see
*when* an effect ends cannot refresh it, dispel it, or show a buff timer.
Declaring these as data pays for itself immediately — see
[Effects](#effects--buffs-debuffs-auras).

**2. Exact over approximate.** Time is integral, not accumulated `double`
seconds. Stat evaluation is one topologically ordered pass, not iteration to a
tolerance. Neither results nor expiry may depend on frame pacing or on a solver
setting, or replay is not reproducible and tests need epsilons everywhere.

**3. Every number is traceable.** Each `Modifier` carries a `source` label, and
`format_breakdown` renders the full derivation of any stat. "Why is my AD 217.8"
must always be answerable — that is the main thing a simulator is *for*.

**4. Derived state is derived.** A `Champion`'s `StatTable` is a pure function
of base data, level, items and effects. It is cached, but the cache is an
optimisation, never a source of truth: throwing it away and rebuilding must give
the same answer.

## `stats/` — the stat pipeline

### The formula

Every stat resolves through three buckets, modelled on Path of Exile's damage
pipeline:

```
value = sum(Base) * (1 + sum(Inc)) * product(1 + More)
```

| Bucket | Combines | Example |
|---|---|---|
| `Base` | additively | `+40 AD` from B.F. Sword |
| `Inc`  | additively into one multiplier | `+10%` and `+20%` → `× 1.3` |
| `More` | multiplicatively, each on its own | `+10%` and `+20%` → `× 1.1 × 1.2` |

`Inc` and `More` differ in how they stack with *each other*, which is the whole
reason for having both: additive percentages dilute as you add more of them,
multiplicative ones do not.

An empty pipeline computes `0`, and `Inc`/`More` with no `Base` also compute `0`
— percentages of nothing are nothing.

### Types

| Type | File | Role |
|---|---|---|
| `StatId` | `stat_id.hpp` | which stat, plus a `Count` sentinel |
| `Modifier` | `modifier.hpp` | one labelled change: stat, kind, value, source |
| `ModifierKind` | `modifier.hpp` | `Base` / `Inc` / `More` |
| `StatPipeline` | `stat_pipeline.hpp` | the three buckets for one stat |
| `StatTable` | `stat_table.hpp` | one pipeline per stat: a unit's stat state |
| `StatMask` | `stat_mask.hpp` | a set of stats (`std::bitset<kStatCount>`) |
| `StatBreakdown` | `stat_breakdown.hpp` | the provenance of one computed value |

### `StatId` cannot drift out of sync

`StatId` ends in a `Count` sentinel, and everything else is derived from it:

```cpp
inline constexpr std::size_t kStatCount = static_cast<std::size_t>(StatId::Count);
inline constexpr std::array<StatId, kStatCount> kAllStats = /* generated */;
```

Every table indexed by `stat_index()` is `kStatCount`-sized and carries a
`static_assert`, so forgetting an entry is a compile error rather than an
out-of-bounds read. `kStatNames` (`stat_id.hpp`) and `kStatSpecs`
(`champions/champion.cpp`) both do this.

No code `switch`es over `StatId`: a `switch` would only produce a `-Wswitch`
warning for a missing case and then undefined behaviour, whereas a table with a
`static_assert` fails the build.

### One `Modifier` type for every source

`Modifier` is deliberately source-neutral — items, effects and champion base
stats all produce the same type, and all reach a pipeline through the single
`apply_modifier` in `modifier.cpp`:

```cpp
void apply_modifier(StatPipeline& pipe, const Modifier& mod, const std::string& source);
```

That function is the only place mapping `ModifierKind` to `add_base` /
`add_inc` / `add_more`. When each source had its own copy of that mapping, they
could disagree about what a kind meant; now there is nothing to disagree about.
Helpers `base_mod(stat, value, source)`, `inc_mod`, `more_mod` keep call sites
readable.

### Provenance

Pipelines store `StatBreakdown::Entry{value, source}` rather than bare doubles,
so `breakdown()` returns every contribution with its label and
`format_breakdown` renders it:

```
AttackDamage = 138
  Base = 138
    + 68  (Ahri base, lvl 6)
    + 40  (B.F. Sword)
    + 30  (Ahri (Fury))
  138 * 1 * 1 = 138
```

This is why label plumbing appears throughout the stack: effects label their
output with their `EffectKey`, items with their name, base stats with
`"<name> base, lvl <n>"`. Provenance is a first-class feature, not debug
scaffolding.

## `sim/` — simulation time

`Tick` is a **point** in time, `TickSpan` a **duration**, and `TickRate` the only
place that knows how many ticks make a second. The distinction is enforced by
the type system: `Tick - Tick` yields a `TickSpan`, `Tick + TickSpan` a `Tick`,
and `Tick + Tick` does not compile.

### Why integers

Suppose time were an accumulated `double` in seconds. A buff applied at `t` with
a 3-second duration expires when `now - t >= 3.0`. Adding `1.0/60.0` to a
`double` 180 times does not produce exactly `3.0`, so the comparison fires a
tick early or late depending on the tick rate and on where in the run it
happens. Consequences:

- the same scenario yields different numbers on a different tick rate;
- replay is not bit-reproducible, which makes desyncs and regressions untraceable;
- every duration test needs an epsilon, and epsilon-based tests hide off-by-one
  bugs.

With integral ticks `now >= expires_at` is exact. `TickSpan{180}` is 3 seconds at
60 ticks/s, always, everywhere. `tests/tick.test.cpp` checks that a 3-second
duration is exact at 10, 30, 60 and 128 ticks per second.

Seconds exist only at the boundary: `TickRate::ticks_from_seconds` (rounding, so
short durations do not collapse to zero) converts game data in, and
`seconds_from_ticks` / `seconds_at` convert back out for display or for effects
that genuinely think in seconds. A non-positive rate is clamped to 1 rather than
dividing by zero.

`Tick`, `TickSpan` and `TickRate` are `constexpr`-usable, so durations from
static data can be compile-time constants.

## `effects/` — buffs, debuffs, auras

This is the most consequential part of the design, so it is worth stating the
shape first and the reasoning after.

### The shape

```cpp
struct Effect {
    EffectKey key{};        // stable identity — drives stacking
    StatMask reads{};       // stats contribute() may query   (must be complete)
    StatMask writes{};      // stats contribute() may modify  (must be complete)
    Lifetime lifetime = Permanent{};
    StackPolicy policy = StackPolicy::Refresh;
    double magnitude = 0.0; // compared by ReplaceIfStronger
    StackCount max_stacks = 1;
    std::function<void(const EffectContext&, ModifierSink&)> contribute{};
};
```

Everything except `contribute` is inspectable data. That is the point: the
framework can order, expire, refresh, stack, dispel and report on effects
precisely because it can see these fields.

For the common case — a flat bonus with no dependencies — no lambda is needed:

```cpp
champion.apply_effect(flat_effect({.source = "Baron", .name = "Hand of Baron"},
                                  {base_mod(StatId::AttackDamage, 20)},
                                  Timed::for_span(champion.now(), TickSpan{180})));
```

### Identity: `EffectKey`, not a counter

```cpp
struct EffectKey { std::string source; std::string name; };
```

Identity is *semantic*: who applied it, and which effect it is. Ahri's Q always
produces `{"Ahri", "Q"}`, so re-casting it is recognised as the same buff and
refreshes rather than stacking.

An auto-incrementing counter would fail exactly here. The same buff cast twice
gets two ids and silently stacks; ids depend on how many effects happened to be
created earlier in the run, so they are neither reproducible across runs nor
writable as literals in tests. Semantic keys are also comparable, printable and
serialisable — prerequisites for replay.

Identity is kept separate from `EffectHandle`, which refers to *one particular
instance* and exists only to remove one of several `Independent` instances.
Conflating "which effect is this" with "which instance is this" is what makes
stacking rules impossible to express.

### Lifetime as data

```cpp
using Lifetime = std::variant<Permanent, Timed, OneShot, Until>;
```

| Alternative | Meaning |
|---|---|
| `Permanent` | lives until explicitly removed — item passives, auras in range |
| `Timed{expires_at}` | expires at a fixed tick; built via `Timed::for_span(now, duration)` |
| `OneShot` | contributes for exactly one step |
| `Until{predicate}` | escape hatch for "while below 50% HP" |

The tempting alternative is to let each effect police itself: return an `alive`
flag and compare `now - start >= duration` internally. It costs more than it
saves:

- **no refresh or extend** — nothing outside the effect can move a deadline it
  cannot see, so "your buffs last 20% longer" is unimplementable;
- **no dispel** — cleansing requires knowing what is a timed debuff;
- **no buff bar** — `remaining()` cannot be answered when the deadline lives in
  a lambda capture;
- **no scheduling** — every effect must be polled every step forever;
- **not serialisable** — a `start_time` in a capture cannot be saved, and copying
  the owner either shares or duplicates it depending on how the capture was
  written;
- **duplicated arithmetic** — the same expiry comparison in every timed effect.

`Until` keeps the escape hatch for genuinely custom lifetimes, but as one
alternative among four rather than the only mechanism.

### Stacking policies

Re-applying an effect whose key is already live is resolved by
`StackPolicy`, chosen explicitly at the application site because picking wrong is
a balance bug:

| Policy | Behaviour |
|---|---|
| `Refresh` | reset the duration, one instance (the common buff re-cast) |
| `Stack` | add an intensity stack up to `max_stacks`, refresh duration |
| `ExtendDuration` | add the new duration to what is left, so early re-casts waste nothing |
| `IgnoreIfPresent` | drop the application while an instance is live |
| `ReplaceIfStronger` | replace only on higher `magnitude`, or equal magnitude with a later expiry |
| `Independent` | coexist; each instance keeps its own lifetime |

`Stack` scales contributions automatically: `ModifierSink` multiplies `Base` and
`Inc` by the stack count, while `More` **compounds** — N stacks of `+10%` become
`1.1^N - 1`, matching "the effect applied N times" rather than a linear sum.
Stacked contributions are labelled `"Nasus (Siphoning Strike) x3"` in
breakdowns.

### Two phases, deliberately separate

```cpp
std::vector<EffectKey> advance(Tick now);                                   // mutating
void contribute_all(StatTable&, Tick now, TickRate) const;                  // pure
```

These have opposite requirements, so they are different functions:

- `advance` runs **exactly once per simulation step**. It expires finished
  effects, retires spent `OneShot`s, and returns the keys that went away so the
  caller can react.
- `contribute_all` is **pure** and may run as often as stats are needed. It never
  removes anything.

Fusing them forces a contradiction. If the function that computes stats also
honours removal, and stats must be computed more than once (as any iterative
scheme requires), then either removal happens on some arbitrary pass or every
side effect fires an unpredictable number of times. Keeping them apart makes
`contribute` pure by construction: `tests/effect.test.cpp` evaluates a `OneShot`
twenty times within one step and it is neither consumed nor doubled.

`contribute` **must not have side effects.** It runs on every stat rebuild.
Anything that must happen once per step belongs in `advance`.

### Ordering: one exact pass, no fixed point

Because every effect declares `reads` and `writes`, the dependency graph is
known before any arithmetic happens. `EffectSet` topologically sorts it and
evaluates each effect once, so an effect that reads a stat sees that stat's
**final** value:

```cpp
// 50% of total AD as armor — sees AD after items and all other AD effects,
// no matter what order things were applied in.
Effect{
    .key = {.source = "Steelcaps", .name = "Sturdy"},
    .reads = {StatId::AttackDamage},
    .writes = {StatId::Armor},
    .contribute = [](const EffectContext& ctx, ModifierSink& sink) {
        sink.add_base(StatId::Armor, 0.5 * ctx.stats(StatId::AttackDamage));
    },
};
```

The alternative — iterate the whole effect set until values stop moving within
some `eps` — has real problems:

- **results depend on the tolerance**, so `eps` becomes a balance parameter;
- **convergence is not guaranteed**, so a legal item build can throw a
  `ConvergenceError` from the middle of a simulation, with no sensible recovery;
- **there is no fixed point in the domain to find.** MOBA stat interactions are a
  DAG: Rabadon multiplies total AP, but AP contributions do not feed themselves.
  Iteration solves a problem the domain does not have.

Two details make this practical:

**Layering is not a cycle.** An effect that reads a stat it also writes is
allowed — "gain 10% of your total AD as bonus AD" reads the value accumulated so
far and adds exactly one layer of amplification. This is well-defined and
finite; it is not a request to sum a geometric series.

**Peers are not a cycle.** Two effects that both read *and* write the same stat
are peers, not dependents — two AD amplifiers cannot each wait for the other.
`depends_on` in `effect_set.cpp` therefore skips the edge when both sides read
and write the same stat. The exception is narrow: a genuine conversion cycle
(AD → Armor and Armor → AD) has each effect reading a stat it does not write, so
both edges survive and the cycle is still caught.

Genuine cycles are rejected by `EffectSet::apply` with `EffectCycleError`,
naming the effects involved, and the set is left unchanged. Rejecting at
application time is right because a cycle is a **modelling error** — the value
such a pair should produce is undefined — and a clear message beats a numerical
failure in the hot path.

The sort is deterministic (Kahn's algorithm, always taking the lowest ready
index). Order must not depend on hash iteration or pointer values: with `More`
modifiers in play, a different order can produce different floating-point
results. The computed order is cached and invalidated only when the set changes,
since it depends on the set of effects and never on stat values.

### Declarations are enforced

The ordering guarantee is only sound if `reads`/`writes` are complete, so they
are checked rather than trusted. `StatView` and `ModifierSink` throw
`UndeclaredStatAccess` — naming the effect and the stat — on any access outside
the declared masks. A missing declaration is a loud failure instead of a
silently stale number.

## `champions/` — resolving a unit

`ChampionData` holds wiki-format base statistics: a level-1 value and a
per-level growth for each of the ten stats, resolved by
`base_value(stat, level) = base + growth * (level - 1)`. `kStatSpecs` maps each
`StatId` to its `(base, growth)` member pointer pair.

`Champion` owns **every** modifier source and resolves them in a fixed order:

```
StatTable  =  base stats at level   →   items (equip order)   →   effects (dependency order)
```

### Every modifier has an owner

There is deliberately **no** way to push a modifier straight into a pipeline.
`Champion::pipeline(stat)` returns a `const&`. The only entry points are
`equip`, `apply_effect` and `set_level`.

This closes a real trap. Rebuilding is how removal works — dropping one modifier
out of a bucket is not supported, so `unequip` rebuilds from the remaining
sources. A modifier pushed directly into a pipeline has no owner, so it is not
part of any source, so the next rebuild silently deletes it. Under the previous
design, unequipping boots also erased every buff. Now temporary bonuses are
effects and permanent gear is items; both survive, and
`tests/champion.test.cpp` pins that behaviour.

### Lazy, pure reads

Mutation marks the table dirty; the next read rebuilds it:

```cpp
mutable StatTable stats_;
mutable bool dirty_ = true;
```

Reading a stat is `const` and pure — the tenth read equals the first. The
`mutable` cache is an optimisation, not state. Only `advance_to` / `advance_by`,
`apply_effect`, `remove_effect`, `equip`, `unequip`, `set_level` and
`set_tick_rate` mutate.

`advance_to` always invalidates, even when nothing expired: time itself changes
what effects contribute (an effect scaling with elapsed time, a `Timed` effect
on its final tick).

### Stepping time

```cpp
std::vector<EffectKey> advance_to(Tick now);   // absolute
std::vector<EffectKey> advance_by(TickSpan);   // relative
```

Both return the keys that expired during the step, which is how a caller logs
"buff ended" or fires a follow-up event without polling.

## `items/`

`Item` is a name plus a `std::vector<Modifier>` — pure data with no behaviour.
Modifiers are the same `Modifier` type effects produce, so `Champion` applies
both through one path.

An item modifier's `source` is normally empty and filled in with the item's name
at rebuild time; setting it explicitly attributes a line to something more
specific, e.g. `"Zeal (passive)"`. Item identity is by name, and `unequip`
removes the first match.

## `events/`

A `std::variant`-based event system with recursive nesting:

```cpp
struct EventSequence;                        // forward-declared for the alias
using Event = std::variant<PlayerDiedEvent, KeyPressedEvent,
                           std::shared_ptr<EventSequence>>;
struct EventSequence { std::vector<Event> events; };
```

`process_event` dispatches via `std::visit` to free `handle_event` overloads,
each in its own translation unit, resolved at the call site in `event.cpp`.
`EventSequence`'s handler recurses through `process_event`, so sequences nest
arbitrarily. `debug_out` defaults to a badbit stream, making event handling
silent unless a stream is passed.

This layer is **standalone today**: handlers only write to a stream and have no
access to a `Champion`. Connecting it to the simulation is the obvious next step
— see [Not yet built](#not-yet-built).

## `view/`

The only SDL-dependent library, kept deliberately thin:

| Type | Role |
|---|---|
| `Window` | RAII over `SDL_Window` + `SDL_Renderer`; non-copyable, non-movable; throws on failure |
| `Renderer2D` | immediate-mode shapes in pixel space, y down |
| `GameLoop` | fixed-timestep loop with an accumulator |
| `Vec2`, `Rect`, `Color` | header-only geometry and colour |

`GameLoop` is a ["Fix Your
Timestep"](https://gafferongames.com/post/fix_your_timestep/) loop: `update(dt)`
runs zero or more times per frame with constant `dt`, `render` once, and long
stalls are clamped to 0.25s so a debugger pause cannot trigger a burst of
catch-up steps.

The core never sees wall-clock time. `demo.cpp` runs the loop at a tick rate
matching the champion's, so each `update` is exactly one `TickSpan{1}` — SDL's
nanosecond clock decides *when* to step, never *how much* simulated time passed.

## Data flow of one simulation step

```
game loop (or test) decides a step happened
        │
        ▼
champion.advance_by(TickSpan{1})
        │
        ├── now_ += 1
        ├── EffectSet::advance(now_)      MUTATING, exactly once
        │      ├── drop effects whose Lifetime ended
        │      ├── retire OneShots applied before this tick
        │      └── return expired keys ──────────────► caller reacts
        └── mark stats dirty
        │
        ▼
champion.compute(StatId::AttackDamage)    (any number of times)
        │
        └── if dirty: rebuild()           PURE
               ├── 1. base stats at level      → StatTable
               ├── 2. items in equip order     → StatTable
               └── 3. EffectSet::contribute_all(table, now_, rate)
                        └── for each effect in topological order:
                               StatView (declared reads)  ──┐
                               ModifierSink (declared writes) ──► StatTable
        │
        ▼
double, and champion.explain(stat) for the full derivation
```

## Invariants

Break these and things fail quietly rather than loudly.

**Stats**

1. `value = sum(Base) * (1 + sum(Inc)) * product(1 + More)`.
2. Every modifier reaches a pipeline through `apply_modifier`.
3. Every modifier carries a `source`; provenance is a feature.
4. No `switch` over `StatId`; use a `kStatCount`-sized table with a
   `static_assert`. Never store `StatId::Count`.

**Time**

5. Simulation time is integral. Never accumulate seconds as `double` in the core.
6. `TickRate` is the only seconds boundary.

**Effects**

7. `contribute` is pure. Side effects belong in `advance`.
8. `reads`/`writes` must be complete — enforced by `UndeclaredStatAccess`.
9. Evaluation is one topological pass. No iteration, no epsilon.
10. Cycles are rejected at `apply`, not discovered during evaluation.
11. Evaluation order is deterministic.

**Champions**

12. Every modifier has an owner: base data, `items()`, or `effects()`.
13. Reading a stat is const and pure; the cache is not state.
14. `StatTable` is fully reconstructible from the owned sources.

**Layering**

15. `moba_sim_core` never links SDL and never does I/O.
16. Dependencies point downward: `stats/` → `sim/` → `effects/` → `champions/`.

## Extending the system

### Adding a stat

Two edits, both compile-time enforced:

1. add the value to `StatId` **before** `Count` (`stats/stat_id.hpp`);
2. add an entry to `kStatNames` (same file) and to `kStatSpecs`
   (`champions/champion.cpp`), plus the `<stat>` / `<stat>_growth` field pair on
   `ChampionData`.

Both tables `static_assert` against `kStatCount`, so a forgotten entry fails the
build.

### Adding an effect

Flat bonus:

```cpp
champion.apply_effect(flat_effect({.source = "Baron", .name = "Hand of Baron"},
                                  {base_mod(StatId::AttackDamage, 20),
                                   base_mod(StatId::Armor, 30)},
                                  Timed::for_span(champion.now(),
                                                  rate.ticks_from_seconds(180.0))));
```

Depends on another stat — declare it, or `StatView` throws:

```cpp
champion.apply_effect(Effect{
    .key = {.source = "Ahri", .name = "Essence Theft"},
    .reads = {StatId::AttackDamage},
    .writes = {StatId::Armor},
    .policy = StackPolicy::Stack,
    .max_stacks = 5,
    .contribute = [](const EffectContext& ctx, ModifierSink& sink) {
        sink.add_base(StatId::Armor, 0.2 * ctx.stats(StatId::AttackDamage));
    },
});
```

Checklist: is `reads`/`writes` complete? is `contribute` free of side effects?
is the `policy` the intended one? does the `key` match on re-application?

### Adding an event type

New `hpp`/`cpp` pair under `src/events/` declaring a `handle_event` overload, add
the type to the `Event` variant, include the header from `event.cpp`, and
register the `.cpp` in `src/CMakeLists.txt`.

### Adding a source file

Sources are listed explicitly — no globbing. Add to `src/CMakeLists.txt` or
`tests/CMakeLists.txt`, or the file is silently not compiled. Nix builds from the
git tree, so `git add` new files before `nix build` / `nix flake check`.

## Testing strategy

114 Catch2 tests. `catch_discover_tests` registers each `TEST_CASE` as its own
ctest test, so filtering by name works through ctest and filtering by tag
requires the binary directly:

```sh
ctest --test-dir build                    # everything
./build/tests/moba_sim_tests "[effects]"  # by tag
nix run .#tests -- "[effects]"            # by tag, without a local build tree
```

| File | Covers |
|---|---|
| `stat_pipeline.test.cpp` | bucket algebra in isolation |
| `stat_table.test.cpp` | `StatId` derivation, `StatMask`, `StatTable`, modifier routing |
| `stat_breakdown.test.cpp` | provenance through items, effects and stacks |
| `tick.test.cpp` | `Tick`/`TickSpan`/`TickRate`, exactness at several rates |
| `effect.test.cpp` | lifetimes, all six stack policies, ordering, cycles, purity |
| `champion.test.cpp` | growth, levels, equip/unequip, effect survival |
| `champion_effects.test.cpp` | effects over simulated time, end to end |
| `item.test.cpp` | item modifiers and labelling |
| `event.test.cpp` | dispatch and nesting, asserted on exact output |
| `game_loop.test.cpp`, `view_geometry.test.cpp` | loop cadence, geometry |

Conventions worth keeping:

- **Tests must be headless.** SDL-touching tests call
  `setenv("SDL_VIDEO_DRIVER", "dummy", 1)` themselves — the environment is not
  set globally.
- **Real numbers with worked-out expectations.** Champion tests use Ahri's actual
  wiki statistics and spell out the arithmetic in comments
  (`// Level 6 AD: 53 + 3 * 5 = 68`). A test whose expected value cannot be
  derived by hand does not document anything.
- **Test the invariant, not just the result.** "evaluation order is independent of
  application order", "repeating `contribute_all` changes nothing" and "a long
  simulation stays exact tick by tick" would all pass trivially on a broken
  implementation that happened to produce one right number.
- **Exact equality where exactness is claimed.** Tick counts and integral stat
  values are compared with `==`; only genuine floating-point arithmetic uses
  `WithinAbs`.

## Not yet built

Deliberate gaps, in rough dependency order:

- **Combat.** No damage, mitigation, attack timing or death. `EffectContext`
  exposes only the effect's own stats — no target, owner, or other units — so
  on-hit effects, allied auras and "gain a stack per kill" need that context
  widened.
- **Abilities and cooldowns.** No `Ability` type, no resource cost, no cast time.
- **Events connected to the simulation.** `process_event` returns `void`, takes
  only a stream, and no handler can touch a `Champion`. There is no queue and no
  scheduling by tick. Effect expiry already returns keys, which is the natural
  place to start emitting events.
- **Serialisation and replay.** Every design decision so far keeps this possible
  — integral time, semantic keys, lifetimes as data, deterministic ordering — but
  nothing writes state out yet. Note that a `contribute` lambda is not
  serialisable, so replay will need effect *definitions* to live in a registry
  keyed by name, with instances storing only the key and runtime state.
- **Item build paths, cost, unique passives, slot limits.** `Item` is a flat bag
  today; the same item can be equipped twice.
- **Ability power.** `StatId` has no `AbilityPower`, which most real scaling
  needs.
