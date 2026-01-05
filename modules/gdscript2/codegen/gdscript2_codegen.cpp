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

// ============================================================================
// Opcode Names (for debugging)
// ============================================================================

static const char *opcode_names[] = {
	"NOP",
	"END",
	"LOAD_CONST",
	"LOAD_NIL",
	"LOAD_TRUE",
	"LOAD_FALSE",
	"LOAD_SMALL_INT",
	"LOAD_LOCAL",
	"STORE_LOCAL",
	"LOAD_MEMBER",
	"STORE_MEMBER",
	"LOAD_GLOBAL",
	"STORE_GLOBAL",
	"LOAD_SELF",
	"ADD",
	"SUB",
	"MUL",
	"DIV",
	"MOD",
	"POW",
	"NEG",
	"POS",
	"BIT_AND",
	"BIT_OR",
	"BIT_XOR",
	"BIT_NOT",
	"BIT_LSH",
	"BIT_RSH",
	"EQ",
	"NE",
	"LT",
	"LE",
	"GT",
	"GE",
	"NOT",
	"IS",
	"AS",
	"TYPEOF",
	"IN",
	"CONSTRUCT_ARRAY",
	"CONSTRUCT_DICT",
	"GET_INDEX",
	"SET_INDEX",
	"GET_NAMED",
	"SET_NAMED",
	"JUMP",
	"JUMP_IF",
	"JUMP_IF_NOT",
	"CALL",
	"CALL_METHOD",
	"CALL_BUILTIN",
	"CALL_SUPER",
	"CALL_SELF",
	"RETURN",
	"RETURN_VOID",
	"ITER_BEGIN",
	"ITER_NEXT",
	"ITER_GET",
	"JUMP_IF_ITER_END",
	"AWAIT",
	"YIELD",
	"PRELOAD",
	"GET_NODE",
	"LAMBDA",
	"COPY",
	"ASSERT",
	"BREAKPOINT",
	"LINE",
};

const char *gdscript2_opcode_name(GDScript2Opcode p_op) {
	int idx = static_cast<int>(p_op);
	if (idx >= 0 && idx < static_cast<int>(GDScript2Opcode::OP_MAX)) {
		return opcode_names[idx];
	}
	return "UNKNOWN";
}

// ============================================================================
// GDScript2BytecodeInstr
// ============================================================================

String GDScript2BytecodeInstr::to_string() const {
	String result = gdscript2_opcode_name(op);
	for (uint32_t i = 0; i < operands.size(); i++) {
		result += " " + itos(operands[i]);
	}
	if (line > 0) {
		result += " ; line " + itos(line);
	}
	return result;
}

// ============================================================================
// GDScript2CompiledFunction
// ============================================================================

String GDScript2CompiledFunction::to_string() const {
	String result;
	result += "function " + String(name) + ":\n";
	result += "  locals: " + itos(local_count) + ", params: " + itos(param_count) + "\n";
	result += "  constants: " + itos(constants.size()) + "\n";

	for (int i = 0; i < constants.size(); i++) {
		result += "    [" + itos(i) + "] = " + String(constants[i]) + "\n";
	}

	result += "  code:\n";
	for (int i = 0; i < code.size(); i++) {
		result += "    " + itos(i) + ": " + code[i].to_string() + "\n";
	}

	return result;
}

// ============================================================================
// GDScript2CompiledModule
// ============================================================================

String GDScript2CompiledModule::to_string() const {
	String result;
	result += "module " + String(name) + ":\n";
	result += "functions: " + itos(functions.size()) + "\n\n";

	for (int i = 0; i < functions.size(); i++) {
		result += functions[i].to_string() + "\n";
	}

	return result;
}

// ============================================================================
// GDScript2CodeGenerator - Helper Methods
// ============================================================================

void GDScript2CodeGenerator::reset_function_state() {
	current_func = nullptr;
	current_ir_func = nullptr;
	block_addresses.clear();
	pending_jumps.clear();
	constant_map.clear();
	name_map.clear();
}

int GDScript2CodeGenerator::emit(const GDScript2BytecodeInstr &p_instr) {
	if (current_func == nullptr) {
		return -1;
	}
	int idx = current_func->code.size();
	current_func->code.push_back(p_instr);
	return idx;
}

int GDScript2CodeGenerator::emit_op(GDScript2Opcode p_op) {
	GDScript2BytecodeInstr instr(p_op);
	return emit(instr);
}

int GDScript2CodeGenerator::emit_op(GDScript2Opcode p_op, int32_t p_arg0) {
	GDScript2BytecodeInstr instr(p_op);
	instr.add_operand(p_arg0);
	return emit(instr);
}

int GDScript2CodeGenerator::emit_op(GDScript2Opcode p_op, int32_t p_arg0, int32_t p_arg1) {
	GDScript2BytecodeInstr instr(p_op);
	instr.add_operand(p_arg0);
	instr.add_operand(p_arg1);
	return emit(instr);
}

int GDScript2CodeGenerator::emit_op(GDScript2Opcode p_op, int32_t p_arg0, int32_t p_arg1, int32_t p_arg2) {
	GDScript2BytecodeInstr instr(p_op);
	instr.add_operand(p_arg0);
	instr.add_operand(p_arg1);
	instr.add_operand(p_arg2);
	return emit(instr);
}

int GDScript2CodeGenerator::add_constant(const Variant &p_value) {
	if (current_func == nullptr) {
		return -1;
	}

	if (constant_map.has(p_value)) {
		return constant_map[p_value];
	}

	int idx = current_func->constants.size();
	current_func->constants.push_back(p_value);
	constant_map[p_value] = idx;
	return idx;
}

int GDScript2CodeGenerator::add_name(const StringName &p_name) {
	if (current_func == nullptr) {
		return -1;
	}

	if (name_map.has(p_name)) {
		return name_map[p_name];
	}

	int idx = current_func->names.size();
	current_func->names.push_back(p_name);
	name_map[p_name] = idx;
	return idx;
}

int32_t GDScript2CodeGenerator::operand_to_reg(const GDScript2IROperand &p_operand) {
	switch (p_operand.type) {
		case GDScript2IROperandType::REGISTER:
			// Registers come after locals in the stack
			return p_operand.reg_id + (current_ir_func ? current_ir_func->locals.size() : 0);
		case GDScript2IROperandType::LOCAL_VAR:
			return p_operand.local_idx;
		default:
			return -1;
	}
}

