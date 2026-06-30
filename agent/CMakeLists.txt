cmake_minimum_required(VERSION 3.16)
project(vdt-agent VERSION 1.0.0 LANGUAGES CXX)

# Cau hinh C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall -Wextra -Wpedantic)

# Thu vien ben ngoai
include(FetchContent)

# yaml-cpp: Doc file cau hinh YAML
FetchContent_Declare(yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG yaml-cpp-0.9.0)
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
FetchContent_MakeAvailable(yaml-cpp)

# nlohmann_json: Thu vien JSON
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3)
FetchContent_MakeAvailable(json)

# libcurl: Thu vien goi HTTP Request
find_package(CURL REQUIRED)

# Danh sach file source
set(AGENT_SOURCES
    src/main.cpp
    src/logger.cpp
    src/config.cpp
    src/event.cpp
    src/event_serializer.cpp
    src/network_client.cpp)

# Build chuong trinh
add_executable(vdt-agent ${AGENT_SOURCES})
target_include_directories(vdt-agent PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(vdt-agent PRIVATE yaml-cpp::yaml-cpp nlohmann_json::nlohmann_json CURL::libcurl)