# GDScript2 模块开发进度

## 项目目标

在 `modules/gdscript2` 重写 GDScript，采用现代分层架构（前端-中端-后端解耦），保持约 99% 兼容常用语法/运行时/内置函数，提升可维护性与可测试性。

## 已完成工作

### M0 基线（已完成）

- 创建模块目录结构：`front/`, `semantic/`, `ir/`, `codegen/`, `vm/`, `runtime/`, `tools/`, `compat/`, `tests/`
- 添加构建文件：`SCsub`, `config.py`, `register_types.h/.cpp`
- 模块可被 SCons 识别并编译链接

### M1 前端（✅ 已完成）

**文件：**
- `front/gdscript2_token.h` - 完整 Token 类型枚举与结构体
- `front/gdscript2_tokenizer.h/.cpp` - **完整词法分析器实现**
- `front/gdscript2_ast.h` - **完整 AST 节点定义（40+ 种节点类型）**
- `front/gdscript2_parser.h/.cpp` - **完整递归下降解析器实现**

**Tokenizer 功能：**
- 完整的 Token 类型定义（90+ 种 token）
- 关键字、运算符、数字、字符串、缩进处理等

**Parser 功能：**
- 完整 AST 节点类型（表达式、语句、声明、类型）
- 顶层/控制流/表达式解析
- 错误恢复机制

### M2 语义分析（✅ 已完成）

**文件：**
- `semantic/gdscript2_type.h/.cpp` - **完整类型系统实现**
- `semantic/gdscript2_symbol_table.h/.cpp` - **完整嵌套作用域符号表**
- `semantic/gdscript2_diagnostic.h/.cpp` - **诊断系统（错误/警告）**
- `semantic/gdscript2_semantic.h/.cpp` - **完整语义分析器**

**功能：**
- 完整类型系统（50+ 种类型、泛型容器、函数签名）
- 嵌套作用域符号表
- 诊断系统（50+ 种错误/警告代码）
- 类型推断与类型检查
- 控制流验证

### M3 IR 中间表示（✅ 已完成）

**文件：**
- `ir/gdscript2_ir.h` - **完整 IR 数据结构**
- `ir/gdscript2_ir.cpp` - **IR 实现（操作数、指令、基本块、函数、模块）**
- `ir/gdscript2_ir_builder.h/.cpp` - **完整 AST 到 IR 转换**
- `ir/gdscript2_ir_pass.h/.cpp` - **优化 Pass 框架与实现**

**IR 数据结构（已完成）：**
- 60+ 种 IR 操作码（算术、比较、逻辑、控制流、调用等）
- IR 操作数类型（寄存器、常量、立即数、基本块引用、名称等）
- IR 指令结构（目标、源操作数、跳转目标、源位置）
- 基本块（指令列表、前驱/后继、循环分析）
- 局部变量管理（参数、变量、索引映射）
- IR 函数（基本块、局部变量、常量池、寄存器分配）
- IR 模块（函数、全局变量）
- CFG 计算（前驱、可达性）
- IR 文本输出（调试用）

**AST 到 IR 转换（已完成）：**
- 类构建（成员变量、函数）
- 函数构建（参数、入口块、隐式返回）
- 语句构建：
  - 变量声明
  - if/elif/else
  - for 循环（迭代器模式）
  - while 循环
  - match 语句（模式匹配）
  - return/break/continue
  - assert/breakpoint
- 表达式构建：
  - 字面量、标识符、self
  - 二元/一元/三元运算符
  - 逻辑短路求值（and/or）
  - 函数调用（直接/方法/内置）
  - 下标访问、属性访问
  - 数组/字典构造
  - lambda、await、preload
  - 类型转换（as）、类型测试（is）
  - $NodePath
- 赋值处理（简单/复合赋值）
- 循环控制栈（break/continue 目标）

**优化 Pass（已完成）：**
- `GDScript2IRPassManager` - Pass 管理器
- `GDScript2ConstFoldPass` - **常量折叠**（编译时计算常量表达式）
- `GDScript2DCEPass` - **死代码消除**（删除不可达代码和无用指令）
- `GDScript2CopyPropPass` - **拷贝传播**（消除冗余拷贝）
- `GDScript2SimplifyCFGPass` - **CFG 简化**（删除空块、合并块）
- `GDScript2StrengthReductionPass` - **强度削减**（x*2 → x+x, x*4 → x<<2）
- `add_standard_passes()` - 标准优化流水线

### M4 代码生成（✅ 已完成）

**文件：**
- `codegen/gdscript2_codegen.h/.cpp` - **完整字节码生成器**

