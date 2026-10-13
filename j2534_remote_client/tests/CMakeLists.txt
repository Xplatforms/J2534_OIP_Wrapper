add_executable(connection_code_test connection_code_test.cpp)
target_link_libraries(connection_code_test PRIVATE j2534_remote_client)

add_executable(remote_e2e_test remote_e2e_test.cpp)
target_link_libraries(remote_e2e_test PRIVATE j2534_remote_client)

foreach(target connection_code_test remote_e2e_test)
    set_target_properties(${target} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endforeach()

add_test(NAME connection_code COMMAND connection_code_test)

# Needs the x64 App (Vehicle mode); skipped when it isn't installed yet.
set(J2534_E2E_APP "${PROJECT_SOURCE_DIR}/dist/x64/bin/j2534_OverIP_App.exe" CACHE FILEPATH
    "App used by remote_e2e_test")
add_test(NAME remote_e2e
    COMMAND remote_e2e_test "${J2534_E2E_APP}" $<TARGET_FILE:j2534_device_host> $<TARGET_FILE:fake_j2534_driver>)
set_tests_properties(remote_e2e PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
