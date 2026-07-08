#include "plugin_nodes/bt_nodes/say.hpp"

#include <iostream>

namespace plugin_nodes
{

Say::Say(const std::string& name, const BT::NodeConfiguration& config)
  : BT::SyncActionNode(name, config)
{}

BT::PortsList Say::providedPorts()
{
    return { BT::InputPort<std::string>("text") };
}

BT::NodeStatus Say::tick()
{
    const auto text = getInput<std::string>("text");
    if(!text)
    {
        throw BT::RuntimeError("Say 缺少 text 输入");
    }

    std::cout << "[plugin][Say] " << text.value() << std::endl;
    return BT::NodeStatus::SUCCESS;
}

}  // namespace plugin_nodes