**字节码操作码（68 种）：**
- 特殊操作（NOP, END）
- 常量加载（LOAD_CONST, LOAD_NIL, LOAD_TRUE, LOAD_FALSE, LOAD_SMALL_INT）
- 变量操作（LOAD/STORE_LOCAL, LOAD/STORE_MEMBER, LOAD/STORE_GLOBAL, LOAD_SELF）
- 算术运算（ADD, SUB, MUL, DIV, MOD, POW, NEG, POS）
- 位运算（BIT_AND, BIT_OR, BIT_XOR, BIT_NOT, BIT_LSH, BIT_RSH）
- 比较运算（EQ, NE, LT, LE, GT, GE）
- 逻辑运算（NOT）
- 类型操作（IS, AS, TYPEOF, IN）
- 容器操作（CONSTRUCT_ARRAY, CONSTRUCT_DICT, GET/SET_INDEX, GET/SET_NAMED）
- 控制流（JUMP, JUMP_IF, JUMP_IF_NOT）
- 函数调用（CALL, CALL_METHOD, CALL_BUILTIN, CALL_SUPER, CALL_SELF）
- 返回（RETURN, RETURN_VOID）
- 迭代器（ITER_BEGIN, ITER_NEXT, ITER_GET, JUMP_IF_ITER_END）
- 特殊构造（AWAIT, YIELD, PRELOAD, GET_NODE, LAMBDA）
- 其他（COPY, ASSERT, BREAKPOINT, LINE）

**功能：**
- 完整 IR 到字节码转换
- 跳转目标修补机制（基本块 ID → 线性地址）
- 常量池管理
- 名称池管理
- 调试信息支持（行号映射）
- 反汇编输出

### M5 虚拟机（✅ 已完成）

**文件：**
- `vm/gdscript2_vm.h/.cpp` - **完整 VM 执行引擎**

**核心结构：**
- `GDScript2IteratorState` - 迭代器状态（支持 Array、Dictionary、String 等）
- `GDScript2CallFrame` - 调用帧（栈、IP、self、迭代器）
- `GDScript2ExecutionResult` - 执行结果（状态、返回值、错误信息）
- `GDScript2VMDebugger` - 调试钩子接口
- `GDScript2VM` - 虚拟机主类

**指令执行（68 种操作码）：**
- 常量加载（LOAD_CONST, LOAD_NIL, LOAD_TRUE, LOAD_FALSE, LOAD_SMALL_INT）
- 变量操作（LOAD/STORE_LOCAL, LOAD/STORE_MEMBER, LOAD/STORE_GLOBAL, LOAD_SELF）
- 算术运算（ADD, SUB, MUL, DIV, MOD, POW, NEG, POS）
- 位运算（BIT_AND, BIT_OR, BIT_XOR, BIT_NOT, BIT_LSH, BIT_RSH）
- 比较运算（EQ, NE, LT, LE, GT, GE）
- 逻辑运算（NOT）
- 类型操作（IS, AS, TYPEOF, IN）
- 容器操作（CONSTRUCT_ARRAY, CONSTRUCT_DICT, GET/SET_INDEX, GET/SET_NAMED）
- 控制流（JUMP, JUMP_IF, JUMP_IF_NOT）
- 函数调用（CALL, CALL_METHOD, CALL_BUILTIN, CALL_SUPER, CALL_SELF）
- 返回（RETURN, RETURN_VOID）
- 迭代器（ITER_BEGIN, ITER_NEXT, ITER_GET, JUMP_IF_ITER_END）
- 其他（COPY, ASSERT, BREAKPOINT）

**功能：**
- 完整字节码解释执行
- 调用栈管理（最大深度 1024）
- 递归函数调用支持
- 全局变量管理
- 迭代器支持（for 循环）
- 调试钩子（行号、断点、断言）
- 错误处理与报告

### 测试用例

**文件：**
- `tests/test_gdscript2_tokenizer.cpp` - **Tokenizer 测试（21 个）**
- `tests/test_gdscript2_parser.cpp` - **Parser 测试（23 个）**
- `tests/test_gdscript2_semantic.cpp` - **语义测试（35 个）**
- `tests/test_gdscript2_ir.cpp` - **IR 测试（21 个）**
- `tests/test_gdscript2_codegen.cpp` - **Codegen 测试（17 个）**
- `tests/test_gdscript2_vm.cpp` - **VM 测试（30 个）**

