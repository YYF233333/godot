# gdscript2 概要

- 目标：在 `modules/gdscript2` 实现现代化架构的 GDScript，保持 ~99% 行为兼容，便于维护与测试。
- 分层：`front/`（词法/语法/AST）、`semantic/`（符号表/类型推断/CFG/SSA）、`ir/`、`codegen/`、`vm/`、`runtime/`、`tools/`、`compat/`、`tests/`。
- 依赖：沿用 Godot 核心（Variant、容器、线程、分配器），C++17。
- 计划：详见 `plan/main_plan.md`，里程碑从 M0 框架搭建到 M8 稳定化。
- 兼容策略：默认保持现有语法/内置函数/信号行为，允许可配置的兼容破坏（字节码版本升级、部分调试接口弃用等）。
