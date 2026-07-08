# retry_timeout：Decorator / Retry / Timeout

这个示例演示两个常用装饰器：

```text
RetryUntilSuccessful
Timeout
```

Decorator 的特点是：

```text
它只包住一个子节点；
它不实现业务动作；
它改变子节点的执行语义。
```

## XML 核心

```xml
<RetryUntilSuccessful num_attempts="3">
  <FlakyDetectObject object="{object}"/>
</RetryUntilSuccessful>

<Timeout msec="1200">
  <SlowMoveTo target="{target}"/>
</Timeout>
```

## 运行

```bash
cd /home/js/behaviorTr/Demo/myproject
cmake -S . -B build
cmake --build build -j4
./build/samples/retry_timeout/retry_timeout_bt
```

## 观察重点

`FlakyDetectObject` 前两次返回 `FAILURE`，第三次返回 `SUCCESS`。

因为它是同步失败，所以 `RetryUntilSuccessful` 可能会在同一次
`tickRoot()` 中连续 tick 它多次。

`SlowMoveTo` 会启动一个后台 worker，预计约 3 秒完成。

但 XML 中的 `Timeout` 只给它 `1200ms`：

```text
SlowMoveTo -> RUNNING
Timeout 计时
超时
Timeout halt SlowMoveTo
SlowMoveTo::onHalted() 发送 cancelMove()
Timeout 返回 FAILURE
```

所以这个示例最终会以 `FAILURE` 结束，这是预期结果。

