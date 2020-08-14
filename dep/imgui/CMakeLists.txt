project(imgui VERSION 3.2.2)


find_package(Vulkan REQUIRED) 

set(imgui_folder "${CMAKE_CURRENT_LIST_DIR}/imgui")

set(imgui_sources
	"${imgui_folder}/imgui.cpp"
	"${imgui_folder}/imgui_demo.cpp"
	"${imgui_folder}/imgui_draw.cpp"   
	"${imgui_folder}/imgui_widgets.cpp"
	"${imgui_folder}/imgui_impl_vulkan.cpp"
	"${imgui_folder}/imgui_impl_glfw.cpp"
	"${imgui_folder}/misc/cpp/imgui_stdlib.cpp"
	)

add_library(${PROJECT_NAME} STATIC ${imgui_sources})


set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${ROOT_FOLDER}/dep)
target_link_libraries(${PROJECT_NAME} 
	glfw ${GLFW_LIBRARIES}
	Vulkan::Vulkan
	)

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/imgui")

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_LIST_DIR}")