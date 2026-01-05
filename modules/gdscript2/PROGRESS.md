# GDScript2 模块开发进度

## 项目目标

在 `modules/gdscript2` 重写 GDScript，采用现代分层架构（前端-中端-后端解耦），保持约 99% 兼容常用语法/运行时/内置函数，提升可维护性与可测试性。

## 已完成工作

### M0 基线（已完成）

- 创建模块目录结构：`front/`, `semantic/`, `ir/`, `codegen/`, `vm/`, `runtime/`, `tools/`, `compat/`, `tests/`
- 添加构建文件：`SCsub`, `config.py`, `register_types.h/.cpp`
- 模块可被 SCons 识别并编译链接

### M1 前端（已完成 - stub）

**文件：**
- `front/gdscript2_token.h` - Token 类型与结构体
- `front/gdscript2_ast.h` - AST 节点枚举与结构
- `front/gdscript2_parser.h/.cpp` - 解析器接口与占位实现

**功能：**
- Token 定义（关键字、运算符、字面量等）
- AST 节点类型（表达式、语句、声明）
- Parser 返回空块根节点的 stub

### M2 语义分析（已完成 - stub）

**文件：**
- `semantic/gdscript2_type.h` - 类型系统定义
- `semantic/gdscript2_symbol_table.h` - 符号表
- `semantic/gdscript2_semantic.h/.cpp` - 语义分析器

**功能：**
- 类型枚举（VARIANT, NIL, BOOL, INT, FLOAT, STRING, ARRAY, DICTIONARY, OBJECT, FUNC）
- 简单符号表（HashMap 存储）
- 语义分析器生成 dummy 全局符号

### M3 IR 中间表示（已完成 - stub）

**文件：**
- `ir/gdscript2_ir.h` - IR 操作码、指令、基本块、函数、模块结构
- `ir/gdscript2_ir_builder.h/.cpp` - 从 AST 构建 IR
- `ir/gdscript2_ir_pass.h/.cpp` - pass 接口与管理器

**功能：**
- 高层 IR 操作码（LOAD_CONST, ADD, SUB, MUL, DIV, JUMP, CALL, RETURN 等）
- IR 指令结构（目标寄存器、源操作数、常量值）
- 基本块与函数表示
- Pass 管理器与占位 pass（ConstFold, DCE）

### M4 代码生成（已完成 - stub）

**文件：**
- `codegen/gdscript2_codegen.h/.cpp` - 字节码生成器

**功能：**
- 字节码操作码定义（OP_NOP, OP_LOAD_CONST, OP_ADD, OP_RETURN 等）
- 字节码指令结构
- 编译函数/模块结构
- 从 IR 生成字节码的占位实现

### M5 虚拟机（已完成 - stub）

**文件：**
- `vm/gdscript2_vm.h/.cpp` - VM 执行引擎

**功能：**
- 调用帧管理
- 执行结果状态（OK, ERROR_RUNTIME, ERROR_STACK_OVERFLOW）
- 字节码解释器（支持基础操作、算术运算、控制流）
- 调用栈管理与栈溢出检测

### 测试用例

**文件：**
- `tests/test_gdscript2_front.cpp` - 前端测试
- `tests/test_gdscript2_semantic.cpp` - 语义测试
- `tests/test_gdscript2_ir.cpp` - IR 测试
- `tests/test_gdscript2_codegen.cpp` - Codegen 测试
- `tests/test_gdscript2_vm.cpp` - VM 测试

**注册的测试命令：**
- `gdscript2-front-minimal`
- `gdscript2-semantic-minimal`
- `gdscript2-ir-build`
- `gdscript2-ir-passes`
- `gdscript2-codegen-minimal`
- `gdscript2-vm-execute`
- `gdscript2-vm-not-found`

### 语言注册

**文件：**
- `gdscript2_language.h/.cpp` - 语言类、脚本类、资源加载/保存器

**功能：**
- `GDScript2Language` 继承 `ScriptLanguage`（占位实现）
- `GDScript2Script` 继承 `Script`（占位实现）
- `GDScript2ResourceLoader/Saver` 资源加载保存器

## 当前架构

```
源码 (.gd2)
    │
    ▼
┌─────────────┐
│   Tokenizer │  (待实现)
└─────────────┘
    │
    ▼
┌─────────────┐
│   Parser    │  front/gdscript2_parser.h
└─────────────┘
    │
    ▼
┌─────────────┐
│     AST     │  front/gdscript2_ast.h
└─────────────┘
    │
    ▼
┌─────────────┐
│  Semantic   │  semantic/gdscript2_semantic.h
│  Analyzer   │
└─────────────┘
    │
    ▼
┌─────────────┐
│     IR      │  ir/gdscript2_ir.h
│   Builder   │  ir/gdscript2_ir_builder.h
└─────────────┘
    │
    ▼
┌─────────────┐
│  IR Passes  │  ir/gdscript2_ir_pass.h
│ (ConstFold, │
│    DCE)     │
└─────────────┘
    │
    ▼
┌─────────────┐
│  Codegen    │  codegen/gdscript2_codegen.h
└─────────────┘
    │
    ▼
┌─────────────┐
│  Bytecode   │
└─────────────┘
    │
    ▼
┌─────────────┐
│     VM      │  vm/gdscript2_vm.h
└─────────────┘
```

## 待完成工作（TODO）

### 高优先级

1. **M1 前端真实实现**
   - 实现完整 Tokenizer（参考 `modules/gdscript/gdscript_tokenizer.cpp`）
   - 实现完整 Parser（支持所有 GDScript 语法）
   - 完善 AST 节点类型（类、函数、变量、表达式等）

2. **M2 语义分析真实实现**
   - 作用域管理（全局/局部/类成员）
   - 类型推断与类型检查
   - 符号解析（变量、函数、类引用）
   - 诊断信息生成

3. **M3 IR 完善**
   - 完整的 AST 到 IR 转换
   - 实现真实的优化 pass（常量折叠、死代码消除、内联）
   - SSA 形式支持（可选）

4. **M4 Codegen 完善**
   - 完整指令集生成
   - 跳转目标修补
   - 常量池管理
   - 调试信息生成

5. **M5 VM 完善**
   - 完整指令集解释
   - 内建函数调用
   - 对象/类实例化
   - 协程/await 支持
   - 调试 hook

### 中优先级

6. **M6 Tools - 语言服务器**
   - LSP 协议实现
   - 代码补全
   - 跳转定义
   - 符号索引
   - 语法着色

7. **M7 Compat - 兼容层**
   - 旧字节码版本转换
   - 警告映射
   - 开关/标志文档

8. **Runtime 运行时**
   - Godot 对象桥接
   - Variant 操作
   - 内建函数注册
   - 信号/RPC 支持

### 低优先级

9. **M8 稳定化**
   - 性能回归测试
   - 内存与并发测试
   - API freeze
   - 文档完善

10. **Editor 集成**
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
│   ├── gdscript2_ast.h
│   ├── gdscript2_parser.h
│   └── gdscript2_parser.cpp
├── semantic/
│   ├── gdscript2_type.h
│   ├── gdscript2_symbol_table.h
│   ├── gdscript2_semantic.h
│   └── gdscript2_semantic.cpp
├── ir/
│   ├── gdscript2_ir.h
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

1. 继续实现 Tokenizer（可直接复用或参考现有实现）
2. 或选择某一层深入完善（如 IR 优化 pass）
3. 或添加 Tools/LSP 支持
