#pragma once

#include <behaviortree_cpp_v3/action_node.h>

namespace plugin_nodes
{

class Say : public BT::SyncActionNode
{
public:
    Say(const std::string& name, const BT::NodeConfiguration& config);

    static BT::PortsList providedPorts();

    BT::NodeStatus tick() override;
};

}  // namespace plugin_nodes
