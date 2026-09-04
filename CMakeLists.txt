cmake_minimum_required(VERSION 3.28)
project(PrimeVisualizer CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# Raylib 5.0
include(FetchContent)
FetchContent_Declare(raylib URL https://github.com/raysan5/raylib/archive/refs/tags/6.0.tar.gz)
FetchContent_MakeAvailable(raylib)
# Raygui 4.0
FetchContent_Declare(raygui URL https://github.com/raysan5/raygui/archive/refs/tags/4.0.tar.gz)
FetchContent_MakeAvailable(raygui)
#find_package(raylib REQUIRED)
#find_package(raygui REQUIRED)

# Fetch .cppm files
file(GLOB_RECURSE MODULE_FILES CONFIGURE_DEPENDS "src/modules/*.cppm")

# Fetch implementations
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "src/*.cpp")

# Creates executable with .cpp files
add_executable(PrimeVis ${SOURCE_FILES})

# Declare modules dynamically
target_sources(PrimeVis
    PUBLIC
      FILE_SET CXX_MODULES TYPE CXX_MODULES
      BASE_DIRS src/modules
      FILES ${MODULE_FILES}
)

target_link_libraries(PrimeVis PRIVATE raylib)
target_include_directories(PrimeVis SYSTEM PRIVATE ${raygui_SOURCE_DIR}/src)
