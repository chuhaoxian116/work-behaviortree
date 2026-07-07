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

# 使用 thirdparty 中的 BehaviorTree.CPP 预编译库。
set(BEHAVIORTREE_CPP_PREBUILT_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/BehaviorTree.CPP-3.8.7-prebuilt"
    CACHE PATH "BehaviorTree.CPP 3.8.7 prebuilt directory")

if(NOT EXISTS "${BEHAVIORTREE_CPP_PREBUILT_ROOT}/lib/cmake/behaviortree_cpp_v3/behaviortree_cpp_v3Config.cmake")
  message(FATAL_ERROR
    "BehaviorTree.CPP prebuilt dependency is missing. Run:\n"
    "  git submodule update --init --recursive\n"
    "or put the prebuilt package under:\n"
    "  ${BEHAVIORTREE_CPP_PREBUILT_ROOT}")
endif()

list(PREPEND CMAKE_PREFIX_PATH
  "${BEHAVIORTREE_CPP_PREBUILT_ROOT}")

find_package(behaviortree_cpp_v3 CONFIG REQUIRED)

# 让 build 目录里的示例程序可以直接运行，不需要手动设置 LD_LIBRARY_PATH。
set(CMAKE_BUILD_RPATH
  "${BEHAVIORTREE_CPP_PREBUILT_ROOT}/lib")

add_subdirectory(samples)
