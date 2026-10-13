# Vehicle PC side of the remote link: loads the real (32-bit) J2534 driver and
# executes calls received from the App over a named pipe. Built in the x86
# preset next to the PassThru DLL.

add_library(j2534_device_host_core STATIC
    device_host.cpp device_host.h
    driver_api.cpp driver_api.h
    pipe_frames.cpp pipe_frames.h)
target_include_directories(j2534_device_host_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/j2534_OverIP)
target_compile_definitions(j2534_device_host_core PUBLIC UNICODE _UNICODE)
target_link_libraries(j2534_device_host_core PUBLIC j2534_remote_proto_static_crt)

add_executable(j2534_device_host main.cpp)
target_link_libraries(j2534_device_host PRIVATE j2534_device_host_core winmm)

foreach(target j2534_device_host_core j2534_device_host)
    set_target_properties(${target} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endforeach()

install(TARGETS j2534_device_host RUNTIME DESTINATION .)

add_subdirectory(tests)
