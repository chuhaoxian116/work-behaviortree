#include "engineering_layout/bt_nodes/battery_ok.hpp"

#include <iostream>

namespace engineering_layout
{

BatteryOK::BatteryOK(const std::string& name, const BT::NodeConfiguration& config)
  : BT::ConditionNode(name, config)
{}

BT::PortsList BatteryOK::providedPorts()
{
    return { BT::InputPort<int>("battery") };
}

BT::NodeStatus BatteryOK::tick()
{
    const auto battery = getInput<int>("battery");
    if(!battery)
    {
        throw BT::RuntimeError("BatteryOK 缺少 battery 输入");
    }

    const auto result = battery.value() >= 60 ? BT::NodeStatus::SUCCESS
                                              : BT::NodeStatus::FAILURE;

    std::cout << "[BatteryOK] 电量: " << battery.value() << "% -> "
              << (result == BT::NodeStatus::SUCCESS ? "SUCCESS" : "FAILURE")
              << std::endl;
    return result;
}

}  // namespace engineering_layout
