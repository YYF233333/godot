/**************************************************************************/
/*  gdscript2_ir_pass.cpp                                                 */
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

#include "gdscript2_ir_pass.h"

// ============================================================================
// Pass Manager
// ============================================================================

bool GDScript2IRPassManager::run_all(GDScript2IRModule &p_module) {
	for (GDScript2IRPass *pass : passes) {
		if (verbose) {
			print_line(vformat("[IR] Running pass: %s", pass->get_name()));
		}

		if (!pass->run(p_module)) {
			if (verbose) {
				print_line(vformat("[IR] Pass %s failed!", pass->get_name()));
			}
			return false;
		}
	}
	return true;
}

void GDScript2IRPassManager::add_standard_passes() {
	// Standard optimization pipeline
	// 1. Constant Folding - 消除常量计算
	add_pass(memnew(GDScript2ConstFoldPass));
	// 2. Copy Propagation - 消除冗余拷贝，辅助其他优化
	add_pass(memnew(GDScript2CopyPropPass));
	// 3. Constant Folding again - 处理传播后的常量
	add_pass(memnew(GDScript2ConstFoldPass));
	// 4. Dead Code Elimination - 删除死代码
	add_pass(memnew(GDScript2DCEPass));
	// 5. Simplify CFG - 简化控制流
	add_pass(memnew(GDScript2SimplifyCFGPass));
}

// ============================================================================
// Constant Folding Pass
// ============================================================================

bool GDScript2ConstFoldPass::run(GDScript2IRModule &p_module) {
	for (GDScript2IRFunction &func : p_module.functions) {
		run_on_function(func);
	}
	return true;
}

bool GDScript2ConstFoldPass::run_on_function(GDScript2IRFunction &p_func) {
	bool changed = false;

	for (GDScript2IRBlock &block : p_func.blocks) {
		for (GDScript2IRInstr &instr : block.instructions) {
			if (fold_instruction(instr, p_func)) {
				changed = true;
			}
		}
	}

	return changed;
}

bool GDScript2ConstFoldPass::fold_instruction(GDScript2IRInstr &p_instr, GDScript2IRFunction &p_func) {
	// Only fold binary and unary operations
	switch (p_instr.op) {
		case GDScript2IROp::OP_ADD:
		case GDScript2IROp::OP_SUB:
		case GDScript2IROp::OP_MUL:
		case GDScript2IROp::OP_DIV:
		case GDScript2IROp::OP_MOD:
		case GDScript2IROp::OP_POW:
		case GDScript2IROp::OP_EQ:
		case GDScript2IROp::OP_NE:
		case GDScript2IROp::OP_LT:
		case GDScript2IROp::OP_LE:
		case GDScript2IROp::OP_GT:
		case GDScript2IROp::OP_GE:
		case GDScript2IROp::OP_BIT_AND:
		case GDScript2IROp::OP_BIT_OR:
		case GDScript2IROp::OP_BIT_XOR:
		case GDScript2IROp::OP_BIT_LSH:
		case GDScript2IROp::OP_BIT_RSH: {
			if (p_instr.args.size() < 2) {
				return false;
			}

			Variant left, right;
			if (!get_constant_value(p_instr.args[0], p_func, left) ||
					!get_constant_value(p_instr.args[1], p_func, right)) {
				return false;
			}

			Variant result;
			if (try_fold_binary(p_instr.op, left, right, result)) {
				// Replace with load const
				p_instr.op = GDScript2IROp::OP_LOAD_CONST;
				p_instr.args.clear();
				p_instr.args.push_back(GDScript2IROperand::make_imm(result));
				return true;
			}
		} break;

		case GDScript2IROp::OP_NEG:
		case GDScript2IROp::OP_POS:
		case GDScript2IROp::OP_NOT:
		case GDScript2IROp::OP_BIT_NOT: {
			if (p_instr.args.is_empty()) {
				return false;
			}

			Variant operand;
			if (!get_constant_value(p_instr.args[0], p_func, operand)) {
				return false;
			}

			Variant result;
			if (try_fold_unary(p_instr.op, operand, result)) {
				p_instr.op = GDScript2IROp::OP_LOAD_CONST;
				p_instr.args.clear();
				p_instr.args.push_back(GDScript2IROperand::make_imm(result));
				return true;
			}
		} break;

		default:
			break;
	}

	return false;
}

