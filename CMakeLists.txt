cmake_minimum_required(VERSION 3.25)

project(J2534_OIP_Wrapper
    VERSION 1.0
    DESCRIPTION "J2534 PassThru driver emulation and Over-IP wrapper"
    LANGUAGES C CXX)

if(NOT WIN32)
    message(FATAL_ERROR "J2534_OIP_Wrapper is Windows-only")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The PassThru DLL is Qt-free and built for x86 (J2534 host apps are 32-bit).
# The Qt App is built for x64; it talks to the DLL over a named pipe using the
# messages in proto/j2534_pipe.proto.
# See CMakePresets.json: preset "x86" builds the DLL, preset "x64" builds the App.
option(J2534_BUILD_DLL "Build the PassThru DLL" ON)
option(J2534_BUILD_APP "Build the Qt configuration/monitor application" ON)
option(J2534_BUILD_TESTS "Build the pipe protocol tests (needs Qt)" ${J2534_BUILD_APP})

enable_testing()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Protoc)

if(J2534_BUILD_DLL OR J2534_BUILD_TESTS)
    include(Nanopb)
endif()

if(J2534_BUILD_APP OR J2534_BUILD_TESTS)
    find_package(Qt6 6.8 REQUIRED COMPONENTS
        Core Gui Quick QuickControls2 Widgets Network WebSockets Protobuf ProtobufTools Test)
    qt_standard_project_setup(REQUIRES 6.8)
endif()

add_subdirectory(proto)

if(J2534_BUILD_DLL)
    add_subdirectory(j2534_remote_client)
    add_subdirectory(j2534_OverIP)
    add_subdirectory(j2534_device_host)
endif()

if(J2534_BUILD_APP)
    add_subdirectory(j2534_OverIP_App)
endif()

if(J2534_BUILD_TESTS)
    add_subdirectory(tests)
endif()
