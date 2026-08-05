find_package(Catch2 3 REQUIRED)

add_executable(moba_sim_tests
    champion.test.cpp
    champion_effects.test.cpp
    effect.test.cpp
    event.test.cpp
    game_loop.test.cpp
    item.test.cpp
    stat_breakdown.test.cpp
    stat_pipeline.test.cpp
    stat_table.test.cpp
    tick.test.cpp
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

# Installed under its own component, excluded from the default install: the
# release output stays free of test binaries, while `cmake --install build
# --component tests --prefix <dir>` can place it somewhere (the Nix `tests`
# output) without building the project a second time.
#
# DESTINATION is spelled out as a relative path on purpose. It would otherwise
# default to CMAKE_INSTALL_BINDIR, which nixpkgs' cmake hook sets to an
# absolute path under $out -- and an absolute install dir silently ignores
# --prefix.
install(TARGETS moba_sim_tests
    RUNTIME
        DESTINATION bin
        COMPONENT tests
        EXCLUDE_FROM_ALL
)