**IR 测试命令：**
- `gdscript2-ir-build-empty` - 空脚本构建
- `gdscript2-ir-build-function` - 函数构建
- `gdscript2-ir-build-variables` - 变量构建
- `gdscript2-ir-build-if` - if 语句构建
- `gdscript2-ir-build-for` - for 循环构建
- `gdscript2-ir-build-while` - while 循环构建
- `gdscript2-ir-build-call` - 函数调用构建
- `gdscript2-ir-build-array` - 数组操作构建
- `gdscript2-ir-build-dictionary` - 字典操作构建
- `gdscript2-ir-build-binary-ops` - 二元运算符构建
- `gdscript2-ir-build-comparison` - 比较运算符构建
- `gdscript2-ir-build-logical` - 逻辑运算符构建
- `gdscript2-ir-build-ternary` - 三元运算符构建
- `gdscript2-ir-build-match` - match 语句构建
- `gdscript2-ir-pass-const-fold` - 常量折叠测试
- `gdscript2-ir-pass-dce` - 死代码消除测试
- `gdscript2-ir-pass-copy-prop` - 拷贝传播测试
- `gdscript2-ir-pass-simplify-cfg` - CFG 简化测试
- `gdscript2-ir-pass-strength-reduction` - 强度削减测试
- `gdscript2-ir-pass-manager` - Pass 管理器测试
- `gdscript2-ir-to-string` - IR 输出测试

**Codegen 测试命令：**
- `gdscript2-codegen-empty` - 空脚本编译
- `gdscript2-codegen-simple-function` - 简单函数编译
- `gdscript2-codegen-return-value` - 返回值编译
- `gdscript2-codegen-constants` - 常量编译
- `gdscript2-codegen-if-statement` - if 语句编译
- `gdscript2-codegen-while-loop` - while 循环编译
- `gdscript2-codegen-for-loop` - for 循环编译
- `gdscript2-codegen-function-call` - 函数调用编译
- `gdscript2-codegen-array-operations` - 数组操作编译
- `gdscript2-codegen-dictionary` - 字典编译
- `gdscript2-codegen-binary-operators` - 二元运算符编译
- `gdscript2-codegen-comparison-operators` - 比较运算符编译
- `gdscript2-codegen-unary-operators` - 一元运算符编译
- `gdscript2-codegen-bitwise-operators` - 位运算符编译
- `gdscript2-codegen-disassemble` - 反汇编测试
- `gdscript2-codegen-jump-patching` - 跳转修补测试
- `gdscript2-codegen-line-info` - 行号信息测试

**VM 测试命令：**
- `gdscript2-vm-empty` - 空脚本执行
- `gdscript2-vm-return-constant` - 返回常量
- `gdscript2-vm-return-string` - 返回字符串
- `gdscript2-vm-return-nil` - 返回 null
- `gdscript2-vm-return-bool` - 返回布尔值
- `gdscript2-vm-arithmetic-add` - 加法运算
- `gdscript2-vm-arithmetic-sub` - 减法运算
- `gdscript2-vm-arithmetic-mul` - 乘法运算
- `gdscript2-vm-arithmetic-div` - 除法运算
- `gdscript2-vm-arithmetic-mod` - 取模运算
- `gdscript2-vm-arithmetic-complex` - 复杂算术
- `gdscript2-vm-comparison-eq` - 相等比较
- `gdscript2-vm-comparison-lt` - 小于比较
- `gdscript2-vm-if-statement` - if 语句执行
- `gdscript2-vm-while-loop` - while 循环执行
- `gdscript2-vm-nested-if` - 嵌套 if 执行
- `gdscript2-vm-local-variables` - 局部变量
- `gdscript2-vm-variable-reassign` - 变量重赋值
- `gdscript2-vm-array-create` - 数组创建
- `gdscript2-vm-array-access` - 数组访问
- `gdscript2-vm-array-modify` - 数组修改
- `gdscript2-vm-dictionary-create` - 字典创建
- `gdscript2-vm-dictionary-access` - 字典访问
- `gdscript2-vm-function-call` - 函数调用
- `gdscript2-vm-recursive-function` - 递归函数
- `gdscript2-vm-fibonacci` - 斐波那契
- `gdscript2-vm-unary-neg` - 一元负号
- `gdscript2-vm-unary-not` - 一元 not
- `gdscript2-vm-globals` - 全局变量
- `gdscript2-vm-function-not-found` - 函数未找到错误
- `gdscript2-vm-no-module` - 无模块错误
- `gdscript2-vm-string-concat` - 字符串连接

### 语言注册

**文件：**
- `gdscript2_language.h/.cpp` - 语言类、脚本类、资源加载/保存器

## 当前架构

