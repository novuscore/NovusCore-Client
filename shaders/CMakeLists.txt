project(shaders VERSION 1.0.0 DESCRIPTION "Shaders for NovusCore")
 
set(SHADER_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/../Data/shaders CACHE PATH "Output Directory for shaders")
 
#Handle GLSL shaders TODO: Enable this again after making the shader cooker support GLSL (if we ever need it idk)
#if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
#  set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
#else()
#  set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin32/glslangValidator.exe")
#endif()
#
#file(GLOB_RECURSE GLSL_SOURCE_FILES
#  "*.geom"
#  "*.vert"
#  "*.frag"
#   "*.comp"
#    )
#
#foreach(GLSL ${GLSL_SOURCE_FILES})
#  get_filename_component(FILE_NAME ${GLSL} NAME)
#  set(SPIRV "${SHADER_OUTPUT}/${FILE_NAME}.spv")
#  add_custom_command(
#    OUTPUT ${SPIRV}
#    COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_OUTPUT}/"
#    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -Od -g -o ${SPIRV}
#    DEPENDS ${GLSL})
#  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
#endforeach(GLSL)
#
#find_assign_files(${GLSL_SOURCE_FILES})
 
#Handle HLSL shaders
set(SHADER_COOKER_STANDALONE "$<TARGET_FILE:shadercookerstandalone>")
 
file(GLOB_RECURSE HLSL_SOURCE_FILES
  "*.ps.hlsl"
  "*.vs.hlsl"
  "*.gs.hlsl"
  "*.hs.hlsl"
  "*.ds.hlsl"
  "*.cs.hlsl"
  "*.lib.hlsl"
  "*.ms.hlsl"
  "*.as.hlsl"
    )

file(GLOB_RECURSE HLSL_INCLUDE_FILES
  "*.inc.hlsl"
    )

foreach(HLSL ${HLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${HLSL} NAME)
  set(SPIRV "${SHADER_OUTPUT}/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_OUTPUT}/"
    COMMAND ${SHADER_COOKER_STANDALONE} ${HLSL} ${SPIRV} ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${HLSL})
    set_source_files_properties(${HLSL} PROPERTIES VS_TOOL_OVERRIDE "None")
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(HLSL)
 
foreach(HLSL ${HLSL_INCLUDE_FILES})
  get_filename_component(FILE_NAME ${HLSL} NAME)
  set_source_files_properties(${HLSL} PROPERTIES VS_TOOL_OVERRIDE "None")
endforeach(HLSL)
 
find_assign_files(${HLSL_SOURCE_FILES} ${HLSL_INCLUDE_FILES})

add_custom_target(${PROJECT_NAME}
    DEPENDS ${SPIRV_BINARY_FILES}
    SOURCES ${HLSL_SOURCE_FILES} ${HLSL_INCLUDE_FILES}
    )
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${ROOT_FOLDER}/)
 
add_dependencies(shaders shadercookerstandalone)