bool GDScript2ConstFoldPass::try_fold_binary(GDScript2IROp p_op, const Variant &p_left, const Variant &p_right, Variant &r_result) {
	bool valid = false;
	Variant::Operator var_op;

	switch (p_op) {
		case GDScript2IROp::OP_ADD:
			var_op = Variant::OP_ADD;
			break;
		case GDScript2IROp::OP_SUB:
			var_op = Variant::OP_SUBTRACT;
			break;
		case GDScript2IROp::OP_MUL:
			var_op = Variant::OP_MULTIPLY;
			break;
		case GDScript2IROp::OP_DIV:
			var_op = Variant::OP_DIVIDE;
			break;
		case GDScript2IROp::OP_MOD:
			var_op = Variant::OP_MODULE;
			break;
		case GDScript2IROp::OP_POW:
			var_op = Variant::OP_POWER;
			break;
		case GDScript2IROp::OP_EQ:
			var_op = Variant::OP_EQUAL;
			break;
		case GDScript2IROp::OP_NE:
			var_op = Variant::OP_NOT_EQUAL;
			break;
		case GDScript2IROp::OP_LT:
			var_op = Variant::OP_LESS;
			break;
		case GDScript2IROp::OP_LE:
			var_op = Variant::OP_LESS_EQUAL;
			break;
		case GDScript2IROp::OP_GT:
			var_op = Variant::OP_GREATER;
			break;
		case GDScript2IROp::OP_GE:
			var_op = Variant::OP_GREATER_EQUAL;
			break;
		case GDScript2IROp::OP_BIT_AND:
			var_op = Variant::OP_BIT_AND;
			break;
		case GDScript2IROp::OP_BIT_OR:
			var_op = Variant::OP_BIT_OR;
			break;
		case GDScript2IROp::OP_BIT_XOR:
			var_op = Variant::OP_BIT_XOR;
			break;
		case GDScript2IROp::OP_BIT_LSH:
			var_op = Variant::OP_SHIFT_LEFT;
			break;
		case GDScript2IROp::OP_BIT_RSH:
			var_op = Variant::OP_SHIFT_RIGHT;
			break;
		default:
			return false;
	}

	Variant::evaluate(var_op, p_left, p_right, r_result, valid);
	return valid;
}

bool GDScript2ConstFoldPass::try_fold_unary(GDScript2IROp p_op, const Variant &p_operand, Variant &r_result) {
	bool valid = false;

	switch (p_op) {
		case GDScript2IROp::OP_NEG:
			Variant::evaluate(Variant::OP_NEGATE, p_operand, Variant(), r_result, valid);
			break;
		case GDScript2IROp::OP_POS:
			Variant::evaluate(Variant::OP_POSITIVE, p_operand, Variant(), r_result, valid);
			break;
		case GDScript2IROp::OP_NOT:
			r_result = !p_operand.booleanize();
			valid = true;
			break;
		case GDScript2IROp::OP_BIT_NOT:
			Variant::evaluate(Variant::OP_BIT_NEGATE, p_operand, Variant(), r_result, valid);
			break;
		default:
			return false;
	}

	return valid;
}

bool GDScript2ConstFoldPass::get_constant_value(const GDScript2IROperand &p_operand, const GDScript2IRFunction &p_func, Variant &r_value) {
	switch (p_operand.type) {
		case GDScript2IROperandType::IMMEDIATE:
			r_value = p_operand.imm_value;
			return true;

		case GDScript2IROperandType::CONSTANT:
			if (p_operand.const_idx >= 0 && p_operand.const_idx < (int)p_func.constants.size()) {
				r_value = p_func.constants[p_operand.const_idx];
				return true;
			}
			return false;

		default:
			return false;
	}
}

// ============================================================================
// Dead Code Elimination Pass
// ============================================================================

bool GDScript2DCEPass::run(GDScript2IRModule &p_module) {
	for (GDScript2IRFunction &func : p_module.functions) {
		run_on_function(func);
	}
	return true;
}

