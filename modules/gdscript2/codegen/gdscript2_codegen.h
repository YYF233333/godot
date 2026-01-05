/**************************************************************************/
/*  gdscript2_codegen.h                                                   */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "core/object/ref_counted.h"
#include "core/string/string_name.h"
#include "core/templates/vector.h"
#include "core/variant/variant.h"
#include "modules/gdscript2/ir/gdscript2_ir.h"

// Bytecode opcodes for GDScript2 VM (simplified stub).
enum class GDScript2Opcode : uint32_t {
	OP_NOP,
	OP_LOAD_CONST,
	OP_LOAD_LOCAL,
	OP_STORE_LOCAL,
	OP_LOAD_GLOBAL,
	OP_STORE_GLOBAL,
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_MOD,
	OP_NEG,
	OP_EQ,
	OP_NE,
	OP_LT,
	OP_LE,
	OP_GT,
	OP_GE,
	OP_NOT,
	OP_JUMP,
	OP_JUMP_IF,
	OP_JUMP_IF_NOT,
	OP_CALL,
	OP_CALL_BUILTIN,
	OP_RETURN,
	OP_END, // Marks end of bytecode.
};

// Single bytecode instruction (variable-length encoding stub).
struct GDScript2BytecodeInstr {
	GDScript2Opcode op = GDScript2Opcode::OP_NOP;
	int32_t arg0 = 0;
	int32_t arg1 = 0;
	int32_t arg2 = 0;
};

// Compiled function bytecode.
struct GDScript2CompiledFunction {
	StringName name;
	Vector<GDScript2BytecodeInstr> code;
	Vector<Variant> constants;
	int local_count = 0;
	int param_count = 0;
};

// Compiled module (collection of functions).
struct GDScript2CompiledModule {
	Vector<GDScript2CompiledFunction> functions;
};

// Code generator: converts IR to bytecode.
class GDScript2CodeGenerator : public RefCounted {
	GDCLASS(GDScript2CodeGenerator, RefCounted);

	GDScript2CompiledModule module;
	GDScript2CompiledFunction *current_func = nullptr;

	int emit(const GDScript2BytecodeInstr &p_instr);
	int add_constant(const Variant &p_value);

protected:
	static void _bind_methods() {}

public:
	struct Result {
		GDScript2CompiledModule module;
		Vector<String> errors;
	};

	Result generate(const GDScript2IRModule &p_ir);
};
