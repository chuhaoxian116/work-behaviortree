#include <behaviortree_cpp_v3/bt_factory.h>

#include "plugin_nodes/bt_nodes/battery_ok.hpp"
#include "plugin_nodes/bt_nodes/move_to.hpp"
#include "plugin_nodes/bt_nodes/say.hpp"

// 这个宏会导出 BehaviorTree.CPP 约定的插件入口：
//
//   BT_RegisterNodesFromPlugin(BT::BehaviorTreeFactory& factory)
//
// 主程序调用 factory.registerFromPlugin(plugin_path) 时，会查找并调用这个符号。
BT_REGISTER_NODES(factory)
{
    factory.registerNodeType<plugin_nodes::BatteryOK>("BatteryOK");
    factory.registerNodeType<plugin_nodes::MoveTo>("MoveTo");
    factory.registerNodeType<plugin_nodes::Say>("Say");
}
