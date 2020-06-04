project(render VERSION 1.0.0 DESCRIPTION "Render Library")

find_package(Vulkan REQUIRED)
file(GLOB_RECURSE RENDER_LIB_FILES "*.cpp" "*.h")

add_library(${PROJECT_NAME} ${RENDER_LIB_FILES})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${ROOT_FOLDER}/libs)

find_assign_files(${RENDER_LIB_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE Vulkan::Vulkan)
target_link_libraries(${PROJECT_NAME} PUBLIC
	asio::asio
	common::common
	glfw ${GLFW_LIBRARIES}
    Vulkan::Vulkan
    gli::gli
)
add_dependencies(${PROJECT_NAME} shaders)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMENT "Compiling shaders..."
	COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${SHADER_OUTPUT}"
        "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/shaders/"
        )

add_compile_definitions(NOMINMAX _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS VK_USE_PLATFORM_WIN32_KHR)
