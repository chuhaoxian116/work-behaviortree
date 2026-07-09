# BehaviorTree.CPP 学习整理：C++ 开发视角

这份文档整理的是最近围绕 BehaviorTree.CPP 3.8.7 开展的学习内容。重点不是单纯记 API，而是从 C++ 工程开发角度理解：

- 行为树到底负责什么；
- 节点应该怎么写；
- XML 如何变成 C++ 对象；
- 黑板、端口、子树如何传递数据；
- 异步动作、状态机、插件化在项目里应该如何组织。

当前学习项目路径：

```text
/home/js/behaviorTr/Demo/myproject
```

BehaviorTree.CPP 源码路径：

```text
/home/js/behaviorTr/Demo/BehaviorTree.CPP-3.8.7
```

---

## 1. 行为树解决什么问题

行为树不是业务执行线程，也不是硬件驱动层。它更像一个“决策调度器”：

```text
行为树负责：
  谁先执行
  谁后执行
  失败后尝试谁
  运行中是否重新检查条件
  超时后是否取消
  多个动作如何组合

行为树不负责：
  直接阻塞等待硬件完成
  替代底层运动控制
  替代视觉识别业务
  替代全局系统状态表
```

一个比较准确的工程分层是：

```text
FSM / Supervisor:
  管理机器人大的运行状态，例如待命、抓取、异常、恢复。

BehaviorTree:
  在某个状态内部组织复杂行为，例如抓取流程、恢复流程、巡检流程。

BT Node:
  对接具体业务接口，例如移动、识别、夹爪、检查电池。

业务库:
  motion / vision / gripper / system 等真实业务模块。
```

行为树真正好用的地方在于：业务节点可以很小，复杂流程靠 XML 组合。

例如：

```xml
<RetryUntilSuccessful num_attempts="3">
  <Timeout msec="2000">
    <DetectObject object="{object}"/>
  </Timeout>
</RetryUntilSuccessful>
```

这表达的是：

```text
检测目标；
每次检测最多 2 秒；
失败或超时就重试；
最多重试 3 次。
```

业务节点 `DetectObject` 本身不需要知道“重试几次”和“超时时间”，这些由行为树组合出来。

---

## 2. tickRoot 是行为树的入口

BehaviorTree.CPP 不会自己后台运行。主程序需要周期性调用：

```cpp
BT::NodeStatus status = tree.tickRoot();
```

源码中 `Tree::tickRoot()` 的核心逻辑是：

```cpp
NodeStatus ret = rootNode()->executeTick();
if (ret == NodeStatus::SUCCESS || ret == NodeStatus::FAILURE)
{
  rootNode()->setStatus(BT::NodeStatus::IDLE);
}
return ret;
```

也就是说：

```text
tickRoot()
  -> rootNode()->executeTick()
  -> 根节点根据自己的类型继续 tick 子节点
  -> 最终返回 SUCCESS / FAILURE / RUNNING
```

如果返回 `RUNNING`，通常主循环下一周期继续 tick：

```cpp
BT::NodeStatus status = BT::NodeStatus::IDLE;

while (true)
{
    status = tree.tickRoot();

    if (status == BT::NodeStatus::SUCCESS ||
        status == BT::NodeStatus::FAILURE)
    {
        break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
}
```

工程上要记住：

```text
行为树是被 tick 驱动的。
外部循环决定 tick 频率。
节点返回 RUNNING 代表“我还没结束，下次 tick 再来看我”。
```

---

## 3. executeTick 的生命周期

`tickRoot()` 调用根节点的 `executeTick()`。每个节点的执行入口也是 `executeTick()`。

源码逻辑可以理解为：

```text
executeTick()
  -> pre tick callback
  -> tick()
  -> post tick callback
  -> setStatus()
  -> return status
```

所以节点不是 `tick()` 返回后就完全结束了。库还会根据返回值更新节点状态。

这个机制会影响调试：

