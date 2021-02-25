project(shaders VERSION 1.0.0 DESCRIPTION "Shaders for NovusCore")
 
set(SHADER_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/../Data/shaders CACHE PATH "Output Directory for shaders")
set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
 
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
  "*.inc.hlsl"
    )

foreach(HLSL ${HLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${HLSL} NAME)
  set_source_files_properties(${HLSL} PROPERTIES VS_TOOL_OVERRIDE "None")
endforeach(HLSL)

add_custom_target(shaders ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_BIN_DIR}/"
    COMMAND ${SHADER_COOKER_STANDALONE} ${SHADER_SOURCE_DIR} ${SHADER_BIN_DIR}
    SOURCES ${HLSL_SOURCE_FILES}
)
 
find_assign_files(${HLSL_SOURCE_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${ROOT_FOLDER}/)
 
add_dependencies(shaders shadercookerstandalone)