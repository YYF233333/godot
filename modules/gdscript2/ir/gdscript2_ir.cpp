/**************************************************************************/
/*  gdscript2_ir.cpp                                                      */
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

#include "gdscript2_ir.h"

// ============================================================================
// Opcode Names
// ============================================================================

const char *gdscript2_ir_op_name(GDScript2IROp p_op) {
	switch (p_op) {
		case GDScript2IROp::OP_NOP:
			return "NOP";
		case GDScript2IROp::OP_COMMENT:
			return "COMMENT";
		case GDScript2IROp::OP_LOAD_CONST:
			return "LOAD_CONST";
		case GDScript2IROp::OP_LOAD_NIL:
			return "LOAD_NIL";
		case GDScript2IROp::OP_LOAD_LOCAL:
			return "LOAD_LOCAL";
		case GDScript2IROp::OP_STORE_LOCAL:
			return "STORE_LOCAL";
		case GDScript2IROp::OP_LOAD_MEMBER:
			return "LOAD_MEMBER";
		case GDScript2IROp::OP_STORE_MEMBER:
			return "STORE_MEMBER";
		case GDScript2IROp::OP_LOAD_GLOBAL:
			return "LOAD_GLOBAL";
		case GDScript2IROp::OP_STORE_GLOBAL:
			return "STORE_GLOBAL";
		case GDScript2IROp::OP_LOAD_SELF:
			return "LOAD_SELF";
		case GDScript2IROp::OP_ADD:
			return "ADD";
		case GDScript2IROp::OP_SUB:
			return "SUB";
		case GDScript2IROp::OP_MUL:
			return "MUL";
		case GDScript2IROp::OP_DIV:
			return "DIV";
		case GDScript2IROp::OP_MOD:
			return "MOD";
		case GDScript2IROp::OP_POW:
			return "POW";
		case GDScript2IROp::OP_NEG:
			return "NEG";
		case GDScript2IROp::OP_POS:
			return "POS";
		case GDScript2IROp::OP_BIT_AND:
			return "BIT_AND";
		case GDScript2IROp::OP_BIT_OR:
			return "BIT_OR";
		case GDScript2IROp::OP_BIT_XOR:
			return "BIT_XOR";
		case GDScript2IROp::OP_BIT_NOT:
			return "BIT_NOT";
		case GDScript2IROp::OP_BIT_LSH:
			return "BIT_LSH";
		case GDScript2IROp::OP_BIT_RSH:
			return "BIT_RSH";
		case GDScript2IROp::OP_EQ:
			return "EQ";
		case GDScript2IROp::OP_NE:
			return "NE";
		case GDScript2IROp::OP_LT:
			return "LT";
		case GDScript2IROp::OP_LE:
			return "LE";
		case GDScript2IROp::OP_GT:
			return "GT";
		case GDScript2IROp::OP_GE:
			return "GE";
		case GDScript2IROp::OP_NOT:
			return "NOT";
		case GDScript2IROp::OP_AND:
			return "AND";
		case GDScript2IROp::OP_OR:
			return "OR";
		case GDScript2IROp::OP_IS:
			return "IS";
		case GDScript2IROp::OP_AS:
			return "AS";
		case GDScript2IROp::OP_TYPEOF:
			return "TYPEOF";
		case GDScript2IROp::OP_IN:
			return "IN";
		case GDScript2IROp::OP_CONSTRUCT_ARRAY:
			return "CONSTRUCT_ARRAY";
		case GDScript2IROp::OP_CONSTRUCT_DICT:
			return "CONSTRUCT_DICT";
		case GDScript2IROp::OP_GET_INDEX:
			return "GET_INDEX";
		case GDScript2IROp::OP_SET_INDEX:
			return "SET_INDEX";
		case GDScript2IROp::OP_GET_NAMED:
			return "GET_NAMED";
		case GDScript2IROp::OP_SET_NAMED:
			return "SET_NAMED";
		case GDScript2IROp::OP_JUMP:
			return "JUMP";
		case GDScript2IROp::OP_JUMP_IF:
			return "JUMP_IF";
		case GDScript2IROp::OP_JUMP_IF_NOT:
			return "JUMP_IF_NOT";
		case GDScript2IROp::OP_CALL:
			return "CALL";
		case GDScript2IROp::OP_CALL_METHOD:
			return "CALL_METHOD";
		case GDScript2IROp::OP_CALL_BUILTIN:
			return "CALL_BUILTIN";
		case GDScript2IROp::OP_CALL_SUPER:
			return "CALL_SUPER";
		case GDScript2IROp::OP_CALL_SELF:
			return "CALL_SELF";
		case GDScript2IROp::OP_RETURN:
			return "RETURN";
		case GDScript2IROp::OP_RETURN_VOID:
			return "RETURN_VOID";
		case GDScript2IROp::OP_ITER_BEGIN:
			return "ITER_BEGIN";
		case GDScript2IROp::OP_ITER_NEXT:
			return "ITER_NEXT";
		case GDScript2IROp::OP_ITER_GET:
			return "ITER_GET";
		case GDScript2IROp::OP_AWAIT:
			return "AWAIT";
		case GDScript2IROp::OP_YIELD:
			return "YIELD";
		case GDScript2IROp::OP_PRELOAD:
			return "PRELOAD";
		case GDScript2IROp::OP_GET_NODE:
			return "GET_NODE";
		case GDScript2IROp::OP_LAMBDA:
			return "LAMBDA";
		case GDScript2IROp::OP_PHI:
			return "PHI";
		case GDScript2IROp::OP_COPY:
			return "COPY";
		case GDScript2IROp::OP_ASSERT:
			return "ASSERT";
		case GDScript2IROp::OP_BREAKPOINT:
			return "BREAKPOINT";
		default:
			return "UNKNOWN";
	}
}

