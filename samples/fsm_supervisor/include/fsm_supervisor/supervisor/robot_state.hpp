#pragma once

#include <behaviortree_cpp_v3/basic_types.h>

namespace fsm_supervisor
{

/**
 * RobotState 是机器人顶层 FSM 状态。
 *
 * 每个状态回答一个不同的工程问题：
 *   IDLE: 是否可以启动新任务？
 *   GRASPING: 当前是否正在 tick 抓取树？
 *   ERROR: 是否需要等待清错命令？
 *   RECOVERING: 当前是否正在 tick 恢复树？
 */
enum class RobotState
{
    IDLE,  // 机器人已使能并等待命令；不 tick 任务树。

    GRASPING,  // 主控正在 tick 抓取树。

    ERROR,  // 存在异常；不 tick 任务树。

    RECOVERING  // 主控正在 tick 恢复树来恢复异常。
};

/**
 * 返回 FSM 状态名称，用于日志打印。
 */
const char* toString(RobotState state);

/**
 * 返回 BehaviorTree.CPP 节点状态名称，用于日志打印。
 */
const char* toString(BT::NodeStatus status);

}  // namespace fsm_supervisor