int32_t GDScript2CodeGenerator::emit_load_operand(const GDScript2IROperand &p_operand, int32_t p_temp_reg) {
	int32_t local_offset = current_ir_func ? current_ir_func->locals.size() : 0;

	switch (p_operand.type) {
		case GDScript2IROperandType::REGISTER:
			// Registers come after locals in the stack
			return p_operand.reg_id + local_offset;

		case GDScript2IROperandType::LOCAL_VAR:
			return p_operand.local_idx;

		case GDScript2IROperandType::CONSTANT: {
			// Load from function constant pool
			// p_temp_reg is already an IR register ID, needs offset
			int const_idx = p_operand.const_idx;
			emit_op(GDScript2Opcode::OP_LOAD_CONST, p_temp_reg + local_offset, const_idx);
			return p_temp_reg + local_offset;
		}

		case GDScript2IROperandType::IMMEDIATE: {
			// Add to constant pool and load
			// p_temp_reg is already an IR register ID, needs offset
			int const_idx = add_constant(p_operand.imm_value);
			emit_op(GDScript2Opcode::OP_LOAD_CONST, p_temp_reg + local_offset, const_idx);
			return p_temp_reg + local_offset;
		}

		case GDScript2IROperandType::NAME: {
			// Load global by name
			// p_temp_reg is already an IR register ID, needs offset
			int name_idx = add_name(p_operand.name);
			emit_op(GDScript2Opcode::OP_LOAD_GLOBAL, p_temp_reg + local_offset, name_idx);
			return p_temp_reg + local_offset;
		}

		default:
			return -1;
	}
}

// ============================================================================
// Jump Patching
// ============================================================================

void GDScript2CodeGenerator::add_jump_patch(int32_t p_bytecode_idx, int32_t p_operand_idx, int p_target_block) {
	JumpPatch patch;
	patch.bytecode_idx = p_bytecode_idx;
	patch.operand_idx = p_operand_idx;
	patch.block_id = p_target_block;
	pending_jumps.push_back(patch);
}

void GDScript2CodeGenerator::patch_jumps() {
	if (current_func == nullptr) {
		return;
	}

	for (uint32_t i = 0; i < pending_jumps.size(); i++) {
		const JumpPatch &patch = pending_jumps[i];

		if (block_addresses.has(patch.block_id)) {
			int32_t target_addr = block_addresses[patch.block_id];
			GDScript2BytecodeInstr &instr = current_func->code.write[patch.bytecode_idx];
			if (patch.operand_idx < (int)instr.operands.size()) {
				instr.operands[patch.operand_idx] = target_addr;
			}
		}
	}
}

// ============================================================================
// IR to Bytecode Conversion - Main Entry
// ============================================================================

GDScript2CodeGenerator::Result GDScript2CodeGenerator::generate(const GDScript2IRModule &p_ir) {
	Result result;
	module = GDScript2CompiledModule();
	module.name = p_ir.name;

	// Copy globals
	for (uint32_t i = 0; i < p_ir.globals.size(); i++) {
		module.globals.push_back(p_ir.globals[i]);
	}

	// Generate each function
	for (uint32_t i = 0; i < p_ir.functions.size(); i++) {
		const GDScript2IRFunction &ir_func = p_ir.functions[i];

		GDScript2CompiledFunction compiled_func;
		compiled_func.name = ir_func.name;
		compiled_func.is_static = ir_func.is_static;
		compiled_func.is_coroutine = ir_func.is_coroutine;
		compiled_func.param_count = ir_func.parameter_count;
		compiled_func.local_count = ir_func.locals.size();
		compiled_func.temp_count = ir_func.next_reg;
		compiled_func.source_start_line = ir_func.source_line;

		module.functions.push_back(compiled_func);
		module.function_map[ir_func.name] = module.functions.size() - 1;

		current_func = &module.functions.write[module.functions.size() - 1];
		current_ir_func = &ir_func;

		generate_function(ir_func);

		reset_function_state();
	}

	result.module = module;
	return result;
}

void GDScript2CodeGenerator::generate_function(const GDScript2IRFunction &p_ir_func) {
	// First pass: compute block addresses
	int current_offset = 0;

	for (uint32_t bi = 0; bi < p_ir_func.blocks.size(); bi++) {
		const GDScript2IRBlock &block = p_ir_func.blocks[bi];
		block_addresses[block.id] = current_offset;

		// Each IR instruction becomes roughly one bytecode instruction
		// (this is a simplification; actual count may vary)
		current_offset += block.instructions.size();
	}

	// Second pass: generate bytecode for each block
	for (uint32_t bi = 0; bi < p_ir_func.blocks.size(); bi++) {
		const GDScript2IRBlock &block = p_ir_func.blocks[bi];

		// Update actual block address
		block_addresses[block.id] = current_func->code.size();

		generate_block(block);
	}

	// Patch jump targets
	patch_jumps();

	// Emit end marker
	emit_op(GDScript2Opcode::OP_END);

	// Calculate max stack - simple and safe: locals + temps with room for extra temporaries
	int32_t base_stack = current_func->local_count + (current_ir_func ? current_ir_func->next_reg : 0);
	// Add extra space for temporaries used during code generation (2x for safety)
	current_func->max_stack = base_stack * 2 + 32;
}

void GDScript2CodeGenerator::generate_block(const GDScript2IRBlock &p_block) {
	for (uint32_t i = 0; i < p_block.instructions.size(); i++) {
		const GDScript2IRInstr &instr = p_block.instructions[i];

		// Skip dead instructions
		if (instr.is_dead) {
			continue;
		}

		// Emit line info if changed
		if (instr.source_line > 0 && (current_func->line_info.is_empty() || current_func->line_info[current_func->line_info.size() - 1].source_line != instr.source_line)) {
			GDScript2LineInfo li;
			li.bytecode_offset = current_func->code.size();
			li.source_line = instr.source_line;
			current_func->line_info.push_back(li);
		}

		generate_instruction(instr);
	}
}