```text
节点的状态变化，不只发生在你自己的 tick() 内。
父节点、Decorator、ControlNode 也可能 reset/halt 子节点。
```

---

## 4. 三种返回状态

行为树节点主要返回三种状态：

| 状态 | 含义 |
|---|---|
| `SUCCESS` | 当前节点完成，并且成功 |
| `FAILURE` | 当前节点完成，但是失败 |
| `RUNNING` | 当前节点还没完成，下次 tick 继续 |

工程判断：

```text
SUCCESS / FAILURE:
  代表本次节点执行已经结束。

RUNNING:
  代表节点生命周期还在继续。
```

注意：

```text
SyncActionNode 不能返回 RUNNING。
StatefulActionNode 可以返回 RUNNING。
ConditionNode 工程上通常不要返回 RUNNING。
```

---

## 5. ConditionNode：条件节点

`ConditionNode` 适合表达“当前状态是否满足”。

例如：

```cpp
class BatteryOK : public BT::ConditionNode
{
public:
    BatteryOK(const std::string& name, const BT::NodeConfiguration& config)
        : BT::ConditionNode(name, config)
    {}

    static BT::PortsList providedPorts()
    {
        return {BT::InputPort<int>("battery")};
    }

    BT::NodeStatus tick() override
    {
        auto battery = getInput<int>("battery");
        if (!battery)
        {
            return BT::NodeStatus::FAILURE;
        }

        return battery.value() >= 30
            ? BT::NodeStatus::SUCCESS
            : BT::NodeStatus::FAILURE;
    }
};
```

条件节点应该：

```text
只读取状态；
快速返回；
不阻塞；
不发起耗时业务；
通常只返回 SUCCESS / FAILURE。
```

适合做 Condition 的业务：

```text
BatteryOK
NoEmergencyStop
SystemReady
IsWorkModeAuto
IsTargetVisible
HardwareError
```

不适合做 Condition 的业务：

```text
MoveTo
GraspObject
WaitVisionResult
RecoverError
SearchObject
```

这些更适合做 Action。

---

## 6. SyncActionNode：立即完成的动作

`SyncActionNode` 是同步动作节点，但它不能返回 `RUNNING`。

源码里有强制检查：

```cpp
NodeStatus SyncActionNode::executeTick()
{
  auto stat = ActionNodeBase::executeTick();
  if (stat == NodeStatus::RUNNING)
  {
    throw LogicError("SyncActionNode MUST never return RUNNING");
  }
  return stat;
}
```

所以它适合：

```text
打印日志；
设置变量；
简单计算；
发一个立即返回的命令；
发布一个通知；
读取一次状态并返回。
```

例如：

```cpp
class Say : public BT::SyncActionNode
{
public:
    Say(const std::string& name, const BT::NodeConfiguration& config)
        : BT::SyncActionNode(name, config)
    {}

    static BT::PortsList providedPorts()
    {
        return {BT::InputPort<std::string>("text")};
    }

    BT::NodeStatus tick() override
    {
        auto text = getInput<std::string>("text");
        if (!text)
        {
            return BT::NodeStatus::FAILURE;
        }

        std::cout << text.value() << std::endl;
        return BT::NodeStatus::SUCCESS;
    }
};
```

不要在 `SyncActionNode` 里写：

```cpp
while (!done)
{
    // 阻塞等待
}
```

这会卡住行为树 tick 线程。

---

## 7. StatefulActionNode：推荐的异步业务写法

真实机器人业务里，移动、抓取、识别、恢复，大多数都不是立即完成的。推荐使用 `StatefulActionNode`。

源码生命周期：

```text
节点状态是 IDLE:
  调用 onStart()

节点状态是 RUNNING:
  调用 onRunning()

节点被 halt 且状态是 RUNNING:
  调用 onHalted()
```

标准写法：

