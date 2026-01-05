/**************************************************************************/
/*  gdscript2_semantic.h                                                  */
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
#include "gdscript2_diagnostic.h"
#include "gdscript2_symbol_table.h"
#include "gdscript2_type.h"
#include "modules/gdscript2/front/gdscript2_ast.h"

// Analyzed type information attached to AST nodes
struct GDScript2AnalyzedType {
	GDScript2Type type;
	bool is_constant = false;
	bool is_hard_type = false; // Explicitly annotated
	Variant constant_value; // For compile-time constants
	bool has_constant_value = false;
};

// Semantic analyzer for GDScript2
class GDScript2SemanticAnalyzer : public RefCounted {
	GDCLASS(GDScript2SemanticAnalyzer, RefCounted);

public:
	// Analysis result
	struct Result {
		GDScript2SymbolTable globals; // Legacy compatibility
		List<String> errors; // Legacy compatibility

		// New diagnostic system
		GDScript2DiagnosticReporter diagnostics;

		// Symbol table with full scope information
		GDScript2SymbolTable *symbol_table = nullptr;

		// Type information for expressions (node -> type)
		HashMap<GDScript2ASTNode *, GDScript2AnalyzedType> node_types;

		bool has_errors() const { return !errors.is_empty() || diagnostics.has_errors(); }

		~Result() {
			if (symbol_table) {
				memdelete(symbol_table);
			}
		}
	};

protected:
	static void _bind_methods() {}

private:
	// Current analysis result
	Result *result = nullptr;

	// Current analysis context
	GDScript2ClassNode *current_class = nullptr;
	GDScript2FunctionNode *current_function = nullptr;
	bool in_static_context = false;
	GDScript2Type expected_return_type;

	// Helper: report error
	void push_error(GDScript2DiagnosticCode p_code, const String &p_message, GDScript2ASTNode *p_node = nullptr);
	void push_warning(GDScript2DiagnosticCode p_code, const String &p_message, GDScript2ASTNode *p_node = nullptr);

	// Set analyzed type for a node
	void set_node_type(GDScript2ASTNode *p_node, const GDScript2Type &p_type, bool p_is_constant = false);
	void set_node_constant(GDScript2ASTNode *p_node, const GDScript2Type &p_type, const Variant &p_value);

	// Get analyzed type for a node
	GDScript2Type get_node_type(GDScript2ASTNode *p_node) const;
	bool has_node_type(GDScript2ASTNode *p_node) const;

	// ========================================================================
	// Declaration Analysis
	// ========================================================================

	void analyze_class(GDScript2ClassNode *p_class);
	void analyze_class_interface(GDScript2ClassNode *p_class);
	void analyze_class_body(GDScript2ClassNode *p_class);
	void analyze_function(GDScript2FunctionNode *p_func);
	void analyze_function_signature(GDScript2FunctionNode *p_func);
	void analyze_function_body(GDScript2FunctionNode *p_func);
	void analyze_variable(GDScript2VariableNode *p_var, bool p_is_member);
	void analyze_constant(GDScript2ConstantNode *p_const, bool p_is_member);
	void analyze_signal(GDScript2SignalNode *p_signal);
	void analyze_enum(GDScript2EnumNode *p_enum);
	void analyze_annotation(GDScript2AnnotationNode *p_annotation);
	void analyze_parameter(GDScript2ParameterNode *p_param);

	// ========================================================================
	// Type Resolution
	// ========================================================================

	GDScript2Type resolve_type_node(GDScript2TypeNode *p_type);
	GDScript2Type resolve_type_from_name(const StringName &p_name, GDScript2ASTNode *p_source);

	// ========================================================================
	// Statement Analysis
	// ========================================================================