bool GDScript2DCEPass::run_on_function(GDScript2IRFunction &p_func) {
	bool changed = false;

	// First, compute reachability
	p_func.compute_reachability();

	// Mark instructions in unreachable blocks as dead
	for (GDScript2IRBlock &block : p_func.blocks) {
		if (!block.is_reachable) {
			for (GDScript2IRInstr &instr : block.instructions) {
				if (!instr.is_dead) {
					instr.is_dead = true;
					changed = true;
				}
			}
		}
	}

	// Find live registers (registers whose values are actually used)
	HashSet<int> live_regs;
	mark_live_values(p_func, live_regs);

	// Mark instructions that define unused registers as dead
	for (GDScript2IRBlock &block : p_func.blocks) {
		if (!block.is_reachable) {
			continue;
		}

		for (GDScript2IRInstr &instr : block.instructions) {
			if (instr.is_dead) {
				continue;
			}

			// Don't eliminate instructions with side effects
			switch (instr.op) {
				case GDScript2IROp::OP_STORE_LOCAL:
				case GDScript2IROp::OP_STORE_GLOBAL:
				case GDScript2IROp::OP_STORE_MEMBER:
				case GDScript2IROp::OP_SET_INDEX:
				case GDScript2IROp::OP_SET_NAMED:
				case GDScript2IROp::OP_CALL:
				case GDScript2IROp::OP_CALL_METHOD:
				case GDScript2IROp::OP_CALL_BUILTIN:
				case GDScript2IROp::OP_CALL_SELF:
				case GDScript2IROp::OP_CALL_SUPER:
				case GDScript2IROp::OP_RETURN:
				case GDScript2IROp::OP_RETURN_VOID:
				case GDScript2IROp::OP_JUMP:
				case GDScript2IROp::OP_JUMP_IF:
				case GDScript2IROp::OP_JUMP_IF_NOT:
				case GDScript2IROp::OP_AWAIT:
				case GDScript2IROp::OP_YIELD:
				case GDScript2IROp::OP_ASSERT:
				case GDScript2IROp::OP_BREAKPOINT:
					continue;
				default:
					break;
			}

			// Check if the destination register is used
			if (instr.dest.is_reg() && !live_regs.has(instr.dest.reg_id)) {
				instr.is_dead = true;
				changed = true;
			}
		}
	}

	return changed;
}

void GDScript2DCEPass::mark_live_values(GDScript2IRFunction &p_func, HashSet<int> &r_live_regs) {
	// Walk backwards from terminators, marking all used registers as live
	for (const GDScript2IRBlock &block : p_func.blocks) {
		if (!block.is_reachable) {
			continue;
		}

		for (const GDScript2IRInstr &instr : block.instructions) {
			if (instr.is_dead) {
				continue;
			}

			// Mark all register operands as live
			for (const GDScript2IROperand &arg : instr.args) {
				if (arg.is_reg()) {
					r_live_regs.insert(arg.reg_id);
				}
			}
		}
	}
}

bool GDScript2DCEPass::is_register_used(int p_reg, const GDScript2IRFunction &p_func) {
	for (const GDScript2IRBlock &block : p_func.blocks) {
		for (const GDScript2IRInstr &instr : block.instructions) {
			if (instr.is_dead) {
				continue;
			}
			for (const GDScript2IROperand &arg : instr.args) {
				if (arg.is_reg() && arg.reg_id == p_reg) {
					return true;
				}
			}
		}
	}
	return false;
}

// ============================================================================
// Copy Propagation Pass
// ============================================================================

bool GDScript2CopyPropPass::run(GDScript2IRModule &p_module) {
	for (GDScript2IRFunction &func : p_module.functions) {
		run_on_function(func);
	}
	return true;
}

bool GDScript2CopyPropPass::run_on_function(GDScript2IRFunction &p_func) {
	bool changed = false;
	copy_map.clear();

	// Build copy map from COPY and LOAD_CONST instructions
	for (GDScript2IRBlock &block : p_func.blocks) {
		for (GDScript2IRInstr &instr : block.instructions) {
			if (instr.is_dead) {
				continue;
			}

			if (instr.op == GDScript2IROp::OP_COPY && instr.dest.is_reg() && !instr.args.is_empty()) {
				copy_map.insert(instr.dest.reg_id, instr.args[0]);
			} else if (instr.op == GDScript2IROp::OP_LOAD_CONST && instr.dest.is_reg() && !instr.args.is_empty()) {
				// Treat load const as a copy from the immediate value
				copy_map.insert(instr.dest.reg_id, instr.args[0]);
			}
		}
	}

	// Propagate copies in all instructions
	for (GDScript2IRBlock &block : p_func.blocks) {
		for (GDScript2IRInstr &instr : block.instructions) {
			if (instr.is_dead) {
				continue;
			}

			// Don't propagate into COPY destination definitions
			if (instr.op == GDScript2IROp::OP_COPY) {
				continue;
			}

			// Propagate operands
			for (GDScript2IROperand &arg : instr.args) {
				if (propagate_operand(arg)) {
					changed = true;
				}
			}
		}
	}

	return changed;
}

