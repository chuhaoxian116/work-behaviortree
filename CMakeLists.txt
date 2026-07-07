cmake_minimum_required(VERSION 3.16)
project(behavior_tree_learning VERSION 0.1.0 LANGUAGES CXX)

# 这个目录现在按“小工程”组织：
# - samples/ 放学习和实验代码；
# - src/     留给以后真正开发库/业务模块；
# - include/ 留给以后对外头文件。
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE
      RelWithDebInfo
      CACHE STRING "Choose the build type." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    Debug Release RelWithDebInfo MinSizeRel)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_NO_PACKAGE_REGISTRY ON)

# 直接使用仓库中自带的 BehaviorTree.CPP，空白环境无需预先安装。
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(BUILD_MANUAL_SELECTOR OFF CACHE BOOL "" FORCE)
set(ENABLE_COROUTINES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(
    "${CMAKE_CURRENT_SOURCE_DIR}/../BehaviorTree.CPP-3.8.7"
    "${CMAKE_BINARY_DIR}/BehaviorTree.CPP"
)

add_subdirectory(samples)
