find_program(GLSLANG_VALIDATOR NAMES glslangValidator REQUIRED)
if(NOT GLSLANG_VALIDATOR)
  message(FATAL_ERROR "Error: `glslangValidator` not found!")
endif()

set(KERNELS
  pbr.glsl
  color.vert
  color.frag
)

set(SHADER_DEFS
  "VERTEX_SHADER"
  "TESSELLATION_CONTROL_SHADER"
  "TESSELLATION_EVALUATION_SHADER"
  "GEOMETRY_SHADER"
  "FRAGMENT_SHADER"
  "COMPUTE_SHADER"
)

set(SHADER_EXTENSIONS
  ".vert"
  ".tesc"
  ".tese"
  ".geom"
  ".frag"
  ".comp"
)

set(COMPILED_KERNELS)

list(LENGTH SHADER_DEFS len1)
math(EXPR len2 "${len1} - 1")

foreach(KERNEL ${KERNELS})
  get_filename_component(EXTENSION ${KERNEL} EXT)
  if("${EXTENSION}" STREQUAL ".glsl")
    # if file's longest extension is .glsl, kernel contains multiple shaders
    # in one file with each shader contained in an #ifdef ${SHADER_DEF} <->
    # #endif block. For each #{SHADER_DEF} found in file, create a new file
    # with appropriate extension and add a #define ${SHADER_DEF) on the second
    # line of file. Then compile each shader individually to SPIR-V format.
    #
    # For example, if the original file "shader.glsl" contains the lines:
    # #ifdef FRAGMENT_SHADER
    # #ifdef VERTEX_SHADER
    #
    # then the following temporary files will be created:
    # shader.vert (with `#define VERTEX_SHADER` on second line)
    # shader.frag (with `#define FRAGMENT_SHADER` on second line)
    #
    # The above two files will then be compiled into SPIR-V format, generating
    # the following two additional files:
    # shader_vert.spv
    # shader_frag.spv
    #
    # The SPIR-V files are then bundled into the shaders.qrc resource.

    file(READ ${KERNEL} TEMP_KERNEL)
    message(STATUS "Processing ${KERNEL}")
    string(REPLACE ".glsl" "" NEW_KERNEL ${KERNEL})
    foreach(val RANGE ${len2})
      list(GET SHADER_DEFS ${val} SHADER_DEF)
      list(GET SHADER_EXTENSIONS ${val} SHADER_EXTENSION)

      if("${TEMP_KERNEL}" MATCHES "^(.*(#ifdef ${SHADER_DEF}).*)$")
        string(REPLACE ".glsl" "" NEW_KERNEL ${KERNEL})
        set(TEMP_FILE ${NEW_KERNEL}${SHADER_EXTENSION})
        string(REPLACE "." "_" NEW_KERNEL ${TEMP_FILE})
        message(STATUS "  Found: ${SHADER_DEF} in ${KERNEL}")

        message(STATUS "  Creating ${TEMP_FILE}")
        add_custom_command(
          OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${TEMP_FILE}"
          COMMAND sed "2i #define ${SHADER_DEF}" ${CMAKE_CURRENT_SOURCE_DIR}/${KERNEL} > ${CMAKE_CURRENT_BINARY_DIR}/${TEMP_FILE}
          DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${KERNEL}
          COMMENT "Generating temp file ${TEMP_FILE}"
        )

        message(STATUS "  Creating ${NEW_KERNEL}.spv")
        add_custom_command(
          OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${NEW_KERNEL}.spv"
          COMMAND glslangValidator ${CMAKE_CURRENT_BINARY_DIR}/${TEMP_FILE} -g -V -o ${CMAKE_CURRENT_BINARY_DIR}/${NEW_KERNEL}.spv
          DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${TEMP_FILE}"
          COMMENT "Rebuilding ${NEW_KERNEL}.spv"
        )

        list(APPEND COMPILED_KERNELS "${CMAKE_CURRENT_BINARY_DIR}/${NEW_KERNEL}.spv")

      endif()
    endforeach()
  else()
    # otherwise, kernel contains only one shader. In this case,
    # compile kernel directly with no modification
    string(REPLACE "." "_" NEW_KERNEL ${KERNEL})
    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${NEW_KERNEL}.spv"
      COMMAND glslangValidator ${CMAKE_CURRENT_SOURCE_DIR}/${KERNEL} -g -V -o ${CMAKE_CURRENT_BINARY_DIR}/${NEW_KERNEL}.spv
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${KERNEL}
      COMMENT "Rebuilding ${NEW_KERNEL}.spv"
    )
    list(APPEND COMPILED_KERNELS "${CMAKE_CURRENT_BINARY_DIR}/${NEW_KERNEL}.spv")
  endif()
endforeach()

add_custom_target(compiled_shaders DEPENDS ${COMPILED_KERNELS})

configure_file(shaders.qrc.in ${CMAKE_CURRENT_BINARY_DIR}/shaders.qrc)

add_library(shaders STATIC
  ${CMAKE_CURRENT_BINARY_DIR}/shaders.qrc
)

add_dependencies(shaders compiled_shaders)
