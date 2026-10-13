# nanopb: small C protobuf runtime for the Qt-free PassThru DLL.
include(FetchContent)

FetchContent_Declare(nanopb
    GIT_REPOSITORY https://github.com/nanopb/nanopb.git
    GIT_TAG        160d4f09e5fabb2b66aa2dea32d4f38ace2c4b3f # nanopb-0.4.9.2
    SOURCE_SUBDIR  _no_cmake_)
FetchContent_MakeAvailable(nanopb)

set(NANOPB_SOURCE_DIR "${nanopb_SOURCE_DIR}")

# The generator is a Python script that needs the `protobuf` package.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
execute_process(
    COMMAND "${Python3_EXECUTABLE}" -c "import google.protobuf"
    RESULT_VARIABLE _nanopb_python_check
    OUTPUT_QUIET ERROR_QUIET)
if(NOT _nanopb_python_check EQUAL 0)
    message(FATAL_ERROR
        "The nanopb generator needs the Python protobuf package: "
        "${Python3_EXECUTABLE} -m pip install protobuf")
endif()

# nanopb_generate(<out_var> <proto_file> <output_dir>)
# Runs protoc to produce a descriptor set, then the nanopb generator on it.
# This avoids nanopb looking for protoc on PATH.
function(nanopb_generate out_var proto_file output_dir)
    get_filename_component(proto_name "${proto_file}" NAME_WE)
    get_filename_component(proto_dir "${proto_file}" DIRECTORY)
    get_filename_component(_protoc_dir "${PROTOC_EXECUTABLE}" DIRECTORY)
    file(TO_NATIVE_PATH "${_protoc_dir}" _protoc_dir)
    set(descriptor "${output_dir}/${proto_name}.pb")
    set(generated "${output_dir}/${proto_name}.pb.c" "${output_dir}/${proto_name}.pb.h")

    # Size limits etc. from <name>.options next to the .proto, if present.
    set(options_args "")
    set(options_file "${proto_dir}/${proto_name}.options")
    if(EXISTS "${options_file}")
        set(options_args --options-file "${options_file}")
    endif()

    add_custom_command(
        OUTPUT ${generated}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${output_dir}"
        COMMAND "${PROTOC_EXECUTABLE}"
                "--proto_path=${proto_dir}"
                "--descriptor_set_out=${descriptor}"
                "${proto_file}"
        # The generator also runs protoc itself (to build nanopb_pb2.py), and
        # only looks for it on PATH.
        COMMAND ${CMAKE_COMMAND} -E env
                "--modify" "PATH=path_list_prepend:${_protoc_dir}"
                "${Python3_EXECUTABLE}"
                "${NANOPB_SOURCE_DIR}/generator/nanopb_generator.py"
                --quiet
                --output-dir "${output_dir}"
                ${options_args}
                "${descriptor}"
        DEPENDS "${proto_file}" $<$<BOOL:${options_args}>:${options_file}>
        COMMENT "nanopb: generating ${proto_name}.pb.c/.pb.h"
        VERBATIM)

    set(${out_var} ${generated} PARENT_SCOPE)
endfunction()
