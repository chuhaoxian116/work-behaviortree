cmake_minimum_required(VERSION 3.16)
project(my_first_behavior_tree LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
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

add_executable(my_first_bt main.cpp)
target_link_libraries(my_first_bt PRIVATE behaviortree_cpp_v3)

# 让程序可以直接从构建目录读取行为树。
configure_file(tree.xml tree.xml COPYONLY)
target_compile_definitions(
    my_first_bt PRIVATE TREE_XML_PATH="${CMAKE_CURRENT_BINARY_DIR}/tree.xml"
)
