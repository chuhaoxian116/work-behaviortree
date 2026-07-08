# blackboard_scope：黑板作用域与 SubTree remap

这个示例专门观察：

```text
父树黑板
子树黑板
普通 SubTree 显式 remap
普通 SubTree 不 remap
SubTreePlus __autoremap
```

## 运行

```bash
cd /home/js/behaviorTr/Demo/myproject
cmake -S . -B build
cmake --build build -j4
./build/samples/blackboard_scope/blackboard_scope_bt
```

## 实验内容

### 实验 1：普通节点读取父树黑板

```xml
<TryReadString key="target"/>
```

它运行在 `MainTree`，所以直接读取父树黑板里的 `target`。

### 实验 2：普通 SubTree + 显式 remap

```xml
<SubTree ID="ExplicitRemapSubtree"
         child_target="target"
         child_result="explicit_result"/>
```

含义：

```text
子树 child_target  <-> 父树 target
子树 child_result  <-> 父树 explicit_result
```

这是推荐的工程写法，数据流最清楚。

### 实验 3：普通 SubTree 不 remap

```xml
<SubTree ID="NoRemapSubtree"/>
```

子树里尝试读取：

```xml
<TryReadString key="target"/>
```

预期读不到。因为普通 `SubTree` 默认有自己的子黑板，不会自动继承父树变量。

它还会写：

```xml
<SetBlackboard output_key="local_only_result" value="..."/>
```

这个值只在子树黑板里，父树最后读不到。

### 实验 4：SubTreePlus + __autoremap

```xml
<SubTreePlus ID="AutoRemapSubtree" __autoremap="true"/>
```

同名 key 会自动映射到父树。

所以子树直接读：

```xml
<TryReadString key="target"/>
```

可以读到父树的 `target`。

子树写：

```xml
<SetBlackboard output_key="auto_result" value="..."/>
```

父树最后也能读到 `auto_result`。

## 记忆规则

```text
普通节点的 {xxx}：
    从当前黑板读取 xxx 的值

普通 SubTree 的 child_key="parent_key"：
    子树 child_key 映射到父树 parent_key

普通 SubTree 不 remap：
    默认读不到父树变量，也写不回父树变量

SubTreePlus __autoremap=true：
    同名 key 自动映射，方便但隐式
```

