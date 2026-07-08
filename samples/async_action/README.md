# async_action：耗时业务接入行为树

这个示例演示真实项目里最常见的写法：

```text
行为树 tick 线程
  -> 行为树节点 AsyncMoveTo
      -> 业务接口 FakeMotionClient
          -> 后台 worker 线程执行耗时移动
```

关键点：

- `AsyncMoveTo::onStart()`：发起业务任务，立刻返回 `RUNNING`；
- `AsyncMoveTo::onRunning()`：非阻塞查询业务状态；
- `AsyncMoveTo::onHalted()`：向业务层发送取消请求；
- 业务耗时工作在 `FakeMotionClient` 的后台线程里执行；
- 行为树通过周期 tick 感知黑板数据变化，不会被黑板变化自动触发。

## 运行

```bash
cd /home/js/behaviorTr/Demo/myproject
cmake -S . -B build
cmake --build build -j4
./build/samples/async_action/async_action_bt
```

## 实验现象

程序开始时电量是 `80%`，行为树允许移动。

第 6 次 tick 后，主循环模拟外部事件，把黑板里的 `battery` 改成 `20%`。

下一次 tick 时：

```text
BatteryOK -> FAILURE
ReactiveSequence -> halt 正在 RUNNING 的 AsyncMoveTo
AsyncMoveTo::onHalted() -> FakeMotionClient::cancelMove()
```

所以你会看到类似输出：

```text
[BT] onHalted: 行为树要求取消移动
[业务接口] 已发送取消移动请求
[业务线程] 移动任务收到取消请求，停止
```

