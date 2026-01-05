/**************************************************************************/
/*  gdscript2_ir_builder.cpp                                              */
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

#include "gdscript2_ir_builder.h"

// ============================================================================
// Error Handling
// ============================================================================

void GDScript2IRBuilder::push_error(const String &p_message, GDScript2ASTNode *p_node) {
	String msg = p_message;
	if (p_node) {
		msg = vformat("[%d:%d] %s", p_node->start_line, p_node->start_column, p_message);
	}
	errors.push_back(msg);
}

// ============================================================================
// Block Management
// ============================================================================

int GDScript2IRBuilder::create_block(const StringName &p_label) {
	if (!current_func) {
		return -1;
	}
	int id = current_func->create_block();
	if (p_label != StringName()) {
		current_func->blocks[id].label = p_label;
	}
	return id;
}

void GDScript2IRBuilder::set_block(int p_block) {
	current_block = p_block;
}

GDScript2IRBlock *GDScript2IRBuilder::get_current_block() {
	if (!current_func || current_block < 0) {
		return nullptr;
	}
	return current_func->get_block(current_block);
}

bool GDScript2IRBuilder::current_block_has_terminator() {
	GDScript2IRBlock *block = get_current_block();
	return block && block->has_terminator();
}

// ============================================================================
// Instruction Emission
// ============================================================================

void GDScript2IRBuilder::emit(const GDScript2IRInstr &p_instr) {
	GDScript2IRBlock *block = get_current_block();
	if (!block) {
		return;
	}
	block->instructions.push_back(p_instr);
}

void GDScript2IRBuilder::emit_jump(int p_target) {
	if (current_block_has_terminator()) {
		return;
	}

	GDScript2IRInstr instr = GDScript2IRInstr::make_jump(p_target);
	emit(instr);

	GDScript2IRBlock *block = get_current_block();
	if (block) {
		block->add_successor(p_target);
	}
}

void GDScript2IRBuilder::emit_jump_if(GDScript2IROperand p_cond, int p_true_block, int p_false_block) {
	if (current_block_has_terminator()) {
		return;
	}

	GDScript2IRInstr instr = GDScript2IRInstr::make_jump_if(p_cond, p_true_block, p_false_block);
	emit(instr);

	GDScript2IRBlock *block = get_current_block();
	if (block) {
		block->add_successor(p_true_block);
		block->add_successor(p_false_block);
	}
}

void GDScript2IRBuilder::emit_return(GDScript2IROperand p_value) {
	if (current_block_has_terminator()) {
		return;
	}
	emit(GDScript2IRInstr::make_return(p_value));
}

void GDScript2IRBuilder::emit_return_void() {
	if (current_block_has_terminator()) {
		return;
	}
	emit(GDScript2IRInstr::make_return_void());
}

// ============================================================================
// Main Entry Points
// ============================================================================

GDScript2IRBuilder::Result GDScript2IRBuilder::build(GDScript2ASTNode *p_root) {
	Result result;

	if (!p_root) {
		result.errors.push_back("AST root is null.");
		return result;
	}

	module = &result.module;
	errors.clear();

	if (p_root->type == GDScript2ASTNodeType::NODE_CLASS) {
		build_class(static_cast<GDScript2ClassNode *>(p_root));
	} else {
		push_error("Expected class node as root.", p_root);
	}

	result.errors = errors;
	module = nullptr;
	return result;
}

GDScript2IRBuilder::Result GDScript2IRBuilder::build_with_semantic(GDScript2ASTNode *p_root, const GDScript2SemanticAnalyzer::Result &p_semantic) {
	semantic_result = &p_semantic;
	Result result = build(p_root);
	semantic_result = nullptr;
	return result;
}

// ============================================================================
// Class Building
// ============================================================================

void GDScript2IRBuilder::build_class(GDScript2ClassNode *p_class) {
	if (!p_class || !module) {
		return;
	}

	// Set module name
	if (p_class->class_name != StringName()) {
		module->name = p_class->class_name;
	}

	// Build class members (variables become globals in the module)
	build_class_members(p_class);

	// Build functions
	for (GDScript2FunctionNode *func : p_class->functions) {
		build_function(func);
	}

	// Build inner classes (recursive)
	for (GDScript2ClassNode *inner : p_class->inner_classes) {
		// TODO: Handle inner classes properly
		(void)inner;
	}
}

void GDScript2IRBuilder::build_class_members(GDScript2ClassNode *p_class) {
	// Register member variables as globals
	for (GDScript2VariableNode *var : p_class->variables) {
		module->add_global(var->name);
	}

	// Register constants
	for (GDScript2ConstantNode *cons : p_class->constants) {
		module->add_global(cons->name);
	}
}

// ============================================================================
// Function Building
// ============================================================================

