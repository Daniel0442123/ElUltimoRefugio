# Dependencias con versiones fijas mediante FetchContent (ADR-0002, ADR-0014).
include(FetchContent)
set(FETCHCONTENT_QUIET ON)

if(REFUGIO_BUILD_GAME)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    set(CUSTOMIZE_BUILD OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG        5.5
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(raylib)

    # raygui se agregará cuando se implemente el panel de monitoreo (RF-070).
endif()

if(REFUGIO_BUILD_TESTS)
    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.7.1
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(Catch2)
endif()
