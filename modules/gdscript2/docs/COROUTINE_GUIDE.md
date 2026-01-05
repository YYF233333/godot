# GDScript2 协程/Await 使用指南

## 概述

GDScript2 提供了完整的协程（Coroutine）和 `await` 支持，允许编写异步代码。协程可以在执行过程中挂起，等待信号或其他协程完成后继续执行。

## 核心概念

### 协程（Coroutine）

协程是可以暂停和恢复执行的函数。当函数中使用 `await` 关键字时，该函数会自动成为协程函数。

### await 表达式

`await` 用于等待以下内容：
- **信号（Signal）**：等待信号触发
- **协程对象**：等待另一个协程完成

## 基本用法

### 1. 等待信号

```gdscript
func wait_for_signal_example():
    var timer = Timer.new()
    add_child(timer)
    timer.start(2.0)

    # 等待定时器超时信号
    await timer.timeout

    print("2 秒已过！")
    timer.queue_free()
```

### 2. 等待协程

```gdscript
func async_operation():
    print("开始异步操作...")
    await get_tree().create_timer(1.0).timeout
    print("异步操作完成！")
    return 42

func main():
    # 等待协程完成
    var result = await async_operation()
    print("结果: ", result)  # 输出: 结果: 42
```

### 3. 串行等待

```gdscript
func sequential_operations():
    print("步骤 1")
    await get_tree().create_timer(1.0).timeout

    print("步骤 2")
    await get_tree().create_timer(1.0).timeout

    print("步骤 3")
    await get_tree().create_timer(1.0).timeout

    print("完成！")
```

## 高级用法

### 协程对象操作

```gdscript
func long_operation():
    for i in range(10):
        await get_tree().process_frame
        print("进度: ", i * 10, "%")
    return "完成"

func main():
    # 启动协程
    var coro = long_operation()

    # 可以检查协程状态
    if coro.is_valid():
        print("协程正在运行...")

    # 等待完成
    var result = await coro
    print(result)
```

### 手动控制协程

```gdscript
func manual_coroutine():
    var coro = GDScript2Coroutine.new()

    # 设置完成回调
    coro.set_completion_callback(func(value):
        print("协程完成，返回值: ", value)
    )

    # 手动恢复协程
    coro.resume(42)
```

### 取消协程

```gdscript
func cancelable_operation():
    var coro = long_running_task()

    # 等待一段时间
    await get_tree().create_timer(2.0).timeout

    # 取消协程
    if coro.is_valid():
        coro.cancel()
        print("操作已取消")
```

## 协程状态

协程有以下状态：

- `STATE_SUSPENDED` - 挂起，等待恢复
- `STATE_RUNNING` - 正在运行
- `STATE_COMPLETED` - 已完成
- `STATE_CANCELLED` - 已取消
- `STATE_ERROR` - 发生错误

```gdscript
func check_coroutine_state(coro):
    match coro.get_state():
        GDScript2Coroutine.STATE_SUSPENDED:
            print("协程已挂起")
        GDScript2Coroutine.STATE_RUNNING:
            print("协程正在运行")
        GDScript2Coroutine.STATE_COMPLETED:
            print("协程已完成")
        GDScript2Coroutine.STATE_CANCELLED:
            print("协程已取消")
        GDScript2Coroutine.STATE_ERROR:
            print("协程出错: ", coro.get_error_message())
```

## 实际应用示例

### 场景加载

```gdscript
func load_scene_async(path: String):
    var loader = ResourceLoader.load_threaded_request(path)

    while true:
        var progress = []
        var status = ResourceLoader.load_threaded_get_status(path, progress)

        if status == ResourceLoader.THREAD_LOAD_LOADED:
            break
        elif status == ResourceLoader.THREAD_LOAD_FAILED:
            push_error("加载失败: " + path)
            return null

        # 等待下一帧
        await get_tree().process_frame

    return ResourceLoader.load_threaded_get(path)

func main():
    print("开始加载场景...")
    var scene = await load_scene_async("res://scenes/level1.tscn")
    if scene:
        get_tree().change_scene_to_packed(scene)
```