void GDScript2CodeGenerator::generate_instruction(const GDScript2IRInstr &p_instr) {
	switch (p_instr.op) {
		case GDScript2IROp::OP_NOP:
			emit_op(GDScript2Opcode::OP_NOP);
			break;

		case GDScript2IROp::OP_COMMENT:
			// Comments are ignored in bytecode
			break;

		case GDScript2IROp::OP_LOAD_CONST:
			gen_load_const(p_instr);
			break;

		case GDScript2IROp::OP_LOAD_NIL:
			gen_load_nil(p_instr);
			break;

		case GDScript2IROp::OP_LOAD_LOCAL:
			gen_load_local(p_instr);
			break;

		case GDScript2IROp::OP_STORE_LOCAL:
			gen_store_local(p_instr);
			break;

		case GDScript2IROp::OP_LOAD_MEMBER:
			gen_load_member(p_instr);
			break;

		case GDScript2IROp::OP_STORE_MEMBER:
			gen_store_member(p_instr);
			break;

		case GDScript2IROp::OP_LOAD_GLOBAL:
			gen_load_global(p_instr);
			break;

		case GDScript2IROp::OP_STORE_GLOBAL:
			gen_store_global(p_instr);
			break;

		case GDScript2IROp::OP_LOAD_SELF:
			gen_load_self(p_instr);
			break;

		// Arithmetic
		case GDScript2IROp::OP_ADD:
		case GDScript2IROp::OP_SUB:
		case GDScript2IROp::OP_MUL:
		case GDScript2IROp::OP_DIV:
		case GDScript2IROp::OP_MOD:
		case GDScript2IROp::OP_POW:
		case GDScript2IROp::OP_BIT_AND:
		case GDScript2IROp::OP_BIT_OR:
		case GDScript2IROp::OP_BIT_XOR:
		case GDScript2IROp::OP_BIT_LSH:
		case GDScript2IROp::OP_BIT_RSH:
			gen_binary_op(p_instr);
			break;

		// Unary
		case GDScript2IROp::OP_NEG:
		case GDScript2IROp::OP_POS:
		case GDScript2IROp::OP_BIT_NOT:
		case GDScript2IROp::OP_NOT:
			gen_unary_op(p_instr);
			break;

		// Comparison
		case GDScript2IROp::OP_EQ:
		case GDScript2IROp::OP_NE:
		case GDScript2IROp::OP_LT:
		case GDScript2IROp::OP_LE:
		case GDScript2IROp::OP_GT:
		case GDScript2IROp::OP_GE:
			gen_comparison(p_instr);
			break;

		// Logical (short-circuit already handled in IR)
		case GDScript2IROp::OP_AND:
		case GDScript2IROp::OP_OR:
			gen_binary_op(p_instr);
			break;

		// Type operations
		case GDScript2IROp::OP_IS:
		case GDScript2IROp::OP_AS:
		case GDScript2IROp::OP_TYPEOF:
		case GDScript2IROp::OP_IN:
			gen_type_op(p_instr);
			break;

		// Container operations
		case GDScript2IROp::OP_CONSTRUCT_ARRAY:
			gen_construct_array(p_instr);
			break;

		case GDScript2IROp::OP_CONSTRUCT_DICT:
			gen_construct_dict(p_instr);
			break;

		case GDScript2IROp::OP_GET_INDEX:
			gen_get_index(p_instr);
			break;

		case GDScript2IROp::OP_SET_INDEX:
			gen_set_index(p_instr);
			break;

		case GDScript2IROp::OP_GET_NAMED:
			gen_get_named(p_instr);
			break;

		case GDScript2IROp::OP_SET_NAMED:
			gen_set_named(p_instr);
			break;

		// Control flow
		case GDScript2IROp::OP_JUMP:
			gen_jump(p_instr);
			break;

		case GDScript2IROp::OP_JUMP_IF:
			gen_jump_if(p_instr);
			break;

		case GDScript2IROp::OP_JUMP_IF_NOT:
			gen_jump_if_not(p_instr);
			break;

		// Function calls
		case GDScript2IROp::OP_CALL:
			gen_call(p_instr);
			break;

		case GDScript2IROp::OP_CALL_METHOD:
			gen_call_method(p_instr);
			break;

		case GDScript2IROp::OP_CALL_BUILTIN:
			gen_call_builtin(p_instr);
			break;

		case GDScript2IROp::OP_CALL_SUPER:
			gen_call_super(p_instr);
			break;

		case GDScript2IROp::OP_CALL_SELF:
			gen_call_self(p_instr);
			break;

		// Return
		case GDScript2IROp::OP_RETURN:
			gen_return(p_instr);
			break;

		case GDScript2IROp::OP_RETURN_VOID:
			gen_return_void(p_instr);
			break;

		// Iterators
		case GDScript2IROp::OP_ITER_BEGIN:
			gen_iter_begin(p_instr);
			break;

		case GDScript2IROp::OP_ITER_NEXT:
			gen_iter_next(p_instr);
			break;

		case GDScript2IROp::OP_ITER_GET:
			gen_iter_get(p_instr);
			break;

		// Special
		case GDScript2IROp::OP_AWAIT:
			gen_await(p_instr);
			break;

		case GDScript2IROp::OP_PRELOAD:
			gen_preload(p_instr);
			break;

		case GDScript2IROp::OP_GET_NODE:
			gen_get_node(p_instr);
			break;

		case GDScript2IROp::OP_LAMBDA:
			gen_lambda(p_instr);
			break;

		case GDScript2IROp::OP_COPY:
			gen_copy(p_instr);
			break;

		case GDScript2IROp::OP_PHI:
			// PHI nodes should be eliminated before codegen
			// If we encounter one, emit a copy (simplified)
			gen_copy(p_instr);
			break;

		case GDScript2IROp::OP_ASSERT:
			gen_assert(p_instr);
			break;

		case GDScript2IROp::OP_BREAKPOINT:
			gen_breakpoint(p_instr);
			break;

		case GDScript2IROp::OP_YIELD:
			// YIELD becomes RETURN_VOID with coroutine flag
			gen_return_void(p_instr);
			break;

		default:
			// Unknown opcode - emit NOP
			emit_op(GDScript2Opcode::OP_NOP);
			break;
	}
}

// ============================================================================
// Specific Instruction Generators
// ============================================================================

