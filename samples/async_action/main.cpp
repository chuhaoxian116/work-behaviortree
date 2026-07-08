#include <behaviortree_cpp_v3/bt_factory.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

using namespace std::chrono_literals;

enum class MotionStatus
{
    IDLE,
    RUNNING,
    SUCCEEDED,
    FAILED,
    CANCELED
};

const char* toString(MotionStatus status)
{
    switch(status)
    {
        case MotionStatus::IDLE:
            return "IDLE";
        case MotionStatus::RUNNING:
            return "RUNNING";
        case MotionStatus::SUCCEEDED:
            return "SUCCEEDED";
        case MotionStatus::FAILED:
            return "FAILED";
        case MotionStatus::CANCELED:
            return "CANCELED";
    }
    return "UNKNOWN";
}

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

// 模拟真实业务层：导航、机械臂、DDS/ROS action client 都可以长得像这样。
// 行为树不在 tick 里阻塞等待它完成，只周期性 queryStatus()。
class FakeMotionClient
{
public:
    void startMove(const std::string& target)
    {
        int generation = 0;
        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            ++state_->generation;
            generation = state_->generation;
            state_->target = target;
            state_->progress = 0;
            state_->cancel_requested = false;
            state_->status = MotionStatus::RUNNING;
        }

        std::cout << "[业务接口] startMove(): 收到移动命令: " << target << std::endl;
        std::cout << "[说明] startMove() 只负责启动后台线程，马上返回；不阻塞行为树 tick"
                  << std::endl;

        auto state = state_;
        std::thread([state, generation]() {
            std::cout << "[业务线程] worker 已启动，开始模拟耗时移动" << std::endl;

            for(int step = 1; step <= 10; ++step)
            {
                std::this_thread::sleep_for(300ms);

                std::lock_guard<std::mutex> lock(state->mutex);
                if(generation != state->generation)
                {
                    return;
                }

                if(state->cancel_requested)
                {
                    state->status = MotionStatus::CANCELED;
                    std::cout << "[业务线程] 检测到 cancel_requested=true，移动任务停止"
                              << std::endl;
                    return;
                }

                state->progress = step;
                std::cout << "[业务线程] 后台移动中: " << state->progress << "/10"
                          << "，行为树 tick 线程没有被阻塞" << std::endl;
            }

            std::lock_guard<std::mutex> lock(state->mutex);
            if(generation == state->generation)
            {
                state->status = MotionStatus::SUCCEEDED;
                std::cout << "[业务线程] 移动完成" << std::endl;
            }
        }).detach();
    }

    MotionStatus queryStatus() const
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        std::cout << "[业务接口] queryStatus(): 非阻塞读取状态 -> "
                  << toString(state_->status) << std::endl;
        return state_->status;
    }

    int queryProgress() const
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        std::cout << "[业务接口] queryProgress(): 非阻塞读取进度 -> "
                  << state_->progress << "/10" << std::endl;
        return state_->progress;
    }

    void cancelMove()
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        if(state_->status == MotionStatus::RUNNING)
        {
            state_->cancel_requested = true;
            std::cout << "[业务接口] cancelMove(): 已发送取消移动请求" << std::endl;
            std::cout << "[说明] cancelMove() 只设置取消标志，真正停止由后台线程自己检查"
                      << std::endl;
        }
    }

private:
    struct State
    {
        mutable std::mutex mutex;
        std::string target;
        int progress = 0;
        int generation = 0;
        bool cancel_requested = false;
        MotionStatus status = MotionStatus::IDLE;
    };

    std::shared_ptr<State> state_ = std::make_shared<State>();
};

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

        std::cout << "[BT][BatteryOK] tick(): 检查黑板 battery = "
                  << battery.value() << "%" << std::endl;
        std::cout << "[返回][BatteryOK] " << toString(result)
                  << (result == BT::NodeStatus::SUCCESS
                          ? "，前置条件满足，ReactiveSequence 可以继续 tick AsyncMoveTo"
                          : "，前置条件失败，ReactiveSequence 会中止后面的 RUNNING 节点")
                  << std::endl;
        return result;
    }
};

