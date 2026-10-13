# Fake J2534 driver with predictable answers, and a test that drives the real
# device host executable through its named pipe.

add_library(fake_j2534_driver SHARED fake_j2534_driver.cpp fake_j2534_driver.def)
target_include_directories(fake_j2534_driver PRIVATE ${PROJECT_SOURCE_DIR}/j2534_OverIP)

add_executable(device_host_test device_host_test.cpp)
target_link_libraries(device_host_test PRIVATE j2534_device_host_core)

foreach(target fake_j2534_driver device_host_test)
    set_target_properties(${target} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endforeach()

add_test(NAME device_host
    COMMAND device_host_test $<TARGET_FILE:j2534_device_host> $<TARGET_FILE:fake_j2534_driver>)
