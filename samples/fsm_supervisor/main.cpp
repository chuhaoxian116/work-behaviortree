#include <chrono>
#include <string>
#include "fsm_supervisor/supervisor/supervisor.hpp"

/**
 * fsm_supervisor 示例程序入口。
 *
 * main() 故意不写 FSM 逻辑，也不注册 BT 节点。这样后续真实工程可以把这里
 * 替换成参数解析、配置加载或服务启动，而不影响 supervisor 的实现。
 */
int main()
{
    fsm_supervisor::FsmSupervisor supervisor({
        std::string(GRASP_TREE_XML_PATH),    // 抓取树 XML，由 CMake 注入源码路径。
        std::string(RECOVER_TREE_XML_PATH),  // 恢复树 XML，由 CMake 注入源码路径。
        std::chrono::milliseconds(200),      // 主控循环周期。
    });

    return supervisor.run();
}
