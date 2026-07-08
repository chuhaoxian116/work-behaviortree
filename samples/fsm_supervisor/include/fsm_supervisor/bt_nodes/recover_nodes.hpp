#pragma once

#include <behaviortree_cpp_v3/action_node.h>

#include <string>

namespace fsm_supervisor::bt_nodes
{

/**
 * ResetMotor 模拟恢复树里的异步复位动作。
 *
 * 它可以类比真实工程中的驱动器复位、轴回零、等待伺服重新使能等操作。
 */
class ResetMotor : public BT::StatefulActionNode
{
public:
    /**
     * 构造 ResetMotor 节点实例。
     */
    ResetMotor(const std::string& name, const BT::NodeConfiguration& config);

    /**
     * 声明端口。
     *
     * ResetMotor 在这个示例里不需要输入/输出端口。
     */
    static BT::PortsList providedPorts();

    /**
     * 启动模拟复位流程，并返回 RUNNING。
     */
    BT::NodeStatus onStart() override;

    /**
     * 后续 tick 中查询复位进度。
     */
    BT::NodeStatus onRunning() override;

    /**
     * 恢复树被 halt 时，清理本节点的本地状态。
     */
    void onHalted() override;

private:
    int progress_ = 0;  // 模拟复位进度；真实工程应查询硬件驱动。
};

/**
 * ClearHardwareError 清除黑板中的硬件异常标志。
 *
 * 它位于 recover_tree.xml 的 ResetMotor 后面。该节点成功后，FSM 可以读到
 * hardware_error=false，并允许系统回到 IDLE。
 */
class ClearHardwareError : public BT::SyncActionNode
{
public:
    /**
     * 构造 ClearHardwareError 节点实例。
     */
    ClearHardwareError(const std::string& name, const BT::NodeConfiguration& config);

    /**
     * 声明用于写出 hardware_error=false 的输出端口。
     */
    static BT::PortsList providedPorts();

    /**
     * 向 XML remap 后的黑板 key 写入 false，并返回 SUCCESS。
     */
    BT::NodeStatus tick() override;
};

}  // namespace fsm_supervisor::bt_nodes