class AsyncMoveTo : public BT::StatefulActionNode
{
public:
    AsyncMoveTo(const std::string& name, const BT::NodeConfiguration& config)
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
            throw BT::RuntimeError("AsyncMoveTo 缺少 target 输入");
        }

        std::cout << "[BT][AsyncMoveTo] onStart(): 第一次进入动作节点" << std::endl;
        std::cout << "[BT][AsyncMoveTo] onStart(): 向业务层发起异步移动 target = "
                  << target.value() << std::endl;
        motion_client_.startMove(target.value());
        std::cout << "[返回][AsyncMoveTo::onStart] RUNNING，告诉行为树：动作已开始，但还没完成"
                  << std::endl;
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        std::cout << "[BT][AsyncMoveTo] onRunning(): 动作上次返回 RUNNING，本轮继续查询"
                  << std::endl;
        const auto status = motion_client_.queryStatus();
        const auto progress = motion_client_.queryProgress();

        std::cout << "[BT][AsyncMoveTo] 当前业务状态 = " << toString(status)
                  << ", progress = " << progress << "/10" << std::endl;

        switch(status)
        {
            case MotionStatus::RUNNING:
                std::cout << "[返回][AsyncMoveTo::onRunning] RUNNING，业务还在后台执行"
                          << std::endl;
                return BT::NodeStatus::RUNNING;
            case MotionStatus::SUCCEEDED:
                std::cout << "[返回][AsyncMoveTo::onRunning] SUCCESS，业务完成"
                          << std::endl;
                return BT::NodeStatus::SUCCESS;
            case MotionStatus::FAILED:
            case MotionStatus::CANCELED:
            case MotionStatus::IDLE:
                std::cout << "[返回][AsyncMoveTo::onRunning] FAILURE，业务失败/取消/未启动"
                          << std::endl;
                return BT::NodeStatus::FAILURE;
        }

        return BT::NodeStatus::FAILURE;
    }

    void onHalted() override
    {
        std::cout << "[BT][AsyncMoveTo] onHalted(): 行为树要求取消移动" << std::endl;
        std::cout << "[关键点] halt 不是业务线程自己触发的，是 ReactiveSequence 发现前置条件失败后触发的"
                  << std::endl;
        motion_client_.cancelMove();
    }

private:
    FakeMotionClient motion_client_;
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

        std::cout << "[BT][Say] tick(): " << text.value() << std::endl;
        std::cout << "[返回][Say] SUCCESS" << std::endl;
        return BT::NodeStatus::SUCCESS;
    }
};

int main()
{
    BT::BehaviorTreeFactory factory;
    std::cout << "[初始化] 注册 C++ 节点类型到 BehaviorTreeFactory" << std::endl;
    factory.registerNodeType<BatteryOK>("BatteryOK");
    factory.registerNodeType<AsyncMoveTo>("AsyncMoveTo");
    factory.registerNodeType<Say>("Say");

    auto blackboard = BT::Blackboard::create();
    blackboard->set("battery", 80);
    blackboard->set("target", std::string("工作区 A"));
    std::cout << "[初始化] 黑板 battery=80, target=工作区 A" << std::endl;

    std::cout << "[初始化] 从 XML 创建行为树: " << TREE_XML_PATH << std::endl;
    auto tree = factory.createTreeFromFile(TREE_XML_PATH, blackboard);
    std::cout << "[说明] XML 使用 ReactiveSequence：每次 tick 都会先重新检查 BatteryOK"
              << std::endl;

    BT::NodeStatus status = BT::NodeStatus::IDLE;
    int tick_count = 0;

    while(status == BT::NodeStatus::IDLE || status == BT::NodeStatus::RUNNING)
    {
        ++tick_count;
        std::cout << "\n========== tick " << tick_count << " ==========" << std::endl;
        std::cout << "[主循环] 调用 tree.tickRoot()" << std::endl;

        status = tree.tickRoot();
        std::cout << "[主循环] tickRoot() 返回 " << toString(status) << std::endl;

        // 模拟外部传感器/系统状态变化：移动过程中电量突然变低。
        // 注意：行为树不会被黑板变化自动触发，它会在下一次 tick 感知到变化。
        if(tick_count == 6 && status == BT::NodeStatus::RUNNING)
        {
            blackboard->set("battery", 20);
            std::cout << "[外部事件] 模拟电量突然下降到 20%" << std::endl;
            std::cout << "[关键点] 黑板变化不会立刻打断动作；下一次 tick 才会被 BatteryOK 读到"
                      << std::endl;
        }

        if(status == BT::NodeStatus::RUNNING)
        {
            std::cout << "[主循环] 行为树仍在 RUNNING，sleep 200ms 后继续 tick"
                      << std::endl;
            std::this_thread::sleep_for(200ms);
        }
    }

    // 给后台线程一点时间打印“收到取消请求”的日志。
    std::this_thread::sleep_for(400ms);

    std::cout << "\n行为树结束，结果: "
              << (status == BT::NodeStatus::SUCCESS ? "SUCCESS" : "FAILURE")
              << std::endl;
    return status == BT::NodeStatus::SUCCESS ? 0 : 1;
}
