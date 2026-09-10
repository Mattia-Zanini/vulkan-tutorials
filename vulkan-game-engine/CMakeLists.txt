cmake_minimum_required(VERSION 3.24)

if(APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_NO_SYSTEM_FROM_IMPORTED ON)

project(VulkanGameEngine VERSION 0.1 LANGUAGES C CXX)

find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(unofficial-shaderc CONFIG REQUIRED)
find_package(glm REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

# Aggiungi il percorso delle directory dei file sorgente
# Utilizza GLOB per raccogliere automaticamente i file sorgente
file(GLOB SOURCES CONFIGURE_DEPENDS "src/*.cpp" "src/systems/*.cpp")
file(GLOB HEADERS CONFIGURE_DEPENDS "include/*.hpp" "include/*.h" "include/systems/*.hpp")

add_executable(vulkanengine
	${SOURCES}
	${HEADERS})

# For MSVC, you often need the 'latest' flag specifically
if(MSVC)
    target_compile_options(vulkanengine PRIVATE /std:c++latest)
endif()

target_include_directories(vulkanengine PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/systems"
)

target_link_libraries(vulkanengine PRIVATE
	Vulkan::Vulkan
	unofficial::shaderc::shaderc
	glfw
	glm::glm
	spdlog::spdlog
	fmt::fmt)

# Compile shaders with glslc and place .spv in the build directory
find_program(GLSLC_EXECUTABLE glslc HINTS /usr/local/bin $ENV{VULKAN_SDK}/bin REQUIRED)

file(GLOB SHADER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag"
)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/shaders")

set(SPV_OUTPUTS "")
foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SPV_FILE "${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_NAME}.spv")

    add_custom_command(
        OUTPUT ${SPV_FILE}
        COMMAND ${GLSLC_EXECUTABLE} "${SHADER}" -o "${SPV_FILE}"
        DEPENDS "${SHADER}"
        COMMENT "Compiling shader ${SHADER_NAME} to SPIR-V..."
        VERBATIM
    )
    list(APPEND SPV_OUTPUTS ${SPV_FILE})
endforeach()

add_custom_target(CompileShaders ALL DEPENDS ${SPV_OUTPUTS})
add_dependencies(vulkanengine CompileShaders)

# Sincronizza automaticamente la cartella "models" nella build directory
# copy_directory_if_different copierà i file solo se sono stati modificati o aggiunti
add_custom_target(SyncModels ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/models"
    "${CMAKE_CURRENT_BINARY_DIR}/models"
    COMMENT "Sincronizzazione della cartella models in corso..."
)
add_dependencies(vulkanengine SyncModels)