```
源码 (.gd2)
    │
    ▼
┌─────────────┐
│   Tokenizer │  front/gdscript2_tokenizer.h ✅ 已完成
└─────────────┘
    │
    ▼
┌─────────────┐
│   Parser    │  front/gdscript2_parser.h ✅ 已完成
└─────────────┘
    │
    ▼
┌─────────────┐
│     AST     │  front/gdscript2_ast.h ✅ 已完成
└─────────────┘
    │
    ▼
┌─────────────┐
│  Semantic   │  semantic/gdscript2_semantic.h ✅ 已完成
│  Analyzer   │
└─────────────┘
    │
    ▼
┌─────────────┐
│     IR      │  ir/gdscript2_ir.h ✅ 已完成
│   Builder   │  ir/gdscript2_ir_builder.h ✅ 已完成
└─────────────┘
    │
    ▼
┌─────────────┐
│  IR Passes  │  ir/gdscript2_ir_pass.h ✅ 已完成
│ (ConstFold, │
│ DCE, Copy,  │
│ CFG, Strength)
└─────────────┘
    │
    ▼
┌─────────────┐
│  Codegen    │  codegen/gdscript2_codegen.h ✅ 已完成
└─────────────┘
    │
    ▼
┌─────────────┐
│  Bytecode   │
└─────────────┘
    │
    ▼
┌─────────────┐
│     VM      │  vm/gdscript2_vm.h ✅ 已完成
└─────────────┘
```

## 待完成工作（TODO）

### 高优先级

1. **Runtime 运行时完善**
   - Godot 对象桥接
   - 完整内建函数支持
   - 信号/RPC 支持
   - 协程/await 完整支持

### 中优先级

3. **M6 Tools - 语言服务器**
   - LSP 协议实现
   - 代码补全
   - 跳转定义
   - 符号索引
   - 语法着色

4. **M7 Compat - 兼容层**
   - 旧字节码版本转换
   - 警告映射
   - 开关/标志文档

5. **Runtime 运行时**
   - Godot 对象桥接
   - Variant 操作
   - 内建函数注册
   - 信号/RPC 支持

### 低优先级

6. **M8 稳定化**
   - 性能回归测试
   - 内存与并发测试
   - API freeze
   - 文档完善

7. **Editor 集成**
   - 语法高亮
   - 错误标记
   - 调试器集成

## 构建命令

```bash
scons dev_mode=yes dev_build=yes module_mono_enabled=yes scu_build=yes -j4
```

## 文件列表

```
modules/gdscript2/
├── config.py
├── SCsub
├── register_types.h
├── register_types.cpp
├── gdscript2_language.h
├── gdscript2_language.cpp
├── README.md
├── PROGRESS.md (本文件)
├── plan/
│   └── main_plan.md
├── front/
│   ├── gdscript2_token.h
│   ├── gdscript2_tokenizer.h
│   ├── gdscript2_tokenizer.cpp
│   ├── gdscript2_ast.h
│   ├── gdscript2_parser.h
│   └── gdscript2_parser.cpp
├── semantic/
│   ├── gdscript2_type.h
│   ├── gdscript2_type.cpp
│   ├── gdscript2_symbol_table.h
│   ├── gdscript2_symbol_table.cpp
│   ├── gdscript2_diagnostic.h
│   ├── gdscript2_diagnostic.cpp
│   ├── gdscript2_semantic.h
│   └── gdscript2_semantic.cpp
├── ir/
│   ├── gdscript2_ir.h
│   ├── gdscript2_ir.cpp
│   ├── gdscript2_ir_builder.h
│   ├── gdscript2_ir_builder.cpp
│   ├── gdscript2_ir_pass.h
│   └── gdscript2_ir_pass.cpp
├── codegen/
│   ├── gdscript2_codegen.h
│   └── gdscript2_codegen.cpp
├── vm/
│   ├── gdscript2_vm.h
│   └── gdscript2_vm.cpp
├── runtime/
├── tools/
├── compat/
└── tests/
    ├── test_gdscript2_front.cpp
    ├── test_gdscript2_tokenizer.cpp
    ├── test_gdscript2_parser.cpp
    ├── test_gdscript2_semantic.cpp
    ├── test_gdscript2_ir.cpp
    ├── test_gdscript2_codegen.cpp
    └── test_gdscript2_vm.cpp
```

## 参考文件

现有 GDScript 模块的关键文件（供实现参考）：
- `modules/gdscript/gdscript_tokenizer.h/.cpp` - Tokenizer 实现
- `modules/gdscript/gdscript_parser.h/.cpp` - Parser 实现
- `modules/gdscript/gdscript_analyzer.h/.cpp` - 语义分析
- `modules/gdscript/gdscript_compiler.h/.cpp` - 编译器
- `modules/gdscript/gdscript_byte_codegen.h/.cpp` - 字节码生成
- `modules/gdscript/gdscript_vm.cpp` - VM 实现
- `modules/gdscript/gdscript_function.h` - 函数与数据类型

## 下次对话建议

1. **完善 Runtime** - Godot 对象桥接、完整内建函数支持
2. 添加 **Tools/LSP 支持** - 代码补全、跳转定义
3. 或添加 **Compat 兼容层** - 与旧 GDScript 的兼容