void GDScript2IRBuilder::build_function(GDScript2FunctionNode *p_func) {
	if (!p_func || !module) {
		return;
	}

	// Create function
	int func_idx = module->add_function(p_func->name);
	current_func = module->get_function(func_idx);
	current_func->is_static = p_func->is_static;
	current_func->source_line = p_func->start_line;

	// Register parameters as locals
	for (GDScript2ParameterNode *param : p_func->parameters) {
		current_func->add_local(param->name, true);
	}

	// Create entry block
	int entry = create_block("entry");
	current_func->entry_block = entry;
	set_block(entry);

	// Build function body
	build_function_body(p_func);

	// Add implicit return if needed
	if (!current_block_has_terminator()) {
		emit_return_void();
	}

	// Compute CFG info
	current_func->compute_predecessors();
	current_func->compute_reachability();

	current_func = nullptr;
	current_block = -1;
}

void GDScript2IRBuilder::build_function_body(GDScript2FunctionNode *p_func) {
	if (p_func->body) {
		build_suite(p_func->body);
	}
}

// ============================================================================
// Statement Building
// ============================================================================

void GDScript2IRBuilder::build_suite(GDScript2SuiteNode *p_suite) {
	if (!p_suite) {
		return;
	}

	for (GDScript2ASTNode *stmt : p_suite->statements) {
		build_statement(stmt);
	}
}

void GDScript2IRBuilder::build_statement(GDScript2ASTNode *p_stmt) {
	if (!p_stmt) {
		return;
	}

	switch (p_stmt->type) {
		case GDScript2ASTNodeType::NODE_VARIABLE:
			build_variable_decl(static_cast<GDScript2VariableNode *>(p_stmt));
			break;
		case GDScript2ASTNodeType::NODE_IF:
			build_if(static_cast<GDScript2IfNode *>(p_stmt));
			break;
		case GDScript2ASTNodeType::NODE_FOR:
			build_for(static_cast<GDScript2ForNode *>(p_stmt));
			break;
		case GDScript2ASTNodeType::NODE_WHILE:
			build_while(static_cast<GDScript2WhileNode *>(p_stmt));
			break;
		case GDScript2ASTNodeType::NODE_MATCH:
			build_match(static_cast<GDScript2MatchNode *>(p_stmt));
			break;
		case GDScript2ASTNodeType::NODE_RETURN:
			build_return(static_cast<GDScript2ReturnNode *>(p_stmt));
			break;
		case GDScript2ASTNodeType::NODE_BREAK:
			build_break(static_cast<GDScript2BreakNode *>(p_stmt));
			break;
		case GDScript2ASTNodeType::NODE_CONTINUE:
			build_continue(static_cast<GDScript2ContinueNode *>(p_stmt));
			break;
		case GDScript2ASTNodeType::NODE_ASSERT:
			build_assert(static_cast<GDScript2AssertNode *>(p_stmt));
			break;
		case GDScript2ASTNodeType::NODE_PASS:
		case GDScript2ASTNodeType::NODE_BREAKPOINT:
			// NOP or emit breakpoint
			if (p_stmt->type == GDScript2ASTNodeType::NODE_BREAKPOINT) {
				emit(GDScript2IRInstr(GDScript2IROp::OP_BREAKPOINT));
			}
			break;
		default:
			// Expression statement
			build_expression(p_stmt);
			break;
	}
}

void GDScript2IRBuilder::build_variable_decl(GDScript2VariableNode *p_var) {
	if (!p_var || !current_func) {
		return;
	}

	// Add local variable
	int local_idx = current_func->add_local(p_var->name);

	// Initialize if there's an initializer
	if (p_var->initializer) {
		GDScript2IROperand value = build_expression(p_var->initializer);
		emit(GDScript2IRInstr::make_store_local(local_idx, value));
	}
}

void GDScript2IRBuilder::build_if(GDScript2IfNode *p_if) {
	if (!p_if) {
		return;
	}

	// Create blocks
	int then_block = create_block("if.then");
	int else_block = p_if->false_block ? create_block("if.else") : -1;
	int merge_block = create_block("if.end");

	// Build condition
	GDScript2IROperand cond = build_expression(p_if->condition);

	// Emit conditional jump
	int false_target = else_block >= 0 ? else_block : merge_block;
	emit_jump_if(cond, then_block, false_target);

	// Build then block
	set_block(then_block);
	if (p_if->true_block) {
		build_suite(p_if->true_block);
	}
	if (!current_block_has_terminator()) {
		emit_jump(merge_block);
	}

	// Build else block
	if (p_if->false_block) {
		set_block(else_block);
		if (p_if->false_block->type == GDScript2ASTNodeType::NODE_IF) {
			build_if(static_cast<GDScript2IfNode *>(p_if->false_block));
		} else if (p_if->false_block->type == GDScript2ASTNodeType::NODE_SUITE) {
			build_suite(static_cast<GDScript2SuiteNode *>(p_if->false_block));
		}
		if (!current_block_has_terminator()) {
			emit_jump(merge_block);
		}
	}

	// Continue in merge block
	set_block(merge_block);
}

