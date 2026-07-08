#include "fsm_supervisor/bt_nodes/recover_nodes.hpp"

#include <behaviortree_cpp_v3/bt_factory.h>

#include <iostream>

namespace fsm_supervisor::bt_nodes
{

/**
 * 构造复位节点。
 */
ResetMotor::ResetMotor(const std::string& name, const BT::NodeConfiguration& config)
  : BT::StatefulActionNode(name, config)
{}

/**
 * 声明 ResetMotor 的端口。
 */
BT::PortsList ResetMotor::providedPorts()
{
    // 空端口列表：示例复位动作只使用内部进度计数。
    return {};
}

/**
 * 启动模拟复位动作。
 */
BT::NodeStatus ResetMotor::onStart()
{
    progress_ = 0;
    std::cout << "[BT][ResetMotor] onStart(): 开始复位电机/驱动器" << std::endl;
    std::cout << "[返回][ResetMotor::onStart] RUNNING，恢复树继续运行中" << std::endl;
    return BT::NodeStatus::RUNNING;
}

/**
 * 查询模拟复位进度。
 */
BT::NodeStatus ResetMotor::onRunning()
{
    // 示例里用自增计数模拟复位进度；真实工程里应该查询驱动器状态。
    ++progress_;
    std::cout << "[BT][ResetMotor] onRunning(): 复位进度 " << progress_ << "/3"
              << std::endl;

    if(progress_ >= 3)
    {
        std::cout << "[返回][ResetMotor::onRunning] SUCCESS，复位完成" << std::endl;
        return BT::NodeStatus::SUCCESS;
    }

    std::cout << "[返回][ResetMotor::onRunning] RUNNING，复位还没完成" << std::endl;
    return BT::NodeStatus::RUNNING;
}

/**
 * 响应恢复树 halt。
 */
void ResetMotor::onHalted()
{
    std::cout << "[BT][ResetMotor] onHalted(): 恢复树被 FSM 停止" << std::endl;
    progress_ = 0;
}

ClearHardwareError::ClearHardwareError(const std::string& name,
                                       const BT::NodeConfiguration& config)
  : BT::SyncActionNode(name, config)
{}

/**
 * 声明 ClearHardwareError 的输出端口。
 */
BT::PortsList ClearHardwareError::providedPorts()
{
    // OutputPort 表示该节点会写入 XML remap 后的黑板 key。
    return { BT::OutputPort<bool>("error_flag") };
}

/**
 * 清除硬件异常标志。
 */
BT::NodeStatus ClearHardwareError::tick()
{
    std::cout << "[BT][ClearHardwareError] 清除黑板里的 hardware_error=false" << std::endl;
    setOutput("error_flag", false);
    std::cout << "[返回][ClearHardwareError] SUCCESS，恢复树完成" << std::endl;
    return BT::NodeStatus::SUCCESS;
}

}  // namespace fsm_supervisor::bt_nodes
