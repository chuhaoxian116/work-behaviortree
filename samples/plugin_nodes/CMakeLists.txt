# =========================================================
# 插件化节点学习示例
# =========================================================

add_library(plugin_nodes_bt_plugin SHARED
  src/bt_nodes/battery_ok.cpp
  src/bt_nodes/move_to.cpp
  src/bt_nodes/say.cpp
  src/plugin.cpp)

target_include_directories(plugin_nodes_bt_plugin
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(plugin_nodes_bt_plugin
  PRIVATE BT::behaviortree_cpp_v3)

# 必须定义 BT_PLUGIN_EXPORT，否则 BT_REGISTER_NODES 会生成 static 函数，
# registerFromPlugin() 找不到 BT_RegisterNodesFromPlugin 符号。
target_compile_definitions(plugin_nodes_bt_plugin
  PRIVATE BT_PLUGIN_EXPORT)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(plugin_nodes_bt_plugin
    PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(plugin_nodes_bt
  main.cpp)

target_link_libraries(plugin_nodes_bt
  PRIVATE BT::behaviortree_cpp_v3)

add_dependencies(plugin_nodes_bt
  plugin_nodes_bt_plugin)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(plugin_nodes_bt
    PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_compile_definitions(plugin_nodes_bt
  PRIVATE
    TREE_XML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tree.xml"
    PLUGIN_PATH="$<TARGET_FILE:plugin_nodes_bt_plugin>")
