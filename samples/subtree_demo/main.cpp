#include <behaviortree_cpp_v3/bt_factory.h>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

using namespace std::chrono_literals;

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

template <typename T>
void printBlackboardValue(const BT::Blackboard::Ptr& blackboard,
                          const std::string& key)
{
    T value{};
    if(blackboard->get(key, value))
    {
        std::cout << "  " << key << " = " << value << std::endl;
    }
    else
    {
        std::cout << "  " << key << " = <未设置或类型不匹配>" << std::endl;
    }
}

class BatteryOK : public BT::ConditionNode
{
public:
    BatteryOK(const std::string& name, const BT::NodeConfiguration& config)
      : BT::ConditionNode(name, config)
    {}

    static BT::PortsList providedPorts()
    {
        return { BT::InputPort<int>("battery") };
    }

    BT::NodeStatus tick() override
    {
        const auto battery = getInput<int>("battery");
        if(!battery)
        {
            throw BT::RuntimeError("BatteryOK 缺少 battery 输入");
        }

        const auto result = battery.value() >= 60 ? BT::NodeStatus::SUCCESS
                                                  : BT::NodeStatus::FAILURE;

        std::cout << "[BT][BatteryOK] 从子树黑板读取 battery = "
                  << battery.value() << "% -> " << toString(result) << std::endl;
        return result;
    }
};

class MoveTo : public BT::StatefulActionNode
{
public:
    MoveTo(const std::string& name, const BT::NodeConfiguration& config)
      : BT::StatefulActionNode(name, config)
    {}

    static BT::PortsList providedPorts()
    {
        return { BT::InputPort<std::string>("target") };
    }

    BT::NodeStatus onStart() override
    {
        const auto target = getInput<std::string>("target");
        if(!target)
        {
            throw BT::RuntimeError("MoveTo 缺少 target 输入");
        }

        target_ = target.value();
        progress_ = 0;
        std::cout << "[BT][MoveTo] onStart(): 子树收到 target = "
                  << target_ << std::endl;
        std::cout << "[返回][MoveTo::onStart] RUNNING" << std::endl;
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        ++progress_;
        std::cout << "[BT][MoveTo] onRunning(): 前往 " << target_
                  << "，进度 " << progress_ << "/4" << std::endl;

        if(progress_ >= 4)
        {
            std::cout << "[返回][MoveTo::onRunning] SUCCESS" << std::endl;
            return BT::NodeStatus::SUCCESS;
        }

        std::cout << "[返回][MoveTo::onRunning] RUNNING" << std::endl;
        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override
    {
        std::cout << "[BT][MoveTo] onHalted(): 移动被取消" << std::endl;
    }

private:
    std::string target_;
    int progress_ = 0;
};

class Say : public BT::SyncActionNode
{
public:
    Say(const std::string& name, const BT::NodeConfiguration& config)
      : BT::SyncActionNode(name, config)
    {}

    static BT::PortsList providedPorts()
    {
        return { BT::InputPort<std::string>("text") };
    }

    BT::NodeStatus tick() override
    {
        const auto text = getInput<std::string>("text");
        if(!text)
        {
            throw BT::RuntimeError("Say 缺少 text 输入");
        }

        std::cout << "[BT][Say] " << text.value() << std::endl;
        return BT::NodeStatus::SUCCESS;
    }
};

int main()
{
    std::cout << "[初始化] SubTree 子树拆分示例" << std::endl;
    std::cout << "[说明] MainTree 只描述任务阶段，PrepareTask/NavigateTask/FinishTask 描述细节"
              << std::endl;
    std::cout << "[说明] 普通 SubTree 会创建子黑板，需要显式 remap 输入/输出 key"
              << std::endl;

    BT::BehaviorTreeFactory factory;
    factory.registerNodeType<BatteryOK>("BatteryOK");
    factory.registerNodeType<MoveTo>("MoveTo");
    factory.registerNodeType<Say>("Say");

    auto blackboard = BT::Blackboard::create();
    blackboard->set("battery", 80);
    blackboard->set("target", std::string("工作区 C"));

    std::cout << "[初始化] 父树黑板 battery=80, target=工作区 C" << std::endl;
    std::cout << "[初始化] 从 XML 创建行为树: " << TREE_XML_PATH << std::endl;

    auto tree = factory.createTreeFromFile(TREE_XML_PATH, blackboard);

    BT::NodeStatus status = BT::NodeStatus::IDLE;
    int tick_count = 0;

    while(status == BT::NodeStatus::IDLE || status == BT::NodeStatus::RUNNING)
    {
        ++tick_count;
        std::cout << "\n========== tick " << tick_count << " ==========" << std::endl;
        std::cout << "[主循环] 调用 tree.tickRoot()" << std::endl;

        status = tree.tickRoot();

        std::cout << "[主循环] tickRoot() 返回 " << toString(status) << std::endl;
        if(status == BT::NodeStatus::RUNNING)
        {
            std::cout << "[主循环] 行为树仍在 RUNNING，sleep 200ms 后继续 tick"
                      << std::endl;
            std::this_thread::sleep_for(200ms);
        }
    }

    std::cout << "\n[调试] 父树黑板内容：" << std::endl;
    blackboard->debugMessage();

    std::cout << "\n[调试] 父树黑板实际值：" << std::endl;
    printBlackboardValue<int>(blackboard, "battery");
    printBlackboardValue<std::string>(blackboard, "target");
    printBlackboardValue<std::string>(blackboard, "prepare_message");
    printBlackboardValue<std::string>(blackboard, "nav_result");
    printBlackboardValue<std::string>(blackboard, "finish_message");

    std::cout << "\n行为树结束，结果: " << toString(status) << std::endl;
    return status == BT::NodeStatus::SUCCESS ? 0 : 1;
}
