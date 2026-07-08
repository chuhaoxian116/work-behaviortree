#pragma once

#include <behaviortree_cpp_v3/action_node.h>

#include <string>

namespace fsm_supervisor::bt_nodes
{

/**
 * DetectCloth 是抓取树里的同步检测节点。
 *
 * 真实机器人里，这个节点通常会调用视觉服务，或者读取视觉线程缓存的检测结果。
 * 这个学习示例里只演示：BT 节点如何从黑板读取输入，并立即返回 SUCCESS。
 */
class DetectCloth : public BT::SyncActionNode
{
public:
    /**
     * 构造 DetectCloth 节点实例。
     *
     * BehaviorTree.CPP 解析 XML 中的 <DetectCloth/> 时会传入 name 和 config。
     */
    DetectCloth(const std::string& name, const BT::NodeConfiguration& config);

    /**
     * 声明节点端口。
     *
     * XML 中的 object="{object}" 会把黑板里的 object 映射到这个输入端口。
     */
    static BT::PortsList providedPorts();

    /**
     * 执行一次同步检测。
     *
     * 返回 SUCCESS 表示检测到了目标物。
     */
    BT::NodeStatus tick() override;
};

/**
 * MoveToCloth 模拟抓取树里的耗时移动动作。
 *
 * 这里继承 StatefulActionNode，是因为移动动作不应该在一次 tick 里阻塞完成。
 * onStart() 负责启动动作，onRunning() 负责查询进度，onHalted() 负责响应取消。
 */
class MoveToCloth : public BT::StatefulActionNode
{
public:
    /**
     * 构造 MoveToCloth 节点实例。
     */
    MoveToCloth(const std::string& name, const BT::NodeConfiguration& config);

    /**
     * 声明 target 输入端口。
     */
    static BT::PortsList providedPorts();

    /**
     * 节点从 IDLE 第一次进入 RUNNING 时调用。
     */
    BT::NodeStatus onStart() override;

    /**
     * 节点已经处于 RUNNING 后，后续每次 tick 调用。
     */
    BT::NodeStatus onRunning() override;

    /**
     * 父节点、Timeout 或 FSM halt 整棵树时调用。
     */
    void onHalted() override;

private:
    std::string target_;  // onStart() 从黑板读取到的目标点。

    int progress_ = 0;  // 模拟移动进度；真实工程应查询运动业务层。
};

/**
 * CloseGripper 是抓取树最后的同步夹爪动作。
 *
 * 示例里它立即成功；如果真实夹爪需要等待到位反馈，可以改成 StatefulActionNode。
 */
class CloseGripper : public BT::SyncActionNode
{
public:
    /**
     * 构造 CloseGripper 节点实例。
     */
    CloseGripper(const std::string& name, const BT::NodeConfiguration& config);

    /**
     * 声明端口。
     *
     * CloseGripper 不需要端口，但构造函数接收 NodeConfiguration 时仍需要提供此函数。
     */
    static BT::PortsList providedPorts();

    /**
     * 执行模拟夹爪闭合动作。
     */
    BT::NodeStatus tick() override;
};

}  // namespace fsm_supervisor::bt_nodes
