
option(NX_MODULE_STORE_STEAM "Build the Steam storefront backend" OFF)

include("${CMAKE_CURRENT_SOURCE_DIR}/NxStoreSteam.cmake")

if (NOT NX_MODULE_STORE_STEAM)
    return()
endif ()

nx_add_steamworks()

nx_module(store_steam
        DISPLAY "Steam storefront backend"
        SOURCES store_steam_module.cpp store_steam_config.cpp
                store_steam_services.cpp
                store_steam_workshop.cpp store_steam_leaderboards.cpp
                store_steam_overlay.cpp store_steam_scripting.cpp
        DEPENDS nx::steamworks nx_module_store
        PLATFORMS windows linux macos)

if (NX_BUILD_TESTS)
    add_subdirectory(tests)
endif ()