void GDScript2IRBuilder::build_for(GDScript2ForNode *p_for) {
	if (!p_for || !current_func) {
		return;
	}

	// Add loop variable as local
	int var_idx = current_func->add_local(p_for->variable);

	// Create blocks
	int init_block = create_block("for.init");
	int cond_block = create_block("for.cond");
	int body_block = create_block("for.body");
	int end_block = create_block("for.end");

	// Push loop context
	push_loop(cond_block, end_block);

	// Jump to init
	emit_jump(init_block);
	set_block(init_block);

	// Build iterable and create iterator
	GDScript2IROperand iterable = build_expression(p_for->iterable);
	GDScript2IROperand iterator = current_func->alloc_temp();

	GDScript2IRInstr iter_begin(GDScript2IROp::OP_ITER_BEGIN);
	iter_begin.dest = iterator;
	iter_begin.args.push_back(iterable);
	emit(iter_begin);

	emit_jump(cond_block);

	// Condition block: check if iterator has next
	set_block(cond_block);
	GDScript2IROperand has_next = current_func->alloc_temp();

	GDScript2IRInstr iter_next(GDScript2IROp::OP_ITER_NEXT);
	iter_next.dest = has_next;
	iter_next.args.push_back(iterator);
	emit(iter_next);

	emit_jump_if(has_next, body_block, end_block);

	// Body block
	set_block(body_block);

	// Get current value and store in loop variable
	GDScript2IROperand value = current_func->alloc_temp();
	GDScript2IRInstr iter_get(GDScript2IROp::OP_ITER_GET);
	iter_get.dest = value;
	iter_get.args.push_back(iterator);
	emit(iter_get);

	emit(GDScript2IRInstr::make_store_local(var_idx, value));

	// Build body
	if (p_for->body) {
		build_suite(p_for->body);
	}

	// Jump back to condition
	if (!current_block_has_terminator()) {
		emit_jump(cond_block);
	}

	// Pop loop context
	pop_loop();

	// Continue in end block
	set_block(end_block);
}

void GDScript2IRBuilder::build_while(GDScript2WhileNode *p_while) {
	if (!p_while) {
		return;
	}

	// Create blocks
	int cond_block = create_block("while.cond");
	int body_block = create_block("while.body");
	int end_block = create_block("while.end");

	// Push loop context
	push_loop(cond_block, end_block);

	// Jump to condition
	emit_jump(cond_block);

	// Condition block
	set_block(cond_block);
	GDScript2IROperand cond = build_expression(p_while->condition);
	emit_jump_if(cond, body_block, end_block);

	// Body block
	set_block(body_block);
	if (p_while->body) {
		build_suite(p_while->body);
	}
	if (!current_block_has_terminator()) {
		emit_jump(cond_block);
	}

	// Pop loop context
	pop_loop();

	// Continue in end block
	set_block(end_block);
}

void GDScript2IRBuilder::build_match(GDScript2MatchNode *p_match) {
	if (!p_match || !current_func) {
		return;
	}

	// Build test value
	GDScript2IROperand test_value = build_expression(p_match->test_value);

	int end_block = create_block("match.end");

	// Build each branch
	for (uint32_t i = 0; i < p_match->branches.size(); i++) {
		GDScript2MatchBranchNode *branch = p_match->branches[i];
		int branch_block = create_block(vformat("match.branch%d", i));
		int next_block = (i + 1 < p_match->branches.size()) ? create_block(vformat("match.check%d", i + 1)) : end_block;

		// Generate pattern check in current block
		// For now, simplified: just check first pattern as literal equality
		// TODO: Implement full pattern matching
		if (!branch->patterns.is_empty()) {
			GDScript2PatternNode *pattern = branch->patterns[0];
			if (pattern->pattern_type == GDScript2PatternType::PATTERN_LITERAL) {
				GDScript2IROperand pattern_val = current_func->alloc_temp();
				emit(GDScript2IRInstr::make_load_const(pattern_val, pattern->literal));

				GDScript2IROperand match_result = current_func->alloc_temp();
				emit(GDScript2IRInstr::make_binary(GDScript2IROp::OP_EQ, match_result, test_value, pattern_val));

				emit_jump_if(match_result, branch_block, next_block);
			} else if (pattern->pattern_type == GDScript2PatternType::PATTERN_EXPRESSION) {
				// Wildcard pattern (_) or other expression - always matches
				emit_jump(branch_block);
			} else {
				// For other pattern types, default to branch for now
				emit_jump(branch_block);
			}
		} else {
			// No patterns means always match (shouldn't happen, but safe fallback)
			emit_jump(branch_block);
		}

		// Branch body
		set_block(branch_block);
		if (branch->body) {
			build_suite(branch->body);
		}
		// Jump to end after branch execution
		if (!current_block_has_terminator()) {
			emit_jump(end_block);
		}

		// Move to next check block (or end_block if this was the last branch)
		if (i + 1 < p_match->branches.size()) {
			set_block(next_block);
		}
	}

	// Set current block to end
	set_block(end_block);
}

void GDScript2IRBuilder::build_return(GDScript2ReturnNode *p_return) {
	if (!p_return) {
		return;
	}

	if (p_return->value) {
		GDScript2IROperand value = build_expression(p_return->value);
		emit_return(value);
	} else {
		emit_return_void();
	}
}

void GDScript2IRBuilder::build_break(GDScript2BreakNode *p_break) {
	(void)p_break;
	LoopContext *loop = get_current_loop();
	if (loop && loop->break_block >= 0) {
		emit_jump(loop->break_block);
	}
}

