#include <behaviortree_cpp_v3/bt_factory.h>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

using namespace std::chrono_literals;

namespace
{

const char* toString(BT::NodeStatus status)
{
    switch(status)
    {
        case BT::NodeStatus::IDLE:
            return "IDLE";
        case BT::NodeStatus::RUNNING:
            return "RUNNING";
        case BT::NodeStatus::SUCCESS:
            return "SUCCESS";
        case BT::NodeStatus::FAILURE:
            return "FAILURE";
    }
    return "UNKNOWN";
}

}  // namespace

int main()
{
    std::cout << "[初始化] plugin_nodes 插件化示例" << std::endl;
    std::cout << "[说明] main.cpp 不 include 任何具体节点类" << std::endl;
    std::cout << "[说明] 具体节点从插件 .so 动态加载" << std::endl;

    BT::BehaviorTreeFactory factory;

    std::cout << "[初始化] 加载插件: " << PLUGIN_PATH << std::endl;
    factory.registerFromPlugin(PLUGIN_PATH);

    auto blackboard = BT::Blackboard::create();
    blackboard->set("battery", 80);
    blackboard->set("target", std::string("工作区 F"));

    std::cout << "[初始化] 创建行为树: " << TREE_XML_PATH << std::endl;
    auto tree = factory.createTreeFromFile(TREE_XML_PATH, blackboard);

    BT::NodeStatus status = BT::NodeStatus::IDLE;
    while(status == BT::NodeStatus::IDLE || status == BT::NodeStatus::RUNNING)
    {
        status = tree.tickRoot();
        std::cout << "[主循环] tickRoot() -> " << toString(status) << std::endl;

        if(status == BT::NodeStatus::RUNNING)
        {
            std::this_thread::sleep_for(200ms);
        }
    }

    std::cout << "行为树结束，结果: " << toString(status) << std::endl;
    return status == BT::NodeStatus::SUCCESS ? 0 : 1;
}
