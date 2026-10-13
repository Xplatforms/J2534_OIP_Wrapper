add_library(j2534_OverIP SHARED
    exj2534device.cpp
    exj2534oipsocks.cpp
    expipeclient.cpp
    j2534.cpp
    j2534cannelloniclient.cpp
    j2534dllwrapper.cpp
    j2534emulator.cpp
    j2534global.cpp
    j2534simplelogger.cpp
    main.cpp
    ExDbg.cpp

    Exj2534WrapperInterface.h
    PassThruSpecial.h
    PassThruStruct.h
    exj2534device.h
    exj2534oipsocks.h
    expipeclient.h
    j2534_OverIP_global.h
    j2534.h
    j2534_defs.h
    j2534_dev_typedefs.h
    ExDbg.h
    j2534cannelloniclient.h
    j2534dllwrapper.h
    j2534emulator.h
    j2534global.h
    j2534simplelogger.h
    timestamp_util.h

    exports.def)

target_include_directories(j2534_OverIP PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(j2534_OverIP PRIVATE J2534OVERIP_LIBRARY UNICODE _UNICODE)
target_link_libraries(j2534_OverIP PRIVATE j2534_pipe_builder_static_crt j2534_remote_client ws2_32 advapi32 kernel32)

# Loaded into third-party diagnostic tools: no Qt, no VC++ redist requirement.
set_target_properties(j2534_OverIP PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

install(TARGETS j2534_OverIP RUNTIME DESTINATION .)