void GDScript2IRBuilder::build_continue(GDScript2ContinueNode *p_continue) {
	(void)p_continue;
	LoopContext *loop = get_current_loop();
	if (loop && loop->continue_block >= 0) {
		emit_jump(loop->continue_block);
	}
}

void GDScript2IRBuilder::build_assert(GDScript2AssertNode *p_assert) {
	if (!p_assert) {
		return;
	}

	GDScript2IRInstr instr(GDScript2IROp::OP_ASSERT);

	if (p_assert->condition) {
		instr.args.push_back(build_expression(p_assert->condition));
	}

	if (p_assert->message) {
		instr.args.push_back(build_expression(p_assert->message));
	}

	emit(instr);
}

// ============================================================================
// Expression Building
// ============================================================================

GDScript2IROperand GDScript2IRBuilder::build_expression(GDScript2ASTNode *p_expr) {
	if (!p_expr) {
		return GDScript2IROperand::make_none();
	}

	switch (p_expr->type) {
		case GDScript2ASTNodeType::NODE_LITERAL:
			return build_literal(static_cast<GDScript2LiteralNode *>(p_expr));
		case GDScript2ASTNodeType::NODE_IDENTIFIER:
			return build_identifier(static_cast<GDScript2IdentifierNode *>(p_expr));
		case GDScript2ASTNodeType::NODE_SELF:
			return build_self(static_cast<GDScript2SelfNode *>(p_expr));
		case GDScript2ASTNodeType::NODE_BINARY_OP:
			return build_binary_op(static_cast<GDScript2BinaryOpNode *>(p_expr));
		case GDScript2ASTNodeType::NODE_UNARY_OP:
			return build_unary_op(static_cast<GDScript2UnaryOpNode *>(p_expr));
		case GDScript2ASTNodeType::NODE_TERNARY_OP:
			return build_ternary_op(static_cast<GDScript2TernaryOpNode *>(p_expr));
		case GDScript2ASTNodeType::NODE_ASSIGNMENT:
			return build_assignment(static_cast<GDScript2AssignmentNode *>(p_expr));
		case GDScript2ASTNodeType::NODE_CALL:
			return build_call(static_cast<GDScript2CallNode *>(p_expr));
		case GDScript2ASTNodeType::NODE_SUBSCRIPT:
			return build_subscript(static_cast<GDScript2SubscriptNode *>(p_expr));
		case GDScript2ASTNodeType::NODE_ATTRIBUTE:
			return build_attribute(static_cast<GDScript2AttributeNode *>(p_expr));
		case GDScript2ASTNodeType::NODE_ARRAY:
			return build_array(static_cast<GDScript2ArrayNode *>(p_expr));
		case GDScript2ASTNodeType::NODE_DICTIONARY:
			return build_dictionary(static_cast<GDScript2DictionaryNode *>(p_expr));
		case GDScript2ASTNodeType::NODE_LAMBDA:
			return build_lambda(static_cast<GDScript2LambdaNode *>(p_expr));
		case GDScript2ASTNodeType::NODE_AWAIT:
			return build_await(static_cast<GDScript2AwaitNode *>(p_expr));
		case GDScript2ASTNodeType::NODE_PRELOAD:
			return build_preload(static_cast<GDScript2PreloadNode *>(p_expr));
		case GDScript2ASTNodeType::NODE_CAST:
			return build_cast(static_cast<GDScript2CastNode *>(p_expr));
		case GDScript2ASTNodeType::NODE_TYPE_TEST:
			return build_type_test(static_cast<GDScript2TypeTestNode *>(p_expr));
		case GDScript2ASTNodeType::NODE_GET_NODE:
			return build_get_node(static_cast<GDScript2GetNodeNode *>(p_expr));
		default:
			push_error("Unknown expression type.", p_expr);
			return GDScript2IROperand::make_none();
	}
}

GDScript2IROperand GDScript2IRBuilder::build_literal(GDScript2LiteralNode *p_literal) {
	if (!current_func) {
		return GDScript2IROperand::make_none();
	}

	GDScript2IROperand dest = current_func->alloc_temp();
	emit(GDScript2IRInstr::make_load_const(dest, p_literal->value));
	return dest;
}

GDScript2IROperand GDScript2IRBuilder::build_identifier(GDScript2IdentifierNode *p_id) {
	if (!current_func) {
		return GDScript2IROperand::make_none();
	}

	// Check if it's a local variable
	int local_idx = current_func->get_local_index(p_id->name);
	if (local_idx >= 0) {
		GDScript2IROperand dest = current_func->alloc_temp();
		emit(GDScript2IRInstr::make_load_local(dest, local_idx));
		return dest;
	}

	// Check if it's a global
	if (module->has_global(p_id->name)) {
		GDScript2IROperand dest = current_func->alloc_temp();
		GDScript2IRInstr instr(GDScript2IROp::OP_LOAD_GLOBAL);
		instr.dest = dest;
		instr.args.push_back(GDScript2IROperand::make_name(p_id->name));
		emit(instr);
		return dest;
	}

	// Assume it's a built-in or external reference
	GDScript2IROperand dest = current_func->alloc_temp();
	GDScript2IRInstr instr(GDScript2IROp::OP_LOAD_GLOBAL);
	instr.dest = dest;
	instr.args.push_back(GDScript2IROperand::make_name(p_id->name));
	emit(instr);
	return dest;
}

