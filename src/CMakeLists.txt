add_library(moba_sim_core
    champions/champion.cpp
    core/greeting.cpp
    events/event.cpp
    events/event_sequence.cpp
    events/key_pressed_event.cpp
    events/player_died_event.cpp
    items/item.cpp
    stats/stat_pipeline.cpp
)

target_include_directories(moba_sim_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(moba_sim_core PUBLIC project_warnings)

add_executable(moba-sim main.cpp)
target_link_libraries(moba-sim PRIVATE moba_sim_core)

install(TARGETS moba-sim)
