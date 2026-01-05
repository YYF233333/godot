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

**Tokenizer 功能（已完成）：**
- 完整的 Token 类型定义（90+ 种 token）
- 关键字识别（if, else, func, var, class, return 等 40+ 关键字）
- 运算符扫描（算术、比较、逻辑、位运算、赋值等）
- 数字字面量（整数、浮点、十六进制 0x、二进制 0b、下划线分隔 1_000）
- 字符串字面量（单引号、双引号、多行字符串、转义序列、原始字符串 r""）
- 特殊字符串（StringName &""、NodePath ^""）
- 缩进处理（INDENT/DEDENT token，Python 风格）
- 注释跳过与保存（TOOLS_ENABLED 时保存注释内容）
- 行继续符（\\ 换行）
- 括号匹配检测
- VCS 冲突标记检测
- Unicode 标识符支持
- 光标位置追踪（IDE 集成）
- 完整的错误报告

**Parser 功能（已完成）：**
- 完整 AST 节点类型（表达式、语句、声明、类型）
- 顶层解析：class, extends, class_name, func, var, const, signal, enum, annotation
- 控制流：if/elif/else, for, while, match/when, break, continue, pass, return
- 表达式：运算符优先级、赋值、三元表达式、函数调用、下标、属性访问
- 字面量：数字、字符串、布尔、null、数组、字典、lambda
- 特殊：await, preload, super, self, $NodePath, 类型标注
- 错误恢复机制

### M2 语义分析（✅ 已完成）

**文件：**
- `semantic/gdscript2_type.h/.cpp` - **完整类型系统实现**
- `semantic/gdscript2_symbol_table.h/.cpp` - **完整嵌套作用域符号表**
- `semantic/gdscript2_diagnostic.h/.cpp` - **诊断系统（错误/警告）**
- `semantic/gdscript2_semantic.h/.cpp` - **完整语义分析器**

**类型系统功能（已完成）：**
- 完整的类型枚举（50+ 种类型）
- 所有 Variant 类型支持（nil, bool, int, float, String, Vector2/3/4 等）
- 容器类型（Array, Dictionary, PackedXxxArray）
- 泛型容器类型（Array[T], Dictionary[K,V]）
- 对象/类类型（native class, script class）
- 函数签名类型（参数类型、返回类型、默认参数）
- 枚举类型
- 元类型（typeof）
- 类型兼容性检查
- 类型推断辅助
- Variant 类型转换

**符号表功能（已完成）：**
- 嵌套作用域支持（全局、类、函数、块、lambda、match）
- 作用域链查找
- 符号类型（变量、常量、参数、函数、类、信号、枚举等）
- 类作用域栈（self 解析）
- 函数作用域栈（return 类型检查）
- 循环深度跟踪（break/continue 验证）
- 内置符号初始化（类型、函数、常量、原生类）

**诊断系统功能（已完成）：**
- 诊断严重级别（错误、警告、信息、提示）
- 诊断代码分类（声明、类型、控制流、访问等 50+ 种）
- AST 节点关联的源位置
- 警告启用/禁用配置
- 警告作为错误处理
- 格式化输出（带源代码片段）

**语义分析器功能（已完成）：**
- 类分析（接口 + 主体两遍）
- 函数分析（签名 + 主体）
- 变量/常量声明分析
- 信号/枚举声明分析
- 类型节点解析
- 语句分析（if, for, while, match, return, break, continue）
- 表达式类型推断（字面量、标识符、运算符、调用、下标、属性等）
- 二元/一元/三元运算符类型推断
- 函数调用返回类型推断
- 类型兼容性验证
- 赋值目标验证（常量/只读检查）
- 常量表达式求值
- 节点类型缓存

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
- `tests/test_gdscript2_tokenizer.cpp` - **Tokenizer 测试（21 个测试用例）**
- `tests/test_gdscript2_parser.cpp` - **Parser 测试（23 个测试用例）**
- `tests/test_gdscript2_semantic.cpp` - **语义测试（35 个测试用例）**
- `tests/test_gdscript2_ir.cpp` - IR 测试
- `tests/test_gdscript2_codegen.cpp` - Codegen 测试
- `tests/test_gdscript2_vm.cpp` - VM 测试

