# 从空白工程开始使用行为树

这个例子不依赖已有 FSM。行为树直接负责以下流程：

1. 检查电量；
2. 电量不足时充电；
3. 前往目标点；
4. 输出任务完成。

## 构建运行

项目直接引用相邻目录中的 `BehaviorTree.CPP-3.8.7`，不需要安装到
`/usr/local`。

```bash
cd Demo/myproject
cmake -S . -B build
cmake --build build -j4
cd build
./my_first_bt
```

## 文件作用

- `tree.xml`：描述“做什么、按什么顺序做”；
- `main.cpp`：实现条件和动作节点，并提供实际数据；
- Blackboard：保存并共享示例中的 `battery` 和 `target`。

## 空白项目需要准备什么

不需要先写 FSM，但仍然需要实现机器人的基础能力，例如读取电量、发送
运动指令、检测运动是否完成。行为树编排这些能力，不会替你实现驱动和
控制算法。

耗时动作应像示例中的 `MoveTo` 一样，每次 tick 做少量工作并返回
`RUNNING`，不要在 `tick()` 中长时间阻塞。能够被抢占的动作还应在
`onHalted()` 中停止电机、取消请求或释放资源。
