# =========================================================
# Retry / Timeout 装饰器学习示例
# =========================================================

add_executable(retry_timeout_bt
  main.cpp)

target_link_libraries(retry_timeout_bt
  PRIVATE BT::behaviortree_cpp_v3)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(retry_timeout_bt
    PRIVATE -Wall -Wextra -Wpedantic)
endif()

# 直接读取源码目录下的 XML。
target_compile_definitions(retry_timeout_bt
  PRIVATE TREE_XML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tree.xml")