// ============================================================================
// GDScript2IROperand
// ============================================================================

String GDScript2IROperand::to_string() const {
	switch (type) {
		case GDScript2IROperandType::NONE:
			return "_";
		case GDScript2IROperandType::REGISTER:
			return vformat("r%d", reg_id);
		case GDScript2IROperandType::CONSTANT:
			return vformat("c%d", const_idx);
		case GDScript2IROperandType::IMMEDIATE:
			return vformat("#%s", imm_value.stringify());
		case GDScript2IROperandType::BLOCK:
			return vformat("bb%d", block_id);
		case GDScript2IROperandType::NAME:
			return vformat("'%s'", String(name));
		case GDScript2IROperandType::LOCAL_VAR:
			return vformat("local[%d]", local_idx);
		case GDScript2IROperandType::FUNC_REF:
			return vformat("func[%d]", func_idx);
		default:
			return "?";
	}
}

// ============================================================================
// GDScript2IRInstr
// ============================================================================

GDScript2IRInstr GDScript2IRInstr::make_nop() {
	return GDScript2IRInstr(GDScript2IROp::OP_NOP);
}

GDScript2IRInstr GDScript2IRInstr::make_load_const(GDScript2IROperand p_dest, const Variant &p_value) {
	GDScript2IRInstr instr(GDScript2IROp::OP_LOAD_CONST);
	instr.dest = p_dest;
	instr.args.push_back(GDScript2IROperand::make_imm(p_value));
	return instr;
}

GDScript2IRInstr GDScript2IRInstr::make_load_local(GDScript2IROperand p_dest, int p_local_idx) {
	GDScript2IRInstr instr(GDScript2IROp::OP_LOAD_LOCAL);
	instr.dest = p_dest;
	instr.args.push_back(GDScript2IROperand::make_local(p_local_idx));
	return instr;
}

GDScript2IRInstr GDScript2IRInstr::make_store_local(int p_local_idx, GDScript2IROperand p_src) {
	GDScript2IRInstr instr(GDScript2IROp::OP_STORE_LOCAL);
	instr.args.push_back(GDScript2IROperand::make_local(p_local_idx));
	instr.args.push_back(p_src);
	return instr;
}

GDScript2IRInstr GDScript2IRInstr::make_binary(GDScript2IROp p_op, GDScript2IROperand p_dest, GDScript2IROperand p_left, GDScript2IROperand p_right) {
	GDScript2IRInstr instr(p_op);
	instr.dest = p_dest;
	instr.args.push_back(p_left);
	instr.args.push_back(p_right);
	return instr;
}

GDScript2IRInstr GDScript2IRInstr::make_unary(GDScript2IROp p_op, GDScript2IROperand p_dest, GDScript2IROperand p_operand) {
	GDScript2IRInstr instr(p_op);
	instr.dest = p_dest;
	instr.args.push_back(p_operand);
	return instr;
}

GDScript2IRInstr GDScript2IRInstr::make_jump(int p_target) {
	GDScript2IRInstr instr(GDScript2IROp::OP_JUMP);
	instr.target_block = p_target;
	return instr;
}

GDScript2IRInstr GDScript2IRInstr::make_jump_if(GDScript2IROperand p_cond, int p_true_block, int p_false_block) {
	GDScript2IRInstr instr(GDScript2IROp::OP_JUMP_IF);
	instr.args.push_back(p_cond);
	instr.target_block = p_true_block;
	instr.else_block = p_false_block;
	return instr;
}

GDScript2IRInstr GDScript2IRInstr::make_return(GDScript2IROperand p_value) {
	GDScript2IRInstr instr(GDScript2IROp::OP_RETURN);
	instr.args.push_back(p_value);
	return instr;
}

GDScript2IRInstr GDScript2IRInstr::make_return_void() {
	return GDScript2IRInstr(GDScript2IROp::OP_RETURN_VOID);
}

