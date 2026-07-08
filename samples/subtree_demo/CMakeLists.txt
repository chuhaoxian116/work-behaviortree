# =========================================================
# SubTree 子树拆分学习示例
# =========================================================

add_executable(subtree_demo_bt
  main.cpp)

target_link_libraries(subtree_demo_bt
  PRIVATE BT::behaviortree_cpp_v3)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(subtree_demo_bt
    PRIVATE -Wall -Wextra -Wpedantic)
endif()

# 直接读取源码目录下的 XML。
target_compile_definitions(subtree_demo_bt
  PRIVATE TREE_XML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tree.xml")
