# =========================================================
# 工程化拆分学习示例
# =========================================================

add_library(engineering_layout_bt_nodes
  src/bt_nodes/battery_ok.cpp
  src/bt_nodes/move_to.cpp
  src/bt_nodes/say.cpp
  src/bt_nodes/register_nodes.cpp)

target_include_directories(engineering_layout_bt_nodes
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(engineering_layout_bt_nodes
  PUBLIC BT::behaviortree_cpp_v3)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(engineering_layout_bt_nodes
    PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(engineering_layout_bt
  main.cpp)

target_link_libraries(engineering_layout_bt
  PRIVATE engineering_layout_bt_nodes)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(engineering_layout_bt
    PRIVATE -Wall -Wextra -Wpedantic)
endif()

# 直接读取源码目录下的 XML。
target_compile_definitions(engineering_layout_bt
  PRIVATE TREE_XML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tree.xml")
