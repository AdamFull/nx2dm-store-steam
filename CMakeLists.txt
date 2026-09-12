
option(NX_MODULE_STORE_STEAM "Build the Steam storefront backend" OFF)

include("${CMAKE_CURRENT_SOURCE_DIR}/NxStoreSteam.cmake")

if (NOT NX_MODULE_STORE_STEAM)
    return()
endif ()

# nx_module()'s own PLATFORMS gate runs too late to help here - a project
# that also builds for a platform Steamworks isn't vendored for still
# passes -DNX_MODULE_STORE_STEAM=ON (its module list isn't per-platform),
# so the vendor-SDK resolution below must skip itself first, the same
# "skipped on <platform>" outcome _nx_module_gate would otherwise report -
# just early enough to not FATAL_ERROR chasing a runtime file that was
# never extracted for that platform.
set(_nx_store_steam_platforms windows linux macos)
_nx_current_platform(_nx_store_steam_here)
if (NOT _nx_store_steam_here IN_LIST _nx_store_steam_platforms)
    message(STATUS "nx2d: module 'store_steam' skipped on ${_nx_store_steam_here}")
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
