# `events/`

A `std::variant` of event types with recursive nesting via `EventSequence`.

```{doxygentypedef} moba_sim::Event
```

```{doxygenstruct} moba_sim::KeyPressedEvent
:members:
```

```{doxygenstruct} moba_sim::PlayerDiedEvent
:members:
```

```{doxygenstruct} moba_sim::EventSequence
:members:
```

```{doxygenfunction} moba_sim::handle_event(const KeyPressedEvent &event, std::ostream &debug_out)
```

```{doxygenfunction} moba_sim::handle_event(const PlayerDiedEvent &event, std::ostream &debug_out)
```

```{doxygenfunction} moba_sim::handle_event(const std::shared_ptr< EventSequence > &sequence, std::ostream &debug_out)
```

```{doxygenfunction} moba_sim::process_event
```
