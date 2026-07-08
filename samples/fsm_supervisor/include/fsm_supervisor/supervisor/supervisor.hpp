#pragma once

#include <chrono>
#include <string>

namespace fsm_supervisor
{

/**
 * 创建并运行 FSM supervisor 所需的配置。
 */
struct SupervisorConfig
{
    std::string grasp_tree_xml_path;  // GRASPING 状态使用的行为树 XML。

    std::string recover_tree_xml_path;  // RECOVERING 状态使用的行为树 XML。

    std::chrono::milliseconds cycle_period{ 200 };  // 主控循环周期。
};

/**
 * FsmSupervisor 管理顶层 FSM 和行为树生命周期。
 *
 * 主控不实现具体 BT 节点动作，而是负责：
 *   * 从 XML 创建行为树；
 *   * 接收外部命令；
 *   * 决定当前 tick 哪棵行为树；
 *   * 状态退出或用户退出时调用 haltTree()。
 */
class FsmSupervisor
{
public:
    /**
     * 保存运行配置。
     */
    explicit FsmSupervisor(SupervisorConfig config);

    /**
     * 启动键盘输入线程和 FSM 主循环。
     *
     * 返回:
     *   进程退出码。正常收到 Quit 命令后返回 0。
     */
    int run();

private:
    SupervisorConfig config_;  // 程序入口传入的 XML 路径和循环周期。
};

}  // namespace fsm_supervisor
