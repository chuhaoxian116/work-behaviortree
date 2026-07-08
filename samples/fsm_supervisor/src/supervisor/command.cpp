#include "fsm_supervisor/supervisor/command.hpp"

#include <iostream>

namespace fsm_supervisor
{

/**
 * 将命令枚举转换为日志字符串。
 */
const char* toString(Command command)
{
    switch(command)
    {
        case Command::None:
            return "None";
        case Command::StartGrasp:
            return "StartGrasp";
        case Command::Stop:
            return "Stop";
        case Command::ClearError:
            return "ClearError";
        case Command::InjectError:
            return "InjectError";
        case Command::Quit:
            return "Quit";
    }
    return "UNKNOWN";
}

/**
 * 解析键盘命令。
 */
bool parseCommand(const std::string& line, Command& command)
{
    // 短命令用于快速交互，长命令用于提升 README 和日志的可读性。
    if(line == "start" || line == "s")
    {
        command = Command::StartGrasp;
        return true;
    }
    if(line == "stop" || line == "x")
    {
        command = Command::Stop;
        return true;
    }
    if(line == "clear" || line == "c")
    {
        command = Command::ClearError;
        return true;
    }
    if(line == "error" || line == "e")
    {
        command = Command::InjectError;
        return true;
    }
    if(line == "quit" || line == "q")
    {
        command = Command::Quit;
        return true;
    }

    return false;
}

/**
 * 打印可用键盘命令。
 */
void printKeyboardHelp()
{
    // 帮助信息和 parseCommand() 放在同一文件，避免命令更新后说明不同步。
    std::cout << "\n[键盘命令]" << std::endl;
    std::cout << "  start / s  : 从 IDLE 进入 GRASPING，开始抓取树" << std::endl;
    std::cout << "  stop  / x  : 在 GRASPING 中 halt 抓取树并回到 IDLE" << std::endl;
    std::cout << "  error / e  : 模拟硬件异常 hardware_error=true" << std::endl;
    std::cout << "  clear / c  : 在 ERROR 中进入 RECOVERING，运行恢复树" << std::endl;
    std::cout << "  quit  / q  : halt 所有树并退出程序" << std::endl;
    std::cout << std::endl;
}

}  // namespace fsm_supervisor
