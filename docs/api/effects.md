# `effects/`

Buffs, debuffs, auras and item passives. Identity, dependencies, lifetime and
stacking are data; only the arithmetic is a callable.

## Lifetime

```{doxygenstruct} moba_sim::Permanent
:members:
```

```{doxygenstruct} moba_sim::Timed
:members:
```

```{doxygenstruct} moba_sim::OneShot
:members:
```

```{doxygenstruct} moba_sim::Until
:members:
```

```{doxygentypedef} moba_sim::Lifetime
```

```{doxygenfunction} moba_sim::is_alive
```

```{doxygenfunction} moba_sim::remaining
```

## The effect itself

```{doxygenstruct} moba_sim::EffectKey
:members:
```

```{doxygenstruct} moba_sim::Effect
:members:
```

```{doxygenstruct} moba_sim::EffectContext
:members:
```

```{doxygenstruct} moba_sim::EffectInstance
:members:
```

```{doxygenfunction} moba_sim::flat_effect
```

## Stacking and handles

```{doxygenenum} moba_sim::StackPolicy
```

```{doxygentypedef} moba_sim::StackCount
```

```{doxygenenum} moba_sim::EffectHandle
```

## The set

```{doxygenclass} moba_sim::EffectSet
:members:
```

```{doxygenclass} moba_sim::StatView
:members:
```

```{doxygenclass} moba_sim::ModifierSink
:members:
```

```{doxygenclass} moba_sim::UndeclaredStatAccess
:members:
```

```{doxygenclass} moba_sim::EffectCycleError
:members:
```