void GDScript2CodeGenerator::gen_load_const(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);

	// Check if it's an immediate value or constant pool reference
	if (p_instr.args.size() > 0) {
		const GDScript2IROperand &arg = p_instr.args[0];
		if (arg.type == GDScript2IROperandType::IMMEDIATE) {
			// Check for special cases
			if (arg.imm_value.get_type() == Variant::NIL) {
				emit_op(GDScript2Opcode::OP_LOAD_NIL, dest);
				return;
			}
			if (arg.imm_value.get_type() == Variant::BOOL) {
				if (arg.imm_value.operator bool()) {
					emit_op(GDScript2Opcode::OP_LOAD_TRUE, dest);
				} else {
					emit_op(GDScript2Opcode::OP_LOAD_FALSE, dest);
				}
				return;
			}
			if (arg.imm_value.get_type() == Variant::INT) {
				int64_t val = arg.imm_value.operator int64_t();
				if (val >= -128 && val <= 127) {
					emit_op(GDScript2Opcode::OP_LOAD_SMALL_INT, dest, static_cast<int32_t>(val));
					return;
				}
			}

			// General case: add to constant pool
			int const_idx = add_constant(arg.imm_value);
			emit_op(GDScript2Opcode::OP_LOAD_CONST, dest, const_idx);
		} else if (arg.type == GDScript2IROperandType::CONSTANT) {
			// Reference to IR function's constant pool
			if (current_ir_func != nullptr && arg.const_idx >= 0 &&
					(uint32_t)arg.const_idx < current_ir_func->constants.size()) {
				int const_idx = add_constant(current_ir_func->constants[arg.const_idx]);
				emit_op(GDScript2Opcode::OP_LOAD_CONST, dest, const_idx);
			}
		}
	}
}

void GDScript2CodeGenerator::gen_load_nil(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);
	emit_op(GDScript2Opcode::OP_LOAD_NIL, dest);
}

void GDScript2CodeGenerator::gen_load_local(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);

	if (p_instr.args.size() > 0) {
		const GDScript2IROperand &arg = p_instr.args[0];
		int32_t local_idx = -1;

		if (arg.type == GDScript2IROperandType::LOCAL_VAR) {
			local_idx = arg.local_idx;
		} else if (arg.type == GDScript2IROperandType::IMMEDIATE) {
			local_idx = arg.imm_value.operator int64_t();
		}

		if (local_idx >= 0) {
			emit_op(GDScript2Opcode::OP_LOAD_LOCAL, dest, local_idx);
		}
	}
}

void GDScript2CodeGenerator::gen_store_local(const GDScript2IRInstr &p_instr) {
	int32_t local_idx = -1;
	int32_t src_reg = -1;

	if (p_instr.dest.type == GDScript2IROperandType::LOCAL_VAR) {
		local_idx = p_instr.dest.local_idx;
	}

	if (p_instr.args.size() > 0) {
		src_reg = operand_to_reg(p_instr.args[0]);
		if (src_reg < 0 && p_instr.args[0].type == GDScript2IROperandType::IMMEDIATE) {
			// Need to load immediate first
			int32_t local_offset = current_ir_func ? current_ir_func->locals.size() : 0;
			int32_t temp = (current_ir_func ? current_ir_func->next_reg : 0) + local_offset;
			int const_idx = add_constant(p_instr.args[0].imm_value);
			emit_op(GDScript2Opcode::OP_LOAD_CONST, temp, const_idx);
			src_reg = temp;
		}
	}

	if (local_idx >= 0 && src_reg >= 0) {
		emit_op(GDScript2Opcode::OP_STORE_LOCAL, local_idx, src_reg);
	}
}

void GDScript2CodeGenerator::gen_load_member(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);

	if (p_instr.args.size() > 0) {
		const GDScript2IROperand &arg = p_instr.args[0];
		if (arg.type == GDScript2IROperandType::NAME) {
			int name_idx = add_name(arg.name);
			emit_op(GDScript2Opcode::OP_LOAD_MEMBER, dest, name_idx);
		}
	}
}

void GDScript2CodeGenerator::gen_store_member(const GDScript2IRInstr &p_instr) {
	if (p_instr.args.size() >= 2) {
		const GDScript2IROperand &name_arg = p_instr.args[0];
		const GDScript2IROperand &src_arg = p_instr.args[1];

		if (name_arg.type == GDScript2IROperandType::NAME) {
			int name_idx = add_name(name_arg.name);
			int32_t src_reg = operand_to_reg(src_arg);

			if (src_reg < 0) {
				// Load to temp
				int32_t temp = current_ir_func ? current_ir_func->next_reg : 0;
				src_reg = emit_load_operand(src_arg, temp);
			}

			emit_op(GDScript2Opcode::OP_STORE_MEMBER, name_idx, src_reg);
		}
	}
}

void GDScript2CodeGenerator::gen_load_global(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);

	if (p_instr.args.size() > 0) {
		const GDScript2IROperand &arg = p_instr.args[0];
		if (arg.type == GDScript2IROperandType::NAME) {
			int name_idx = add_name(arg.name);
			emit_op(GDScript2Opcode::OP_LOAD_GLOBAL, dest, name_idx);
		}
	}
}

void GDScript2CodeGenerator::gen_store_global(const GDScript2IRInstr &p_instr) {
	if (p_instr.args.size() >= 2) {
		const GDScript2IROperand &name_arg = p_instr.args[0];
		const GDScript2IROperand &src_arg = p_instr.args[1];

		if (name_arg.type == GDScript2IROperandType::NAME) {
			int name_idx = add_name(name_arg.name);
			int32_t src_reg = operand_to_reg(src_arg);

			if (src_reg < 0) {
				int32_t temp = current_ir_func ? current_ir_func->next_reg : 0;
				src_reg = emit_load_operand(src_arg, temp);
			}

			emit_op(GDScript2Opcode::OP_STORE_GLOBAL, name_idx, src_reg);
		}
	}
}

void GDScript2CodeGenerator::gen_load_self(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);
	emit_op(GDScript2Opcode::OP_LOAD_SELF, dest);
}

