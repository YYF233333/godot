# GDScript2 信号系统使用指南

## 概述

GDScript2 提供了完整的信号（Signal）系统，允许对象之间进行事件驱动的通信。信号是一种观察者模式的实现，当某个事件发生时，可以通知所有连接到该信号的监听者。

## 核心概念

### 信号（Signal）

信号是对象发出的事件通知。当特定事件发生时，对象可以触发（emit）信号，所有连接到该信号的回调函数都会被调用。

### 连接（Connection）

连接是信号和回调函数之间的关联。一个信号可以有多个连接，每个连接指向一个回调函数。

### 触发（Emit）

触发信号会调用所有连接到该信号的回调函数，可以传递参数给这些回调函数。

## 基本用法

### 1. 定义信号

```gdscript
# 在类中定义信号
signal health_changed
signal damage_taken(amount)
signal player_died(player_name, cause)
```

### 2. 连接信号

```gdscript
# 连接信号到函数
func _ready():
    health_changed.connect(_on_health_changed)
    damage_taken.connect(_on_damage_taken)
    player_died.connect(_on_player_died)

func _on_health_changed():
    print("Health changed!")

func _on_damage_taken(amount):
    print("Took ", amount, " damage")

func _on_player_died(player_name, cause):
    print(player_name, " died from ", cause)
```

### 3. 触发信号

```gdscript
func take_damage(amount):
    health -= amount
    damage_taken.emit(amount)
    health_changed.emit()

    if health <= 0:
        player_died.emit(player_name, "damage")
```

## 高级用法

### 连接到其他对象的信号

```gdscript
func _ready():
    # 连接到其他对象的信号
    $Player.health_changed.connect(_on_player_health_changed)
    $Enemy.died.connect(_on_enemy_died)

func _on_player_health_changed():
    update_health_bar()

func _on_enemy_died():
    spawn_reward()
```

### 使用 Lambda 表达式

```gdscript
func _ready():
    # 使用 lambda 作为回调
    button.pressed.connect(func():
        print("Button pressed!")
    )

    # 带参数的 lambda
    slider.value_changed.connect(func(value):
        print("Slider value: ", value)
    )
```

### 连接标志

```gdscript
func _ready():
    # 一次性连接（触发后自动断开）
    timer.timeout.connect(_on_timeout, CONNECT_ONE_SHOT)

    # 延迟连接（在空闲帧调用）
    signal_name.connect(_callback, CONNECT_DEFERRED)
```

### 断开信号

```gdscript
func disconnect_all():
    # 断开特定连接
    health_changed.disconnect(_on_health_changed)

    # 检查是否已连接
    if damage_taken.is_connected(_on_damage_taken):
        damage_taken.disconnect(_on_damage_taken)
```

### 创建 Signal 对象

```gdscript
func connect_to_external_signal(obj, signal_name):
    # 创建 Signal 对象
    var sig = Signal(obj, signal_name)

    # 连接
    sig.connect(_on_signal_received)

    # 检查连接
    if sig.is_connected(_on_signal_received):
        print("Connected!")

func _on_signal_received():
    print("External signal received")
```

## 实际应用示例

### 游戏事件系统

```gdscript
# EventBus.gd - 全局事件总线
extends Node

signal game_started
signal game_paused
signal game_resumed
signal level_completed(level_id, score)
signal player_spawned(player)
signal enemy_spawned(enemy)

func emit_game_started():
    game_started.emit()

func emit_level_completed(level_id: int, score: int):
    level_completed.emit(level_id, score)
```

```gdscript
# 使用事件总线
func _ready():
    EventBus.game_started.connect(_on_game_started)
    EventBus.level_completed.connect(_on_level_completed)

func _on_game_started():
    start_gameplay()

func _on_level_completed(level_id, score):
    save_progress(level_id, score)
    show_level_complete_screen()
```

### UI 更新系统

```gdscript
# Player.gd
extends CharacterBody2D

signal health_changed(new_health, max_health)
signal coins_changed(new_coins)
signal level_up(new_level)

var health = 100
var max_health = 100
var coins = 0
var level = 1

func take_damage(amount):
    health -= amount
    health = clamp(health, 0, max_health)
    health_changed.emit(health, max_health)

func add_coins(amount):
    coins += amount
    coins_changed.emit(coins)

    # 检查升级
    if coins >= level * 100:
        level += 1
        level_up.emit(level)
```

```gdscript
# HUD.gd
extends CanvasLayer

func _ready():
    var player = get_node("/root/Game/Player")
    player.health_changed.connect(_on_player_health_changed)
    player.coins_changed.connect(_on_player_coins_changed)
    player.level_up.connect(_on_player_level_up)

func _on_player_health_changed(health, max_health):
    $HealthBar.value = health
    $HealthBar.max_value = max_health
    $HealthLabel.text = str(health) + "/" + str(max_health)

func _on_player_coins_changed(coins):
    $CoinsLabel.text = "Coins: " + str(coins)

func _on_player_level_up(level):
    $LevelLabel.text = "Level: " + str(level)
    show_level_up_animation()
```

### 状态机

