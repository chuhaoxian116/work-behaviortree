#include "engineering_layout/bt_nodes/say.hpp"

#include <iostream>

namespace engineering_layout
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

    std::cout << "[Say] " << text.value() << std::endl;
    return BT::NodeStatus::SUCCESS;
}

}  // namespace engineering_layout