GDScript2IRInstr GDScript2IRInstr::make_call(GDScript2IROperand p_dest, GDScript2IROperand p_callee, const LocalVector<GDScript2IROperand> &p_args) {
	GDScript2IRInstr instr(GDScript2IROp::OP_CALL);
	instr.dest = p_dest;
	instr.args.push_back(p_callee);
	for (const GDScript2IROperand &arg : p_args) {
		instr.args.push_back(arg);
	}
	return instr;
}

String GDScript2IRInstr::to_string() const {
	String result;

	if (dest.is_valid()) {
		result += dest.to_string() + " = ";
	}

	result += gdscript2_ir_op_name(op);

	for (uint32_t i = 0; i < args.size(); i++) {
		result += (i == 0 ? " " : ", ") + args[i].to_string();
	}

	if (target_block >= 0) {
		result += vformat(" -> bb%d", target_block);
	}
	if (else_block >= 0) {
		result += vformat(", else bb%d", else_block);
	}

	return result;
}

// ============================================================================
// GDScript2IRBlock
// ============================================================================

void GDScript2IRBlock::add_successor(int p_block_id) {
	for (int succ : successors) {
		if (succ == p_block_id) {
			return; // Already added
		}
	}
	successors.push_back(p_block_id);
}

void GDScript2IRBlock::add_predecessor(int p_block_id) {
	for (int pred : predecessors) {
		if (pred == p_block_id) {
			return; // Already added
		}
	}
	predecessors.push_back(p_block_id);
}

bool GDScript2IRBlock::has_terminator() const {
	if (instructions.is_empty()) {
		return false;
	}
	const GDScript2IRInstr &last = instructions[instructions.size() - 1];
	switch (last.op) {
		case GDScript2IROp::OP_JUMP:
		case GDScript2IROp::OP_JUMP_IF:
		case GDScript2IROp::OP_JUMP_IF_NOT:
		case GDScript2IROp::OP_RETURN:
		case GDScript2IROp::OP_RETURN_VOID:
			return true;
		default:
			return false;
	}
}

GDScript2IRInstr *GDScript2IRBlock::get_terminator() {
	if (instructions.is_empty()) {
		return nullptr;
	}
	GDScript2IRInstr &last = instructions[instructions.size() - 1];
	switch (last.op) {
		case GDScript2IROp::OP_JUMP:
		case GDScript2IROp::OP_JUMP_IF:
		case GDScript2IROp::OP_JUMP_IF_NOT:
		case GDScript2IROp::OP_RETURN:
		case GDScript2IROp::OP_RETURN_VOID:
			return &last;
		default:
			return nullptr;
	}
}

const GDScript2IRInstr *GDScript2IRBlock::get_terminator() const {
	if (instructions.is_empty()) {
		return nullptr;
	}
	const GDScript2IRInstr &last = instructions[instructions.size() - 1];
	switch (last.op) {
		case GDScript2IROp::OP_JUMP:
		case GDScript2IROp::OP_JUMP_IF:
		case GDScript2IROp::OP_JUMP_IF_NOT:
		case GDScript2IROp::OP_RETURN:
		case GDScript2IROp::OP_RETURN_VOID:
			return &last;
		default:
			return nullptr;
	}
}

String GDScript2IRBlock::to_string() const {
	String result;
	result += vformat("bb%d", id);
	if (label != StringName()) {
		result += vformat(" (%s)", String(label));
	}
	result += ":\n";

	for (const GDScript2IRInstr &instr : instructions) {
		if (!instr.is_dead) {
			result += "  " + instr.to_string() + "\n";
		}
	}

	return result;
}

// ============================================================================
// GDScript2IRFunction
// ============================================================================

int GDScript2IRFunction::create_block() {
	int id = (int)blocks.size();
	blocks.push_back(GDScript2IRBlock(id));
	return id;
}

GDScript2IRBlock *GDScript2IRFunction::get_block(int p_id) {
	if (p_id < 0 || p_id >= (int)blocks.size()) {
		return nullptr;
	}
	return &blocks[p_id];
}

const GDScript2IRBlock *GDScript2IRFunction::get_block(int p_id) const {
	if (p_id < 0 || p_id >= (int)blocks.size()) {
		return nullptr;
	}
	return &blocks[p_id];
}

int GDScript2IRFunction::add_local(const StringName &p_name, bool p_is_param) {
	int idx = (int)locals.size();
	GDScript2IRLocal local(p_name, idx);
	local.is_parameter = p_is_param;
	if (p_is_param) {
		local.parameter_index = parameter_count++;
	}
	locals.push_back(local);
	local_map.insert(p_name, idx);
	return idx;
}

int GDScript2IRFunction::get_local_index(const StringName &p_name) const {
	const int *idx = local_map.getptr(p_name);
	return idx ? *idx : -1;
}

