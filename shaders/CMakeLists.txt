project(shaders VERSION 1.0.0 DESCRIPTION "Shaders for NovusCore")

set(SHADER_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../Data/shaders CACHE PATH "Output Directory for shaders")

if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
  set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
else()
  set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin32/glslangValidator.exe")
endif()

file(GLOB_RECURSE GLSL_SOURCE_FILES
  "*.geom"
  "*.vert"
  "*.frag"
	"*.comp"
    )

foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${SHADER_OUTPUT}/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_OUTPUT}/"
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -Od -g -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(${PROJECT_NAME}
    DEPENDS ${SPIRV_BINARY_FILES}
    SOURCES ${GLSL_SOURCE_FILES}
    )
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${ROOT_FOLDER}/libs)
    