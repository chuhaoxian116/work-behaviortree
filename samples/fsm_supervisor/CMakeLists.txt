# =========================================================
# FSM supervisor + 多棵行为树学习示例
# =========================================================

add_library(fsm_supervisor_sample_lib
  src/bt_nodes/grasp_nodes.cpp
  src/bt_nodes/recover_nodes.cpp
  src/bt_nodes/register_nodes.cpp
  src/supervisor/command.cpp
  src/supervisor/robot_state.cpp
  src/supervisor/supervisor.cpp)

target_include_directories(fsm_supervisor_sample_lib
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(fsm_supervisor_sample_lib
  PRIVATE BT::behaviortree_cpp_v3)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(fsm_supervisor_sample_lib
    PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(fsm_supervisor_bt
  main.cpp)

target_link_libraries(fsm_supervisor_bt
  PRIVATE fsm_supervisor_sample_lib)

# 直接读取源码目录下的 XML，方便改 XML 后重新 make 即可更新。
target_compile_definitions(fsm_supervisor_bt
  PRIVATE
    GRASP_TREE_XML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/grasp_tree.xml"
    RECOVER_TREE_XML_PATH="${CMAKE_CURRENT_SOURCE_DIR}/recover_tree.xml")
