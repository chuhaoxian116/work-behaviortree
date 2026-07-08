#include <behaviortree_cpp_v3/bt_factory.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

using namespace std::chrono_literals;

enum class TaskStatus
{
    IDLE,
    RUNNING,
    SUCCEEDED,
    CANCELED
};

const char* toString(TaskStatus status)
{
    switch(status)
    {
        case TaskStatus::IDLE:
            return "IDLE";
        case TaskStatus::RUNNING:
            return "RUNNING";
        case TaskStatus::SUCCEEDED:
            return "SUCCEEDED";
        case TaskStatus::CANCELED:
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

class FakeMoveClient
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
            state_->status = TaskStatus::RUNNING;
        }

        std::cout << "[业务接口][SlowMove] startMove(): 启动慢速移动 worker，target="
                  << target << std::endl;

        auto state = state_;
        std::thread([state, generation]() {
            std::cout << "[业务线程][SlowMove] worker 已启动，预计约 3 秒完成"
                      << std::endl;

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
                    state->status = TaskStatus::CANCELED;
                    std::cout << "[业务线程][SlowMove] 检测到取消请求，停止移动"
                              << std::endl;
                    return;
                }

                state->progress = step;
                std::cout << "[业务线程][SlowMove] 移动中: " << state->progress
                          << "/10" << std::endl;
            }

            std::lock_guard<std::mutex> lock(state->mutex);
            if(generation == state->generation)
            {
                state->status = TaskStatus::SUCCEEDED;
                std::cout << "[业务线程][SlowMove] 移动完成" << std::endl;
            }
        }).detach();
    }

    TaskStatus queryStatus() const
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        return state_->status;
    }

    int queryProgress() const
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        return state_->progress;
    }

    void cancelMove()
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        if(state_->status == TaskStatus::RUNNING)
        {
            state_->cancel_requested = true;
            std::cout << "[业务接口][SlowMove] cancelMove(): 收到 Timeout/halt 的取消请求"
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
        TaskStatus status = TaskStatus::IDLE;
    };

    std::shared_ptr<State> state_ = std::make_shared<State>();
};

class FlakyDetectObject : public BT::SyncActionNode
{
public:
    FlakyDetectObject(const std::string& name, const BT::NodeConfiguration& config)
      : BT::SyncActionNode(name, config)
    {}

    static BT::PortsList providedPorts()
    {
        return { BT::InputPort<std::string>("object") };
    }

    BT::NodeStatus tick() override
    {
        const auto object = getInput<std::string>("object");
        if(!object)
        {
            throw BT::RuntimeError("FlakyDetectObject 缺少 object 输入");
        }

        ++attempt_;
        std::cout << "[BT][FlakyDetectObject] tick(): 第 " << attempt_
                  << " 次尝试检测 " << object.value() << std::endl;

        if(attempt_ < 3)
        {
            std::cout << "[返回][FlakyDetectObject] FAILURE，模拟检测失败；Retry 会重新 tick 我"
                      << std::endl;
            return BT::NodeStatus::FAILURE;  
        }

        std::cout << "[返回][FlakyDetectObject] SUCCESS，第三次检测成功"
                  << std::endl;
        return BT::NodeStatus::SUCCESS;
    }

private:
    int attempt_ = 0;
};



class StatefulFlakyDetectObject : public BT::StatefulActionNode
{
public:
    StatefulFlakyDetectObject(const std::string& name,
                              const BT::NodeConfiguration& config)
      : BT::StatefulActionNode(name, config)
    {}

    static BT::PortsList providedPorts()
    {
        return { BT::InputPort<std::string>("object") };
    }

    BT::NodeStatus onStart() override
    {
        auto object = getInput<std::string>("object");
        if(!object)
        {
            throw BT::RuntimeError("FlakyDetectObject 缺少 object 输入");
        }

        object_ = object.value();
        progress_ = 0;

        std::cout << "[检测] 开始检测: " << object_ << std::endl;

        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        ++progress_;

        std::cout << "[检测] 尝试检测 "<< object_ << " 中: " << progress_ << "/3" << std::endl;

        if(progress_ < 3)
        {
            return BT::NodeStatus::RUNNING;
        }

        std::cout << "[检测] 检测成功" << std::endl;
        return BT::NodeStatus::SUCCESS;
    }

    void onHalted() override
    {
        std::cout << "[检测] 被取消" << std::endl;
    }

private:
    std::string object_;
    int progress_ = 0;
};



class SlowMoveTo : public BT::StatefulActionNode
{
public:
    SlowMoveTo(const std::string& name, const BT::NodeConfiguration& config)
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
            throw BT::RuntimeError("SlowMoveTo 缺少 target 输入");
        }

        std::cout << "[BT][SlowMoveTo] onStart(): 发起慢速移动" << std::endl;
        move_client_.startMove(target.value());
        std::cout << "[返回][SlowMoveTo::onStart] RUNNING，Timeout 开始计时"
                  << std::endl;
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        const auto status = move_client_.queryStatus();
        const auto progress = move_client_.queryProgress();

        std::cout << "[BT][SlowMoveTo] onRunning(): status=" << toString(status)
                  << ", progress=" << progress << "/10" << std::endl;

        switch(status)
        {
            case TaskStatus::RUNNING:
                std::cout << "[返回][SlowMoveTo::onRunning] RUNNING，移动还没结束"
                          << std::endl;
                return BT::NodeStatus::RUNNING;
            case TaskStatus::SUCCEEDED:
                std::cout << "[返回][SlowMoveTo::onRunning] SUCCESS，移动完成"
                          << std::endl;
                return BT::NodeStatus::SUCCESS;
            case TaskStatus::CANCELED:
            case TaskStatus::IDLE:
                std::cout << "[返回][SlowMoveTo::onRunning] FAILURE，移动取消/未启动"
                          << std::endl;
                return BT::NodeStatus::FAILURE;
        }

        return BT::NodeStatus::FAILURE;
    }

    void onHalted() override
    {
        std::cout << "[BT][SlowMoveTo] onHalted(): 子节点被 halt" << std::endl;
        std::cout << "[关键点] 这里通常是 Timeout 到点了，或父节点决定取消动作"
                  << std::endl;
        move_client_.cancelMove();
    }

private:
    FakeMoveClient move_client_;
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
    std::cout << "[初始化] Retry / Timeout 装饰器示例" << std::endl;
    std::cout << "[说明] RetryUntilSuccessful 会在子节点 FAILURE 后重新 tick 子节点"
              << std::endl;
    std::cout << "[说明] Timeout 会在子节点 RUNNING 超过 msec 后 halt 子节点并返回 FAILURE"
              << std::endl;

    BT::BehaviorTreeFactory factory;
    factory.registerNodeType<FlakyDetectObject>("FlakyDetectObject");
    factory.registerNodeType<StatefulFlakyDetectObject>("StatefulFlakyDetectObject");
    factory.registerNodeType<SlowMoveTo>("SlowMoveTo");
    factory.registerNodeType<Say>("Say");

    auto blackboard = BT::Blackboard::create();
    blackboard->set("object", std::string("红色方块"));
    blackboard->set("target", std::string("工作区 B"));

    std::cout << "[初始化] 黑板 object=红色方块, target=工作区 B" << std::endl;
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

    // 给后台线程一点时间打印 cancel 后的停止日志。
    std::this_thread::sleep_for(500ms);

    std::cout << "\n行为树结束，结果: " << toString(status) << std::endl;
    return status == BT::NodeStatus::SUCCESS ? 0 : 1;
}
