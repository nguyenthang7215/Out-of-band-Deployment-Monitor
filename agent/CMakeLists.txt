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

# Google Test (GTest)
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# libcurl: Thu vien goi HTTP Request
find_package(CURL REQUIRED)

# Danh sach file source cua Core (Khong chua main.cpp)
set(AGENT_CORE_SOURCES
    src/logger.cpp
    src/config.cpp
    src/event.cpp
    src/event_serializer.cpp
    src/network_client.cpp
    src/event_queue.cpp
    src/session_tracker.cpp
    src/audit_watcher.cpp)

# Tao thu vien tinh core de tai su dung cho ca vdt-agent va cac file tests
add_library(vdt_agent_core STATIC ${AGENT_CORE_SOURCES})
target_include_directories(vdt_agent_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(vdt_agent_core PUBLIC yaml-cpp::yaml-cpp nlohmann_json::nlohmann_json CURL::libcurl)

# Build chuong trinh chinh (vdt-agent)
add_executable(vdt-agent src/main.cpp)
target_link_libraries(vdt-agent PRIVATE vdt_agent_core)

# ==========================================
# CẤU HÌNH KIỂM THỬ (UNIT TESTS)
# ==========================================
enable_testing()

# Layer 0: Core Utilities
add_executable(test_event_helper tests/test_event_helper.cpp)
target_link_libraries(test_event_helper PRIVATE vdt_agent_core gtest_main)
add_test(NAME EventHelperTest COMMAND test_event_helper)

add_executable(test_config tests/test_config.cpp)
target_link_libraries(test_config PRIVATE vdt_agent_core gtest_main)
add_test(NAME ConfigTest COMMAND test_config)

# Layer 1: Data Processing
add_executable(test_event_serializer tests/test_event_serializer.cpp)
target_link_libraries(test_event_serializer PRIVATE vdt_agent_core gtest_main)
add_test(NAME EventSerializerTest COMMAND test_event_serializer)

# (Chúng ta sẽ thêm test_event_queue ở phần sau)
add_executable(test_event_queue tests/test_event_queue.cpp)
target_link_libraries(test_event_queue PRIVATE vdt_agent_core gtest_main)
add_test(NAME EventQueueTest COMMAND test_event_queue)

# Layer 2: Integration & Stateful
add_executable(test_session_tracker tests/test_session_tracker.cpp)
target_link_libraries(test_session_tracker PRIVATE vdt_agent_core gtest_main)
add_test(NAME SessionTrackerTest COMMAND test_session_tracker)

add_executable(test_audit_watcher tests/test_audit_watcher.cpp)
target_link_libraries(test_audit_watcher PRIVATE vdt_agent_core gtest_main)
add_test(NAME AuditWatcherTest COMMAND test_audit_watcher)