
add_executable(nx_store_steam_tests
        test_store_steam_config.cpp
        test_store_steam_module.cpp
        test_store_steam_services.cpp
        test_store_steam_extras.cpp
        test_store_steam_scripting.cpp
        ../store_steam_module.cpp
        ../store_steam_config.cpp
        ../store_steam_services.cpp
        ../store_steam_workshop.cpp
        ../store_steam_leaderboards.cpp
        ../store_steam_overlay.cpp
        ../store_steam_scripting.cpp)

nx_module_registry(nx_store_steam_tests)

target_include_directories(nx_store_steam_tests PRIVATE
        "${NX_ENGINE_ROOT}/engine" "${NX_ENGINE_ROOT}/modules")

target_link_libraries(nx_store_steam_tests
        PRIVATE nx::test nx::foundation nx::engine nx::steamworks nx::compile_options)

set_target_properties(nx_store_steam_tests PROPERTIES FOLDER "tests")
nxtest_discover_tests(nx_store_steam_tests LABELS store_steam)