GDScript2IROperand GDScript2CopyPropPass::get_propagated_value(const GDScript2IROperand &p_operand) {
	if (!p_operand.is_reg()) {
		return p_operand;
	}

	// Follow copy chain
	GDScript2IROperand current = p_operand;
	HashSet<int> visited; // Prevent infinite loops

	while (current.is_reg()) {
		if (visited.has(current.reg_id)) {
			break; // Cycle detected
		}
		visited.insert(current.reg_id);

		const GDScript2IROperand *source = copy_map.getptr(current.reg_id);
		if (!source) {
			break; // No more copies to follow
		}
		current = *source;
	}

	return current;
}

bool GDScript2CopyPropPass::propagate_operand(GDScript2IROperand &p_operand) {
	if (!p_operand.is_reg()) {
		return false;
	}

	GDScript2IROperand propagated = get_propagated_value(p_operand);
	if (propagated.type != p_operand.type || propagated.reg_id != p_operand.reg_id) {
		p_operand = propagated;
		return true;
	}

	return false;
}

// ============================================================================
// Simplify CFG Pass
// ============================================================================

bool GDScript2SimplifyCFGPass::run(GDScript2IRModule &p_module) {
	for (GDScript2IRFunction &func : p_module.functions) {
		run_on_function(func);
	}
	return true;
}

bool GDScript2SimplifyCFGPass::run_on_function(GDScript2IRFunction &p_func) {
	bool changed = false;

	// Recompute CFG info
	p_func.compute_predecessors();
	p_func.compute_reachability();

	// Apply simplifications
	if (remove_unreachable_blocks(p_func)) {
		changed = true;
	}

	if (remove_empty_blocks(p_func)) {
		changed = true;
	}

	// Recompute after changes
	if (changed) {
		p_func.compute_predecessors();
		p_func.compute_reachability();
	}

	return changed;
}

bool GDScript2SimplifyCFGPass::remove_unreachable_blocks(GDScript2IRFunction &p_func) {
	bool changed = false;

	for (GDScript2IRBlock &block : p_func.blocks) {
		if (!block.is_reachable && block.id != p_func.entry_block) {
			// Mark all instructions as dead
			for (GDScript2IRInstr &instr : block.instructions) {
				if (!instr.is_dead) {
					instr.is_dead = true;
					changed = true;
				}
			}
			block.successors.clear();
			block.predecessors.clear();
		}
	}

	return changed;
}

bool GDScript2SimplifyCFGPass::merge_blocks(GDScript2IRFunction &p_func) {
	// TODO: Implement block merging
	// Merge block A into B if A has single successor B and B has single predecessor A
	return false;
}

bool GDScript2SimplifyCFGPass::remove_empty_blocks(GDScript2IRFunction &p_func) {
	bool changed = false;

	for (GDScript2IRBlock &block : p_func.blocks) {
		if (!block.is_reachable || block.id == p_func.entry_block) {
			continue;
		}

		// Check if block only contains a single unconditional jump
		int live_count = 0;
		GDScript2IRInstr *only_instr = nullptr;

		for (GDScript2IRInstr &instr : block.instructions) {
			if (!instr.is_dead) {
				live_count++;
				only_instr = &instr;
			}
		}

		if (live_count == 1 && only_instr && only_instr->op == GDScript2IROp::OP_JUMP) {
			// This block just jumps somewhere - update predecessors to jump directly
			int target = only_instr->target_block;

			for (int pred_id : block.predecessors) {
				if (pred_id < 0 || pred_id >= (int)p_func.blocks.size()) {
					continue;
				}

				GDScript2IRBlock &pred = p_func.blocks[pred_id];
				GDScript2IRInstr *term = pred.get_terminator();
				if (!term) {
					continue;
				}

				// Update jump targets
				if (term->target_block == block.id) {
					term->target_block = target;
					changed = true;
				}
				if (term->else_block == block.id) {
					term->else_block = target;
					changed = true;
				}
			}

			if (changed) {
				// Mark this block's instruction as dead
				only_instr->is_dead = true;
			}
		}
	}

	return changed;
}
