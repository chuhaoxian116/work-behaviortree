# plugin_nodes：行为树节点插件化

这个示例演示把行为树节点编译成独立 `.so` 插件，然后主程序运行时加载。

结构：

```text
plugin_nodes/
├── CMakeLists.txt
├── README.md
├── tree.xml
├── main.cpp
├── include/
│   └── plugin_nodes/
│       └── bt_nodes/
│           ├── battery_ok.hpp
│           ├── move_to.hpp
│           └── say.hpp
└── src/
    ├── plugin.cpp
    └── bt_nodes/
        ├── battery_ok.cpp
        ├── move_to.cpp
        └── say.cpp
```

## 核心思想

普通静态注册是：

```cpp
factory.registerNodeType<BatteryOK>("BatteryOK");
```

插件化注册是：

```cpp
factory.registerFromPlugin("libplugin_nodes_bt_plugin.so");
```

主程序不需要 include 具体节点类。节点类和注册逻辑放在插件 `.so` 里。

## 插件入口

插件中必须有：

```cpp
BT_REGISTER_NODES(factory)
{
    factory.registerNodeType<plugin_nodes::BatteryOK>("BatteryOK");
    factory.registerNodeType<plugin_nodes::MoveTo>("MoveTo");
    factory.registerNodeType<plugin_nodes::Say>("Say");
}
```

这个宏会导出 BehaviorTree.CPP 约定的符号：

```text
BT_RegisterNodesFromPlugin
```

主程序的 `registerFromPlugin()` 会加载 `.so`，查找这个符号，然后调用它。

## CMake 关键点

插件库：

```cmake
add_library(plugin_nodes_bt_plugin SHARED ...)
target_compile_definitions(plugin_nodes_bt_plugin
  PRIVATE BT_PLUGIN_EXPORT)
```

`BT_PLUGIN_EXPORT` 必须定义，否则 `BT_REGISTER_NODES` 会生成 `static` 函数，
主程序找不到插件入口。

主程序：

```cmake
target_compile_definitions(plugin_nodes_bt
  PRIVATE PLUGIN_PATH="$<TARGET_FILE:plugin_nodes_bt_plugin>")
```

这样主程序可以拿到插件 `.so` 的完整路径。

## 运行

```bash
cd /home/js/behaviorTr/Demo/myproject
cmake -S . -B build
cmake --build build -j4
./build/samples/plugin_nodes/plugin_nodes_bt
```

预期看到：

```text
[初始化] 加载插件: .../libplugin_nodes_bt_plugin.so
[plugin][BatteryOK] ...
[plugin][MoveTo] ...
[plugin][Say] 插件化节点示例完成
行为树结束，结果: SUCCESS
```