```cpp
class MoveTo : public BT::StatefulActionNode
{
public:
    MoveTo(const std::string& name, const BT::NodeConfiguration& config)
        : BT::StatefulActionNode(name, config)
    {}

    static BT::PortsList providedPorts()
    {
        return {BT::InputPort<std::string>("target")};
    }

    BT::NodeStatus onStart() override
    {
        auto target = getInput<std::string>("target");
        if (!target)
        {
            return BT::NodeStatus::FAILURE;
        }

        motion_.startMove(target.value());
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        const auto status = motion_.queryMoveStatus();

        if (status == MoveStatus::DONE)
        {
            return BT::NodeStatus::SUCCESS;
        }
        if (status == MoveStatus::FAILED)
        {
            return BT::NodeStatus::FAILURE;
        }

        return BT::NodeStatus::RUNNING;
    }

    void onHalted() override
    {
        motion_.cancelMove();
    }

private:
    MotionClient motion_;
};
```

工程原则：

```text
onStart:
  发起业务。

onRunning:
  非阻塞查询业务状态。

onHalted:
  取消业务。
```

这就是我们反复强调的：

```text
行为树节点不阻塞等待。
耗时业务放业务模块或后台线程里。
行为树只负责发起、查询、取消。
```

---

## 8. AsyncActionNode 和 CoroActionNode

`AsyncActionNode` 会由库内部帮你开线程执行 `tick()`：

```cpp
thread_handle_ = std::async(std::launch::async, [this]() {
  auto status = tick();
  setStatus(status);
});
```

但是官方源码注释也提醒：这个节点比较难正确实现。

原因是：

```text
你的 tick() 在另一个线程运行；
halt 时需要自己检查 isHaltRequested()；
如果 tick() 内部阻塞太久，取消不一定及时。
```

所以工程上优先推荐：

```text
StatefulActionNode + 业务层异步接口
```

`CoroActionNode` 是协程版异步节点，可以用 `setStatusRunningAndYield()` 暂停并返回 `RUNNING`。它适合非阻塞请求/响应模型，但初期可以先不用。

---

## 9. 控制节点：决定子节点如何执行

控制节点是行为树组合能力的核心。

### 9.1 Sequence

```xml
<Sequence>
  <BatteryOK/>
  <MoveTo target="{target}"/>
  <Say text="完成"/>
</Sequence>
```

语义：

```text
从左到右执行。
子节点 SUCCESS -> 继续下一个。
子节点 FAILURE -> 整个 Sequence FAILURE。
子节点 RUNNING -> 整个 Sequence RUNNING。
```

适合普通步骤流程。

### 9.2 ReactiveSequence

```xml
<ReactiveSequence>
  <BatteryOK/>
  <NoEmergencyStop/>
  <MoveTo target="{target}"/>
</ReactiveSequence>
```

语义：

```text
每次 tick 都从第一个子节点重新检查。
如果前置条件失败，会 halt 后面正在 RUNNING 的动作。
```

适合安全检查、前置条件持续监控。

### 9.3 Fallback

```xml
<Fallback>
  <BatteryOK/>
  <ChargeBattery/>
</Fallback>
```

语义：

```text
第一个成功，Fallback 就成功。
前一个失败，才尝试下一个。
所有都失败，Fallback 失败。
```

适合“方案选择”或“补救”。

### 9.4 ReactiveFallback

```xml
<ReactiveFallback>
  <EmergencyStop/>
  <NormalTask/>
</ReactiveFallback>
```

语义：

```text
每次 tick 都从第一个子节点检查。
高优先级分支可以抢占低优先级 RUNNING 分支。
```

适合高优先级事件抢占。

### 9.5 Parallel

```xml
<Parallel success_threshold="1" failure_threshold="1">
  <MoveTo target="{target}"/>
  <KeepRunningUntilFailure>
    <BatteryOK/>
  </KeepRunningUntilFailure>
</Parallel>
```

注意：

```text
Parallel 不是开线程。
它是在一次 tick 中顺序 tick 多个子节点。
真正并发来自子节点内部异步业务。
```

