/**************************************************************************/
/*  gdscript2_ir_builder.h                                                */
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
#include "gdscript2_ir.h"
#include "modules/gdscript2/front/gdscript2_ast.h"
#include "modules/gdscript2/semantic/gdscript2_semantic.h"

// IR Builder: converts AST to IR
class GDScript2IRBuilder : public RefCounted {
	GDCLASS(GDScript2IRBuilder, RefCounted);

public:
	struct Result {
		GDScript2IRModule module;
		LocalVector<String> errors;

		bool has_errors() const { return !errors.is_empty(); }
	};

protected:
	static void _bind_methods() {}

private:
	// Build context
	GDScript2IRModule *module = nullptr;
	GDScript2IRFunction *current_func = nullptr;
	int current_block = -1;

	// Semantic info (from semantic analysis)
	const GDScript2SemanticAnalyzer::Result *semantic_result = nullptr;

	// Error handling
	LocalVector<String> errors;
	void push_error(const String &p_message, GDScript2ASTNode *p_node = nullptr);

	// ========================================================================
	// Block Management
	// ========================================================================

	int create_block(const StringName &p_label = StringName());
	void set_block(int p_block);
	GDScript2IRBlock *get_current_block();
	bool current_block_has_terminator();

	// ========================================================================
	// Instruction Emission
	// ========================================================================

	void emit(const GDScript2IRInstr &p_instr);
	void emit_jump(int p_target);
	void emit_jump_if(GDScript2IROperand p_cond, int p_true_block, int p_false_block);
	void emit_return(GDScript2IROperand p_value);
	void emit_return_void();

	// ========================================================================
	// Class Building
	// ========================================================================

	void build_class(GDScript2ClassNode *p_class);
	void build_class_members(GDScript2ClassNode *p_class);

	// ========================================================================
	// Function Building
	// ========================================================================

	void build_function(GDScript2FunctionNode *p_func);
	void build_function_body(GDScript2FunctionNode *p_func);

	// ========================================================================
	// Statement Building
	// ========================================================================

	void build_suite(GDScript2SuiteNode *p_suite);
	void build_statement(GDScript2ASTNode *p_stmt);
	void build_variable_decl(GDScript2VariableNode *p_var);
	void build_if(GDScript2IfNode *p_if);
	void build_for(GDScript2ForNode *p_for);
	void build_while(GDScript2WhileNode *p_while);
	void build_match(GDScript2MatchNode *p_match);
	void build_return(GDScript2ReturnNode *p_return);
	void build_break(GDScript2BreakNode *p_break);
	void build_continue(GDScript2ContinueNode *p_continue);
	void build_assert(GDScript2AssertNode *p_assert);

	// ========================================================================
	// Expression Building
	// ========================================================================

	// Build expression and return the register holding the result
	GDScript2IROperand build_expression(GDScript2ASTNode *p_expr);
	GDScript2IROperand build_literal(GDScript2LiteralNode *p_literal);
	GDScript2IROperand build_identifier(GDScript2IdentifierNode *p_id);
	GDScript2IROperand build_self(GDScript2SelfNode *p_self);
	GDScript2IROperand build_binary_op(GDScript2BinaryOpNode *p_binary);
	GDScript2IROperand build_unary_op(GDScript2UnaryOpNode *p_unary);
	GDScript2IROperand build_ternary_op(GDScript2TernaryOpNode *p_ternary);
	GDScript2IROperand build_assignment(GDScript2AssignmentNode *p_assign);
	GDScript2IROperand build_call(GDScript2CallNode *p_call);
	GDScript2IROperand build_subscript(GDScript2SubscriptNode *p_subscript);
	GDScript2IROperand build_attribute(GDScript2AttributeNode *p_attr);
	GDScript2IROperand build_array(GDScript2ArrayNode *p_array);
	GDScript2IROperand build_dictionary(GDScript2DictionaryNode *p_dict);
	GDScript2IROperand build_lambda(GDScript2LambdaNode *p_lambda);
	GDScript2IROperand build_await(GDScript2AwaitNode *p_await);
	GDScript2IROperand build_preload(GDScript2PreloadNode *p_preload);
	GDScript2IROperand build_cast(GDScript2CastNode *p_cast);
	GDScript2IROperand build_type_test(GDScript2TypeTestNode *p_test);
	GDScript2IROperand build_get_node(GDScript2GetNodeNode *p_get_node);

	// Short-circuit evaluation for logical operators
	GDScript2IROperand build_short_circuit_and(GDScript2BinaryOpNode *p_binary);
	GDScript2IROperand build_short_circuit_or(GDScript2BinaryOpNode *p_binary);

	// ========================================================================
	// Assignment Target Building
	// ========================================================================

	// Build assignment: stores p_value into the target
	void build_store(GDScript2ASTNode *p_target, GDScript2IROperand p_value);

	// ========================================================================
	// Loop Control
	// ========================================================================

	struct LoopContext {
		int continue_block = -1;
		int break_block = -1;
	};
	LocalVector<LoopContext> loop_stack;

	void push_loop(int p_continue_block, int p_break_block);
	void pop_loop();
	LoopContext *get_current_loop();

	// ========================================================================
	// Helper Methods
	// ========================================================================

	GDScript2IROp binary_op_to_ir_op(GDScript2BinaryOp p_op);
	GDScript2IROp unary_op_to_ir_op(GDScript2UnaryOp p_op);
	GDScript2IROp assign_op_to_binary_ir_op(GDScript2AssignOp p_op);

public:
	// Build IR from AST (without semantic info)
	Result build(GDScript2ASTNode *p_root);

	// Build IR from AST with semantic analysis results
	Result build_with_semantic(GDScript2ASTNode *p_root, const GDScript2SemanticAnalyzer::Result &p_semantic);
};
