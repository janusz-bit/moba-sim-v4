find_package(Catch2 3 REQUIRED)

add_executable(moba_sim_tests
    champion.test.cpp
    event.test.cpp
    greeting.test.cpp
    item.test.cpp
    stat_pipeline.test.cpp
)

target_link_libraries(moba_sim_tests
    PRIVATE
        moba_sim_core
        Catch2::Catch2WithMain
)

include(Catch)
catch_discover_tests(moba_sim_tests)