上面这个例子的语义是：

```text
MoveTo 异步移动；
BatteryOK 持续监控；
MoveTo 成功，则 Parallel 成功；
BatteryOK 失败，则 Parallel 失败并 halt MoveTo。
```

### 9.6 Switch

```xml
<Switch3 variable="{mode}" case_1="idle" case_2="grasp" case_3="recover">
  <IdleTask/>
  <GraspTask/>
  <RecoverTask/>
  <DefaultTask/>
</Switch3>
```

语义：

```text
根据黑板变量选择分支。
最后一个子节点是 default。
```

适合按模式值选择不同流程。

---

## 10. Decorator：修饰一个子节点

Decorator 只有一个子节点。它不做业务，只改变子节点的执行策略或返回值。

### 10.1 RetryUntilSuccessful

```xml
<RetryUntilSuccessful num_attempts="3">
  <DetectObject object="{object}"/>
</RetryUntilSuccessful>
```

语义：

```text
子节点 FAILURE -> 重试。
子节点 SUCCESS -> 返回 SUCCESS。
子节点 RUNNING -> 返回 RUNNING，等下一次 tick。
```

重要细节：

```text
如果子节点快速返回 FAILURE，Retry 会在同一次 tickRoot 内继续 while 重试。
如果子节点返回 RUNNING，Retry 会直接返回 RUNNING。
try_count_ 是成员变量，不是每次 tick 自动归零。
```

### 10.2 Timeout

```xml
<Timeout msec="3000">
  <MoveTo target="{target}"/>
</Timeout>
```

语义：

```text
子节点 RUNNING 超过 3000ms，Timeout halt 子节点并返回 FAILURE。
```

重要细节：

```text
Timeout 触发 halt 后，下一次 tick 不会再调用子节点 onRunning。
它会直接返回 FAILURE。
```

### 10.3 Delay

```xml
<Delay delay_msec="1000">
  <Say text="延迟后执行"/>
</Delay>
```

语义：

```text
延迟期间返回 RUNNING。
时间到了以后才 tick 子节点。
```

### 10.4 ForceSuccess

```xml
<ForceSuccess>
  <ReportTaskFinished/>
</ForceSuccess>
```

语义：

```text
子节点结束后，不管 SUCCESS 还是 FAILURE，都返回 SUCCESS。
如果子节点 RUNNING，则继续 RUNNING。
```

适合：

```text
日志上报；
通知服务器；
释放临时资源；
清理现场；
不应该影响主任务结果的附带动作。
```

### 10.5 ForceFailure

```xml
<ForceFailure>
  <Log text="强制切换到下一个 Fallback 分支"/>
</ForceFailure>
```

适合在 `Fallback` 中强制让当前分支失败，从而尝试下一个方案。

### 10.6 KeepRunningUntilFailure

```xml
<KeepRunningUntilFailure>
  <BatteryOK/>
</KeepRunningUntilFailure>
```

语义：

```text
子节点 SUCCESS -> 自己返回 RUNNING。
子节点 RUNNING -> 自己返回 RUNNING。
子节点 FAILURE -> 自己返回 FAILURE。
```

它单独放在 `Sequence` 里通常会卡住后续流程。正确用法通常是放在 `Parallel` 里做持续监控。

---

## 11. Port、Blackboard、`{}`

### 11.1 providedPorts 是节点的端口声明

```cpp
static BT::PortsList providedPorts()
{
    return {
        BT::InputPort<int>("battery"),
        BT::InputPort<std::string>("target")
    };
}
```

这表示节点支持这些 XML 属性。

如果 XML 写了未声明的端口：

```xml
<MoveTo battery="{battery}"/>
```

但 `MoveTo` 没有声明 `battery`，会报错：

```text
Possible typo? In the XML, you tried to remap port "battery" ...
```

### 11.2 `{}` 表示黑板引用

```xml
<BatteryOK battery="{battery}"/>
```

意思是：