GDScript2IROperand GDScript2IRBuilder::build_self(GDScript2SelfNode *p_self) {
	(void)p_self;
	if (!current_func) {
		return GDScript2IROperand::make_none();
	}

	GDScript2IROperand dest = current_func->alloc_temp();
	emit(GDScript2IRInstr(GDScript2IROp::OP_LOAD_SELF));
	return dest;
}

GDScript2IROperand GDScript2IRBuilder::build_binary_op(GDScript2BinaryOpNode *p_binary) {
	if (!current_func) {
		return GDScript2IROperand::make_none();
	}

	// Handle short-circuit evaluation for logical operators
	if (p_binary->op == GDScript2BinaryOp::OP_AND) {
		return build_short_circuit_and(p_binary);
	}
	if (p_binary->op == GDScript2BinaryOp::OP_OR) {
		return build_short_circuit_or(p_binary);
	}

	// Regular binary operation
	GDScript2IROperand left = build_expression(p_binary->left);
	GDScript2IROperand right = build_expression(p_binary->right);

	GDScript2IROp ir_op = binary_op_to_ir_op(p_binary->op);
	GDScript2IROperand dest = current_func->alloc_temp();

	emit(GDScript2IRInstr::make_binary(ir_op, dest, left, right));
	return dest;
}

GDScript2IROperand GDScript2IRBuilder::build_short_circuit_and(GDScript2BinaryOpNode *p_binary) {
	// left and right:
	// if (!left) result = false
	// else result = right

	GDScript2IROperand result = current_func->alloc_temp();

	int right_block = create_block("and.right");
	int merge_block = create_block("and.end");

	GDScript2IROperand left = build_expression(p_binary->left);
	emit_jump_if(left, right_block, merge_block);

	// False path: store false
	emit(GDScript2IRInstr::make_load_const(result, false));
	emit_jump(merge_block);

	// Right block: evaluate right
	set_block(right_block);
	GDScript2IROperand right = build_expression(p_binary->right);
	GDScript2IRInstr copy(GDScript2IROp::OP_COPY);
	copy.dest = result;
	copy.args.push_back(right);
	emit(copy);
	emit_jump(merge_block);

	set_block(merge_block);
	return result;
}

GDScript2IROperand GDScript2IRBuilder::build_short_circuit_or(GDScript2BinaryOpNode *p_binary) {
	// left or right:
	// if (left) result = true
	// else result = right

	GDScript2IROperand result = current_func->alloc_temp();

	int right_block = create_block("or.right");
	int merge_block = create_block("or.end");

	GDScript2IROperand left = build_expression(p_binary->left);
	emit_jump_if(left, merge_block, right_block);

	// True path: store true
	emit(GDScript2IRInstr::make_load_const(result, true));
	emit_jump(merge_block);

	// Right block: evaluate right
	set_block(right_block);
	GDScript2IROperand right = build_expression(p_binary->right);
	GDScript2IRInstr copy(GDScript2IROp::OP_COPY);
	copy.dest = result;
	copy.args.push_back(right);
	emit(copy);
	emit_jump(merge_block);

	set_block(merge_block);
	return result;
}

GDScript2IROperand GDScript2IRBuilder::build_unary_op(GDScript2UnaryOpNode *p_unary) {
	if (!current_func) {
		return GDScript2IROperand::make_none();
	}

	GDScript2IROperand operand = build_expression(p_unary->operand);
	GDScript2IROp ir_op = unary_op_to_ir_op(p_unary->op);
	GDScript2IROperand dest = current_func->alloc_temp();

	emit(GDScript2IRInstr::make_unary(ir_op, dest, operand));
	return dest;
}

GDScript2IROperand GDScript2IRBuilder::build_ternary_op(GDScript2TernaryOpNode *p_ternary) {
	if (!current_func) {
		return GDScript2IROperand::make_none();
	}

	GDScript2IROperand result = current_func->alloc_temp();

	int true_block = create_block("ternary.true");
	int false_block = create_block("ternary.false");
	int merge_block = create_block("ternary.end");

	GDScript2IROperand cond = build_expression(p_ternary->condition);
	emit_jump_if(cond, true_block, false_block);

	// True branch
	set_block(true_block);
	GDScript2IROperand true_val = build_expression(p_ternary->true_expr);
	GDScript2IRInstr copy_true(GDScript2IROp::OP_COPY);
	copy_true.dest = result;
	copy_true.args.push_back(true_val);
	emit(copy_true);
	emit_jump(merge_block);

	// False branch
	set_block(false_block);
	GDScript2IROperand false_val = build_expression(p_ternary->false_expr);
	GDScript2IRInstr copy_false(GDScript2IROp::OP_COPY);
	copy_false.dest = result;
	copy_false.args.push_back(false_val);
	emit(copy_false);
	emit_jump(merge_block);

	set_block(merge_block);
	return result;
}

