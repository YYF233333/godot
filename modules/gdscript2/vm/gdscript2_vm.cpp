/**************************************************************************/
/*  gdscript2_vm.cpp                                                      */
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

#include "gdscript2_vm.h"

void GDScript2VM::load_module(const GDScript2CompiledModule *p_module) {
	module = p_module;
	call_stack.clear();
	call_depth = 0;
}

const GDScript2CompiledFunction *GDScript2VM::find_function(const StringName &p_name) const {
	if (module == nullptr) {
		return nullptr;
	}
	for (int i = 0; i < module->functions.size(); i++) {
		if (module->functions[i].name == p_name) {
			return &module->functions[i];
		}
	}
	return nullptr;
}

bool GDScript2VM::execute_instruction(GDScript2CallFrame &p_frame, GDScript2ExecutionResult &r_result) {
	if (p_frame.function == nullptr || p_frame.ip >= p_frame.function->code.size()) {
		r_result.status = GDScript2ExecutionResult::ERROR_RUNTIME;
		r_result.error_message = "Invalid instruction pointer.";
		return false;
	}

	const GDScript2BytecodeInstr &instr = p_frame.function->code[p_frame.ip];

	switch (instr.op) {
		case GDScript2Opcode::OP_NOP: {
			p_frame.ip++;
		} break;

		case GDScript2Opcode::OP_LOAD_CONST: {
			int dest = instr.arg0;
			int const_idx = instr.arg1;
			if (dest >= 0 && dest < p_frame.locals.size() &&
					const_idx >= 0 && const_idx < p_frame.function->constants.size()) {
				p_frame.locals.write[dest] = p_frame.function->constants[const_idx];
			}
			p_frame.ip++;
		} break;

		case GDScript2Opcode::OP_LOAD_LOCAL: {
			int dest = instr.arg0;
			int src = instr.arg1;
			if (dest >= 0 && dest < p_frame.locals.size() &&
					src >= 0 && src < p_frame.locals.size()) {
				p_frame.locals.write[dest] = p_frame.locals[src];
			}
			p_frame.ip++;
		} break;

		case GDScript2Opcode::OP_STORE_LOCAL: {
			int dest = instr.arg0;
			int src = instr.arg1;
			if (dest >= 0 && dest < p_frame.locals.size() &&
					src >= 0 && src < p_frame.locals.size()) {
				p_frame.locals.write[dest] = p_frame.locals[src];
			}
			p_frame.ip++;
		} break;

		case GDScript2Opcode::OP_ADD: {
			int dest = instr.arg0;
			int left = instr.arg1;
			int right = instr.arg2;
			if (dest >= 0 && dest < p_frame.locals.size() &&
					left >= 0 && left < p_frame.locals.size() &&
					right >= 0 && right < p_frame.locals.size()) {
				Variant result;
				bool valid = true;
				Variant::evaluate(Variant::OP_ADD, p_frame.locals[left], p_frame.locals[right], result, valid);
				p_frame.locals.write[dest] = result;
			}
			p_frame.ip++;
		} break;

		case GDScript2Opcode::OP_SUB: {
			int dest = instr.arg0;
			int left = instr.arg1;
			int right = instr.arg2;
			if (dest >= 0 && dest < p_frame.locals.size() &&
					left >= 0 && left < p_frame.locals.size() &&
					right >= 0 && right < p_frame.locals.size()) {
				Variant result;
				bool valid = true;
				Variant::evaluate(Variant::OP_SUBTRACT, p_frame.locals[left], p_frame.locals[right], result, valid);
				p_frame.locals.write[dest] = result;
			}
			p_frame.ip++;
		} break;

		case GDScript2Opcode::OP_MUL: {
			int dest = instr.arg0;
			int left = instr.arg1;
			int right = instr.arg2;
			if (dest >= 0 && dest < p_frame.locals.size() &&
					left >= 0 && left < p_frame.locals.size() &&
					right >= 0 && right < p_frame.locals.size()) {
				Variant result;
				bool valid = true;
				Variant::evaluate(Variant::OP_MULTIPLY, p_frame.locals[left], p_frame.locals[right], result, valid);
				p_frame.locals.write[dest] = result;
			}
			p_frame.ip++;
		} break;

		case GDScript2Opcode::OP_DIV: {
			int dest = instr.arg0;
			int left = instr.arg1;
			int right = instr.arg2;
			if (dest >= 0 && dest < p_frame.locals.size() &&
					left >= 0 && left < p_frame.locals.size() &&
					right >= 0 && right < p_frame.locals.size()) {
				Variant result;
				bool valid = true;
				Variant::evaluate(Variant::OP_DIVIDE, p_frame.locals[left], p_frame.locals[right], result, valid);
				p_frame.locals.write[dest] = result;
			}
			p_frame.ip++;
		} break;

		case GDScript2Opcode::OP_JUMP: {
			p_frame.ip = instr.arg0;
		} break;

		case GDScript2Opcode::OP_JUMP_IF: {
			int cond = instr.arg0;
			int target = instr.arg1;
			if (cond >= 0 && cond < p_frame.locals.size()) {
				if (p_frame.locals[cond].booleanize()) {
					p_frame.ip = target;
				} else {
					p_frame.ip++;
				}
			} else {
				p_frame.ip++;
			}
		} break;

		case GDScript2Opcode::OP_JUMP_IF_NOT: {
			int cond = instr.arg0;
			int target = instr.arg1;
			if (cond >= 0 && cond < p_frame.locals.size()) {
				if (!p_frame.locals[cond].booleanize()) {
					p_frame.ip = target;
				} else {
					p_frame.ip++;
				}
			} else {
				p_frame.ip++;
			}
		} break;

		case GDScript2Opcode::OP_RETURN: {
			int ret_reg = instr.arg0;
			if (ret_reg >= 0 && ret_reg < p_frame.locals.size()) {
				r_result.return_value = p_frame.locals[ret_reg];
			}
			return false; // Stop execution.
		} break;

		case GDScript2Opcode::OP_END: {
			return false; // Stop execution.
		} break;

		default: {
			// Unknown opcode, skip.
			p_frame.ip++;
		} break;
	}

	return true;
}

GDScript2ExecutionResult GDScript2VM::call(const StringName &p_function, const Vector<Variant> &p_args) {
	GDScript2ExecutionResult result;

	if (module == nullptr) {
		result.status = GDScript2ExecutionResult::ERROR_RUNTIME;
		result.error_message = "No module loaded.";
		return result;
	}

	const GDScript2CompiledFunction *func = find_function(p_function);
	if (func == nullptr) {
		result.status = GDScript2ExecutionResult::ERROR_RUNTIME;
		result.error_message = "Function not found: " + String(p_function);
		return result;
	}

	if (call_depth >= MAX_CALL_DEPTH) {
		result.status = GDScript2ExecutionResult::ERROR_STACK_OVERFLOW;
		result.error_message = "Stack overflow.";
		return result;
	}

	// Setup call frame.
	GDScript2CallFrame frame;
	frame.function = func;
	frame.ip = 0;
	frame.locals.resize(func->local_count);

	// Copy arguments to locals.
	for (int i = 0; i < p_args.size() && i < func->param_count; i++) {
		frame.locals.write[i] = p_args[i];
	}

	call_stack.push_back(frame);
	call_depth++;

	// Execute until done.
	while (execute_instruction(call_stack.write[call_stack.size() - 1], result)) {
		// Continue.
	}

	call_stack.resize(call_stack.size() - 1);
	call_depth--;

	return result;
}

GDScript2ExecutionResult GDScript2VM::execute() {
	return call(StringName("_main"), Vector<Variant>());
}
