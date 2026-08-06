# `view/`

The thin SDL3 layer: RAII window, 2D renderer and a fixed-timestep game
loop. Links `SDL3::SDL3`; the core never touches it.

## Geometry

```{doxygenstruct} moba_sim::view::Vec2
:members:
```

```{doxygenfunction} moba_sim::view::length
```

```{doxygenfunction} moba_sim::view::normalized
```

```{doxygenfunction} moba_sim::view::operator*
```

```{doxygenstruct} moba_sim::view::Rect
:members:
```

```{doxygenstruct} moba_sim::view::Color
:members:
```

## Window and loop

```{doxygenclass} moba_sim::view::Window
:members:
```

```{doxygenclass} moba_sim::view::Renderer2D
:members:
```

```{doxygenclass} moba_sim::view::GameLoop
:members:
```
