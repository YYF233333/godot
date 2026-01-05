/**************************************************************************/
/*  gdscript2_ir_pass.h                                                   */
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

#include "gdscript2_ir.h"

#include "core/templates/hash_map.h"
#include "core/templates/hash_set.h"

// ============================================================================
// Abstract IR Pass Base
// ============================================================================

class GDScript2IRPass {
public:
	virtual ~GDScript2IRPass() = default;
	virtual const char *get_name() const = 0;
	virtual bool run(GDScript2IRModule &p_module) = 0;

protected:
	// Helper: run pass on a single function
	virtual bool run_on_function(GDScript2IRFunction &p_func) { return true; }
};

// ============================================================================
// Pass Manager
// ============================================================================

class GDScript2IRPassManager {
	LocalVector<GDScript2IRPass *> passes;
	bool verbose = false;

public:
	void add_pass(GDScript2IRPass *p_pass) { passes.push_back(p_pass); }

	void set_verbose(bool p_verbose) { verbose = p_verbose; }

	bool run_all(GDScript2IRModule &p_module);

	// Add standard optimization pipeline
	void add_standard_passes();

	~GDScript2IRPassManager() {
		for (GDScript2IRPass *pass : passes) {
			memdelete(pass);
		}
	}
};

// ============================================================================
// Constant Folding Pass
// ============================================================================

// Evaluates constant expressions at compile time
class GDScript2ConstFoldPass : public GDScript2IRPass {
public:
	const char *get_name() const override { return "ConstantFolding"; }
	bool run(GDScript2IRModule &p_module) override;

private:
	bool run_on_function(GDScript2IRFunction &p_func) override;
	bool fold_instruction(GDScript2IRInstr &p_instr, GDScript2IRFunction &p_func);

	// Try to fold binary operation
	bool try_fold_binary(GDScript2IROp p_op, const Variant &p_left, const Variant &p_right, Variant &r_result);

	// Try to fold unary operation
	bool try_fold_unary(GDScript2IROp p_op, const Variant &p_operand, Variant &r_result);

	// Get constant value for operand (if available)
	bool get_constant_value(const GDScript2IROperand &p_operand, const GDScript2IRFunction &p_func, Variant &r_value);
};

// ============================================================================
// Dead Code Elimination Pass
// ============================================================================

// Removes unreachable blocks and unused instructions
class GDScript2DCEPass : public GDScript2IRPass {
public:
	const char *get_name() const override { return "DeadCodeElimination"; }
	bool run(GDScript2IRModule &p_module) override;

private:
	bool run_on_function(GDScript2IRFunction &p_func) override;

	// Mark live registers by walking use-def chains
	void mark_live_values(GDScript2IRFunction &p_func, HashSet<int> &r_live_regs);

	// Check if a register is used in any instruction
	bool is_register_used(int p_reg, const GDScript2IRFunction &p_func);
};

// ============================================================================
// Copy Propagation Pass
// ============================================================================

// Eliminates redundant copy instructions by propagating values
class GDScript2CopyPropPass : public GDScript2IRPass {
public:
	const char *get_name() const override { return "CopyPropagation"; }
	bool run(GDScript2IRModule &p_module) override;

private:
	bool run_on_function(GDScript2IRFunction &p_func) override;

	// Map from register to its copy source
	HashMap<int, GDScript2IROperand> copy_map;

	// Get the ultimate source of a value (following copy chains)
	GDScript2IROperand get_propagated_value(const GDScript2IROperand &p_operand);

	// Replace operand with propagated value
	bool propagate_operand(GDScript2IROperand &p_operand);
};

// ============================================================================
// Simplify CFG Pass
// ============================================================================

// Simplifies the control flow graph by:
// - Removing empty blocks
// - Merging blocks with single predecessor/successor
// - Removing unreachable blocks
class GDScript2SimplifyCFGPass : public GDScript2IRPass {
public:
	const char *get_name() const override { return "SimplifyCFG"; }
	bool run(GDScript2IRModule &p_module) override;

private:
	bool run_on_function(GDScript2IRFunction &p_func) override;

	// Remove blocks that are not reachable
	bool remove_unreachable_blocks(GDScript2IRFunction &p_func);

	// Merge blocks that can be combined
	bool merge_blocks(GDScript2IRFunction &p_func);

	// Remove empty blocks (only contain jump to single successor)
	bool remove_empty_blocks(GDScript2IRFunction &p_func);
};

// ============================================================================
// Strength Reduction Pass
// ============================================================================

// Replaces expensive operations with cheaper equivalents
// e.g., x * 2 -> x + x, x * 4 -> x << 2
class GDScript2StrengthReductionPass : public GDScript2IRPass {
public:
	const char *get_name() const override { return "StrengthReduction"; }
	bool run(GDScript2IRModule &p_module) override;

private:
	bool run_on_function(GDScript2IRFunction &p_func) override;
	bool reduce_instruction(GDScript2IRInstr &p_instr, GDScript2IRFunction &p_func);
};
