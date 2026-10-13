set(J2534_PIPE_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/j2534_pipe.proto")
set(J2534_REMOTE_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/j2534_remote.proto")

# ---------------------------------------------------------------------------
# nanopb side (Qt-free): PassThru DLL, device host and the tests

if(J2534_BUILD_DLL OR J2534_BUILD_TESTS)
    set(_nanopb_out "${CMAKE_CURRENT_BINARY_DIR}/nanopb")
    nanopb_generate(_pipe_generated "${J2534_PIPE_PROTO}" "${_nanopb_out}")
    nanopb_generate(_remote_generated "${J2534_REMOTE_PROTO}" "${_nanopb_out}")
    # One generation step shared by every library variant below.
    add_custom_target(j2534_nanopb_generate DEPENDS ${_pipe_generated} ${_remote_generated})

    # j2534_add_proto_libraries(<suffix> <msvc runtime>)
    # Creates j2534_nanopb<suffix>, j2534_pipe_builder<suffix> and
    # j2534_remote_proto<suffix>. The DLL and device host need the static CRT,
    # the Qt tests the dynamic one.
    function(j2534_add_proto_libraries suffix runtime)
        add_library(j2534_nanopb${suffix} STATIC
            ${NANOPB_SOURCE_DIR}/pb_common.c
            ${NANOPB_SOURCE_DIR}/pb_encode.c
            ${NANOPB_SOURCE_DIR}/pb_decode.c)
        target_include_directories(j2534_nanopb${suffix} PUBLIC ${NANOPB_SOURCE_DIR})
        # Remote frames are larger than 64 KB.
        target_compile_definitions(j2534_nanopb${suffix} PUBLIC PB_FIELD_32BIT)

        add_library(j2534_pipe_builder${suffix} STATIC
            pipe_message_builder.cpp
            pipe_message_builder.h
            ${_pipe_generated})

        add_library(j2534_remote_proto${suffix} STATIC
            remote_frame.cpp
            remote_frame.h
            ${_remote_generated})

        foreach(target j2534_nanopb${suffix} j2534_pipe_builder${suffix} j2534_remote_proto${suffix})
            add_dependencies(${target} j2534_nanopb_generate)
            set_target_properties(${target} PROPERTIES MSVC_RUNTIME_LIBRARY "${runtime}")
        endforeach()

        foreach(target j2534_pipe_builder${suffix} j2534_remote_proto${suffix})
            target_include_directories(${target} PUBLIC
                ${CMAKE_CURRENT_SOURCE_DIR}
                ${_nanopb_out}
                ${PROJECT_SOURCE_DIR}/j2534_OverIP)   # PassThruStruct.h
            target_link_libraries(${target} PUBLIC j2534_nanopb${suffix})
        endforeach()
    endfunction()

    if(J2534_BUILD_DLL)
        j2534_add_proto_libraries(_static_crt "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
    if(J2534_BUILD_TESTS)
        j2534_add_proto_libraries("" "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    endif()
endif()

# ---------------------------------------------------------------------------
# QtProtobuf side: App and the tests

if(J2534_BUILD_APP OR J2534_BUILD_TESTS)
    qt_add_library(j2534_pipe_qt STATIC)
    qt_add_protobuf(j2534_pipe_qt
        PROTO_FILES ${J2534_PIPE_PROTO} ${J2534_REMOTE_PROTO}
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qt")
    target_link_libraries(j2534_pipe_qt PUBLIC Qt6::Protobuf)
endif()
