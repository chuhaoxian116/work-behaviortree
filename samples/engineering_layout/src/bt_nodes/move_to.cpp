#include "engineering_layout/bt_nodes/move_to.hpp"

#include <iostream>

namespace engineering_layout
{

MoveTo::MoveTo(const std::string& name, const BT::NodeConfiguration& config)
  : BT::StatefulActionNode(name, config)
{}

BT::PortsList MoveTo::providedPorts()
{
    return { BT::InputPort<std::string>("target") };
}

BT::NodeStatus MoveTo::onStart()
{
    const auto target = getInput<std::string>("target");
    if(!target)
    {
        throw BT::RuntimeError("MoveTo 缺少 target 输入");
    }

    target_ = target.value();
    progress_ = 0;
    std::cout << "[MoveTo] onStart(): 开始前往 " << target_ << std::endl;
    return BT::NodeStatus::RUNNING;
}

BT::NodeStatus MoveTo::onRunning()
{
    ++progress_;
    std::cout << "[MoveTo] onRunning(): 进度 " << progress_ << "/3"
              << std::endl;

    return progress_ >= 3 ? BT::NodeStatus::SUCCESS
                          : BT::NodeStatus::RUNNING;
}

void MoveTo::onHalted()
{
    std::cout << "[MoveTo] onHalted(): 移动被取消" << std::endl;
}

}  // namespace engineering_layout