```gdscript
# StateMachine.gd
extends Node

signal state_changed(old_state, new_state)
signal state_entered(state)
signal state_exited(state)

var current_state = null

func change_state(new_state):
    var old_state = current_state

    if current_state:
        state_exited.emit(current_state)

    current_state = new_state
    state_changed.emit(old_state, new_state)
    state_entered.emit(new_state)

func _ready():
    state_changed.connect(_on_state_changed)

func _on_state_changed(old_state, new_state):
    print("State changed from ", old_state, " to ", new_state)
```

### 对话系统

```gdscript
# DialogueManager.gd
extends Node

signal dialogue_started(dialogue_id)
signal dialogue_line_shown(line_text, character)
signal dialogue_choice_shown(choices)
signal dialogue_ended

func start_dialogue(dialogue_id):
    dialogue_started.emit(dialogue_id)
    show_next_line()

func show_next_line():
    var line = get_next_line()
    dialogue_line_shown.emit(line.text, line.character)

func show_choices(choices):
    dialogue_choice_shown.emit(choices)

func end_dialogue():
    dialogue_ended.emit()
```

### 成就系统

```gdscript
# AchievementManager.gd
extends Node

signal achievement_unlocked(achievement_id, achievement_name)
signal achievement_progress(achievement_id, current, total)

var achievements = {}

func check_achievement(achievement_id):
    if is_achievement_unlocked(achievement_id):
        var achievement = get_achievement(achievement_id)
        achievement_unlocked.emit(achievement_id, achievement.name)

func update_progress(achievement_id, progress):
    var achievement = get_achievement(achievement_id)
    achievement.progress = progress
    achievement_progress.emit(achievement_id, progress, achievement.total)

    if progress >= achievement.total:
        check_achievement(achievement_id)
```

## 最佳实践

### 1. 信号命名规范

```gdscript
# ✅ 好的命名：使用过去时态
signal health_changed
signal item_collected
signal enemy_defeated

# ❌ 不好的命名
signal change_health
signal collect_item
signal defeat_enemy
```

### 2. 参数设计

```gdscript
# ✅ 好：提供有用的上下文信息
signal damage_taken(amount, source, damage_type)
signal item_picked_up(item, pickup_position)

# ❌ 不好：参数过少或过多
signal damage_taken()  # 缺少信息
signal damage_taken(a, b, c, d, e, f, g)  # 参数过多
```

### 3. 避免循环依赖

```gdscript
# ❌ 不好：A 和 B 互相连接信号可能导致无限循环
# A.gd
func _ready():
    B.signal_b.connect(_on_b_signal)

func _on_b_signal():
    my_signal.emit()

# B.gd
func _ready():
    A.my_signal.connect(_on_a_signal)

func _on_a_signal():
    signal_b.emit()
```

### 4. 及时断开连接

```gdscript
func _ready():
    timer.timeout.connect(_on_timeout)

func _exit_tree():
    # 清理连接
    if timer.timeout.is_connected(_on_timeout):
        timer.timeout.disconnect(_on_timeout)
```

### 5. 使用事件总线模式

```gdscript
# 对于全局事件，使用单例事件总线
# EventBus.gd (AutoLoad)
extends Node

signal game_event(event_type, data)

func emit_event(event_type: String, data: Dictionary = {}):
    game_event.emit(event_type, data)
```

## 性能考虑

1. **连接数量**：避免为同一信号创建过多连接
2. **信号触发频率**：高频信号（如 `_process`）应谨慎使用
3. **参数大小**：避免传递大型对象，考虑传递引用或 ID
4. **延迟连接**：对于非紧急操作，使用 `CONNECT_DEFERRED`

## 调试技巧

```gdscript
# 打印信号连接信息
func debug_signals():
    var connections = my_signal.get_connections()
    print("Signal connections: ", connections.size())
    for conn in connections:
        print("  - ", conn)

# 使用信号监听器进行调试
func _ready():
    # 监听所有信号
    for sig in get_signal_list():
        sig.connect(func():
            print("Signal fired: ", sig.get_name())
        )
```

## 与 Godot 内置信号的集成

GDScript2 的信号系统与 Godot 内置信号完全兼容：

```gdscript
func _ready():
    # 连接到 Godot 内置信号
    $Button.pressed.connect(_on_button_pressed)
    $Timer.timeout.connect(_on_timer_timeout)
    get_tree().node_added.connect(_on_node_added)

func _on_button_pressed():
    print("Button pressed")

func _on_timer_timeout():
    print("Timer timeout")

func _on_node_added(node):
    print("Node added: ", node.name)
```

## 注意事项

1. **信号不会自动传播**：信号只在定义它的对象上触发
2. **连接顺序**：回调函数按连接顺序调用
3. **内存管理**：断开的连接会自动清理，但最好手动断开
4. **线程安全**：信号在主线程中触发，跨线程使用需要特别注意

## 参考

- `runtime/gdscript2_signal.h` - 信号系统核心实现
- `vm/gdscript2_vm.cpp` - VM 信号操作支持
- `tests/test_gdscript2_signal.cpp` - 测试用例
- `docs/COROUTINE_GUIDE.md` - 协程与信号的配合使用