GDScript2IROperand GDScript2IRBuilder::build_assignment(GDScript2AssignmentNode *p_assign) {
	if (!current_func) {
		return GDScript2IROperand::make_none();
	}

	GDScript2IROperand value = build_expression(p_assign->value);

	// Handle compound assignment
	if (p_assign->op != GDScript2AssignOp::OP_ASSIGN) {
		GDScript2IROperand current = build_expression(p_assign->target);
		GDScript2IROp ir_op = assign_op_to_binary_ir_op(p_assign->op);
		GDScript2IROperand result = current_func->alloc_temp();
		emit(GDScript2IRInstr::make_binary(ir_op, result, current, value));
		value = result;
	}

	// Store to target
	build_store(p_assign->target, value);

	return value;
}

void GDScript2IRBuilder::build_store(GDScript2ASTNode *p_target, GDScript2IROperand p_value) {
	if (!p_target || !current_func) {
		return;
	}

	switch (p_target->type) {
		case GDScript2ASTNodeType::NODE_IDENTIFIER: {
			GDScript2IdentifierNode *id = static_cast<GDScript2IdentifierNode *>(p_target);
			int local_idx = current_func->get_local_index(id->name);
			if (local_idx >= 0) {
				emit(GDScript2IRInstr::make_store_local(local_idx, p_value));
			} else {
				GDScript2IRInstr instr(GDScript2IROp::OP_STORE_GLOBAL);
				instr.args.push_back(GDScript2IROperand::make_name(id->name));
				instr.args.push_back(p_value);
				emit(instr);
			}
		} break;

		case GDScript2ASTNodeType::NODE_SUBSCRIPT: {
			GDScript2SubscriptNode *sub = static_cast<GDScript2SubscriptNode *>(p_target);
			GDScript2IROperand base = build_expression(sub->base);
			GDScript2IROperand index = build_expression(sub->index);

			GDScript2IRInstr instr(GDScript2IROp::OP_SET_INDEX);
			instr.args.push_back(base);
			instr.args.push_back(index);
			instr.args.push_back(p_value);
			emit(instr);
		} break;

		case GDScript2ASTNodeType::NODE_ATTRIBUTE: {
			GDScript2AttributeNode *attr = static_cast<GDScript2AttributeNode *>(p_target);
			GDScript2IROperand base = build_expression(attr->base);

			GDScript2IRInstr instr(GDScript2IROp::OP_SET_NAMED);
			instr.args.push_back(base);
			instr.args.push_back(GDScript2IROperand::make_name(attr->attribute));
			instr.args.push_back(p_value);
			emit(instr);
		} break;

		default:
			push_error("Invalid assignment target.", p_target);
			break;
	}
}

GDScript2IROperand GDScript2IRBuilder::build_call(GDScript2CallNode *p_call) {
	if (!current_func) {
		return GDScript2IROperand::make_none();
	}

	GDScript2IROperand dest = current_func->alloc_temp();

	// Build arguments
	LocalVector<GDScript2IROperand> args;
	for (GDScript2ASTNode *arg : p_call->arguments) {
		args.push_back(build_expression(arg));
	}

	// Determine call type
	if (p_call->callee->type == GDScript2ASTNodeType::NODE_ATTRIBUTE) {
		// Method call: base.method(args)
		GDScript2AttributeNode *attr = static_cast<GDScript2AttributeNode *>(p_call->callee);
		GDScript2IROperand base = build_expression(attr->base);

		GDScript2IRInstr instr(GDScript2IROp::OP_CALL_METHOD);
		instr.dest = dest;
		instr.args.push_back(base);
		instr.args.push_back(GDScript2IROperand::make_name(attr->attribute));
		for (const GDScript2IROperand &arg : args) {
			instr.args.push_back(arg);
		}
		emit(instr);
	} else if (p_call->callee->type == GDScript2ASTNodeType::NODE_IDENTIFIER) {
		// Direct call: func(args) or builtin(args)
		GDScript2IdentifierNode *id = static_cast<GDScript2IdentifierNode *>(p_call->callee);

		GDScript2IRInstr instr(GDScript2IROp::OP_CALL);
		instr.dest = dest;
		instr.args.push_back(GDScript2IROperand::make_name(id->name));
		for (const GDScript2IROperand &arg : args) {
			instr.args.push_back(arg);
		}
		emit(instr);
	} else {
		// Indirect call (callable)
		GDScript2IROperand callee = build_expression(p_call->callee);

		GDScript2IRInstr instr(GDScript2IROp::OP_CALL);
		instr.dest = dest;
		instr.args.push_back(callee);
		for (const GDScript2IROperand &arg : args) {
			instr.args.push_back(arg);
		}
		emit(instr);
	}

	return dest;
}

GDScript2IROperand GDScript2IRBuilder::build_subscript(GDScript2SubscriptNode *p_subscript) {
	if (!current_func) {
		return GDScript2IROperand::make_none();
	}

	GDScript2IROperand base = build_expression(p_subscript->base);
	GDScript2IROperand index = build_expression(p_subscript->index);
	GDScript2IROperand dest = current_func->alloc_temp();

	GDScript2IRInstr instr(GDScript2IROp::OP_GET_INDEX);
	instr.dest = dest;
	instr.args.push_back(base);
	instr.args.push_back(index);
	emit(instr);

	return dest;
}

