#include <behaviortree_cpp_v3/bt_factory.h>

#include <iostream>
#include <string>

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
void printParentValue(const BT::Blackboard::Ptr& blackboard, const std::string& key)
{
    T value{};
    if(blackboard->get(key, value))
    {
        std::cout << "  " << key << " = " << value << std::endl;
    }
    else
    {
        std::cout << "  " << key << " = <父树黑板没有这个 key，或类型不匹配>"
                  << std::endl;
    }
}

class TryReadString : public BT::SyncActionNode
{
public:
    TryReadString(const std::string& name, const BT::NodeConfiguration& config)
      : BT::SyncActionNode(name, config)
    {}

    static BT::PortsList providedPorts()
    {
        return { BT::InputPort<std::string>("key") };
    }

    BT::NodeStatus tick() override
    {
        const auto key = getInput<std::string>("key");
        if(!key)
        {
            throw BT::RuntimeError("TryReadString 缺少 key 输入");
        }

        std::string value;
        const bool found = config().blackboard->get(key.value(), value);

        if(found)
        {
            std::cout << "[TryReadString] 当前黑板读到 key='" << key.value()
                      << "', value='" << value << "'" << std::endl;
        }
        else
        {
            std::cout << "[TryReadString] 当前黑板读不到 key='" << key.value()
                      << "'" << std::endl;
        }

        // 这是观察型节点：读不到也返回 SUCCESS，避免打断后续实验。
        return BT::NodeStatus::SUCCESS;
    }
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

        std::cout << "\n[BT][Say] " << text.value() << std::endl;
        return BT::NodeStatus::SUCCESS;
    }
};

int main()
{
    std::cout << "[初始化] Blackboard scope / SubTree remap 示例" << std::endl;
    std::cout << "[说明] TryReadString 会直接从当前节点所在 blackboard 按 key 查值"
              << std::endl;
    std::cout << "[说明] 读不到也返回 SUCCESS，所以整个实验能继续跑完"
              << std::endl;

    BT::BehaviorTreeFactory factory;
    factory.registerNodeType<TryReadString>("TryReadString");
    factory.registerNodeType<Say>("Say");

    auto blackboard = BT::Blackboard::create();
    blackboard->set("target", std::string("工作区 D"));

    std::cout << "[初始化] 父树黑板 target=工作区 D" << std::endl;
    std::cout << "[初始化] 从 XML 创建行为树: " << TREE_XML_PATH << std::endl;

    auto tree = factory.createTreeFromFile(TREE_XML_PATH, blackboard);

    const auto status = tree.tickRoot();
    std::cout << "\n[主循环] tickRoot() 返回 " << toString(status) << std::endl;

    std::cout << "\n[调试] 父树黑板 debugMessage()：" << std::endl;
    blackboard->debugMessage();

    std::cout << "\n[调试] 父树黑板实际值：" << std::endl;
    printParentValue<std::string>(blackboard, "target");
    printParentValue<std::string>(blackboard, "explicit_result");
    printParentValue<std::string>(blackboard, "auto_result");
    printParentValue<std::string>(blackboard, "local_only_result");

    std::cout << "\n行为树结束，结果: " << toString(status) << std::endl;
    return status == BT::NodeStatus::SUCCESS ? 0 : 1;
}