void GDScript2CodeGenerator::gen_binary_op(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);

	int32_t left_reg = -1;
	int32_t right_reg = -1;

	if (p_instr.args.size() >= 2) {
		left_reg = operand_to_reg(p_instr.args[0]);
		right_reg = operand_to_reg(p_instr.args[1]);

		// Handle immediate operands
		int32_t temp_base = current_ir_func ? current_ir_func->next_reg : 0;

		if (left_reg < 0) {
			left_reg = emit_load_operand(p_instr.args[0], temp_base);
			temp_base++;
		}
		if (right_reg < 0) {
			right_reg = emit_load_operand(p_instr.args[1], temp_base);
		}
	}

	// Map IR op to bytecode op
	GDScript2Opcode bc_op = GDScript2Opcode::OP_NOP;
	switch (p_instr.op) {
		case GDScript2IROp::OP_ADD:
			bc_op = GDScript2Opcode::OP_ADD;
			break;
		case GDScript2IROp::OP_SUB:
			bc_op = GDScript2Opcode::OP_SUB;
			break;
		case GDScript2IROp::OP_MUL:
			bc_op = GDScript2Opcode::OP_MUL;
			break;
		case GDScript2IROp::OP_DIV:
			bc_op = GDScript2Opcode::OP_DIV;
			break;
		case GDScript2IROp::OP_MOD:
			bc_op = GDScript2Opcode::OP_MOD;
			break;
		case GDScript2IROp::OP_POW:
			bc_op = GDScript2Opcode::OP_POW;
			break;
		case GDScript2IROp::OP_BIT_AND:
			bc_op = GDScript2Opcode::OP_BIT_AND;
			break;
		case GDScript2IROp::OP_BIT_OR:
			bc_op = GDScript2Opcode::OP_BIT_OR;
			break;
		case GDScript2IROp::OP_BIT_XOR:
			bc_op = GDScript2Opcode::OP_BIT_XOR;
			break;
		case GDScript2IROp::OP_BIT_LSH:
			bc_op = GDScript2Opcode::OP_BIT_LSH;
			break;
		case GDScript2IROp::OP_BIT_RSH:
			bc_op = GDScript2Opcode::OP_BIT_RSH;
			break;
		default:
			bc_op = GDScript2Opcode::OP_NOP;
			break;
	}

	emit_op(bc_op, dest, left_reg, right_reg);
}

void GDScript2CodeGenerator::gen_unary_op(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);
	int32_t operand_reg = -1;

	if (p_instr.args.size() >= 1) {
		operand_reg = operand_to_reg(p_instr.args[0]);

		if (operand_reg < 0) {
			int32_t temp = current_ir_func ? current_ir_func->next_reg : 0;
			operand_reg = emit_load_operand(p_instr.args[0], temp);
		}
	}

	GDScript2Opcode bc_op = GDScript2Opcode::OP_NOP;
	switch (p_instr.op) {
		case GDScript2IROp::OP_NEG:
			bc_op = GDScript2Opcode::OP_NEG;
			break;
		case GDScript2IROp::OP_POS:
			bc_op = GDScript2Opcode::OP_POS;
			break;
		case GDScript2IROp::OP_BIT_NOT:
			bc_op = GDScript2Opcode::OP_BIT_NOT;
			break;
		case GDScript2IROp::OP_NOT:
			bc_op = GDScript2Opcode::OP_NOT;
			break;
		default:
			break;
	}

	emit_op(bc_op, dest, operand_reg);
}

void GDScript2CodeGenerator::gen_comparison(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);

	int32_t left_reg = -1;
	int32_t right_reg = -1;

	if (p_instr.args.size() >= 2) {
		int32_t temp_base = current_ir_func ? current_ir_func->next_reg : 0;

		left_reg = operand_to_reg(p_instr.args[0]);
		if (left_reg < 0) {
			left_reg = emit_load_operand(p_instr.args[0], temp_base);
			temp_base++;
		}

		right_reg = operand_to_reg(p_instr.args[1]);
		if (right_reg < 0) {
			right_reg = emit_load_operand(p_instr.args[1], temp_base);
		}
	}

	GDScript2Opcode bc_op = GDScript2Opcode::OP_NOP;
	switch (p_instr.op) {
		case GDScript2IROp::OP_EQ:
			bc_op = GDScript2Opcode::OP_EQ;
			break;
		case GDScript2IROp::OP_NE:
			bc_op = GDScript2Opcode::OP_NE;
			break;
		case GDScript2IROp::OP_LT:
			bc_op = GDScript2Opcode::OP_LT;
			break;
		case GDScript2IROp::OP_LE:
			bc_op = GDScript2Opcode::OP_LE;
			break;
		case GDScript2IROp::OP_GT:
			bc_op = GDScript2Opcode::OP_GT;
			break;
		case GDScript2IROp::OP_GE:
			bc_op = GDScript2Opcode::OP_GE;
			break;
		default:
			break;
	}

	emit_op(bc_op, dest, left_reg, right_reg);
}

void GDScript2CodeGenerator::gen_type_op(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);

	GDScript2Opcode bc_op = GDScript2Opcode::OP_NOP;
	switch (p_instr.op) {
		case GDScript2IROp::OP_IS:
			bc_op = GDScript2Opcode::OP_IS;
			break;
		case GDScript2IROp::OP_AS:
			bc_op = GDScript2Opcode::OP_AS;
			break;
		case GDScript2IROp::OP_TYPEOF:
			bc_op = GDScript2Opcode::OP_TYPEOF;
			break;
		case GDScript2IROp::OP_IN:
			bc_op = GDScript2Opcode::OP_IN;
			break;
		default:
			break;
	}

	if (p_instr.args.size() >= 2) {
		int32_t temp_base = current_ir_func ? current_ir_func->next_reg : 0;

		int32_t left_reg = operand_to_reg(p_instr.args[0]);
		if (left_reg < 0) {
			left_reg = emit_load_operand(p_instr.args[0], temp_base);
			temp_base++;
		}

		int32_t right_reg = operand_to_reg(p_instr.args[1]);
		if (right_reg < 0) {
			right_reg = emit_load_operand(p_instr.args[1], temp_base);
		}

		emit_op(bc_op, dest, left_reg, right_reg);
	} else if (p_instr.args.size() >= 1) {
		// TYPEOF only has one operand
		int32_t operand_reg = operand_to_reg(p_instr.args[0]);
		if (operand_reg < 0) {
			int32_t temp = current_ir_func ? current_ir_func->next_reg : 0;
			operand_reg = emit_load_operand(p_instr.args[0], temp);
		}
		emit_op(bc_op, dest, operand_reg);
	}
}