```text
BatteryOK 的 battery 端口，从黑板 key = battery 读取值。
```

不带 `{}` 通常表示常量：

```xml
<Say text="任务完成"/>
```

意思是：

```text
text 端口的值就是字符串 "任务完成"。
```

### 11.3 普通节点判断规则

| XML | 含义 |
|---|---|
| `text="hello"` | 常量字符串 `hello` |
| `text="{msg}"` | 从黑板 `msg` 取值 |
| `battery="{battery}"` | 从黑板 `battery` 取 int 值 |

### 11.4 节点端口到底想要“值”还是“key 名”

这个非常容易绕。

例如：

```xml
<BatteryOK battery="{battery}"/>
```

`BatteryOK` 里：

```cpp
getInput<int>("battery");
```

它想拿最终值，所以用 `{battery}`。

而我们实验里的 `TryReadString`：

```xml
<TryReadString key="target"/>
```

它的 C++ 逻辑是：

```cpp
auto key = getInput<std::string>("key");
config().blackboard->get(key.value(), value);
```

它第一步想拿的是 key 名字 `target`，所以不带 `{}`。

规则：

```text
节点端口想要最终值 -> 用 {key}
节点端口想要 key 名字 -> 不用 {}
```

---

## 12. SubTree 和 Blackboard scope

普通 `SubTree` 会给子树创建独立黑板，但可以通过 remap 连接父树变量。

```xml
<SubTree ID="PrepareTask"
         prep_battery="battery"
         prep_message="prepare_message"/>
```

在 BehaviorTree.CPP 3.8.7 的普通 `<SubTree>` 里，这表示：

```text
子树 key prep_battery  -> 父树 key battery
子树 key prep_message  -> 父树 key prepare_message
```

子树里：

```xml
<BatteryOK battery="{prep_battery}"/>
<SetBlackboard output_key="prep_message" value="准备完成"/>
```

实际效果：

```text
BatteryOK 读取父树 battery。
SetBlackboard 写回父树 prepare_message。
```

这不是简单复制值，而是建立名字映射。

可以理解为：

```text
子树局部变量名 prep_battery
背后连接父树变量 battery
```

### SubTreePlus

`SubTreePlus` 更清晰：

```xml
<SubTreePlus ID="Child" child_key="{parent_key}"/>
```

表示 remap：

```text
子树 child_key -> 父树 parent_key
```

而：

```xml
<SubTreePlus ID="Child" child_key="hello"/>
```

表示子树局部常量：

```text
child_key = "hello"
```

---

## 13. BehaviorTreeFactory：注册、创建、插件化

`BehaviorTreeFactory` 是节点注册表和对象工厂。

```cpp
BT::BehaviorTreeFactory factory;
factory.registerNodeType<MoveTo>("MoveTo");
```

内部保存两张表：

```text
builders_:
  "MoveTo" -> 创建 MoveTo 对象的函数

manifests_:
  "MoveTo" -> MoveTo 的端口、类型说明
```

XML：

```xml
<MoveTo target="{target}"/>
```

创建流程：

```text
XMLParser 读取标签名 MoveTo
  -> 查 factory.builders_ 是否有 MoveTo
  -> 查 manifest 校验 target 端口
  -> 创建 NodeConfiguration
  -> 调用 builder
  -> new MoveTo(name, config)
  -> 挂到父节点 children 里
```

完整理解：

```text
registerNodeType:
  只是注册 C++ 类型和字符串 ID 的关系。

createTreeFromFile:
  根据 XML 创建节点对象。

tickRoot:
  真正开始执行行为树。
```

---

## 14. 插件化工程组织

插件本质是把节点注册进 factory。

插件代码：

```cpp
BT_REGISTER_NODES(factory)
{
    factory.registerNodeType<MoveTo>("MoveTo");
    factory.registerNodeType<BatteryOK>("BatteryOK");
}
```

主程序：

