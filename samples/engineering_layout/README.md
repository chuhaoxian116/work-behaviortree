# engineering_layout：sample 内部的工程化拆分

这个示例仍然放在 `samples/` 下，但内部模拟正式工程结构：

```text
engineering_layout/
├── CMakeLists.txt
├── README.md
├── tree.xml
├── main.cpp
├── include/
│   └── engineering_layout/
│       └── bt_nodes/
│           ├── battery_ok.hpp
│           ├── move_to.hpp
│           ├── say.hpp
│           └── register_nodes.hpp
└── src/
    └── bt_nodes/
        ├── battery_ok.cpp
        ├── move_to.cpp
        ├── say.cpp
        └── register_nodes.cpp
```

目标是学习：

- 节点声明放 `include/`；
- 节点实现放 `src/`；
- 节点注册集中到 `register_nodes.cpp`；
- `main.cpp` 只负责组装行为树；
- CMake 先编译节点库，再让示例程序链接节点库。

## 运行

```bash
cd /home/js/behaviorTr/Demo/myproject
cmake -S . -B build
cmake --build build -j4
./build/samples/engineering_layout/engineering_layout_bt
```

## 关键结构

节点库：

```cmake
add_library(engineering_layout_bt_nodes
  src/bt_nodes/battery_ok.cpp
  src/bt_nodes/move_to.cpp
  src/bt_nodes/say.cpp
  src/bt_nodes/register_nodes.cpp)
```

示例程序：

```cmake
add_executable(engineering_layout_bt main.cpp)
target_link_libraries(engineering_layout_bt
  PRIVATE engineering_layout_bt_nodes)
```

注册函数：

```cpp
void registerNodes(BT::BehaviorTreeFactory& factory)
{
    factory.registerNodeType<BatteryOK>("BatteryOK");
    factory.registerNodeType<MoveTo>("MoveTo");
    factory.registerNodeType<Say>("Say");
}
```

