# subtree_demo：SubTree 子树拆分

这个示例演示如何把一棵大行为树拆成多个子树：

```text
MainTree
  -> PrepareTask
  -> NavigateTask
  -> FinishTask
```

`MainTree` 只表达任务阶段；具体细节放进不同的 `BehaviorTree ID`。

## XML 核心

```xml
<BehaviorTree ID="MainTree">
  <Sequence>
    <SubTree ID="PrepareTask"
             prep_battery="battery"
             prep_message="prepare_message"/>

    <SubTree ID="NavigateTask"
             nav_target="target"
             nav_result="nav_result"/>

    <SubTree ID="FinishTask"
             finish_message="finish_message"/>
  </Sequence>
</BehaviorTree>
```

普通 `SubTree` 会创建自己的子黑板。属性用于做 remap：

```text
子树内部 key = 父树 key
```

例如：

```xml
<SubTree ID="NavigateTask" nav_target="target"/>
```

意思是：

```text
子树里的 {nav_target}
映射到父树黑板里的 target
```

注意普通 `SubTree` 这里写的是：

```xml
nav_target="target"
```

不是：

```xml
nav_target="{target}"
```

## 运行

```bash
cd /home/js/behaviorTr/Demo/myproject
cmake -S . -B build
cmake --build build -j4
./build/samples/subtree_demo/subtree_demo_bt
```

## 观察重点

- `PrepareTask` 读取父树的 `battery`；
- `PrepareTask` 写出 `prepare_message` 给父树；
- `NavigateTask` 读取父树的 `target`；
- `NavigateTask` 写出 `nav_result` 给父树；
- `FinishTask` 写出 `finish_message` 给父树；
- 主树中的 `Say` 再读取这些输出。

