add_library(moba_sim_core
    champions/champion.cpp
    effects/effect.cpp
    effects/effect_set.cpp
    effects/lifetime.cpp
    events/event.cpp
    events/event_sequence.cpp
    events/key_pressed_event.cpp
    events/player_died_event.cpp
    items/item.cpp
    stats/modifier.cpp
    stats/stat_breakdown.cpp
    stats/stat_pipeline.cpp
)

target_include_directories(moba_sim_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(moba_sim_core PUBLIC project_warnings)

find_package(SDL3 REQUIRED)

add_library(moba_sim_view
    view/game_loop.cpp
    view/renderer2d.cpp
    view/window.cpp
)

target_include_directories(moba_sim_view PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(moba_sim_view PUBLIC project_warnings SDL3::SDL3)

add_executable(moba-sim main.cpp)
target_link_libraries(moba-sim PRIVATE moba_sim_core)

add_executable(moba-sim-view view/demo.cpp)
target_link_libraries(moba-sim-view PRIVATE moba_sim_core moba_sim_view)

install(TARGETS moba-sim moba-sim-view)