GDScript2IROperand GDScript2IRBuilder::build_attribute(GDScript2AttributeNode *p_attr) {
	if (!current_func) {
		return GDScript2IROperand::make_none();
	}

	GDScript2IROperand base = build_expression(p_attr->base);
	GDScript2IROperand dest = current_func->alloc_temp();

	GDScript2IRInstr instr(GDScript2IROp::OP_GET_NAMED);
	instr.dest = dest;
	instr.args.push_back(base);
	instr.args.push_back(GDScript2IROperand::make_name(p_attr->attribute));
	emit(instr);

	return dest;
}

GDScript2IROperand GDScript2IRBuilder::build_array(GDScript2ArrayNode *p_array) {
	if (!current_func) {
		return GDScript2IROperand::make_none();
	}

	GDScript2IROperand dest = current_func->alloc_temp();

	GDScript2IRInstr instr(GDScript2IROp::OP_CONSTRUCT_ARRAY);
	instr.dest = dest;

	for (GDScript2ASTNode *elem : p_array->elements) {
		instr.args.push_back(build_expression(elem));
	}

	emit(instr);
	return dest;
}

GDScript2IROperand GDScript2IRBuilder::build_dictionary(GDScript2DictionaryNode *p_dict) {
	if (!current_func) {
		return GDScript2IROperand::make_none();
	}

	GDScript2IROperand dest = current_func->alloc_temp();

	GDScript2IRInstr instr(GDScript2IROp::OP_CONSTRUCT_DICT);
	instr.dest = dest;

	for (uint32_t i = 0; i < p_dict->keys.size(); i++) {
		instr.args.push_back(build_expression(p_dict->keys[i]));
		if (i < p_dict->values.size()) {
			instr.args.push_back(build_expression(p_dict->values[i]));
		}
	}

	emit(instr);
	return dest;
}

GDScript2IROperand GDScript2IRBuilder::build_lambda(GDScript2LambdaNode *p_lambda) {
	if (!current_func) {
		return GDScript2IROperand::make_none();
	}

	// TODO: Build lambda as a separate function and capture variables
	GDScript2IROperand dest = current_func->alloc_temp();

	GDScript2IRInstr instr(GDScript2IROp::OP_LAMBDA);
	instr.dest = dest;
	// Add lambda function index when implemented
	emit(instr);

	return dest;
}

GDScript2IROperand GDScript2IRBuilder::build_await(GDScript2AwaitNode *p_await) {
	if (!current_func) {
		return GDScript2IROperand::make_none();
	}

	GDScript2IROperand expr = build_expression(p_await->expression);
	GDScript2IROperand dest = current_func->alloc_temp();

	GDScript2IRInstr instr(GDScript2IROp::OP_AWAIT);
	instr.dest = dest;
	instr.args.push_back(expr);
	emit(instr);

	return dest;
}

GDScript2IROperand GDScript2IRBuilder::build_preload(GDScript2PreloadNode *p_preload) {
	if (!current_func) {
		return GDScript2IROperand::make_none();
	}

	GDScript2IROperand dest = current_func->alloc_temp();

	GDScript2IRInstr instr(GDScript2IROp::OP_PRELOAD);
	instr.dest = dest;
	instr.args.push_back(GDScript2IROperand::make_imm(p_preload->path));
	emit(instr);

	return dest;
}

GDScript2IROperand GDScript2IRBuilder::build_cast(GDScript2CastNode *p_cast) {
	if (!current_func) {
		return GDScript2IROperand::make_none();
	}

	GDScript2IROperand operand = build_expression(p_cast->operand);
	GDScript2IROperand dest = current_func->alloc_temp();

	GDScript2IRInstr instr(GDScript2IROp::OP_AS);
	instr.dest = dest;
	instr.args.push_back(operand);
	// TODO: Add type info
	emit(instr);

	return dest;
}

GDScript2IROperand GDScript2IRBuilder::build_type_test(GDScript2TypeTestNode *p_test) {
	if (!current_func) {
		return GDScript2IROperand::make_none();
	}

	GDScript2IROperand operand = build_expression(p_test->operand);
	GDScript2IROperand dest = current_func->alloc_temp();

	GDScript2IRInstr instr(GDScript2IROp::OP_IS);
	instr.dest = dest;
	instr.args.push_back(operand);
	// TODO: Add type info
	emit(instr);

	return dest;
}

GDScript2IROperand GDScript2IRBuilder::build_get_node(GDScript2GetNodeNode *p_get_node) {
	if (!current_func) {
		return GDScript2IROperand::make_none();
	}

	GDScript2IROperand dest = current_func->alloc_temp();

	GDScript2IRInstr instr(GDScript2IROp::OP_GET_NODE);
	instr.dest = dest;
	instr.args.push_back(GDScript2IROperand::make_imm(p_get_node->path));
	emit(instr);

	return dest;
}

// ============================================================================
// Loop Control
// ============================================================================

