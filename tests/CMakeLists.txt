set(IOR_GAME_LIBRARY "" CACHE FILEPATH "Independent game3 library for reload integration tests")

foreach(kind IN ITEMS bad_abi missing_entry failed_attach bad_layout bad_runtime lifecycle)
    add_library(game_${kind} SHARED game_fixture.cpp)
    target_compile_definitions(game_${kind} PRIVATE IOR_TEST_${kind})
    target_include_directories(game_${kind} PRIVATE ${PROJECT_SOURCE_DIR}/src/game ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(game_${kind} PRIVATE flecs::flecs cglm)
endforeach()

add_executable(game_reload_test game_reload_test.cpp)
target_link_libraries(game_reload_test PRIVATE ior_engine::game flecs::flecs)

if(IOR_GAME_LIBRARY)
    add_test(NAME game_reload COMMAND game_reload_test
        "${IOR_GAME_LIBRARY}"
        $<TARGET_FILE:game_bad_abi>
        $<TARGET_FILE:game_missing_entry>
        $<TARGET_FILE:game_failed_attach>
        $<TARGET_FILE:game_bad_layout>
        $<TARGET_FILE:game_bad_runtime>
        $<TARGET_FILE:game_lifecycle>)
    set_tests_properties(game_reload PROPERTIES TIMEOUT 30)
endif()
