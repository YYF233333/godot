/**************************************************************************/
/*  gdscript2_codegen.cpp                                                 */
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

#include "gdscript2_codegen.h"

int GDScript2CodeGenerator::emit(const GDScript2BytecodeInstr &p_instr) {
	if (current_func == nullptr) {
		return -1;
	}
	current_func->code.push_back(p_instr);
	return current_func->code.size() - 1;
}

int GDScript2CodeGenerator::add_constant(const Variant &p_value) {
	if (current_func == nullptr) {
		return -1;
	}
	int idx = current_func->constants.size();
	current_func->constants.push_back(p_value);
	return idx;
}

static GDScript2Opcode ir_op_to_bytecode(GDScript2IROp p_op) {
	switch (p_op) {
		case GDScript2IROp::NOP:
			return GDScript2Opcode::OP_NOP;
		case GDScript2IROp::LOAD_CONST:
			return GDScript2Opcode::OP_LOAD_CONST;
		case GDScript2IROp::LOAD_LOCAL:
			return GDScript2Opcode::OP_LOAD_LOCAL;
		case GDScript2IROp::STORE_LOCAL:
			return GDScript2Opcode::OP_STORE_LOCAL;
		case GDScript2IROp::LOAD_GLOBAL:
			return GDScript2Opcode::OP_LOAD_GLOBAL;
		case GDScript2IROp::STORE_GLOBAL:
			return GDScript2Opcode::OP_STORE_GLOBAL;
		case GDScript2IROp::ADD:
			return GDScript2Opcode::OP_ADD;
		case GDScript2IROp::SUB:
			return GDScript2Opcode::OP_SUB;
		case GDScript2IROp::MUL:
			return GDScript2Opcode::OP_MUL;
		case GDScript2IROp::DIV:
			return GDScript2Opcode::OP_DIV;
		case GDScript2IROp::MOD:
			return GDScript2Opcode::OP_MOD;
		case GDScript2IROp::NEG:
			return GDScript2Opcode::OP_NEG;
		case GDScript2IROp::EQ:
			return GDScript2Opcode::OP_EQ;
		case GDScript2IROp::NE:
			return GDScript2Opcode::OP_NE;
		case GDScript2IROp::LT:
			return GDScript2Opcode::OP_LT;
		case GDScript2IROp::LE:
			return GDScript2Opcode::OP_LE;
		case GDScript2IROp::GT:
			return GDScript2Opcode::OP_GT;
		case GDScript2IROp::GE:
			return GDScript2Opcode::OP_GE;
		case GDScript2IROp::NOT:
			return GDScript2Opcode::OP_NOT;
		case GDScript2IROp::JUMP:
			return GDScript2Opcode::OP_JUMP;
		case GDScript2IROp::JUMP_IF:
			return GDScript2Opcode::OP_JUMP_IF;
		case GDScript2IROp::JUMP_IF_NOT:
			return GDScript2Opcode::OP_JUMP_IF_NOT;
		case GDScript2IROp::CALL:
			return GDScript2Opcode::OP_CALL;
		case GDScript2IROp::CALL_BUILTIN:
			return GDScript2Opcode::OP_CALL_BUILTIN;
		case GDScript2IROp::RETURN:
			return GDScript2Opcode::OP_RETURN;
		default:
			return GDScript2Opcode::OP_NOP;
	}
}

GDScript2CodeGenerator::Result GDScript2CodeGenerator::generate(const GDScript2IRModule &p_ir) {
	Result result;
	module = GDScript2CompiledModule();

	for (int fi = 0; fi < p_ir.functions.size(); fi++) {
		const GDScript2IRFunction &ir_func = p_ir.functions[fi];
		GDScript2CompiledFunction compiled_func;
		compiled_func.name = ir_func.name;
		compiled_func.local_count = ir_func.next_reg;
		module.functions.push_back(compiled_func);
		current_func = &module.functions.write[module.functions.size() - 1];

		// Flatten blocks into linear bytecode (simple approach, no jump patching yet).
		for (int bi = 0; bi < ir_func.blocks.size(); bi++) {
			const GDScript2IRBlock &block = ir_func.blocks[bi];
			for (int ii = 0; ii < block.instructions.size(); ii++) {
				const GDScript2IRInstr &ir_instr = block.instructions[ii];
				GDScript2BytecodeInstr bc;
				bc.op = ir_op_to_bytecode(ir_instr.op);
				bc.arg0 = ir_instr.dest;
				if (ir_instr.args.size() > 0) {
					bc.arg1 = ir_instr.args[0];
				}
				if (ir_instr.args.size() > 1) {
					bc.arg2 = ir_instr.args[1];
				}
				// Handle LOAD_CONST: add constant and store index.
				if (ir_instr.op == GDScript2IROp::LOAD_CONST) {
					int const_idx = add_constant(ir_instr.const_value);
					bc.arg1 = const_idx;
				}
				emit(bc);
			}
		}

		// Emit end marker.
		GDScript2BytecodeInstr end_bc;
		end_bc.op = GDScript2Opcode::OP_END;
		emit(end_bc);
	}

	result.module = module;
	return result;
}
