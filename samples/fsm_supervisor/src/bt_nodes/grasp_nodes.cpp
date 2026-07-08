#include "fsm_supervisor/bt_nodes/grasp_nodes.hpp"

#include <behaviortree_cpp_v3/bt_factory.h>

#include <iostream>

namespace fsm_supervisor::bt_nodes
{

/**
 * 构造检测节点。
 */
DetectCloth::DetectCloth(const std::string& name, const BT::NodeConfiguration& config)
  : BT::SyncActionNode(name, config)
{}

/**
 * 声明 DetectCloth 的输入端口。
 */
BT::PortsList DetectCloth::providedPorts()
{
    // XML 使用 object="{object}"，BehaviorTree.CPP 会从黑板读取 object 的值。
    return { BT::InputPort<std::string>("object") };
}

/**
 * 执行一次目标检测。
 */
BT::NodeStatus DetectCloth::tick()
{
    const auto object = getInput<std::string>("object");  // 从输入端口读取目标物名称。
    if(!object)
    {
        throw BT::RuntimeError("DetectCloth 缺少 object 输入");
    }

    std::cout << "[BT][DetectCloth] 检测目标: " << object.value() << std::endl;
    std::cout << "[返回][DetectCloth] SUCCESS，抓取树继续执行 MoveToCloth" << std::endl;
    return BT::NodeStatus::SUCCESS;
}

/**
 * 构造移动节点。
 */
MoveToCloth::MoveToCloth(const std::string& name, const BT::NodeConfiguration& config)
  : BT::StatefulActionNode(name, config)
{}

/**
 * 声明 MoveToCloth 的输入端口。
 */
BT::PortsList MoveToCloth::providedPorts()
{
    // XML 使用 target="{target}"，移动目标来自黑板。
    return { BT::InputPort<std::string>("target") };
}

/**
 * 启动模拟移动动作。
 */
BT::NodeStatus MoveToCloth::onStart()
{
    const auto target = getInput<std::string>("target");  // 本次移动的目标点。
    if(!target)
    {
        throw BT::RuntimeError("MoveToCloth 缺少 target 输入");
    }

    target_ = target.value();
    progress_ = 0;
    std::cout << "[BT][MoveToCloth] onStart(): 开始前往 " << target_ << std::endl;
    std::cout << "[返回][MoveToCloth::onStart] RUNNING，FSM 下一轮还要继续 tick 抓取树"
              << std::endl;
    return BT::NodeStatus::RUNNING;
}

/**
 * 查询模拟移动进度。
 */
BT::NodeStatus MoveToCloth::onRunning()
{
    // 示例里用自增计数模拟进度；真实工程里应该非阻塞查询运动业务接口。
    ++progress_;
    std::cout << "[BT][MoveToCloth] onRunning(): 前往 " << target_
              << "，进度 " << progress_ << "/4" << std::endl;

    if(progress_ >= 4)
    {
        std::cout << "[返回][MoveToCloth::onRunning] SUCCESS，已经到达抓取位置"
                  << std::endl;
        return BT::NodeStatus::SUCCESS;
    }

    std::cout << "[返回][MoveToCloth::onRunning] RUNNING，移动还没完成" << std::endl;
    return BT::NodeStatus::RUNNING;
}

/**
 * 响应 FSM 或父节点对抓取树的 halt。
 */
void MoveToCloth::onHalted()
{
    std::cout << "[BT][MoveToCloth] onHalted(): FSM 调用了 grasp_tree.haltTree()" << std::endl;
    std::cout << "[关键点] FSM 不直接控制 MoveToCloth；它 halt 整棵树，halt 会传到 RUNNING 子节点"
              << std::endl;
    progress_ = 0;
}

/**
 * 构造夹爪闭合节点。
 */
CloseGripper::CloseGripper(const std::string& name, const BT::NodeConfiguration& config)
  : BT::SyncActionNode(name, config)
{}

/**
 * 声明 CloseGripper 的端口。
 */
BT::PortsList CloseGripper::providedPorts()
{
    // 空端口列表：闭合夹爪不需要从黑板读取数据。
    return {};
}

/**
 * 执行模拟夹爪闭合动作。
 */
BT::NodeStatus CloseGripper::tick()
{
    std::cout << "[BT][CloseGripper] 闭合夹爪，抓住衣服" << std::endl;
    std::cout << "[返回][CloseGripper] SUCCESS，抓取树完成" << std::endl;
    return BT::NodeStatus::SUCCESS;
}

}  // namespace fsm_supervisor::bt_nodes
