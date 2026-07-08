#pragma once

#include <string>

namespace fsm_supervisor
{

/**
 * Command 是 FSM 消费的外部控制命令。
 *
 * 本示例中命令来自键盘线程。真实工程里，同样的枚举可以由 DDS、ROS、
 * TCP 服务或 HMI 回调写入。
 */
enum class Command
{
    None,  // 当前 supervisor 周期没有新命令。

    StartGrasp,  // 在 IDLE 状态启动抓取行为树。

    Stop,  // 停止正在运行的抓取行为树，并回到 IDLE。

    ClearError,  // 在 ERROR 状态启动恢复行为树。

    InjectError,  // 模拟硬件异常，将 hardware_error 置为 true。

    Quit  // 停止所有行为树，并退出主控循环。
};

/**
 * 返回命令名称，用于日志打印。
 */
const char* toString(Command command);

/**
 * 将一行键盘输入解析成 Command。
 *
 * 参数:
 *   line: 原始命令文本，例如 "start" 或 "s"。
 *   command: 输出参数；解析成功时写入对应命令。
 *
 * 返回:
 *   如果命令可识别，返回 true；否则返回 false。
 */
bool parseCommand(const std::string& line, Command& command);

/**
 * 打印键盘命令帮助信息。
 */
void printKeyboardHelp();

}  // namespace fsm_supervisor
