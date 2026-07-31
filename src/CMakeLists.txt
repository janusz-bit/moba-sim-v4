add_library(moba_sim_core
    core/greeting.cpp
    events/event.cpp
)

target_include_directories(moba_sim_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(moba_sim_core PUBLIC project_warnings)

add_executable(moba-sim main.cpp)
target_link_libraries(moba-sim PRIVATE moba_sim_core)

install(TARGETS moba-sim)
