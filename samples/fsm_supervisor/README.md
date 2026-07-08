# fsm_supervisor

这个示例演示“FSM / Supervisor 控制多棵行为树”的工程关系。

## 目录结构

```text
fsm_supervisor/
├── include/fsm_supervisor/
│   ├── bt_nodes/
│   │   ├── grasp_nodes.hpp       抓取树用到的 BT 节点声明
│   │   ├── recover_nodes.hpp     恢复树用到的 BT 节点声明
│   │   └── register_nodes.hpp    集中注册 BT 节点
│   └── supervisor/
│       ├── command.hpp           外部命令定义和解析
│       ├── robot_state.hpp       FSM 状态定义
│       └── supervisor.hpp        FSM supervisor 类
├── src/
│   ├── bt_nodes/                 BT 节点实现
│   └── supervisor/               FSM、键盘线程、tick/halt 控制
├── main.cpp                      只负责配置并启动 supervisor
├── grasp_tree.xml                抓取行为树
└── recover_tree.xml              恢复行为树
```

核心关系：

```text
FSM 决定当前大状态：
  IDLE / GRASPING / ERROR / RECOVERING

行为树负责一个状态里的任务流程：
  grasp_tree.xml    抓取流程
  recover_tree.xml  故障恢复流程

FSM 控制行为树生命周期：
  进入状态时开始 tick 对应 tree
  状态退出/异常/停止时调用 tree.haltTree()
  根据 tree.tickRoot() 返回值决定 FSM 跳转
```

这里故意把 BT 节点和 FSM 拆开：

```text
bt_nodes/    只关心“行为树有哪些动作/条件节点”
supervisor/  只关心“当前状态、外部命令、tick 哪棵 tree、何时 halt”
main.cpp     不写业务逻辑，只负责启动
```

运行：

```bash
cd /home/js/behaviorTr/Demo/myproject/build
cmake --build . -j4
./samples/fsm_supervisor/fsm_supervisor_bt
```

键盘命令：

```text
start / s  : 从 IDLE 进入 GRASPING，开始抓取树
stop  / x  : 在 GRASPING 中 halt 抓取树并回到 IDLE
error / e  : 模拟硬件异常 hardware_error=true
clear / c  : 在 ERROR 中进入 RECOVERING，运行恢复树
quit  / q  : halt 所有树并退出程序
```

推荐实验顺序：

```text
s      开始抓取
e      抓取运行中模拟硬件异常，观察 grasp_tree.haltTree()
c      清错恢复，观察 recover_tree.tickRoot()
s      再次抓取，观察任务成功后回到 IDLE
q      退出
```

重点观察：

- 行为树没有单独的 `start()` 后台线程；开始执行就是 FSM 周期性调用 `tickRoot()`。
- 取消行为树不是杀线程，而是 FSM 调用 `haltTree()`，再停止 tick 这棵树。
- `tickRoot()` 返回 `RUNNING / SUCCESS / FAILURE`，FSM 用这个结果决定是否留在当前状态或跳转。
- FSM 不应该钻进行为树内部控制某个子节点；它只控制“哪棵树在运行、什么时候停止、根节点结果是什么”。
