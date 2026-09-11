
# The Steamworks SDK (Steam Partner license) is a licence-gated download from
# partner.steamgames.com - there is no fetchable URL, so it stays a required
# local, gitignored, developer-provided directory, the same shape
# modules/live2d/NxLive2D.cmake already established for a big vendor SDK
# this repo cannot vendor itself.

set(NX_STORE_STEAM_SDK_DIR "" CACHE PATH
        "An extracted Steamworks SDK (its own 'sdk' directory, or that directory itself). Empty uses modules/store_steam/third_party/SteamSDK.")

function(nx_add_steamworks)
    if (NX_STORE_STEAM_SDK_DIR)
        set(_search "${NX_STORE_STEAM_SDK_DIR}")
    else ()
        set(_search "${CMAKE_CURRENT_LIST_DIR}/third_party/SteamSDK")
    endif ()

    _nx_resolve_vendor_root("${_search}/sdk;${_search}" "public/steam/steam_api.h" _root)
    if (NOT _root)
        message(FATAL_ERROR
                "nx2d: no Steamworks SDK. Download it (partner account "
                "required) from https://partner.steamgames.com/downloads/list, "
                "then extract it to modules/store_steam/third_party/SteamSDK "
                "(or point NX_STORE_STEAM_SDK_DIR at it). It is not fetchable "
                "here: Valve distributes it only to registered partners. Never "
                "commit it - modules/store_steam/third_party/SteamSDK is "
                "gitignored on purpose.")
    endif ()

    set(_bin "${_root}/redistributable_bin")
    set(_implib "")
    if (WIN32)
        if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
            message(FATAL_ERROR "nx2d: Steamworks 165 ships 64-bit Windows only")
        endif ()
        set(_runtime "${_bin}/win64/steam_api64.dll")
        set(_implib "${_bin}/win64/steam_api64.lib")
    elseif (APPLE AND NOT IOS)
        set(_runtime "${_bin}/osx/libsteam_api.dylib")
    elseif (ANDROID)
        set(_runtime "${_bin}/androidarm64/libsteam_api.so")
    elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set(_runtime "${_bin}/linuxarm64/libsteam_api.so")
    else ()
        set(_runtime "${_bin}/linux64/libsteam_api.so")
    endif ()

    set(_args RUNTIME "${_runtime}")
    if (_implib)
        list(APPEND _args IMPLIB "${_implib}")
    endif ()
    _nx_imported_shared_library(nx_steamworks ${_args})
    add_library(nx::steamworks ALIAS nx_steamworks)
    set_target_properties(nx_steamworks PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${_root}/public")

    message(STATUS "nx2d: Steamworks SDK from ${_root}")
endfunction()
