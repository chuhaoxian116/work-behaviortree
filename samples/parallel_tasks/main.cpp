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
    FAILED,
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
        case TaskStatus::FAILED:
            return "FAILED";
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

// 一个通用的“假业务客户端”。
//
// 真实项目里，这里可能是：
// - MotionClient：发导航命令、查导航状态、取消导航；
// - VisionClient：发检测任务、查检测结果、取消检测；
// - ArmClient：发机械臂准备命令、查准备状态、取消动作。
//
// 行为树节点只调用 start/query/cancel，不在 tick 线程里阻塞等待。
class FakeAsyncClient
{
public:
    void startTask(const std::string& task_name,
                   int total_steps,
                   std::chrono::milliseconds step_period)
    {
        int generation = 0;
        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            ++state_->generation;
            generation = state_->generation;
            state_->task_name = task_name;
            state_->progress = 0;
            state_->total_steps = total_steps;
            state_->cancel_requested = false;
            state_->status = TaskStatus::RUNNING;
        }

        std::cout << "[业务接口][" << task_name << "] startTask(): 启动后台 worker"
                  << std::endl;

        auto state = state_;
        std::thread([state, generation, total_steps, step_period]() {
            std::string task_name;
            {
                std::lock_guard<std::mutex> lock(state->mutex);
                task_name = state->task_name;
            }

            std::cout << "[业务线程][" << task_name << "] worker 已启动" << std::endl;

            for(int step = 1; step <= total_steps; ++step)
            {
                std::this_thread::sleep_for(step_period);

                std::lock_guard<std::mutex> lock(state->mutex);
                if(generation != state->generation)
                {
                    return;
                }

                if(state->cancel_requested)
                {
                    state->status = TaskStatus::CANCELED;
                    std::cout << "[业务线程][" << state->task_name
                              << "] 检测到取消请求，停止" << std::endl;
                    return;
                }

                state->progress = step;
                std::cout << "[业务线程][" << state->task_name << "] 执行中: "
                          << state->progress << "/" << state->total_steps
                          << std::endl;
            }

            std::lock_guard<std::mutex> lock(state->mutex);
            if(generation == state->generation)
            {
                state->status = TaskStatus::SUCCEEDED;
                std::cout << "[业务线程][" << state->task_name << "] 完成"
                          << std::endl;
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

    int queryTotalSteps() const
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        return state_->total_steps;
    }

    void cancelTask()
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        if(state_->status == TaskStatus::RUNNING)
        {
            state_->cancel_requested = true;
            std::cout << "[业务接口][" << state_->task_name
                      << "] cancelTask(): 发送取消请求" << std::endl;
        }
    }

private:
    struct State
    {
        mutable std::mutex mutex;
        std::string task_name;
        int progress = 0;
        int total_steps = 0;
        int generation = 0;
        bool cancel_requested = false;
        TaskStatus status = TaskStatus::IDLE;
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
        std::cout << "[BT][BatteryOK] battery=" << battery.value()
                  << "% -> " << toString(result) << std::endl;
        return result;
    }
};

class AsyncTaskNode : public BT::StatefulActionNode
{
public:
    AsyncTaskNode(const std::string& name,
                  const BT::NodeConfiguration& config,
                  std::string task_label,
                  int total_steps,
                  std::chrono::milliseconds step_period)
      : BT::StatefulActionNode(name, config),
        task_label_(std::move(task_label)),
        total_steps_(total_steps),
        step_period_(step_period)
    {}

    BT::NodeStatus onRunning() override
    {
        const auto status = client_.queryStatus();
        const auto progress = client_.queryProgress();
        const auto total_steps = client_.queryTotalSteps();

        std::cout << "[BT][" << task_label_ << "] onRunning(): status="
                  << toString(status) << ", progress=" << progress << "/"
                  << total_steps << std::endl;

        switch(status)
        {
            case TaskStatus::RUNNING:
                std::cout << "[返回][" << task_label_ << "] RUNNING" << std::endl;
                return BT::NodeStatus::RUNNING;
            case TaskStatus::SUCCEEDED:
                std::cout << "[返回][" << task_label_ << "] SUCCESS" << std::endl;
                return BT::NodeStatus::SUCCESS;
            case TaskStatus::FAILED:
            case TaskStatus::CANCELED:
            case TaskStatus::IDLE:
                std::cout << "[返回][" << task_label_ << "] FAILURE" << std::endl;
                return BT::NodeStatus::FAILURE;
        }

        return BT::NodeStatus::FAILURE;
    }

