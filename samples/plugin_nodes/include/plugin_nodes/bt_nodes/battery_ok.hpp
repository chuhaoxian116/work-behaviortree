#pragma once

#include <behaviortree_cpp_v3/condition_node.h>

namespace plugin_nodes
{

class BatteryOK : public BT::ConditionNode
{
public:
    BatteryOK(const std::string& name, const BT::NodeConfiguration& config);

    static BT::PortsList providedPorts();

    BT::NodeStatus tick() override;
};

}  // namespace plugin_nodes