```cpp
BT::BehaviorTreeFactory factory;

factory.registerFromPlugin("libmotion_bt_nodes.so");
factory.registerFromPlugin("libvision_bt_nodes.so");
factory.registerFromPlugin("libgripper_bt_nodes.so");
factory.registerFromPlugin("libsystem_bt_nodes.so");

auto tree = factory.createTreeFromFile("grasp_tree.xml");
```

工程含义：

```text
插件库提供节点能力。
XML 决定使用哪些节点。
主程序负责加载插件、创建树、tick 树。
```

加载插件不等于执行插件里所有节点。

```text
只有 XML 中出现的节点才会创建；
只有 tick 到的节点才会执行。
```

推荐结构：

```text
业务库:
  librobot_motion.so
  librobot_vision.so
  librobot_gripper.so
  librobot_system.so

BT 插件:
  libmotion_bt_nodes.so
  libvision_bt_nodes.so
  libgripper_bt_nodes.so
  libsystem_bt_nodes.so

主程序:
  加载插件
  加载 XML
  tick tree
```

BT 插件可以链接业务库，节点内部调用业务接口。

---

## 15. FSM 和行为树如何配合

我们讨论过机器人流程：

```text
待命
抓取
异常
错误清除
恢复
回到待命
```

这个层级适合 FSM / Supervisor 管。

FSM 负责：

```text
根据上层命令切换大状态；
根据硬件反馈进入异常；
决定启动哪棵行为树；
决定停止当前行为树；
根据树的最终结果切换状态。
```

行为树负责：

```text
在某个大状态内部执行复杂流程；
例如抓取流程、恢复流程、巡检流程。
```

典型结构：

```text
FSM 状态：IDLE
  等待命令

FSM 状态：GRASPING
  tick grasp_tree
  tree SUCCESS -> 回 IDLE
  tree FAILURE -> 进入 ERROR

FSM 状态：ERROR
  等待 clear 命令

FSM 状态：RECOVERING
  tick recover_tree
  tree SUCCESS -> 回 IDLE
  tree FAILURE -> 继续 ERROR
```

FSM 不一定需要知道行为树内部每个节点状态。它通常只需要知道：

```text
当前 tree 是否 RUNNING；
当前 tree 最终 SUCCESS 还是 FAILURE；
是否需要 halt 当前 tree；
系统硬件状态是否异常。
```

真实系统里，建议用一个 `RobotContext / SystemStatus` 保存全局系统状态：

```text
电池状态
急停状态
硬件错误
当前命令
运动状态
视觉状态
夹爪状态
```

FSM 和 BT 节点都可以读取这个状态表，但职责不同。

---

## 16. 当前 samples 学习内容

项目中目前的学习 samples 包括：

```text
samples/basic_bt:
  最基础的 BatteryOK / ChargeBattery / MoveTo / Say。

samples/async_action:
  StatefulActionNode 异步动作。
  演示 onStart / onRunning / onHalted。

samples/parallel_tasks:
  多个异步任务组合。
  演示 Parallel 不开线程，真正并发来自业务 worker。

samples/retry_timeout:
  RetryUntilSuccessful 和 Timeout。
  演示一次 tick 内多次 retry，以及 timeout halt。

samples/subtree_demo:
  大行为树拆成多个 SubTree。
  演示普通 SubTree remap。

samples/blackboard_scope:
  黑板作用域、显式 remap、无 remap、SubTreePlus autoremap。

samples/engineering_layout:
  更接近真实工程的 include/src 组织。

samples/plugin_nodes:
  插件化节点注册。

samples/fsm_supervisor:
  FSM + BT supervisor。
  键盘线程输入命令，FSM 选择不同树执行。
```

---

## 17. C++ 开发时的关键规则

### 规则 1：耗时动作不要阻塞 tick 线程

不要这样：

```cpp
BT::NodeStatus MoveTo::tick()
{
    motion.moveTo(target);
    motion.waitUntilDone();
    return BT::NodeStatus::SUCCESS;
}
```