	void analyze_suite(GDScript2SuiteNode *p_suite);
	void analyze_statement(GDScript2ASTNode *p_stmt);
	void analyze_if(GDScript2IfNode *p_if);
	void analyze_for(GDScript2ForNode *p_for);
	void analyze_while(GDScript2WhileNode *p_while);
	void analyze_match(GDScript2MatchNode *p_match);
	void analyze_match_branch(GDScript2MatchBranchNode *p_branch, const GDScript2Type &p_test_type);
	void analyze_pattern(GDScript2PatternNode *p_pattern, const GDScript2Type &p_test_type);
	void analyze_return(GDScript2ReturnNode *p_return);
	void analyze_break(GDScript2BreakNode *p_break);
	void analyze_continue(GDScript2ContinueNode *p_continue);
	void analyze_assert(GDScript2AssertNode *p_assert);

	// ========================================================================
	// Expression Analysis
	// ========================================================================

	GDScript2Type analyze_expression(GDScript2ASTNode *p_expr);
	GDScript2Type analyze_literal(GDScript2LiteralNode *p_literal);
	GDScript2Type analyze_identifier(GDScript2IdentifierNode *p_identifier);
	GDScript2Type analyze_self(GDScript2SelfNode *p_self);
	GDScript2Type analyze_binary_op(GDScript2BinaryOpNode *p_binary);
	GDScript2Type analyze_unary_op(GDScript2UnaryOpNode *p_unary);
	GDScript2Type analyze_ternary_op(GDScript2TernaryOpNode *p_ternary);
	GDScript2Type analyze_assignment(GDScript2AssignmentNode *p_assign);
	GDScript2Type analyze_call(GDScript2CallNode *p_call);
	GDScript2Type analyze_subscript(GDScript2SubscriptNode *p_subscript);
	GDScript2Type analyze_attribute(GDScript2AttributeNode *p_attribute);
	GDScript2Type analyze_array(GDScript2ArrayNode *p_array);
	GDScript2Type analyze_dictionary(GDScript2DictionaryNode *p_dict);
	GDScript2Type analyze_lambda(GDScript2LambdaNode *p_lambda);
	GDScript2Type analyze_await(GDScript2AwaitNode *p_await);
	GDScript2Type analyze_preload(GDScript2PreloadNode *p_preload);
	GDScript2Type analyze_cast(GDScript2CastNode *p_cast);
	GDScript2Type analyze_type_test(GDScript2TypeTestNode *p_test);
	GDScript2Type analyze_get_node(GDScript2GetNodeNode *p_get_node);

	// ========================================================================
	// Type Inference Helpers
	// ========================================================================

	GDScript2Type infer_binary_op_type(GDScript2BinaryOp p_op, const GDScript2Type &p_left, const GDScript2Type &p_right, GDScript2ASTNode *p_node);
	GDScript2Type infer_unary_op_type(GDScript2UnaryOp p_op, const GDScript2Type &p_operand, GDScript2ASTNode *p_node);
	GDScript2Type infer_subscript_type(const GDScript2Type &p_base, const GDScript2Type &p_index, GDScript2ASTNode *p_node);
	GDScript2Type infer_attribute_type(const GDScript2Type &p_base, const StringName &p_attribute, GDScript2ASTNode *p_node);
	GDScript2Type infer_call_return_type(const GDScript2Type &p_callee, const LocalVector<GDScript2Type> &p_arg_types, GDScript2ASTNode *p_node);

	// ========================================================================
	// Validation Helpers
	// ========================================================================

	bool check_type_compatible(const GDScript2Type &p_target, const GDScript2Type &p_source, GDScript2ASTNode *p_node, bool p_allow_implicit = true);
	bool validate_assignment_target(GDScript2ASTNode *p_target);
	bool validate_call_arguments(GDScript2FunctionSignature *p_sig, const LocalVector<GDScript2ASTNode *> &p_args, GDScript2ASTNode *p_call);

	// ========================================================================
	// Constant Evaluation
	// ========================================================================

	bool try_evaluate_constant(GDScript2ASTNode *p_expr, Variant &r_value);

public:
	// Main entry point
	virtual Result analyze(GDScript2ASTNode *p_root);

	// Analyze with options
	struct Options {
		bool strict_typing = false; // Require type annotations
		bool warn_unused = true; // Warn about unused symbols
		bool warn_shadowing = true; // Warn about variable shadowing
	};

	Result analyze_with_options(GDScript2ASTNode *p_root, const Options &p_options);
};