### 网络请求

```gdscript
func http_get_async(url: String):
    var http = HTTPRequest.new()
    add_child(http)

    http.request(url)
    var result = await http.request_completed

    http.queue_free()
    return result

func fetch_user_data():
    var response = await http_get_async("https://api.example.com/user")
    var json = JSON.parse_string(response[3].get_string_from_utf8())
    return json
```

### 动画序列

```gdscript
func play_animation_sequence():
    var anim_player = $AnimationPlayer

    anim_player.play("intro")
    await anim_player.animation_finished

    anim_player.play("loop")
    await get_tree().create_timer(5.0).timeout

    anim_player.play("outro")
    await anim_player.animation_finished

    print("动画序列完成")
```

### 对话系统

```gdscript
func show_dialogue(lines: Array):
    for line in lines:
        $DialogueLabel.text = line
        $DialogueBox.show()

        # 等待玩家点击
        await $DialogueBox.confirmed

        $DialogueBox.hide()
        await get_tree().create_timer(0.5).timeout

    print("对话结束")

func main():
    await show_dialogue([
        "你好，欢迎来到游戏！",
        "这是一个使用协程的对话系统。",
        "按确认键继续..."
    ])
```

## 最佳实践

### 1. 避免长时间阻塞

```gdscript
# ❌ 不好：长时间循环阻塞
func bad_example():
    for i in range(1000000):
        process_item(i)

# ✅ 好：使用 await 分帧处理
func good_example():
    for i in range(1000000):
        process_item(i)
        if i % 1000 == 0:
            await get_tree().process_frame
```

### 2. 错误处理

```gdscript
func safe_async_operation():
    var coro = risky_operation()

    # 等待完成或超时
    var timer = get_tree().create_timer(5.0)

    # 使用信号竞争
    var result = await coro

    if coro.get_state() == GDScript2Coroutine.STATE_ERROR:
        push_error("操作失败: " + coro.get_error_message())
        return null

    return result
```

### 3. 资源清理

```gdscript
func cleanup_example():
    var resources = []

    try:
        var res1 = await load_resource("res://data1.tres")
        resources.append(res1)

        var res2 = await load_resource("res://data2.tres")
        resources.append(res2)

        # 使用资源...

    finally:
        # 确保清理
        for res in resources:
            res.free()
```

## 性能考虑

1. **协程开销**：每个协程会保存调用栈状态，避免创建过多协程
2. **信号连接**：await 信号会自动管理连接/断开，无需手动处理
3. **内存管理**：完成的协程会自动清理，但可以手动调用 `cleanup_completed()`

## 调试技巧

```gdscript
func debug_coroutine():
    var manager = get_coroutine_manager()
    print("活跃协程数: ", manager.get_active_count())

    for i in range(manager.get_active_count()):
        var coro = manager.get_coroutine(i)
        print("协程 ", i, " 状态: ", coro.get_state())
```

## 注意事项

1. **await 只能在函数内使用**：不能在类级别或全局作用域使用
2. **自动标记为协程**：使用 `await` 的函数会自动成为协程函数
3. **返回值**：协程函数返回 `GDScript2Coroutine` 对象，使用 `await` 获取实际返回值
4. **信号参数**：await 信号时，信号的参数会作为返回值

## 与 GDScript 1.0 的区别

GDScript2 的协程系统相比 GDScript 1.0 的 `yield` 有以下改进：

- 使用更现代的 `await` 语法
- 更好的类型安全
- 完整的状态管理
- 支持协程取消
- 更清晰的错误处理

## 参考

- `vm/gdscript2_coroutine.h` - 协程核心实现
- `vm/gdscript2_vm.cpp` - VM 协程支持
- `tests/test_gdscript2_coroutine.cpp` - 测试用例