void GDScript2IRBuilder::push_loop(int p_continue_block, int p_break_block) {
	LoopContext ctx;
	ctx.continue_block = p_continue_block;
	ctx.break_block = p_break_block;
	loop_stack.push_back(ctx);
}

void GDScript2IRBuilder::pop_loop() {
	if (!loop_stack.is_empty()) {
		loop_stack.resize(loop_stack.size() - 1);
	}
}

GDScript2IRBuilder::LoopContext *GDScript2IRBuilder::get_current_loop() {
	if (loop_stack.is_empty()) {
		return nullptr;
	}
	return &loop_stack[loop_stack.size() - 1];
}

// ============================================================================
// Helper Methods
// ============================================================================

GDScript2IROp GDScript2IRBuilder::binary_op_to_ir_op(GDScript2BinaryOp p_op) {
	switch (p_op) {
		case GDScript2BinaryOp::OP_ADD:
			return GDScript2IROp::OP_ADD;
		case GDScript2BinaryOp::OP_SUB:
			return GDScript2IROp::OP_SUB;
		case GDScript2BinaryOp::OP_MUL:
			return GDScript2IROp::OP_MUL;
		case GDScript2BinaryOp::OP_DIV:
			return GDScript2IROp::OP_DIV;
		case GDScript2BinaryOp::OP_MOD:
			return GDScript2IROp::OP_MOD;
		case GDScript2BinaryOp::OP_POW:
			return GDScript2IROp::OP_POW;
		case GDScript2BinaryOp::OP_EQ:
			return GDScript2IROp::OP_EQ;
		case GDScript2BinaryOp::OP_NE:
			return GDScript2IROp::OP_NE;
		case GDScript2BinaryOp::OP_LT:
			return GDScript2IROp::OP_LT;
		case GDScript2BinaryOp::OP_LE:
			return GDScript2IROp::OP_LE;
		case GDScript2BinaryOp::OP_GT:
			return GDScript2IROp::OP_GT;
		case GDScript2BinaryOp::OP_GE:
			return GDScript2IROp::OP_GE;
		case GDScript2BinaryOp::OP_AND:
			return GDScript2IROp::OP_AND;
		case GDScript2BinaryOp::OP_OR:
			return GDScript2IROp::OP_OR;
		case GDScript2BinaryOp::OP_BIT_AND:
			return GDScript2IROp::OP_BIT_AND;
		case GDScript2BinaryOp::OP_BIT_OR:
			return GDScript2IROp::OP_BIT_OR;
		case GDScript2BinaryOp::OP_BIT_XOR:
			return GDScript2IROp::OP_BIT_XOR;
		case GDScript2BinaryOp::OP_BIT_LSH:
			return GDScript2IROp::OP_BIT_LSH;
		case GDScript2BinaryOp::OP_BIT_RSH:
			return GDScript2IROp::OP_BIT_RSH;
		case GDScript2BinaryOp::OP_IN:
			return GDScript2IROp::OP_IN;
		default:
			return GDScript2IROp::OP_NOP;
	}
}

GDScript2IROp GDScript2IRBuilder::unary_op_to_ir_op(GDScript2UnaryOp p_op) {
	switch (p_op) {
		case GDScript2UnaryOp::OP_NEG:
			return GDScript2IROp::OP_NEG;
		case GDScript2UnaryOp::OP_POS:
			return GDScript2IROp::OP_POS;
		case GDScript2UnaryOp::OP_NOT:
			return GDScript2IROp::OP_NOT;
		case GDScript2UnaryOp::OP_BIT_NOT:
			return GDScript2IROp::OP_BIT_NOT;
		default:
			return GDScript2IROp::OP_NOP;
	}
}

GDScript2IROp GDScript2IRBuilder::assign_op_to_binary_ir_op(GDScript2AssignOp p_op) {
	switch (p_op) {
		case GDScript2AssignOp::OP_ADD_ASSIGN:
			return GDScript2IROp::OP_ADD;
		case GDScript2AssignOp::OP_SUB_ASSIGN:
			return GDScript2IROp::OP_SUB;
		case GDScript2AssignOp::OP_MUL_ASSIGN:
			return GDScript2IROp::OP_MUL;
		case GDScript2AssignOp::OP_DIV_ASSIGN:
			return GDScript2IROp::OP_DIV;
		case GDScript2AssignOp::OP_MOD_ASSIGN:
			return GDScript2IROp::OP_MOD;
		case GDScript2AssignOp::OP_POW_ASSIGN:
			return GDScript2IROp::OP_POW;
		case GDScript2AssignOp::OP_LSH_ASSIGN:
			return GDScript2IROp::OP_BIT_LSH;
		case GDScript2AssignOp::OP_RSH_ASSIGN:
			return GDScript2IROp::OP_BIT_RSH;
		case GDScript2AssignOp::OP_AND_ASSIGN:
			return GDScript2IROp::OP_BIT_AND;
		case GDScript2AssignOp::OP_OR_ASSIGN:
			return GDScript2IROp::OP_BIT_OR;
		case GDScript2AssignOp::OP_XOR_ASSIGN:
			return GDScript2IROp::OP_BIT_XOR;
		default:
			return GDScript2IROp::OP_NOP;
	}
}