void GDScript2CodeGenerator::gen_construct_array(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);

	// First, load all immediates into temporaries
	LocalVector<int32_t> element_regs;
	int32_t temp_base = current_ir_func ? current_ir_func->next_reg : 0;
	for (uint32_t i = 0; i < p_instr.args.size(); i++) {
		int32_t reg = operand_to_reg(p_instr.args[i]);
		if (reg < 0) {
			// Need to load constant/immediate first
			reg = emit_load_operand(p_instr.args[i], temp_base);
			temp_base++;
		}
		element_regs.push_back(reg);
	}

	// Now emit the construct array instruction
	GDScript2BytecodeInstr bc(GDScript2Opcode::OP_CONSTRUCT_ARRAY);
	bc.add_operand(dest);
	bc.add_operand(static_cast<int32_t>(element_regs.size()));
	for (int32_t reg : element_regs) {
		bc.add_operand(reg);
	}

	emit(bc);
}

void GDScript2CodeGenerator::gen_construct_dict(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);

	// First, load all immediates into temporaries
	LocalVector<int32_t> element_regs;
	int32_t temp_base = current_ir_func ? current_ir_func->next_reg : 0;
	for (uint32_t i = 0; i < p_instr.args.size(); i++) {
		int32_t reg = operand_to_reg(p_instr.args[i]);
		if (reg < 0) {
			// Need to load constant/immediate first
			reg = emit_load_operand(p_instr.args[i], temp_base);
			temp_base++;
		}
		element_regs.push_back(reg);
	}

	// Now emit the construct dict instruction
	GDScript2BytecodeInstr bc(GDScript2Opcode::OP_CONSTRUCT_DICT);
	bc.add_operand(dest);
	bc.add_operand(static_cast<int32_t>(element_regs.size() / 2)); // Pair count
	for (int32_t reg : element_regs) {
		bc.add_operand(reg);
	}

	emit(bc);
}

void GDScript2CodeGenerator::gen_get_index(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);

	if (p_instr.args.size() >= 2) {
		int32_t temp_base = current_ir_func ? current_ir_func->next_reg : 0;

		int32_t base_reg = operand_to_reg(p_instr.args[0]);
		if (base_reg < 0) {
			base_reg = emit_load_operand(p_instr.args[0], temp_base);
			temp_base++;
		}

		int32_t idx_reg = operand_to_reg(p_instr.args[1]);
		if (idx_reg < 0) {
			idx_reg = emit_load_operand(p_instr.args[1], temp_base);
		}

		emit_op(GDScript2Opcode::OP_GET_INDEX, dest, base_reg, idx_reg);
	}
}

void GDScript2CodeGenerator::gen_set_index(const GDScript2IRInstr &p_instr) {
	if (p_instr.args.size() >= 3) {
		int32_t temp_base = current_ir_func ? current_ir_func->next_reg : 0;

		int32_t base_reg = operand_to_reg(p_instr.args[0]);
		if (base_reg < 0) {
			base_reg = emit_load_operand(p_instr.args[0], temp_base);
			temp_base++;
		}

		int32_t idx_reg = operand_to_reg(p_instr.args[1]);
		if (idx_reg < 0) {
			idx_reg = emit_load_operand(p_instr.args[1], temp_base);
			temp_base++;
		}

		int32_t val_reg = operand_to_reg(p_instr.args[2]);
		if (val_reg < 0) {
			val_reg = emit_load_operand(p_instr.args[2], temp_base);
		}

		emit_op(GDScript2Opcode::OP_SET_INDEX, base_reg, idx_reg, val_reg);
	}
}

void GDScript2CodeGenerator::gen_get_named(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);

	if (p_instr.args.size() >= 2) {
		int32_t base_reg = operand_to_reg(p_instr.args[0]);
		if (base_reg < 0) {
			int32_t temp = current_ir_func ? current_ir_func->next_reg : 0;
			base_reg = emit_load_operand(p_instr.args[0], temp);
		}

		int name_idx = -1;
		if (p_instr.args[1].type == GDScript2IROperandType::NAME) {
			name_idx = add_name(p_instr.args[1].name);
		}

		emit_op(GDScript2Opcode::OP_GET_NAMED, dest, base_reg, name_idx);
	}
}

void GDScript2CodeGenerator::gen_set_named(const GDScript2IRInstr &p_instr) {
	if (p_instr.args.size() >= 3) {
		int32_t temp_base = current_ir_func ? current_ir_func->next_reg : 0;

		int32_t base_reg = operand_to_reg(p_instr.args[0]);
		if (base_reg < 0) {
			base_reg = emit_load_operand(p_instr.args[0], temp_base);
			temp_base++;
		}

		int name_idx = -1;
		if (p_instr.args[1].type == GDScript2IROperandType::NAME) {
			name_idx = add_name(p_instr.args[1].name);
		}

		int32_t val_reg = operand_to_reg(p_instr.args[2]);
		if (val_reg < 0) {
			val_reg = emit_load_operand(p_instr.args[2], temp_base);
		}

		emit_op(GDScript2Opcode::OP_SET_NAMED, base_reg, name_idx, val_reg);
	}
}

void GDScript2CodeGenerator::gen_jump(const GDScript2IRInstr &p_instr) {
	GDScript2BytecodeInstr bc(GDScript2Opcode::OP_JUMP);
	bc.add_operand(0); // Placeholder for target address

	int bc_idx = emit(bc);
	add_jump_patch(bc_idx, 0, p_instr.target_block);
}

void GDScript2CodeGenerator::gen_jump_if(const GDScript2IRInstr &p_instr) {
	int32_t cond_reg = -1;

	if (p_instr.args.size() >= 1) {
		cond_reg = operand_to_reg(p_instr.args[0]);
		if (cond_reg < 0) {
			int32_t temp = current_ir_func ? current_ir_func->next_reg : 0;
			cond_reg = emit_load_operand(p_instr.args[0], temp);
		}
	}

	// Emit JUMP_IF for true branch
	GDScript2BytecodeInstr bc(GDScript2Opcode::OP_JUMP_IF);
	bc.add_operand(cond_reg);
	bc.add_operand(0); // Placeholder for true target

	int bc_idx = emit(bc);
	add_jump_patch(bc_idx, 1, p_instr.target_block);

	// If there's an else block, emit JUMP for it
	if (p_instr.else_block >= 0) {
		GDScript2BytecodeInstr jmp(GDScript2Opcode::OP_JUMP);
		jmp.add_operand(0); // Placeholder

		int jmp_idx = emit(jmp);
		add_jump_patch(jmp_idx, 0, p_instr.else_block);
	}
}

