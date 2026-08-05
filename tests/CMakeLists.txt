find_package(Catch2 3 REQUIRED)

add_executable(moba_sim_tests
    champion.test.cpp
    event.test.cpp
    game_loop.test.cpp
    greeting.test.cpp
    item.test.cpp
    stat_breakdown.test.cpp
    stat_pipeline.test.cpp
    view_geometry.test.cpp
)

target_link_libraries(moba_sim_tests
    PRIVATE
        moba_sim_core
        moba_sim_view
        Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(moba_sim_tests)
