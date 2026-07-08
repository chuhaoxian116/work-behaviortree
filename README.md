# BehaviorTree.CPP 学习工程

这个目录模仿 `/home/js/cyclonedds/myproject` 的组织方式：

- `samples/`：学习、实验、验证行为树概念；
- `src/`：以后真正开发业务模块时再放；
- `include/`：以后真正需要公共头文件时再放；
- 顶层 `CMakeLists.txt`：只做工程配置、引入 BehaviorTree.CPP、加载 samples。

当前学习代码在：

```text
samples/basic_bt/
samples/async_action/
samples/parallel_tasks/
samples/retry_timeout/
samples/subtree_demo/
samples/blackboard_scope/
samples/engineering_layout/
samples/plugin_nodes/
samples/fsm_supervisor/
```

`samples/basic_bt/` 不依赖已有 FSM。行为树直接负责以下流程：

1. 检查电量；
2. 电量满足后前往目标点；
3. 移动过程中模拟电量下降；
4. `ReactiveSequence` 重新检查电量并中止移动。

`samples/async_action/` 演示真实工程里更常见的异步接入方式：

1. `onStart()` 发起业务任务；
2. 后台 worker 线程模拟耗时移动；
3. `onRunning()` 非阻塞查询业务状态；
4. `onHalted()` 向业务层发送取消请求。

`samples/parallel_tasks/` 演示多个异步业务同时被行为树编排：

1. `Parallel` 同时推进移动、视觉、机械臂准备三个子节点；
2. 每个子节点内部各自启动 worker 线程；
3. `Parallel` 等三个任务全部成功后返回 `SUCCESS`；
4. 任意任务失败时，`Parallel` 会 halt 其它仍在运行的任务。

`samples/retry_timeout/` 演示 Decorator 装饰器：

1. `RetryUntilSuccessful` 让检测节点失败后自动重试；
2. 同步失败的子节点可能在同一次 `tickRoot()` 里被连续 tick 多次；
3. `Timeout` 给异步移动设置最大运行时间；
4. 超时后 `Timeout` 会 halt 正在 `RUNNING` 的子节点并返回 `FAILURE`。

`samples/subtree_demo/` 演示大树拆分：

1. `MainTree` 只描述准备、导航、收尾三个阶段；
2. `PrepareTask`、`NavigateTask`、`FinishTask` 分别描述细节；
3. 普通 `SubTree` 会创建子黑板；
4. 父树和子树之间通过显式 remap 传递输入/输出。

`samples/blackboard_scope/` 专门观察黑板作用域：

1. 普通节点直接读取父树黑板；
2. 普通 `SubTree` 通过显式 remap 读写父树变量；
3. 普通 `SubTree` 不 remap 时读不到父树变量，写入也留在子树黑板；
4. `SubTreePlus __autoremap=true` 会自动映射同名 key。

`samples/engineering_layout/` 演示 sample 内部的工程化拆分：

1. 节点声明放在 sample 自己的 `include/`；
2. 节点实现放在 sample 自己的 `src/`；
3. 节点注册集中在 `register_nodes.cpp`；
4. `main.cpp` 只负责注册、创建黑板、加载 XML 和 tick 行为树。

`samples/plugin_nodes/` 演示节点插件化：

1. 节点编译成独立 `.so` 插件；
2. 插件通过 `BT_REGISTER_NODES` 导出注册入口；
3. 主程序不 include 具体节点类；
4. 主程序通过 `factory.registerFromPlugin()` 动态加载节点。

`samples/fsm_supervisor/` 演示 FSM / Supervisor 如何控制多棵行为树：

1. `IDLE` 状态等待外部命令，不 tick 任务树；
2. `GRASPING` 状态周期性 tick `grasp_tree`；
3. 硬件异常时 FSM 调用 `grasp_tree.haltTree()` 并进入 `ERROR`；
4. `RECOVERING` 状态 tick `recover_tree`，恢复成功后回到 `IDLE`。

## 构建运行

项目使用 `thirdparty/BehaviorTree.CPP-3.8.7-prebuilt` 中的预编译
BehaviorTree.CPP 动态库，不需要安装到 `/usr/local`。

```bash
cd Demo/myproject
cmake -S . -B build
cmake --build build -j4
./build/samples/basic_bt/my_first_bt
./build/samples/async_action/async_action_bt
./build/samples/parallel_tasks/parallel_tasks_bt
./build/samples/retry_timeout/retry_timeout_bt
./build/samples/subtree_demo/subtree_demo_bt
./build/samples/blackboard_scope/blackboard_scope_bt
./build/samples/engineering_layout/engineering_layout_bt
./build/samples/plugin_nodes/plugin_nodes_bt
./build/samples/fsm_supervisor/fsm_supervisor_bt
```

## 文件作用

- `samples/basic_bt/tree.xml`：描述“做什么、按什么顺序做”；
- `samples/basic_bt/main.cpp`：实现条件和动作节点，并提供实际数据；
- `samples/async_action/tree.xml`：演示反应式前置条件如何中止异步动作；
- `samples/async_action/main.cpp`：演示行为树节点如何适配耗时业务接口；
- `samples/parallel_tasks/tree.xml`：演示 `Parallel` 如何编排多个异步动作；
- `samples/parallel_tasks/main.cpp`：演示移动、视觉、机械臂三个 worker 同时工作；
- `samples/retry_timeout/tree.xml`：演示 `RetryUntilSuccessful` 和 `Timeout`；
- `samples/retry_timeout/main.cpp`：演示失败重试和超时取消；
- `samples/subtree_demo/tree.xml`：演示 `MainTree` 如何调用多个 `SubTree`；
- `samples/subtree_demo/main.cpp`：演示子树黑板 remap 和阶段拆分；
- `samples/blackboard_scope/tree.xml`：演示普通 `SubTree`、显式 remap、`SubTreePlus autoremap`；
- `samples/blackboard_scope/main.cpp`：安全打印当前黑板和父树黑板里的真实值；
- `samples/engineering_layout/`：演示节点声明/实现/注册/main 的工程化拆分；
- `samples/plugin_nodes/`：演示节点 `.so` 插件和 `registerFromPlugin()`；
- `samples/fsm_supervisor/`：演示 FSM 根据命令、硬件反馈和 BT 根节点状态切换大状态；
- Blackboard：保存并共享示例中的 `battery` 和 `target`。

现在程序直接读取源码目录中的 sample XML。所以只改 XML 时，
重新运行程序即可生效；只有改 C++ 时才需要重新编译。

## 空白项目需要准备什么

不需要先写 FSM，但仍然需要实现机器人的基础能力，例如读取电量、发送
运动指令、检测运动是否完成。行为树编排这些能力，不会替你实现驱动和
控制算法。

耗时动作应像示例中的 `MoveTo` 一样，每次 tick 做少量工作并返回
`RUNNING`，不要在 `tick()` 中长时间阻塞。能够被抢占的动作还应在
`onHalted()` 中停止电机、取消请求或释放资源。
