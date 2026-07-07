#include <behaviortree_cpp_v3/bt_factory.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

// 条件节点：只检查当前电量，不保存状态。
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

        std::cout << "[检查] 当前电量: " << battery.value() << "%" << std::endl;
        return battery.value() >= 60 ? BT::NodeStatus::SUCCESS
                                     : BT::NodeStatus::FAILURE;
    }
};

// 有状态动作：每次 tick 只充一部分电，因此不会阻塞主循环。
class ChargeBattery : public BT::StatefulActionNode
{
public:
    ChargeBattery(const std::string& name, const BT::NodeConfiguration& config)
      : BT::StatefulActionNode(name, config)
    {}

    static BT::PortsList providedPorts()
    {
        return { BT::BidirectionalPort<int>("battery") };
    }

    BT::NodeStatus onStart() override
    {
        std::cout << "[充电] 开始" << std::endl;
        return chargeOnce();
    }

    BT::NodeStatus onRunning() override
    {
        return chargeOnce();
    }

    void onHalted() override
    {
        std::cout << "[充电] 被中止" << std::endl;
    }

private:
    BT::NodeStatus chargeOnce()
    {
        const auto input = getInput<int>("battery");
        if(!input)
        {
            throw BT::RuntimeError("ChargeBattery 缺少 battery 输入");
        }

        const int battery = std::min(input.value() + 20, 100);
        setOutput("battery", battery);
        std::cout << "[充电] 当前电量: " << battery << "%" << std::endl;

        return battery >= 60 ? BT::NodeStatus::SUCCESS
                             : BT::NodeStatus::RUNNING;
    }
};

// 有状态动作：用三个 tick 模拟移动过程。
class MoveTo : public BT::StatefulActionNode
{
public:
    MoveTo(const std::string& name, const BT::NodeConfiguration& config)
      : BT::StatefulActionNode(name, config)
    {}

    static BT::PortsList providedPorts()
    {
        return {
            BT::InputPort<std::string>("target"),
            BT::BidirectionalPort<int>("battery")
        };
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
        std::cout << "[移动] 开始前往: " << target_ << std::endl;
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        
        ++progress_;

        if(progress_ == 2)
        {
            setOutput("battery", 20);
            std::cout << "[移动] 模拟电量突然下降到 20%" << std::endl;
        }
        std::cout << "[移动] 进度: " << progress_ << "/3" << std::endl;
        return progress_ >= 3 ? BT::NodeStatus::SUCCESS
                              : BT::NodeStatus::RUNNING;
    }

    void onHalted() override
    {
        std::cout << "[移动] 被中止" << std::endl;
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
        std::cout << "[消息] " << text.value() << std::endl;
        return BT::NodeStatus::SUCCESS;
    }
};

int main()
{
    BT::BehaviorTreeFactory factory;
    factory.registerNodeType<BatteryOK>("BatteryOK");
    factory.registerNodeType<ChargeBattery>("ChargeBattery");
    factory.registerNodeType<MoveTo>("MoveTo");
    factory.registerNodeType<Say>("Say");

    auto blackboard = BT::Blackboard::create();
    blackboard->set("battery", 80);
    blackboard->set("target", std::string("工作区 A"));

    auto tree = factory.createTreeFromFile(TREE_XML_PATH, blackboard);

    BT::NodeStatus status = BT::NodeStatus::IDLE;
    while(status == BT::NodeStatus::IDLE || status == BT::NodeStatus::RUNNING)
    {
        status = tree.tickRoot();
        std::cout << "循环中" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "行为树结束，结果: "
              << (status == BT::NodeStatus::SUCCESS ? "SUCCESS" : "FAILURE")
              << std::endl;
    return status == BT::NodeStatus::SUCCESS ? 0 : 1;
}
