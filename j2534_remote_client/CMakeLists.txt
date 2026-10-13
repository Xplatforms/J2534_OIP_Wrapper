# Diagnostic PC side of the remote link (J2534Remote interface): WinHTTP
# WebSocket client, session and the PassThru mapping. Linked into the PassThru
# DLL and the end-to-end test.

add_library(j2534_remote_client STATIC
    connection_code.cpp connection_code.h
    websocket_client.cpp websocket_client.h
    remote_session.cpp remote_session.h
    j2534remoteclient.cpp j2534remoteclient.h)
target_include_directories(j2534_remote_client PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/j2534_OverIP)
# j2534.h declares the DLL's exports only when this is defined.
target_compile_definitions(j2534_remote_client PUBLIC J2534OVERIP_LIBRARY UNICODE _UNICODE NOMINMAX)
target_link_libraries(j2534_remote_client PUBLIC j2534_remote_proto_static_crt winhttp)
set_target_properties(j2534_remote_client PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_subdirectory(tests)