应该这样：

```cpp
onStart()    -> startMove()
onRunning()  -> queryStatus()
onHalted()   -> cancelMove()
```

### 规则 2：Condition 只读状态

Condition 不要发命令、不要等待、不要阻塞。

### 规则 3：节点越小越好

不要把一整个流程写在一个节点里。

推荐：

```text
检查条件 -> Condition
执行业务 -> Action
重试/超时/并行/选择 -> XML 组合
```

### 规则 4：安全条件用 ReactiveSequence

```xml
<ReactiveSequence>
  <NoEmergencyStop/>
  <BatteryOK/>
  <MoveTo target="{target}"/>
</ReactiveSequence>
```

这样移动中也会持续检查安全条件。

### 规则 5：持续监控用 Parallel

```xml
<Parallel success_threshold="1" failure_threshold="1">
  <MoveTo target="{target}"/>
  <KeepRunningUntilFailure>
    <BatteryOK/>
  </KeepRunningUntilFailure>
</Parallel>
```

### 规则 6：业务库和 BT 节点分开

BT 节点不要自己实现复杂业务算法。节点应该调用业务库：

```cpp
motion_client.startMove();
vision_client.startDetect();
gripper_client.close();
```

### 规则 7：插件只注册节点，不写流程

流程应该在 XML 里组合。

---

## 18. 一个推荐的机器人行为树开发模板

### C++ 节点层

```cpp
class DetectObject : public BT::StatefulActionNode
{
public:
    DetectObject(const std::string& name, const BT::NodeConfiguration& config)
        : BT::StatefulActionNode(name, config)
    {}

    static BT::PortsList providedPorts()
    {
        return {
            BT::InputPort<std::string>("object"),
            BT::OutputPort<std::string>("pose")
        };
    }

    BT::NodeStatus onStart() override;
    BT::NodeStatus onRunning() override;
    void onHalted() override;
};
```

### XML 流程层

```xml
<ReactiveSequence>
  <SystemReady/>
  <NoEmergencyStop/>

  <RetryUntilSuccessful num_attempts="3">
    <Timeout msec="2000">
      <DetectObject object="{object}" pose="{object_pose}"/>
    </Timeout>
  </RetryUntilSuccessful>

  <Timeout msec="5000">
    <MoveTo target="{object_pose}"/>
  </Timeout>

  <GraspObject object="{object}"/>
</ReactiveSequence>
```

这个结构里：

```text
SystemReady / NoEmergencyStop:
  条件检查。

RetryUntilSuccessful:
  重试策略。

Timeout:
  超时策略。

DetectObject / MoveTo / GraspObject:
  业务动作。

Blackboard:
  在节点之间传 object_pose。
```

这就是比较健康的行为树设计。

---

## 19. 当前阶段的学习结论

到目前为止，已经形成一个完整认知：

```text
BehaviorTree.CPP 的核心不是“类很多”。
核心是节点状态如何被父节点解释。
```

你以后设计树时，先问：

```text
这个节点是判断状态，还是执行动作？
这个动作是否耗时？
失败后要不要重试？
运行中是否要持续检查安全条件？
是否需要超时取消？
多个任务是否要同时进行？
数据是常量，还是黑板变量？
子树变量是否要映射回父树？
```

对应选择：

```text
判断状态:
  ConditionNode

立即完成动作:
  SyncActionNode

耗时业务:
  StatefulActionNode

顺序流程:
  Sequence

持续前置检查:
  ReactiveSequence

方案选择:
  Fallback / ReactiveFallback

并行监控:
  Parallel

重试:
  RetryUntilSuccessful

超时:
  Timeout

数据传递:
  Blackboard + Port

大流程拆分:
  SubTree

工程插件化:
  BehaviorTreeFactory + BT_REGISTER_NODES
```

这套规则比单纯背 API 更重要。真正开发时，行为树负责组合和调度，C++ 节点负责对接业务能力，FSM 负责管理系统大状态。
