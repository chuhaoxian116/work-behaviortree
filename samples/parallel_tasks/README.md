# parallel_tasks：Parallel 编排多个异步动作

这个示例演示：

```text
一个行为树 tick 线程
  -> Parallel 控制节点
      -> AsyncMoveTo       -> 移动 worker 线程
      -> AsyncDetectObject -> 视觉 worker 线程
      -> AsyncPrepareArm   -> 机械臂 worker 线程
```

关键点：

- `Parallel` 本身不会自动创建系统线程；
- `Parallel` 在每次 `tickRoot()` 中依次 tick 多个子节点；
- 真正的并发来自各个异步节点内部的业务 worker；
- `success_threshold="-1"` 表示所有孩子成功后，`Parallel` 才成功；
- `failure_threshold="1"` 表示任意一个孩子失败，`Parallel` 就失败，并 halt 其它正在运行的孩子。

## 运行

```bash
cd /home/js/behaviorTr/Demo/myproject
cmake -S . -B build
cmake --build build -j4
./build/samples/parallel_tasks/parallel_tasks_bt
```

## 观察重点

第 1 次 tick 会依次启动三个异步任务：

```text
AsyncMoveTo
AsyncDetectObject
AsyncPrepareArm
```

之后每次 tick，行为树只查询状态，不阻塞等待。

视觉任务步骤最少，会先完成；机械臂随后完成；移动最后完成。
`Parallel` 会等三个子节点全部 `SUCCESS` 后，才返回 `SUCCESS`。

你还会观察到：某个子节点已经 `SUCCESS` 后，后续 tick 不会再打印它的
`onRunning()`。这是 BehaviorTree.CPP 3.x `Parallel` 的 skip-list 行为：
已经成功/失败但尚未触发阈值的孩子会被记住，不会每轮重复 tick。