bool GDScript2IRFunction::has_local(const StringName &p_name) const {
	return local_map.has(p_name);
}

int GDScript2IRFunction::add_constant(const Variant &p_value) {
	const int *existing = constant_map.getptr(p_value);
	if (existing) {
		return *existing;
	}
	int idx = (int)constants.size();
	constants.push_back(p_value);
	constant_map.insert(p_value, idx);
	return idx;
}

const Variant &GDScript2IRFunction::get_constant(int p_index) const {
	static Variant null_variant;
	if (p_index < 0 || p_index >= (int)constants.size()) {
		return null_variant;
	}
	return constants[p_index];
}

void GDScript2IRFunction::compute_predecessors() {
	// Clear existing predecessors
	for (GDScript2IRBlock &block : blocks) {
		block.predecessors.clear();
	}

	// Compute from successors
	for (GDScript2IRBlock &block : blocks) {
		for (int succ_id : block.successors) {
			if (succ_id >= 0 && succ_id < (int)blocks.size()) {
				blocks[succ_id].add_predecessor(block.id);
			}
		}
	}
}

void GDScript2IRFunction::compute_reachability() {
	// Mark all unreachable first
	for (GDScript2IRBlock &block : blocks) {
		block.is_reachable = false;
	}

	// BFS from entry
	if (blocks.is_empty()) {
		return;
	}

	LocalVector<int> worklist;
	worklist.push_back(entry_block);
	blocks[entry_block].is_reachable = true;

	while (!worklist.is_empty()) {
		int current = worklist[worklist.size() - 1];
		worklist.resize(worklist.size() - 1);

		GDScript2IRBlock &block = blocks[current];
		for (int succ_id : block.successors) {
			if (succ_id >= 0 && succ_id < (int)blocks.size() && !blocks[succ_id].is_reachable) {
				blocks[succ_id].is_reachable = true;
				worklist.push_back(succ_id);
			}
		}
	}
}

String GDScript2IRFunction::to_string() const {
	String result;
	result += vformat("func %s", String(name));
	if (is_static) {
		result += " [static]";
	}
	if (is_coroutine) {
		result += " [coroutine]";
	}
	result += ":\n";

	// Print locals
	if (!locals.is_empty()) {
		result += "  ; locals:\n";
		for (const GDScript2IRLocal &local : locals) {
			result += vformat("  ;   %d: %s%s\n", local.index, String(local.name),
					local.is_parameter ? " (param)" : "");
		}
	}

	// Print blocks
	for (const GDScript2IRBlock &block : blocks) {
		result += block.to_string();
	}

	return result;
}

// ============================================================================
// GDScript2IRModule
// ============================================================================

int GDScript2IRModule::add_function(const StringName &p_name) {
	int idx = (int)functions.size();
	functions.push_back(GDScript2IRFunction(p_name));
	function_map.insert(p_name, idx);
	return idx;
}

GDScript2IRFunction *GDScript2IRModule::get_function(int p_index) {
	if (p_index < 0 || p_index >= (int)functions.size()) {
		return nullptr;
	}
	return &functions[p_index];
}

GDScript2IRFunction *GDScript2IRModule::get_function(const StringName &p_name) {
	const int *idx = function_map.getptr(p_name);
	return idx ? get_function(*idx) : nullptr;
}

const GDScript2IRFunction *GDScript2IRModule::get_function(int p_index) const {
	if (p_index < 0 || p_index >= (int)functions.size()) {
		return nullptr;
	}
	return &functions[p_index];
}

int GDScript2IRModule::get_function_index(const StringName &p_name) const {
	const int *idx = function_map.getptr(p_name);
	return idx ? *idx : -1;
}

bool GDScript2IRModule::has_function(const StringName &p_name) const {
	return function_map.has(p_name);
}

int GDScript2IRModule::add_global(const StringName &p_name) {
	if (global_map.has(p_name)) {
		return global_map[p_name];
	}
	int idx = (int)globals.size();
	globals.push_back(p_name);
	global_map.insert(p_name, idx);
	return idx;
}

int GDScript2IRModule::get_global_index(const StringName &p_name) const {
	const int *idx = global_map.getptr(p_name);
	return idx ? *idx : -1;
}

bool GDScript2IRModule::has_global(const StringName &p_name) const {
	return global_map.has(p_name);
}

String GDScript2IRModule::to_string() const {
	String result;
	result += vformat("; Module: %s\n", String(name));

	// Print globals
	if (!globals.is_empty()) {
		result += "; Globals:\n";
		for (uint32_t i = 0; i < globals.size(); i++) {
			result += vformat(";   %d: %s\n", i, String(globals[i]));
		}
	}

	// Print functions
	for (const GDScript2IRFunction &func : functions) {
		result += "\n" + func.to_string();
	}

	return result;
}
