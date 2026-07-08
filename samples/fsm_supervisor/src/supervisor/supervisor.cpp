#include "fsm_supervisor/supervisor/supervisor.hpp"

#include "fsm_supervisor/bt_nodes/register_nodes.hpp"
#include "fsm_supervisor/supervisor/command.hpp"
#include "fsm_supervisor/supervisor/robot_state.hpp"

#include <behaviortree_cpp_v3/bt_factory.h>

#include <deque>
#include <iostream>
#include <mutex>
#include <thread>
#include <utility>

namespace fsm_supervisor
{

/**
 * 保存主控运行配置。
 */
FsmSupervisor::FsmSupervisor(SupervisorConfig config)
  : config_(std::move(config))
{}

/**
 * 运行键盘输入线程和 FSM 主循环。
 */
int FsmSupervisor::run()
{
    std::cout << "[初始化] FSM supervisor + 多棵行为树示例" << std::endl;
    std::cout << "[说明] FSM 负责 IDLE / GRASPING / ERROR / RECOVERING 大状态" << std::endl;
    std::cout << "[说明] GRASPING 状态 tick grasp_tree；RECOVERING 状态 tick recover_tree"
              << std::endl;
    std::cout << "[说明] 状态退出/异常时，FSM 调用 tree.haltTree() 取消正在 RUNNING 的树"
              << std::endl;
    std::cout << "[说明] 另起一个键盘线程接收外部命令；FSM 主循环按队列逐条消费命令"
              << std::endl;
    printKeyboardHelp();

    BT::BehaviorTreeFactory factory;  // 保存节点类型注册信息，用来创建多棵行为树。
    bt_nodes::registerNodes(factory);

    auto blackboard = BT::Blackboard::create();  // 抓取树和恢复树共享的黑板。
    blackboard->set("object", std::string("衣服"));
    blackboard->set("target", std::string("抓取点 A"));
    blackboard->set("hardware_error", false);

    std::cout << "[初始化] 创建 grasp_tree: " << config_.grasp_tree_xml_path << std::endl;
    auto grasp_tree =
        factory.createTreeFromFile(config_.grasp_tree_xml_path, blackboard);  // 抓取状态运行的树。

    std::cout << "[初始化] 创建 recover_tree: " << config_.recover_tree_xml_path << std::endl;
    auto recover_tree =
        factory.createTreeFromFile(config_.recover_tree_xml_path, blackboard);  // 恢复状态运行的树。

    RobotState state = RobotState::IDLE;  // 当前机器人顶层状态机状态。
    std::mutex command_mutex;  // 保护 pending_commands 和 quit_requested。
    std::deque<Command> pending_commands;  // 外部命令 FIFO 队列，避免快速输入时命令被覆盖。
    bool quit_requested = false;  // 标准输入关闭或收到退出命令时置 true。

    /**
     * 将键盘线程产生的命令压入队列。
     */
    auto pushCommand = [&](Command command) {
        std::lock_guard<std::mutex> lock(command_mutex);
        pending_commands.push_back(command);
    };

    /**
     * 从队列取出当前主控周期要处理的一条命令。
     */
    auto consumeCommand = [&]() {
        std::lock_guard<std::mutex> lock(command_mutex);
        if(pending_commands.empty())
        {
            return Command::None;
        }
        const auto command = pending_commands.front();
        pending_commands.pop_front();
        return command;
    };

    /**
     * 检查主控循环是否应该退出。
     */
    auto shouldQuit = [&]() {
        std::lock_guard<std::mutex> lock(command_mutex);
        return quit_requested;
    };

    /**
     * 键盘输入线程。
     *
     * 它相当于真实工程里的 DDS/ROS/HMI 命令输入线程。注意它不直接控制 FSM，
     * 只把解析后的命令放进线程安全队列。
     */
    std::thread keyboard_thread([&]() {
        std::string line;  // 当前读取到的一行键盘输入。
        while(true)
        {
            if(!std::getline(std::cin, line))
            {
                std::lock_guard<std::mutex> lock(command_mutex);
                quit_requested = true;
                return;
            }

            Command command = Command::None;  // 当前输入行解析出的命令。
            if(!parseCommand(line, command))
            {
                std::cout << "[键盘线程] 未识别命令: " << line << std::endl;
                printKeyboardHelp();
                continue;
            }

            std::cout << "[键盘线程] 收到命令: " << toString(command) << std::endl;
            pushCommand(command);

            if(command == Command::Quit)
            {
                return;
            }
        }
    });

    /**
     * 统一状态跳转和日志打印。
     */
    auto transitionTo = [&](RobotState next) {
        std::cout << "[FSM] 状态跳转: " << toString(state) << " -> " << toString(next)
                  << std::endl;
        state = next;
    };

    /**
     * 主控循环。
     *
     * 每轮循环最多消费一条外部命令，读取一次全局反馈，并最多 tick 一棵行为树。
     */
    for(int cycle = 1; !shouldQuit(); ++cycle)
    {
        std::cout << "\n========== supervisor cycle " << cycle << " ==========" << std::endl;

        const auto command = consumeCommand();  // 本周期 FSM 要处理的外部命令。
        if(command != Command::None)
        {
            std::cout << "[外部命令] " << toString(command) << std::endl;
        }

        if(command == Command::Quit)
        {
            std::cout << "[FSM] 收到 Quit，halt 所有树并退出 supervisor" << std::endl;
            grasp_tree.haltTree();
            recover_tree.haltTree();
            {
                std::lock_guard<std::mutex> lock(command_mutex);
                quit_requested = true;
            }
            break;
        }

        if(command == Command::InjectError)
        {
            // 模拟异步硬件反馈变成异常；真实工程里可能来自 EtherCAT/DDS 线程。
            std::cout << "[外部事件] 键盘命令模拟硬件异常 hardware_error=true" << std::endl;
            blackboard->set("hardware_error", true);
        }

        bool hardware_error = false;  // 当前周期读取到的硬件异常标志。
        blackboard->get("hardware_error", hardware_error);
        std::cout << "[FSM] 当前状态=" << toString(state)
                  << ", command=" << toString(command)
                  << ", hardware_error=" << (hardware_error ? "true" : "false")
                  << std::endl;

        if(hardware_error && state != RobotState::ERROR && state != RobotState::RECOVERING)
        {
            // 全局异常优先级高于正常任务流程；如果任务树正在运行，先 halt 再进 ERROR。
            std::cout << "[FSM][全局异常] 任何非恢复状态发现 hardware_error=true，都进入 ERROR"
                      << std::endl;
            if(state == RobotState::GRASPING)
            {
                std::cout << "[FSM][全局异常] 当前抓取树正在运行，先 halt grasp_tree"
                          << std::endl;
                grasp_tree.haltTree();
            }
            transitionTo(RobotState::ERROR);
            std::this_thread::sleep_for(config_.cycle_period);
            continue;
        }

        switch(state)
        {
            case RobotState::IDLE:
            {
                // IDLE 不 tick 任务树，只等待有效启动命令。
                if(command == Command::StartGrasp)
                {
                    std::cout << "[FSM][IDLE] 收到抓取命令，准备运行 grasp_tree" << std::endl;
                    std::cout << "[FSM][IDLE] 进入任务前先 haltTree()，确保上一轮残留状态被清理"
                              << std::endl;
                    grasp_tree.haltTree();
                    transitionTo(RobotState::GRASPING);
                }
                else
                {
                    std::cout << "[FSM][IDLE] 待命中，不 tick 任何任务树" << std::endl;
                }
                break;
            }

            case RobotState::GRASPING:
            {
                // Stop 是操作员命令，即使没有硬件异常，也会取消抓取树。
                if(command == Command::Stop)
                {
                    std::cout << "[FSM][GRASPING] 收到 Stop，halt grasp_tree 并回到 IDLE"
                              << std::endl;
                    grasp_tree.haltTree();
                    transitionTo(RobotState::IDLE);
                    break;
                }

                if(hardware_error)
                {
                    std::cout << "[FSM][GRASPING] 检测到硬件异常，halt grasp_tree 并进入 ERROR"
                              << std::endl;
                    grasp_tree.haltTree();
                    transitionTo(RobotState::ERROR);
                    break;
                }

                // GRASPING 中由 FSM 主动 tick 抓取树；行为树不会自己后台运行。
                std::cout << "[FSM][GRASPING] 调用 grasp_tree.tickRoot()" << std::endl;
                const auto status = grasp_tree.tickRoot();  // 抓取树根节点本轮返回状态。
                std::cout << "[FSM][GRASPING] grasp_tree 返回 " << toString(status)
                          << std::endl;

                if(status == BT::NodeStatus::SUCCESS)
                {
                    std::cout << "[FSM][GRASPING] 抓取任务完成，回到 IDLE" << std::endl;
                    transitionTo(RobotState::IDLE);
                    std::cout << "[FSM][GRASPING] 程序不退出，继续等待下一条键盘命令"
                              << std::endl;
                }

                if(status == BT::NodeStatus::FAILURE)
                {
                    std::cout << "[FSM][GRASPING] 抓取树失败，进入 ERROR" << std::endl;
                    transitionTo(RobotState::ERROR);
                }
                break;
            }

            case RobotState::ERROR:
            {
                // ERROR 是保持状态；没有明确清错命令前，不 tick 抓取树。
                if(command == Command::ClearError)
                {
                    std::cout << "[FSM][ERROR] 收到 ClearError，开始运行 recover_tree" << std::endl;
                    recover_tree.haltTree();
                    transitionTo(RobotState::RECOVERING);
                }
                else
                {
                    std::cout << "[FSM][ERROR] 异常保持中，不 tick 抓取树；等待 ClearError 命令"
                              << std::endl;
                }
                break;
            }

            case RobotState::RECOVERING:
            {
                // 恢复流程也是行为树，但只有 RECOVERING 状态才会 tick 它。
                std::cout << "[FSM][RECOVERING] 调用 recover_tree.tickRoot()" << std::endl;
                const auto status = recover_tree.tickRoot();  // 恢复树根节点本轮返回状态。
                std::cout << "[FSM][RECOVERING] recover_tree 返回 " << toString(status)
                          << std::endl;

                if(status == BT::NodeStatus::SUCCESS)
                {
                    std::cout << "[FSM][RECOVERING] 恢复完成，回到 IDLE" << std::endl;
                    transitionTo(RobotState::IDLE);
                }
                else if(status == BT::NodeStatus::FAILURE)
                {
                    std::cout << "[FSM][RECOVERING] 恢复失败，回到 ERROR" << std::endl;
                    transitionTo(RobotState::ERROR);
                }
                break;
            }
        }

        std::this_thread::sleep_for(config_.cycle_period);
    }

    // 退出前统一 halt 所有树，避免 StatefulActionNode 仍停留在 RUNNING。
    grasp_tree.haltTree();
    recover_tree.haltTree();

    // 等待键盘线程退出，保证程序干净收尾。
    if(keyboard_thread.joinable())
    {
        keyboard_thread.join();
    }

    std::cout << "\n[实验结束] 用户请求退出，supervisor 已停止" << std::endl;
    return 0;
}

}  // namespace fsm_supervisor
