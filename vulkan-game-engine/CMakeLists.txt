cmake_minimum_required(VERSION 3.24)

if(APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0")
endif()

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

project(VulkanLearning VERSION 0.1 LANGUAGES C CXX)

find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(unofficial-shaderc CONFIG REQUIRED)
find_package(glm REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_executable(vulkanapp
	"main.cpp")

# For MSVC, you often need the 'latest' flag specifically
if(MSVC)
    target_compile_options(vulkanapp PRIVATE /std:c++latest)
endif()

target_link_libraries(vulkanapp PRIVATE
	Vulkan::Vulkan
	unofficial::shaderc::shaderc
	glfw
	glm::glm
	spdlog::spdlog
	fmt::fmt)
