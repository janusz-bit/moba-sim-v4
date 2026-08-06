# `stats/`

Stat identifiers, source-neutral modifiers and the three-bucket pipeline:
`result = sum(Base) * (1 + sum(Inc)) * product(1 + More)`.

## `StatId`

```{doxygenenum} moba_sim::StatId
```

```{doxygenvariable} moba_sim::kStatCount
```

```{doxygenvariable} moba_sim::kAllStats
```

```{doxygenvariable} moba_sim::kStatNames
```

```{doxygenfunction} moba_sim::stat_index
```

```{doxygenfunction} moba_sim::stat_name
```

## Modifiers

```{doxygenenum} moba_sim::ModifierKind
```

```{doxygenstruct} moba_sim::Modifier
:members:
```

```{doxygenfunction} moba_sim::base_mod
```

```{doxygenfunction} moba_sim::inc_mod
```

```{doxygenfunction} moba_sim::more_mod
```

```{doxygenfunction} moba_sim::apply_modifier(StatPipeline &pipe, const Modifier &mod)
```

```{doxygenfunction} moba_sim::apply_modifier(StatPipeline &pipe, const Modifier &mod, const std::string &source)
```

## Pipeline

```{doxygenclass} moba_sim::StatPipeline
:members:
```

```{doxygenclass} moba_sim::StatTable
:members:
```

```{doxygenstruct} moba_sim::StatBreakdown
:members:
```

```{doxygenfunction} moba_sim::format_breakdown
```

## Dependency masks

```{doxygenclass} moba_sim::StatMask
:members:
```
