#pragma once

#include <behaviortree_cpp_v3/action_node.h>

#include <string>

namespace engineering_layout
{

class MoveTo : public BT::StatefulActionNode
{
public:
    MoveTo(const std::string& name, const BT::NodeConfiguration& config);

    static BT::PortsList providedPorts();

    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override;

private:
    std::string target_;
    int progress_ = 0;
};

}  // namespace engineering_layout