    void onHalted() override
    {
        std::cout << "[BT][" << task_label_
                  << "] onHalted(): Parallel/上层控制节点要求取消" << std::endl;
        client_.cancelTask();
    }

protected:
    void startBusinessTask(const std::string& detail)
    {
        const std::string task_name = task_label_ + ": " + detail;
        std::cout << "[BT][" << task_label_ << "] onStart(): 发起业务任务 -> "
                  << task_name << std::endl;
        client_.startTask(task_name, total_steps_, step_period_);
        std::cout << "[返回][" << task_label_
                  << "] RUNNING，任务已交给 worker，行为树继续周期查询"
                  << std::endl;
    }

private:
    std::string task_label_;
    int total_steps_;
    std::chrono::milliseconds step_period_;
    FakeAsyncClient client_;
};

class AsyncMoveTo : public AsyncTaskNode
{
public:
    AsyncMoveTo(const std::string& name, const BT::NodeConfiguration& config)
      : AsyncTaskNode(name, config, "移动", 8, 300ms)
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

        startBusinessTask("前往 " + target.value());
        return BT::NodeStatus::RUNNING;
    }
};

class AsyncDetectObject : public AsyncTaskNode
{
public:
    AsyncDetectObject(const std::string& name, const BT::NodeConfiguration& config)
      : AsyncTaskNode(name, config, "视觉", 4, 450ms)
    {}

    static BT::PortsList providedPorts()
    {
        return { BT::InputPort<std::string>("object") };
    }

    BT::NodeStatus onStart() override
    {
        const auto object = getInput<std::string>("object");
        if(!object)
        {
            throw BT::RuntimeError("AsyncDetectObject 缺少 object 输入");
        }

        startBusinessTask("检测 " + object.value());
        return BT::NodeStatus::RUNNING;
    }
};

class AsyncPrepareArm : public AsyncTaskNode
{
public:
    AsyncPrepareArm(const std::string& name, const BT::NodeConfiguration& config)
      : AsyncTaskNode(name, config, "机械臂", 6, 350ms)
    {}

    static BT::PortsList providedPorts()
    {
        return {};
    }

    BT::NodeStatus onStart() override
    {
        startBusinessTask("进入抓取准备位");
        return BT::NodeStatus::RUNNING;
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

        std::cout << "[BT][Say] " << text.value() << std::endl;
        return BT::NodeStatus::SUCCESS;
    }
};

int main()
{
    std::cout << "[初始化] Parallel 多异步任务示例" << std::endl;
    std::cout << "[说明] Parallel 会 tick 多个孩子，但不会自动创建系统线程" << std::endl;
    std::cout << "[说明] 真正的并发来自每个异步节点内部的 worker 线程" << std::endl;

    BT::BehaviorTreeFactory factory;
    factory.registerNodeType<BatteryOK>("BatteryOK");
    factory.registerNodeType<AsyncMoveTo>("AsyncMoveTo");
    factory.registerNodeType<AsyncDetectObject>("AsyncDetectObject");
    factory.registerNodeType<AsyncPrepareArm>("AsyncPrepareArm");
    factory.registerNodeType<Say>("Say");

    auto blackboard = BT::Blackboard::create();
    blackboard->set("battery", 80);
    blackboard->set("target", std::string("工作区 A"));
    blackboard->set("object", std::string("红色方块"));

    std::cout << "[初始化] 黑板 battery=80, target=工作区 A, object=红色方块"
              << std::endl;
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
            std::cout << "[主循环] Parallel 至少还有一个孩子 RUNNING，200ms 后继续 tick"
                      << std::endl;
            std::this_thread::sleep_for(200ms);
        }
    }

    // 给后台线程一点时间打印最后的完成日志。
    std::this_thread::sleep_for(300ms);

    std::cout << "\n行为树结束，结果: " << toString(status) << std::endl;
    return status == BT::NodeStatus::SUCCESS ? 0 : 1;
}
