# BehaviorTree.CPP 学习工程

这个目录模仿 `/home/js/cyclonedds/myproject` 的组织方式：

- `samples/`：学习、实验、验证行为树概念；
- `src/`：以后真正开发业务模块时再放；
- `include/`：以后真正需要公共头文件时再放；
- 顶层 `CMakeLists.txt`：只做工程配置、引入 BehaviorTree.CPP、加载 samples。

当前学习代码在：

```text
samples/basic_bt/
```

它不依赖已有 FSM。行为树直接负责以下流程：

1. 检查电量；
2. 电量满足后前往目标点；
3. 移动过程中模拟电量下降；
4. `ReactiveSequence` 重新检查电量并中止移动。

## 构建运行

项目直接引用相邻目录中的 `BehaviorTree.CPP-3.8.7`，不需要安装到
`/usr/local`。

```bash
cd Demo/myproject
cmake -S . -B build
cmake --build build -j4
cd build
./samples/basic_bt/my_first_bt
```

## 文件作用

- `samples/basic_bt/tree.xml`：描述“做什么、按什么顺序做”；
- `samples/basic_bt/main.cpp`：实现条件和动作节点，并提供实际数据；
- Blackboard：保存并共享示例中的 `battery` 和 `target`。

现在程序直接读取源码目录中的 `samples/basic_bt/tree.xml`。所以只改 XML 时，
重新运行程序即可生效；只有改 C++ 时才需要重新编译。

## 空白项目需要准备什么

不需要先写 FSM，但仍然需要实现机器人的基础能力，例如读取电量、发送
运动指令、检测运动是否完成。行为树编排这些能力，不会替你实现驱动和
控制算法。

耗时动作应像示例中的 `MoveTo` 一样，每次 tick 做少量工作并返回
`RUNNING`，不要在 `tick()` 中长时间阻塞。能够被抢占的动作还应在
`onHalted()` 中停止电机、取消请求或释放资源。