void GDScript2CodeGenerator::gen_jump_if_not(const GDScript2IRInstr &p_instr) {
	int32_t cond_reg = -1;

	if (p_instr.args.size() >= 1) {
		cond_reg = operand_to_reg(p_instr.args[0]);
		if (cond_reg < 0) {
			int32_t temp = current_ir_func ? current_ir_func->next_reg : 0;
			cond_reg = emit_load_operand(p_instr.args[0], temp);
		}
	}

	GDScript2BytecodeInstr bc(GDScript2Opcode::OP_JUMP_IF_NOT);
	bc.add_operand(cond_reg);
	bc.add_operand(0); // Placeholder

	int bc_idx = emit(bc);
	add_jump_patch(bc_idx, 1, p_instr.target_block);
}

void GDScript2CodeGenerator::gen_call(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);

	// Check if this is a direct call by name or indirect call
	if (p_instr.args.size() >= 1 && p_instr.args[0].type == GDScript2IROperandType::NAME) {
		// Direct function call by name - use OP_CALL_SELF
		GDScript2BytecodeInstr bc(GDScript2Opcode::OP_CALL_SELF);
		bc.add_operand(dest);

		int32_t name_idx = add_name(p_instr.args[0].name);
		bc.add_operand(name_idx);

		// Load arguments into registers and collect their indices
		LocalVector<int32_t> arg_regs;
		int32_t temp_base = current_ir_func ? current_ir_func->next_reg : 0;

		for (uint32_t i = 1; i < p_instr.args.size(); i++) {
			int32_t arg_reg = emit_load_operand(p_instr.args[i], temp_base + (i - 1));
			arg_regs.push_back(arg_reg);
		}

		// Arg count
		bc.add_operand(static_cast<int32_t>(arg_regs.size()));

		// Arguments
		for (int32_t reg : arg_regs) {
			bc.add_operand(reg);
		}

		emit(bc);
	} else {
		// Indirect call (callable in register) - use OP_CALL
		GDScript2BytecodeInstr bc(GDScript2Opcode::OP_CALL);
		bc.add_operand(dest);

		// First arg is callee register
		if (p_instr.args.size() >= 1) {
			int32_t callee_reg = operand_to_reg(p_instr.args[0]);
			bc.add_operand(callee_reg);
		}

		// Load arguments into registers and collect their indices
		LocalVector<int32_t> arg_regs;
		int32_t temp_base = current_ir_func ? current_ir_func->next_reg : 0;

		for (uint32_t i = 1; i < p_instr.args.size(); i++) {
			int32_t arg_reg = emit_load_operand(p_instr.args[i], temp_base + (i - 1));
			arg_regs.push_back(arg_reg);
		}

		// Arg count
		bc.add_operand(static_cast<int32_t>(arg_regs.size()));

		// Arguments
		for (int32_t reg : arg_regs) {
			bc.add_operand(reg);
		}

		emit(bc);
	}
}

void GDScript2CodeGenerator::gen_call_method(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);

	GDScript2BytecodeInstr bc(GDScript2Opcode::OP_CALL_METHOD);
	bc.add_operand(dest);

	// First arg is base object
	if (p_instr.args.size() >= 1) {
		int32_t base_reg = operand_to_reg(p_instr.args[0]);
		bc.add_operand(base_reg);
	}

	// Second arg is method name
	if (p_instr.args.size() >= 2 && p_instr.args[1].type == GDScript2IROperandType::NAME) {
		int name_idx = add_name(p_instr.args[1].name);
		bc.add_operand(name_idx);
	}

	// Arg count (excluding base and method name)
	int arg_count = p_instr.args.size() > 2 ? static_cast<int>(p_instr.args.size()) - 2 : 0;
	bc.add_operand(arg_count);

	// Arguments
	for (uint32_t i = 2; i < p_instr.args.size(); i++) {
		int32_t reg = operand_to_reg(p_instr.args[i]);
		bc.add_operand(reg);
	}

	emit(bc);
}

void GDScript2CodeGenerator::gen_call_builtin(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);

	GDScript2BytecodeInstr bc(GDScript2Opcode::OP_CALL_BUILTIN);
	bc.add_operand(dest);

	// First arg is function name
	if (p_instr.args.size() >= 1 && p_instr.args[0].type == GDScript2IROperandType::NAME) {
		int name_idx = add_name(p_instr.args[0].name);
		bc.add_operand(name_idx);
	}

	// Arg count (excluding function name)
	int arg_count = p_instr.args.size() > 1 ? static_cast<int>(p_instr.args.size()) - 1 : 0;
	bc.add_operand(arg_count);

	// Arguments
	for (uint32_t i = 1; i < p_instr.args.size(); i++) {
		int32_t reg = operand_to_reg(p_instr.args[i]);
		bc.add_operand(reg);
	}

	emit(bc);
}

void GDScript2CodeGenerator::gen_call_super(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);

	GDScript2BytecodeInstr bc(GDScript2Opcode::OP_CALL_SUPER);
	bc.add_operand(dest);

	// First arg is method name
	if (p_instr.args.size() >= 1 && p_instr.args[0].type == GDScript2IROperandType::NAME) {
		int name_idx = add_name(p_instr.args[0].name);
		bc.add_operand(name_idx);
	}

	// Arg count
	int arg_count = p_instr.args.size() > 1 ? static_cast<int>(p_instr.args.size()) - 1 : 0;
	bc.add_operand(arg_count);

	// Arguments
	for (uint32_t i = 1; i < p_instr.args.size(); i++) {
		int32_t reg = operand_to_reg(p_instr.args[i]);
		bc.add_operand(reg);
	}

	emit(bc);
}

void GDScript2CodeGenerator::gen_call_self(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);

	GDScript2BytecodeInstr bc(GDScript2Opcode::OP_CALL_SELF);
	bc.add_operand(dest);

	// First arg is method name
	if (p_instr.args.size() >= 1 && p_instr.args[0].type == GDScript2IROperandType::NAME) {
		int name_idx = add_name(p_instr.args[0].name);
		bc.add_operand(name_idx);
	}

	// Arg count
	int arg_count = p_instr.args.size() > 1 ? static_cast<int>(p_instr.args.size()) - 1 : 0;
	bc.add_operand(arg_count);

	// Arguments
	for (uint32_t i = 1; i < p_instr.args.size(); i++) {
		int32_t reg = operand_to_reg(p_instr.args[i]);
		bc.add_operand(reg);
	}

	emit(bc);
}

