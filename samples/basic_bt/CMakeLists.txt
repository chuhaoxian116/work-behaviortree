# =========================================================
# 基础行为树学习示例
# =========================================================

add_executable(my_first_bt
  main.cpp)

target_link_libraries(my_first_bt
  PRIVATE behaviortree_cpp_v3)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(my_first_bt
    PRIVATE -Wall -Wextra -Wpedantic)
endif()

# 直接读取源码目录下的 XML。
# 这样只改 tree.xml 时，不需要重新 make，重新运行程序即可生效。
target_compile_definitions(my_first_bt
  PRIVATE TREE_XML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tree.xml")
