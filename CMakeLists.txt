cmake_minimum_required(VERSION 3.26)
project(VulkanPlayground)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanHeaders CONFIG)

add_executable(vulkan-playground src/main.cpp)

target_link_libraries(vulkan-playground
	PRIVATE
		glfw	
		glm::glm
		Vulkan::Vulkan
		Vulkan::Headers
)

set_target_properties(vulkan-playground PROPERTIES CXX_STANDARD 23)