void GDScript2CodeGenerator::gen_return(const GDScript2IRInstr &p_instr) {
	int32_t ret_reg = -1;

	if (p_instr.args.size() >= 1) {
		ret_reg = operand_to_reg(p_instr.args[0]);
		if (ret_reg < 0) {
			int32_t temp = current_ir_func ? current_ir_func->next_reg : 0;
			ret_reg = emit_load_operand(p_instr.args[0], temp);
		}
	}

	emit_op(GDScript2Opcode::OP_RETURN, ret_reg);
}

void GDScript2CodeGenerator::gen_return_void(const GDScript2IRInstr &p_instr) {
	emit_op(GDScript2Opcode::OP_RETURN_VOID);
}

void GDScript2CodeGenerator::gen_iter_begin(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);

	if (p_instr.args.size() >= 1) {
		int32_t container_reg = operand_to_reg(p_instr.args[0]);
		if (container_reg < 0) {
			int32_t temp = current_ir_func ? current_ir_func->next_reg : 0;
			container_reg = emit_load_operand(p_instr.args[0], temp);
		}
		emit_op(GDScript2Opcode::OP_ITER_BEGIN, dest, container_reg);
	}
}

void GDScript2CodeGenerator::gen_iter_next(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);

	if (p_instr.args.size() >= 1) {
		int32_t iter_reg = operand_to_reg(p_instr.args[0]);
		emit_op(GDScript2Opcode::OP_ITER_NEXT, dest, iter_reg);
	}

	// If there's a target block for "end of iteration", emit conditional jump
	if (p_instr.target_block >= 0) {
		GDScript2BytecodeInstr bc(GDScript2Opcode::OP_JUMP_IF_ITER_END);
		bc.add_operand(dest);
		bc.add_operand(0); // Placeholder

		int bc_idx = emit(bc);
		add_jump_patch(bc_idx, 1, p_instr.target_block);
	}
}

void GDScript2CodeGenerator::gen_iter_get(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);

	if (p_instr.args.size() >= 1) {
		int32_t iter_reg = operand_to_reg(p_instr.args[0]);
		emit_op(GDScript2Opcode::OP_ITER_GET, dest, iter_reg);
	}
}

void GDScript2CodeGenerator::gen_await(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);

	if (p_instr.args.size() >= 1) {
		int32_t signal_reg = operand_to_reg(p_instr.args[0]);
		if (signal_reg < 0) {
			int32_t temp = current_ir_func ? current_ir_func->next_reg : 0;
			signal_reg = emit_load_operand(p_instr.args[0], temp);
		}
		emit_op(GDScript2Opcode::OP_AWAIT, dest, signal_reg);
	}
}

void GDScript2CodeGenerator::gen_preload(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);

	if (p_instr.args.size() >= 1) {
		// Path is typically an immediate string
		int const_idx = -1;
		if (p_instr.args[0].type == GDScript2IROperandType::IMMEDIATE) {
			const_idx = add_constant(p_instr.args[0].imm_value);
		}
		emit_op(GDScript2Opcode::OP_PRELOAD, dest, const_idx);
	}
}

void GDScript2CodeGenerator::gen_get_node(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);

	if (p_instr.args.size() >= 1) {
		int32_t path_reg = operand_to_reg(p_instr.args[0]);
		if (path_reg < 0) {
			int32_t temp = current_ir_func ? current_ir_func->next_reg : 0;
			path_reg = emit_load_operand(p_instr.args[0], temp);
		}
		emit_op(GDScript2Opcode::OP_GET_NODE, dest, path_reg);
	}
}

void GDScript2CodeGenerator::gen_lambda(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);

	GDScript2BytecodeInstr bc(GDScript2Opcode::OP_LAMBDA);
	bc.add_operand(dest);

	// First arg is lambda function index
	if (p_instr.args.size() >= 1) {
		if (p_instr.args[0].type == GDScript2IROperandType::FUNC_REF) {
			bc.add_operand(p_instr.args[0].func_idx);
		} else if (p_instr.args[0].type == GDScript2IROperandType::IMMEDIATE) {
			bc.add_operand(p_instr.args[0].imm_value.operator int64_t());
		}
	}

	// Capture count
	int capture_count = p_instr.args.size() > 1 ? static_cast<int>(p_instr.args.size()) - 1 : 0;
	bc.add_operand(capture_count);

	// Capture registers
	for (uint32_t i = 1; i < p_instr.args.size(); i++) {
		int32_t reg = operand_to_reg(p_instr.args[i]);
		bc.add_operand(reg);
	}

	emit(bc);
}

void GDScript2CodeGenerator::gen_copy(const GDScript2IRInstr &p_instr) {
	int32_t dest = operand_to_reg(p_instr.dest);

	if (p_instr.args.size() >= 1) {
		int32_t src_reg = operand_to_reg(p_instr.args[0]);
		if (src_reg < 0) {
			int32_t temp = current_ir_func ? current_ir_func->next_reg : 0;
			src_reg = emit_load_operand(p_instr.args[0], temp);
		}
		emit_op(GDScript2Opcode::OP_COPY, dest, src_reg);
	}
}

void GDScript2CodeGenerator::gen_assert(const GDScript2IRInstr &p_instr) {
	GDScript2BytecodeInstr bc(GDScript2Opcode::OP_ASSERT);

	// Condition
	if (p_instr.args.size() >= 1) {
		int32_t cond_reg = operand_to_reg(p_instr.args[0]);
		bc.add_operand(cond_reg);
	}

	// Message (optional)
	if (p_instr.args.size() >= 2) {
		int32_t msg_reg = operand_to_reg(p_instr.args[1]);
		bc.add_operand(msg_reg);
	} else {
		bc.add_operand(-1); // No message
	}

	emit(bc);
}

void GDScript2CodeGenerator::gen_breakpoint(const GDScript2IRInstr &p_instr) {
	emit_op(GDScript2Opcode::OP_BREAKPOINT);
}

// ============================================================================
// Disassembly Utilities
// ============================================================================

String GDScript2CodeGenerator::disassemble(const GDScript2CompiledModule &p_module) {
	return p_module.to_string();
}

String GDScript2CodeGenerator::disassemble_function(const GDScript2CompiledFunction &p_func) {
	return p_func.to_string();
}
