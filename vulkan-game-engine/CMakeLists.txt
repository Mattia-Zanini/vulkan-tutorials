cmake_minimum_required(VERSION 3.24)

if(APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

project(VulkanGameEngine VERSION 0.1 LANGUAGES C CXX)

find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(unofficial-shaderc CONFIG REQUIRED)
find_package(glm REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

# Aggiungi il percorso delle directory dei file sorgente
# Utilizza GLOB per raccogliere automaticamente i file sorgente
file(GLOB SOURCES CONFIGURE_DEPENDS "*.cpp")
file(GLOB HEADERS CONFIGURE_DEPENDS "*.hpp")

add_executable(vulkanengine
	${SOURCES}
	${HEADERS})

# For MSVC, you often need the 'latest' flag specifically
if(MSVC)
    target_compile_options(vulkanengine PRIVATE /std:c++latest)
endif()

target_link_libraries(vulkanengine PRIVATE
	Vulkan::Vulkan
	unofficial::shaderc::shaderc
	glfw
	glm::glm
	spdlog::spdlog
	fmt::fmt)

# Copy shaders folder to the build directory every time the project is built
add_custom_target(CopyShaders ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders"
        "${CMAKE_CURRENT_BINARY_DIR}/shaders"
    COMMENT "Copying shaders to build directory..."
)
add_dependencies(vulkanengine CopyShaders)