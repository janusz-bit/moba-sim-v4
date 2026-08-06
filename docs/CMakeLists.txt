# API documentation, generated from the `///` comments in the headers.
#
# Optional by design: the target only exists when Doxygen is installed, so a
# plain `cmake --build build` never depends on it. Enable explicitly with
# -DMOBA_SIM_BUILD_DOCS=ON, or use the Nix app (`nix run .#docs`), which brings
# its own toolchain.

find_package(Doxygen OPTIONAL_COMPONENTS dot)

if(NOT DOXYGEN_FOUND)
    if(MOBA_SIM_BUILD_DOCS)
        message(FATAL_ERROR
            "MOBA_SIM_BUILD_DOCS=ON but Doxygen was not found. "
            "Install it, or use `nix run .#docs`."
        )
    endif()
    message(STATUS "Doxygen not found - the `docs` target is unavailable")
    return()
endif()

set(DOXYGEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(DOXYGEN_INPUT_SRC "${PROJECT_SOURCE_DIR}/src")
set(DOXYGEN_INPUT_README "${PROJECT_SOURCE_DIR}/README.md")
set(DOXYGEN_STRIP_PATH "${PROJECT_SOURCE_DIR}")

# Both prose documents, so cross-links between them and the README resolve.
# Quoted individually and joined with spaces, since Doxygen expects a
# whitespace-separated list rather than CMake's semicolons.
set(DOXYGEN_INPUT_DOCS
    "\"${CMAKE_CURRENT_SOURCE_DIR}/architecture.md\" \"${PROJECT_SOURCE_DIR}/AGENTS.md\""
)

if(TARGET Doxygen::dot)
    set(DOXYGEN_HAVE_DOT YES)
else()
    set(DOXYGEN_HAVE_DOT NO)
    message(STATUS "graphviz (dot) not found - documentation will omit diagrams")
endif()

# Undocumented public API is a warning by default and an error on demand, the
# same split the C++ warnings use (MOBA_SIM_WARNINGS_AS_ERRORS). Keeping it off
# by default means adding a header never blocks a local build; CI turns it on.
option(MOBA_SIM_DOCS_WARNINGS_AS_ERRORS
    "Fail documentation generation on Doxygen warnings" OFF
)
if(MOBA_SIM_DOCS_WARNINGS_AS_ERRORS)
    set(DOXYGEN_WARN_AS_ERROR FAIL_ON_WARNINGS)
else()
    set(DOXYGEN_WARN_AS_ERROR NO)
endif()

set(doxyfile "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile")
configure_file(Doxyfile.in "${doxyfile}" @ONLY)

# Depends on the headers themselves, so `cmake --build build --target docs`
# regenerates after an edit but is a no-op when nothing changed.
file(GLOB_RECURSE doc_sources CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/src/*.hpp"
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

add_custom_command(
    OUTPUT "${DOXYGEN_OUTPUT_DIR}/html/index.html"
    COMMAND Doxygen::doxygen "${doxyfile}"
    DEPENDS "${doxyfile}" ${doc_sources} "${DOXYGEN_INPUT_README}"
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
)

add_custom_target(docs
    DEPENDS "${DOXYGEN_OUTPUT_DIR}/html/index.html"
)

if(MOBA_SIM_BUILD_DOCS)
    set_target_properties(docs PROPERTIES EXCLUDE_FROM_ALL FALSE)
    install(
        DIRECTORY "${DOXYGEN_OUTPUT_DIR}/html/"
        DESTINATION share/doc/moba-sim/html
        COMPONENT docs
        EXCLUDE_FROM_ALL
    )
endif()
