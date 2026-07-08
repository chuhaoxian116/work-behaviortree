#pragma once

#include <behaviortree_cpp_v3/condition_node.h>

namespace engineering_layout
{

class BatteryOK : public BT::ConditionNode
{
public:
    BatteryOK(const std::string& name, const BT::NodeConfiguration& config);

    static BT::PortsList providedPorts();

    BT::NodeStatus tick() override;
};

}  // namespace engineering_layout
