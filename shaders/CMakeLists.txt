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
    )

file(GLOB_RECURSE HLSL_INCLUDE_FILES
  "*.inc.hlsl"
    )

foreach(HLSL ${HLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${HLSL} NAME)

  file(RELATIVE_PATH rel ${SHADER_SOURCE_DIR} ${HLSL})

  set(SHADER_REL_PATH ${rel})
  set(SPIRV "${SHADER_BIN_DIR}/${SHADER_REL_PATH}.spv")
  
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_BIN_DIR}/"
    COMMAND ${SHADER_COOKER_STANDALONE} ${SHADER_REL_PATH} ${SHADER_SOURCE_DIR} ${SHADER_BIN_DIR}
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