**语义分析测试命令：**
- `gdscript2-semantic-minimal` - 空脚本测试
- `gdscript2-semantic-variable` - 变量声明测试
- `gdscript2-semantic-constant` - 常量声明测试
- `gdscript2-semantic-function` - 函数声明测试
- `gdscript2-semantic-type-mismatch` - 类型不匹配测试
- `gdscript2-semantic-numeric-promotion` - 数值提升测试
- `gdscript2-semantic-array-type` - 数组类型测试
- `gdscript2-semantic-dictionary-type` - 字典类型测试
- `gdscript2-semantic-if` - if 语句测试
- `gdscript2-semantic-for` - for 循环测试
- `gdscript2-semantic-while` - while 循环测试
- `gdscript2-semantic-break-outside-loop` - break 外部循环测试
- `gdscript2-semantic-continue-outside-loop` - continue 外部循环测试
- `gdscript2-semantic-return-outside-func` - return 外部函数测试
- `gdscript2-semantic-undefined-var` - 未定义变量测试
- `gdscript2-semantic-duplicate-decl` - 重复声明测试
- `gdscript2-semantic-scope-isolation` - 作用域隔离测试
- `gdscript2-semantic-class-members` - 类成员测试
- `gdscript2-semantic-enum` - 枚举测试
- `gdscript2-semantic-self-in-static` - static 中 self 测试
- `gdscript2-semantic-binary-ops` - 二元运算符测试
- `gdscript2-semantic-unary-ops` - 一元运算符测试
- `gdscript2-semantic-ternary-op` - 三元运算符测试
- `gdscript2-semantic-assign-to-const` - 赋值常量测试
- `gdscript2-semantic-function-call` - 函数调用测试
- `gdscript2-semantic-builtin-call` - 内置函数测试
- `gdscript2-semantic-match` - match 语句测试
- `gdscript2-semantic-match-binding` - match 绑定测试
- `gdscript2-semantic-lambda` - lambda 测试
- `gdscript2-semantic-await-in-func` - await 函数内测试
- `gdscript2-semantic-await-outside-func` - await 函数外测试
- `gdscript2-semantic-type-from-variant` - Variant 类型推断测试
- `gdscript2-semantic-type-compat` - 类型兼容性测试
- `gdscript2-semantic-symbol-lookup` - 符号查找测试
- `gdscript2-semantic-nested-scope` - 嵌套作用域测试

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
│     IR      │  ir/gdscript2_ir.h (stub)
│   Builder   │  ir/gdscript2_ir_builder.h
└─────────────┘
    │
    ▼
┌─────────────┐
│  IR Passes  │  ir/gdscript2_ir_pass.h (stub)
│ (ConstFold, │
│    DCE)     │
└─────────────┘
    │
    ▼
┌─────────────┐
│  Codegen    │  codegen/gdscript2_codegen.h (stub)
└─────────────┘
    │
    ▼
┌─────────────┐
│  Bytecode   │
└─────────────┘
    │
    ▼
┌─────────────┐
│     VM      │  vm/gdscript2_vm.h (stub)
└─────────────┘
```

## 待完成工作（TODO）

### 高优先级

1. **M3 IR 完善**
   - 完整的 AST 到 IR 转换
   - 实现真实的优化 pass（常量折叠、死代码消除、内联）
   - SSA 形式支持（可选）

2. **M4 Codegen 完善**
   - 完整指令集生成
   - 跳转目标修补
   - 常量池管理
   - 调试信息生成

3. **M5 VM 完善**
   - 完整指令集解释
   - 内建函数调用
   - 对象/类实例化
   - 协程/await 支持
   - 调试 hook

### 中优先级

4. **M6 Tools - 语言服务器**
   - LSP 协议实现
   - 代码补全
   - 跳转定义
   - 符号索引
   - 语法着色

5. **M7 Compat - 兼容层**
   - 旧字节码版本转换
   - 警告映射
   - 开关/标志文档

6. **Runtime 运行时**
   - Godot 对象桥接
   - Variant 操作
   - 内建函数注册
   - 信号/RPC 支持

### 低优先级

7. **M8 稳定化**
   - 性能回归测试
   - 内存与并发测试
   - API freeze
   - 文档完善

8. **Editor 集成**
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

1. **完善 IR 构建** - 从 AST 生成 IR，实现优化 pass
2. **完善代码生成** - 从 IR 生成字节码
3. **完善 VM** - 实现完整指令集解释执行
4. 或添加 Tools/LSP 支持
