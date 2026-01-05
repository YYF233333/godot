/**************************************************************************/
/*  gdscript2_semantic.cpp                                                */
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

#include "gdscript2_semantic.h"

#include "core/object/class_db.h"

// ============================================================================
// Helper Methods
// ============================================================================

void GDScript2SemanticAnalyzer::push_error(GDScript2DiagnosticCode p_code, const String &p_message, GDScript2ASTNode *p_node) {
	if (!result) {
		return;
	}
	result->diagnostics.report_error(p_code, p_message, p_node);
	result->errors.push_back(p_message);
}

void GDScript2SemanticAnalyzer::push_warning(GDScript2DiagnosticCode p_code, const String &p_message, GDScript2ASTNode *p_node) {
	if (!result) {
		return;
	}
	result->diagnostics.report_warning(p_code, p_message, p_node);
}

void GDScript2SemanticAnalyzer::set_node_type(GDScript2ASTNode *p_node, const GDScript2Type &p_type, bool p_is_constant) {
	if (!result || !p_node) {
		return;
	}
	GDScript2AnalyzedType analyzed;
	analyzed.type = p_type;
	analyzed.is_constant = p_is_constant;
	result->node_types.insert(p_node, analyzed);
}

void GDScript2SemanticAnalyzer::set_node_constant(GDScript2ASTNode *p_node, const GDScript2Type &p_type, const Variant &p_value) {
	if (!result || !p_node) {
		return;
	}
	GDScript2AnalyzedType analyzed;
	analyzed.type = p_type;
	analyzed.is_constant = true;
	analyzed.constant_value = p_value;
	analyzed.has_constant_value = true;
	result->node_types.insert(p_node, analyzed);
}

GDScript2Type GDScript2SemanticAnalyzer::get_node_type(GDScript2ASTNode *p_node) const {
	if (!result || !p_node) {
		return GDScript2Type::make_unknown();
	}
	const GDScript2AnalyzedType *found = result->node_types.getptr(p_node);
	return found ? found->type : GDScript2Type::make_unknown();
}

bool GDScript2SemanticAnalyzer::has_node_type(GDScript2ASTNode *p_node) const {
	return result && p_node && result->node_types.has(p_node);
}

// ============================================================================
// Main Entry Point
// ============================================================================

GDScript2SemanticAnalyzer::Result GDScript2SemanticAnalyzer::analyze(GDScript2ASTNode *p_root) {
	return analyze_with_options(p_root, Options());
}

GDScript2SemanticAnalyzer::Result GDScript2SemanticAnalyzer::analyze_with_options(GDScript2ASTNode *p_root, const Options &p_options) {
	Result local_result;
	result = &local_result;

	if (p_root == nullptr) {
		push_error(GDScript2DiagnosticCode::ERR_INTERNAL, "AST root is null.");
		result = nullptr;
		return local_result;
	}

	// Create symbol table
	local_result.symbol_table = memnew(GDScript2SymbolTable);

	// Analyze based on root node type
	if (p_root->type == GDScript2ASTNodeType::NODE_CLASS) {
		analyze_class(static_cast<GDScript2ClassNode *>(p_root));
	} else {
		push_error(GDScript2DiagnosticCode::ERR_INTERNAL, "Expected class node as root.");
	}

	// Copy to legacy globals
	if (local_result.symbol_table) {
		// Copy global symbols for legacy compatibility
		local_result.symbol_table->get_global_scope()->for_each_symbol([&local_result](GDScript2Symbol *sym) {
			if (sym) {
				local_result.globals.set(sym->name, sym->type);
			}
		});
	}

	result = nullptr;
	return local_result;
}

// ============================================================================
// Class Analysis
// ============================================================================

void GDScript2SemanticAnalyzer::analyze_class(GDScript2ClassNode *p_class) {
	if (!p_class) {
		return;
	}

	current_class = p_class;
	result->symbol_table->enter_class(p_class);

	// First pass: analyze interface (declarations)
	analyze_class_interface(p_class);

	// Second pass: analyze bodies
	analyze_class_body(p_class);

	result->symbol_table->exit_class();
	current_class = nullptr;
}

void GDScript2SemanticAnalyzer::analyze_class_interface(GDScript2ClassNode *p_class) {
	// Register class name if specified
	if (p_class->class_name != StringName()) {
		GDScript2Symbol *class_sym = memnew(GDScript2Symbol);
		class_sym->name = p_class->class_name;
		class_sym->kind = GDScript2SymbolKind::SYMBOL_CLASS;
		class_sym->type = GDScript2Type::make_script_class(p_class->class_name);
		class_sym->definition_node = p_class;

		if (!result->symbol_table->declare(class_sym)) {
			push_error(GDScript2DiagnosticCode::ERR_DUPLICATE_DECLARATION,
					vformat("Class name '%s' is already declared.", String(p_class->class_name)), p_class);
			memdelete(class_sym);
		}
	}

	// Analyze annotations
	for (GDScript2AnnotationNode *annotation : p_class->annotations) {
		analyze_annotation(annotation);
	}

	// Analyze enums (they define types and constants)
	for (GDScript2EnumNode *enum_node : p_class->enums) {
		analyze_enum(enum_node);
	}

	// Analyze constants
	for (GDScript2ConstantNode *const_node : p_class->constants) {
		analyze_constant(const_node, true);
	}

	// Analyze signals
	for (GDScript2SignalNode *signal_node : p_class->signals) {
		analyze_signal(signal_node);
	}

	// Analyze member variables (declarations only)
	for (GDScript2VariableNode *var_node : p_class->variables) {
		analyze_variable(var_node, true);
	}

	// Analyze function signatures
	for (GDScript2FunctionNode *func_node : p_class->functions) {
		analyze_function_signature(func_node);
	}

	// Analyze inner classes
	for (GDScript2ClassNode *inner_class : p_class->inner_classes) {
		analyze_class(inner_class);
	}
}

void GDScript2SemanticAnalyzer::analyze_class_body(GDScript2ClassNode *p_class) {
	// Analyze function bodies
	for (GDScript2FunctionNode *func_node : p_class->functions) {
		analyze_function_body(func_node);
	}

	// Analyze variable initializers
	for (GDScript2VariableNode *var_node : p_class->variables) {
		if (var_node->initializer) {
			GDScript2Type init_type = analyze_expression(var_node->initializer);

			// Check type compatibility if variable has type hint
			GDScript2Symbol *sym = result->symbol_table->lookup(var_node->name);
			if (sym && sym->type.is_valid() && !sym->type.is_variant()) {
				check_type_compatible(sym->type, init_type, var_node->initializer);
			}
		}
	}
}

// ============================================================================
// Function Analysis
// ============================================================================

void GDScript2SemanticAnalyzer::analyze_function(GDScript2FunctionNode *p_func) {
	analyze_function_signature(p_func);
	analyze_function_body(p_func);
}

void GDScript2SemanticAnalyzer::analyze_function_signature(GDScript2FunctionNode *p_func) {
	if (!p_func) {
		return;
	}

	// Create function symbol
	GDScript2Symbol *func_sym = memnew(GDScript2Symbol);
	func_sym->name = p_func->name;
	func_sym->kind = GDScript2SymbolKind::SYMBOL_FUNCTION;
	func_sym->definition_node = p_func;
	func_sym->is_static = p_func->is_static;
	func_sym->line = p_func->start_line;
	func_sym->column = p_func->start_column;

	// Create function signature
	GDScript2FunctionSignature *sig = memnew(GDScript2FunctionSignature);
	sig->is_static = p_func->is_static;

	// Resolve return type
	if (p_func->return_type) {
		GDScript2Type ret_type = resolve_type_node(p_func->return_type);
		sig->return_type = memnew(GDScript2Type(ret_type));
	} else {
		sig->return_type = memnew(GDScript2Type(GDScript2Type::make_variant()));
	}

	// Resolve parameter types
	for (GDScript2ParameterNode *param : p_func->parameters) {
		GDScript2Type param_type;
		if (param->type_hint) {
			param_type = resolve_type_node(param->type_hint);
		} else if (param->default_value) {
			// Infer from default value
			param_type = analyze_expression(param->default_value);
		} else {
			param_type = GDScript2Type::make_variant();
		}

		sig->parameter_types.push_back(memnew(GDScript2Type(param_type)));
		sig->parameter_names.push_back(param->name);
		sig->parameter_has_default.push_back(param->default_value != nullptr);

		if (param->default_value) {
			sig->default_arg_count++;
		}
	}

	func_sym->func_signature = sig;
	func_sym->type = GDScript2Type::make_func(nullptr); // Don't duplicate signature

	// Register function
	if (!result->symbol_table->declare(func_sym)) {
		push_error(GDScript2DiagnosticCode::ERR_DUPLICATE_DECLARATION,
				vformat("Function '%s' is already declared.", String(p_func->name)), p_func);
		memdelete(func_sym);
	}
}

void GDScript2SemanticAnalyzer::analyze_function_body(GDScript2FunctionNode *p_func) {
	if (!p_func || !p_func->body) {
		return;
	}

	current_function = p_func;
	in_static_context = p_func->is_static;

	// Set expected return type
	if (p_func->return_type) {
		expected_return_type = resolve_type_node(p_func->return_type);
	} else {
		expected_return_type = GDScript2Type::make_variant();
	}

	// For coroutine functions, the actual return type is wrapped in a coroutine
	// But internally we still check against the declared return type
	if (p_func->is_coroutine) {
		// Coroutine functions implicitly return GDScript2Coroutine objects
		// The declared return type is what the coroutine will eventually yield
	}

	result->symbol_table->enter_function(p_func);

	// Register parameters as local variables
	for (GDScript2ParameterNode *param : p_func->parameters) {
		analyze_parameter(param);
	}

	// Analyze function body
	analyze_suite(p_func->body);

	result->symbol_table->exit_function();

	expected_return_type = GDScript2Type::make_unknown();
	in_static_context = false;
	current_function = nullptr;
}

void GDScript2SemanticAnalyzer::analyze_parameter(GDScript2ParameterNode *p_param) {
	if (!p_param) {
		return;
	}

	GDScript2Symbol *param_sym = memnew(GDScript2Symbol);
	param_sym->name = p_param->name;
	param_sym->kind = GDScript2SymbolKind::SYMBOL_PARAMETER;
	param_sym->definition_node = p_param;
	param_sym->line = p_param->start_line;
	param_sym->column = p_param->start_column;

	// Resolve type
	if (p_param->type_hint) {
		param_sym->type = resolve_type_node(p_param->type_hint);
		param_sym->type.is_hard_type = true;
	} else if (p_param->default_value) {
		param_sym->type = analyze_expression(p_param->default_value);
	} else {
		param_sym->type = GDScript2Type::make_variant();
	}

	if (!result->symbol_table->declare(param_sym)) {
		push_error(GDScript2DiagnosticCode::ERR_DUPLICATE_DECLARATION,
				vformat("Parameter '%s' is already declared.", String(p_param->name)), p_param);
		memdelete(param_sym);
	}
}

// ============================================================================
// Variable/Constant Analysis
// ============================================================================

void GDScript2SemanticAnalyzer::analyze_variable(GDScript2VariableNode *p_var, bool p_is_member) {
	if (!p_var) {
		return;
	}

	GDScript2Symbol *var_sym = memnew(GDScript2Symbol);
	var_sym->name = p_var->name;
	var_sym->kind = GDScript2SymbolKind::SYMBOL_VARIABLE;
	var_sym->definition_node = p_var;
	var_sym->is_static = p_var->is_static;
	var_sym->line = p_var->start_line;
	var_sym->column = p_var->start_column;

	// Check for @export annotation
	for (GDScript2AnnotationNode *annot : p_var->annotations) {
		if (annot->name == "export") {
			var_sym->is_exported = true;
		}
	}

	// Resolve type
	if (p_var->type_hint) {
		var_sym->type = resolve_type_node(p_var->type_hint);
		var_sym->type.is_hard_type = true;
	} else if (p_var->initializer) {
		// Type will be inferred when analyzing initializer
		var_sym->type = GDScript2Type::make_variant();
	} else {
		var_sym->type = GDScript2Type::make_variant();
	}

	if (!result->symbol_table->declare(var_sym)) {
		push_error(GDScript2DiagnosticCode::ERR_DUPLICATE_DECLARATION,
				vformat("Variable '%s' is already declared.", String(p_var->name)), p_var);
		memdelete(var_sym);
	}
}

void GDScript2SemanticAnalyzer::analyze_constant(GDScript2ConstantNode *p_const, bool p_is_member) {
	if (!p_const) {
		return;
	}

	GDScript2Symbol *const_sym = memnew(GDScript2Symbol);
	const_sym->name = p_const->name;
	const_sym->kind = GDScript2SymbolKind::SYMBOL_CONSTANT;
	const_sym->definition_node = p_const;
	const_sym->line = p_const->start_line;
	const_sym->column = p_const->start_column;

	// Constants must have an initializer
	if (!p_const->initializer) {
		push_error(GDScript2DiagnosticCode::ERR_CONST_NOT_CONSTANT,
				vformat("Constant '%s' must have an initializer.", String(p_const->name)), p_const);
		memdelete(const_sym);
		return;
	}

	// Analyze initializer and get type
	GDScript2Type init_type = analyze_expression(p_const->initializer);

	// Try to evaluate as constant
	Variant const_value;
	if (!try_evaluate_constant(p_const->initializer, const_value)) {
		push_error(GDScript2DiagnosticCode::ERR_CONST_NOT_CONSTANT,
				vformat("Cannot evaluate constant '%s' at compile time.", String(p_const->name)), p_const->initializer);
	}

	// Resolve type
	if (p_const->type_hint) {
		const_sym->type = resolve_type_node(p_const->type_hint);
		const_sym->type.is_hard_type = true;

		// Check compatibility
		check_type_compatible(const_sym->type, init_type, p_const->initializer);
	} else {
		const_sym->type = init_type;
	}

	const_sym->type.is_constant = true;

	if (!result->symbol_table->declare(const_sym)) {
		push_error(GDScript2DiagnosticCode::ERR_DUPLICATE_DECLARATION,
				vformat("Constant '%s' is already declared.", String(p_const->name)), p_const);
		memdelete(const_sym);
	}
}

void GDScript2SemanticAnalyzer::analyze_signal(GDScript2SignalNode *p_signal) {
	if (!p_signal) {
		return;
	}

	GDScript2Symbol *sig_sym = memnew(GDScript2Symbol);
	sig_sym->name = p_signal->name;
	sig_sym->kind = GDScript2SymbolKind::SYMBOL_SIGNAL;
	sig_sym->type = GDScript2Type::make_signal();
	sig_sym->definition_node = p_signal;
	sig_sym->line = p_signal->start_line;
	sig_sym->column = p_signal->start_column;

	if (!result->symbol_table->declare(sig_sym)) {
		push_error(GDScript2DiagnosticCode::ERR_DUPLICATE_DECLARATION,
				vformat("Signal '%s' is already declared.", String(p_signal->name)), p_signal);
		memdelete(sig_sym);
	}
}

void GDScript2SemanticAnalyzer::analyze_enum(GDScript2EnumNode *p_enum) {
	if (!p_enum) {
		return;
	}

	// Register enum type if named
	if (p_enum->name != StringName()) {
		GDScript2Symbol *enum_sym = memnew(GDScript2Symbol);
		enum_sym->name = p_enum->name;
		enum_sym->kind = GDScript2SymbolKind::SYMBOL_ENUM;
		enum_sym->type = GDScript2Type::make_enum(p_enum->name);
		enum_sym->definition_node = p_enum;
		enum_sym->line = p_enum->start_line;
		enum_sym->column = p_enum->start_column;

		if (!result->symbol_table->declare(enum_sym)) {
			push_error(GDScript2DiagnosticCode::ERR_DUPLICATE_DECLARATION,
					vformat("Enum '%s' is already declared.", String(p_enum->name)), p_enum);
			memdelete(enum_sym);
		}
	}

	// Register enum values as constants
	int64_t current_value = 0;
	for (uint32_t i = 0; i < p_enum->value_names.size(); i++) {
		StringName value_name = p_enum->value_names[i];
		GDScript2ASTNode *value_expr = (i < p_enum->value_expressions.size()) ? p_enum->value_expressions[i] : nullptr;

		// Evaluate value
		if (value_expr) {
			Variant eval_result;
			if (try_evaluate_constant(value_expr, eval_result)) {
				if (eval_result.get_type() == Variant::INT) {
					current_value = eval_result;
				} else {
					push_error(GDScript2DiagnosticCode::ERR_ENUM_VALUE_NOT_CONSTANT,
							vformat("Enum value '%s' must be an integer.", String(value_name)), value_expr);
				}
			} else {
				push_error(GDScript2DiagnosticCode::ERR_ENUM_VALUE_NOT_CONSTANT,
						vformat("Cannot evaluate enum value '%s' at compile time.", String(value_name)), value_expr);
			}
		}

		// Create symbol for enum value
		GDScript2Symbol *val_sym = memnew(GDScript2Symbol);
		val_sym->name = value_name;
		val_sym->kind = GDScript2SymbolKind::SYMBOL_ENUM_VALUE;
		val_sym->type = GDScript2Type::make_int();
		val_sym->type.is_constant = true;
		val_sym->enum_value = current_value;
		val_sym->enum_name = p_enum->name;
		val_sym->definition_node = p_enum;

		if (!result->symbol_table->declare(val_sym)) {
			push_error(GDScript2DiagnosticCode::ERR_DUPLICATE_DECLARATION,
					vformat("Enum value '%s' is already declared.", String(value_name)), p_enum);
			memdelete(val_sym);
		}

		current_value++;
	}
}

void GDScript2SemanticAnalyzer::analyze_annotation(GDScript2AnnotationNode *p_annotation) {
	// For now, just validate known annotations
	// TODO: Implement annotation-specific validation
}

// ============================================================================
// Type Resolution
// ============================================================================

GDScript2Type GDScript2SemanticAnalyzer::resolve_type_node(GDScript2TypeNode *p_type) {
	if (!p_type) {
		return GDScript2Type::make_unknown();
	}

	return resolve_type_from_name(p_type->type_name, p_type);
}

GDScript2Type GDScript2SemanticAnalyzer::resolve_type_from_name(const StringName &p_name, GDScript2ASTNode *p_source) {
	// Try built-in types first
	GDScript2Type builtin = GDScript2Type::from_type_name(p_name);
	if (builtin.is_valid()) {
		return builtin;
	}

	// Look up in symbol table
	GDScript2Symbol *sym = result->symbol_table->lookup(p_name);
	if (sym) {
		switch (sym->kind) {
			case GDScript2SymbolKind::SYMBOL_CLASS:
			case GDScript2SymbolKind::SYMBOL_NATIVE_CLASS:
				return GDScript2Type::make_object(p_name);
			case GDScript2SymbolKind::SYMBOL_ENUM:
				return GDScript2Type::make_enum(p_name);
			case GDScript2SymbolKind::SYMBOL_BUILTIN_TYPE:
				return GDScript2Type::from_type_name(p_name);
			default:
				break;
		}
	}

	// Check native classes
	if (ClassDB::class_exists(p_name)) {
		return GDScript2Type::make_object(p_name);
	}

	push_error(GDScript2DiagnosticCode::ERR_UNDEFINED_TYPE,
			vformat("Unknown type '%s'.", String(p_name)), p_source);
	return GDScript2Type::make_unknown();
}

// ============================================================================
// Statement Analysis
// ============================================================================

void GDScript2SemanticAnalyzer::analyze_suite(GDScript2SuiteNode *p_suite) {
	if (!p_suite) {
		return;
	}

	result->symbol_table->push_scope(GDScript2ScopeKind::SCOPE_BLOCK);

	for (GDScript2ASTNode *stmt : p_suite->statements) {
		analyze_statement(stmt);
	}

	result->symbol_table->pop_scope();
}

void GDScript2SemanticAnalyzer::analyze_statement(GDScript2ASTNode *p_stmt) {
	if (!p_stmt) {
		return;
	}

	switch (p_stmt->type) {
		case GDScript2ASTNodeType::NODE_VARIABLE:
			analyze_variable(static_cast<GDScript2VariableNode *>(p_stmt), false);
			break;
		case GDScript2ASTNodeType::NODE_CONSTANT:
			analyze_constant(static_cast<GDScript2ConstantNode *>(p_stmt), false);
			break;
		case GDScript2ASTNodeType::NODE_IF:
			analyze_if(static_cast<GDScript2IfNode *>(p_stmt));
			break;
		case GDScript2ASTNodeType::NODE_FOR:
			analyze_for(static_cast<GDScript2ForNode *>(p_stmt));
			break;
		case GDScript2ASTNodeType::NODE_WHILE:
			analyze_while(static_cast<GDScript2WhileNode *>(p_stmt));
			break;
		case GDScript2ASTNodeType::NODE_MATCH:
			analyze_match(static_cast<GDScript2MatchNode *>(p_stmt));
			break;
		case GDScript2ASTNodeType::NODE_RETURN:
			analyze_return(static_cast<GDScript2ReturnNode *>(p_stmt));
			break;
		case GDScript2ASTNodeType::NODE_BREAK:
			analyze_break(static_cast<GDScript2BreakNode *>(p_stmt));
			break;
		case GDScript2ASTNodeType::NODE_CONTINUE:
			analyze_continue(static_cast<GDScript2ContinueNode *>(p_stmt));
			break;
		case GDScript2ASTNodeType::NODE_ASSERT:
			analyze_assert(static_cast<GDScript2AssertNode *>(p_stmt));
			break;
		case GDScript2ASTNodeType::NODE_PASS:
		case GDScript2ASTNodeType::NODE_BREAKPOINT:
			// No analysis needed
			break;
		default:
			// Expression statement
			analyze_expression(p_stmt);
			break;
	}
}

void GDScript2SemanticAnalyzer::analyze_if(GDScript2IfNode *p_if) {
	if (!p_if) {
		return;
	}

	// Analyze condition
	GDScript2Type cond_type = analyze_expression(p_if->condition);
	// Any type can be used as boolean in GDScript

	// Analyze true block
	if (p_if->true_block) {
		analyze_suite(p_if->true_block);
	}

	// Analyze else/elif block
	if (p_if->false_block) {
		if (p_if->false_block->type == GDScript2ASTNodeType::NODE_IF) {
			analyze_if(static_cast<GDScript2IfNode *>(p_if->false_block));
		} else if (p_if->false_block->type == GDScript2ASTNodeType::NODE_SUITE) {
			analyze_suite(static_cast<GDScript2SuiteNode *>(p_if->false_block));
		}
	}
}

void GDScript2SemanticAnalyzer::analyze_for(GDScript2ForNode *p_for) {
	if (!p_for) {
		return;
	}

	result->symbol_table->push_scope(GDScript2ScopeKind::SCOPE_BLOCK);
	result->symbol_table->enter_loop();

	// Analyze iterable
	GDScript2Type iter_type = analyze_expression(p_for->iterable);

	// Determine element type
	GDScript2Type elem_type = GDScript2Type::make_variant();
	if (iter_type.kind == GDScript2TypeKind::TYPE_ARRAY && iter_type.element_type) {
		elem_type = *iter_type.element_type;
	} else if (iter_type.kind == GDScript2TypeKind::TYPE_STRING) {
		elem_type = GDScript2Type::make_string();
	} else if (iter_type.kind == GDScript2TypeKind::TYPE_DICTIONARY) {
		if (iter_type.key_type) {
			elem_type = *iter_type.key_type;
		}
	}
	// For range(), element is int (handled specially in call analysis)

	// Register loop variable
	GDScript2Symbol *var_sym = memnew(GDScript2Symbol);
	var_sym->name = p_for->variable;
	var_sym->kind = GDScript2SymbolKind::SYMBOL_VARIABLE;
	var_sym->type = elem_type;
	var_sym->line = p_for->start_line;
	var_sym->column = p_for->start_column;
	result->symbol_table->declare(var_sym);

	// Analyze body
	if (p_for->body) {
		analyze_suite(p_for->body);
	}

	result->symbol_table->exit_loop();
	result->symbol_table->pop_scope();
}

void GDScript2SemanticAnalyzer::analyze_while(GDScript2WhileNode *p_while) {
	if (!p_while) {
		return;
	}

	result->symbol_table->enter_loop();

	// Analyze condition
	analyze_expression(p_while->condition);

	// Analyze body
	if (p_while->body) {
		analyze_suite(p_while->body);
	}

	result->symbol_table->exit_loop();
}

void GDScript2SemanticAnalyzer::analyze_match(GDScript2MatchNode *p_match) {
	if (!p_match) {
		return;
	}

	// Analyze test value
	GDScript2Type test_type = analyze_expression(p_match->test_value);

	// Analyze branches
	for (GDScript2MatchBranchNode *branch : p_match->branches) {
		analyze_match_branch(branch, test_type);
	}
}

void GDScript2SemanticAnalyzer::analyze_match_branch(GDScript2MatchBranchNode *p_branch, const GDScript2Type &p_test_type) {
	if (!p_branch) {
		return;
	}

	result->symbol_table->push_scope(GDScript2ScopeKind::SCOPE_MATCH);

	// Analyze patterns
	for (GDScript2PatternNode *pattern : p_branch->patterns) {
		analyze_pattern(pattern, p_test_type);
	}

	// Analyze guard
	if (p_branch->guard) {
		analyze_expression(p_branch->guard);
	}

	// Analyze body
	if (p_branch->body) {
		analyze_suite(p_branch->body);
	}

	result->symbol_table->pop_scope();
}

void GDScript2SemanticAnalyzer::analyze_pattern(GDScript2PatternNode *p_pattern, const GDScript2Type &p_test_type) {
	if (!p_pattern) {
		return;
	}

	switch (p_pattern->pattern_type) {
		case GDScript2PatternType::PATTERN_LITERAL:
			// Check literal type compatibility
			set_node_type(p_pattern, GDScript2Type::from_variant(p_pattern->literal));
			break;

		case GDScript2PatternType::PATTERN_BIND: {
			// Create binding variable
			GDScript2Symbol *bind_sym = memnew(GDScript2Symbol);
			bind_sym->name = p_pattern->bind_name;
			bind_sym->kind = GDScript2SymbolKind::SYMBOL_VARIABLE;
			bind_sym->type = p_test_type;
			bind_sym->definition_node = p_pattern;
			result->symbol_table->declare(bind_sym);
		} break;

		case GDScript2PatternType::PATTERN_EXPRESSION:
			if (p_pattern->expression) {
				analyze_expression(p_pattern->expression);
			}
			break;

		case GDScript2PatternType::PATTERN_ARRAY:
			for (GDScript2PatternNode *sub : p_pattern->array_patterns) {
				GDScript2Type elem_type = GDScript2Type::make_variant();
				if (p_test_type.kind == GDScript2TypeKind::TYPE_ARRAY && p_test_type.element_type) {
					elem_type = *p_test_type.element_type;
				}
				analyze_pattern(sub, elem_type);
			}
			break;

		case GDScript2PatternType::PATTERN_DICTIONARY:
			for (uint32_t i = 0; i < p_pattern->dictionary_patterns.size(); i++) {
				if (i < p_pattern->dictionary_keys.size()) {
					analyze_expression(p_pattern->dictionary_keys[i]);
				}
				GDScript2Type val_type = GDScript2Type::make_variant();
				if (p_test_type.kind == GDScript2TypeKind::TYPE_DICTIONARY && p_test_type.value_type) {
					val_type = *p_test_type.value_type;
				}
				analyze_pattern(p_pattern->dictionary_patterns[i], val_type);
			}
			break;

		case GDScript2PatternType::PATTERN_REST:
			// Rest pattern matches remaining elements
			break;
	}
}

void GDScript2SemanticAnalyzer::analyze_return(GDScript2ReturnNode *p_return) {
	if (!p_return) {
		return;
	}

	if (!current_function) {
		push_error(GDScript2DiagnosticCode::ERR_RETURN_OUTSIDE_FUNCTION,
				"Return statement outside of function.", p_return);
		return;
	}

	if (p_return->value) {
		GDScript2Type ret_type = analyze_expression(p_return->value);

		// Check return type compatibility
		if (expected_return_type.is_valid() && !expected_return_type.is_variant()) {
			if (expected_return_type.is_nil()) {
				push_error(GDScript2DiagnosticCode::ERR_VOID_RETURN_VALUE,
						"Cannot return a value from a void function.", p_return);
			} else {
				check_type_compatible(expected_return_type, ret_type, p_return->value);
			}
		}
	}
}

void GDScript2SemanticAnalyzer::analyze_break(GDScript2BreakNode *p_break) {
	if (!result->symbol_table->is_in_loop()) {
		push_error(GDScript2DiagnosticCode::ERR_BREAK_OUTSIDE_LOOP,
				"Break statement outside of loop.", p_break);
	}
}

void GDScript2SemanticAnalyzer::analyze_continue(GDScript2ContinueNode *p_continue) {
	if (!result->symbol_table->is_in_loop()) {
		push_error(GDScript2DiagnosticCode::ERR_CONTINUE_OUTSIDE_LOOP,
				"Continue statement outside of loop.", p_continue);
	}
}

void GDScript2SemanticAnalyzer::analyze_assert(GDScript2AssertNode *p_assert) {
	if (!p_assert) {
		return;
	}

	if (p_assert->condition) {
		analyze_expression(p_assert->condition);
	}
	if (p_assert->message) {
		analyze_expression(p_assert->message);
	}
}

// ============================================================================
// Expression Analysis
// ============================================================================

GDScript2Type GDScript2SemanticAnalyzer::analyze_expression(GDScript2ASTNode *p_expr) {
	if (!p_expr) {
		return GDScript2Type::make_unknown();
	}

	GDScript2Type result_type;

	switch (p_expr->type) {
		case GDScript2ASTNodeType::NODE_LITERAL:
			result_type = analyze_literal(static_cast<GDScript2LiteralNode *>(p_expr));
			break;
		case GDScript2ASTNodeType::NODE_IDENTIFIER:
			result_type = analyze_identifier(static_cast<GDScript2IdentifierNode *>(p_expr));
			break;
		case GDScript2ASTNodeType::NODE_SELF:
			result_type = analyze_self(static_cast<GDScript2SelfNode *>(p_expr));
			break;
		case GDScript2ASTNodeType::NODE_BINARY_OP:
			result_type = analyze_binary_op(static_cast<GDScript2BinaryOpNode *>(p_expr));
			break;
		case GDScript2ASTNodeType::NODE_UNARY_OP:
			result_type = analyze_unary_op(static_cast<GDScript2UnaryOpNode *>(p_expr));
			break;
		case GDScript2ASTNodeType::NODE_TERNARY_OP:
			result_type = analyze_ternary_op(static_cast<GDScript2TernaryOpNode *>(p_expr));
			break;
		case GDScript2ASTNodeType::NODE_ASSIGNMENT:
			result_type = analyze_assignment(static_cast<GDScript2AssignmentNode *>(p_expr));
			break;
		case GDScript2ASTNodeType::NODE_CALL:
			result_type = analyze_call(static_cast<GDScript2CallNode *>(p_expr));
			break;
		case GDScript2ASTNodeType::NODE_SUBSCRIPT:
			result_type = analyze_subscript(static_cast<GDScript2SubscriptNode *>(p_expr));
			break;
		case GDScript2ASTNodeType::NODE_ATTRIBUTE:
			result_type = analyze_attribute(static_cast<GDScript2AttributeNode *>(p_expr));
			break;
		case GDScript2ASTNodeType::NODE_ARRAY:
			result_type = analyze_array(static_cast<GDScript2ArrayNode *>(p_expr));
			break;
		case GDScript2ASTNodeType::NODE_DICTIONARY:
			result_type = analyze_dictionary(static_cast<GDScript2DictionaryNode *>(p_expr));
			break;
		case GDScript2ASTNodeType::NODE_LAMBDA:
			result_type = analyze_lambda(static_cast<GDScript2LambdaNode *>(p_expr));
			break;
		case GDScript2ASTNodeType::NODE_AWAIT:
			result_type = analyze_await(static_cast<GDScript2AwaitNode *>(p_expr));
			break;
		case GDScript2ASTNodeType::NODE_PRELOAD:
			result_type = analyze_preload(static_cast<GDScript2PreloadNode *>(p_expr));
			break;
		case GDScript2ASTNodeType::NODE_CAST:
			result_type = analyze_cast(static_cast<GDScript2CastNode *>(p_expr));
			break;
		case GDScript2ASTNodeType::NODE_TYPE_TEST:
			result_type = analyze_type_test(static_cast<GDScript2TypeTestNode *>(p_expr));
			break;
		case GDScript2ASTNodeType::NODE_GET_NODE:
			result_type = analyze_get_node(static_cast<GDScript2GetNodeNode *>(p_expr));
			break;
		default:
			result_type = GDScript2Type::make_unknown();
			break;
	}

	set_node_type(p_expr, result_type);
	return result_type;
}

GDScript2Type GDScript2SemanticAnalyzer::analyze_literal(GDScript2LiteralNode *p_literal) {
	if (!p_literal) {
		return GDScript2Type::make_unknown();
	}

	GDScript2Type type = GDScript2Type::from_variant(p_literal->value);
	set_node_constant(p_literal, type, p_literal->value);
	return type;
}

GDScript2Type GDScript2SemanticAnalyzer::analyze_identifier(GDScript2IdentifierNode *p_identifier) {
	if (!p_identifier) {
		return GDScript2Type::make_unknown();
	}

	GDScript2Symbol *sym = result->symbol_table->lookup(p_identifier->name);
	if (!sym) {
		push_error(GDScript2DiagnosticCode::ERR_UNDEFINED_IDENTIFIER,
				vformat("Identifier '%s' not declared in the current scope.", String(p_identifier->name)), p_identifier);
		return GDScript2Type::make_unknown();
	}

	sym->is_used = true;
	return sym->type;
}

GDScript2Type GDScript2SemanticAnalyzer::analyze_self(GDScript2SelfNode *p_self) {
	if (in_static_context) {
		push_error(GDScript2DiagnosticCode::ERR_STATIC_ACCESS,
				"Cannot use 'self' in static function.", p_self);
		return GDScript2Type::make_unknown();
	}

	if (!current_class) {
		push_error(GDScript2DiagnosticCode::ERR_SELF_OUTSIDE_CLASS,
				"Cannot use 'self' outside of a class.", p_self);
		return GDScript2Type::make_unknown();
	}

	if (current_class->class_name != StringName()) {
		return GDScript2Type::make_script_class(current_class->class_name);
	}
	return GDScript2Type::make_object();
}

GDScript2Type GDScript2SemanticAnalyzer::analyze_binary_op(GDScript2BinaryOpNode *p_binary) {
	if (!p_binary) {
		return GDScript2Type::make_unknown();
	}

	GDScript2Type left_type = analyze_expression(p_binary->left);
	GDScript2Type right_type = analyze_expression(p_binary->right);

	return infer_binary_op_type(p_binary->op, left_type, right_type, p_binary);
}

GDScript2Type GDScript2SemanticAnalyzer::analyze_unary_op(GDScript2UnaryOpNode *p_unary) {
	if (!p_unary) {
		return GDScript2Type::make_unknown();
	}

	GDScript2Type operand_type = analyze_expression(p_unary->operand);
	return infer_unary_op_type(p_unary->op, operand_type, p_unary);
}

GDScript2Type GDScript2SemanticAnalyzer::analyze_ternary_op(GDScript2TernaryOpNode *p_ternary) {
	if (!p_ternary) {
		return GDScript2Type::make_unknown();
	}

	analyze_expression(p_ternary->condition);
	GDScript2Type true_type = analyze_expression(p_ternary->true_expr);
	GDScript2Type false_type = analyze_expression(p_ternary->false_expr);

	return GDScript2TypeChecker::get_common_type(true_type, false_type);
}

GDScript2Type GDScript2SemanticAnalyzer::analyze_assignment(GDScript2AssignmentNode *p_assign) {
	if (!p_assign) {
		return GDScript2Type::make_unknown();
	}

	// Validate assignment target
	validate_assignment_target(p_assign->target);

	GDScript2Type target_type = analyze_expression(p_assign->target);
	GDScript2Type value_type = analyze_expression(p_assign->value);

	// Check type compatibility
	if (target_type.is_valid() && !target_type.is_variant()) {
		check_type_compatible(target_type, value_type, p_assign->value);
	}

	// For compound assignments, verify the operation is valid
	if (p_assign->op != GDScript2AssignOp::OP_ASSIGN) {
		// Map compound op to binary op and check
		GDScript2BinaryOp bin_op = GDScript2BinaryOp::OP_NONE;
		switch (p_assign->op) {
			case GDScript2AssignOp::OP_ADD_ASSIGN:
				bin_op = GDScript2BinaryOp::OP_ADD;
				break;
			case GDScript2AssignOp::OP_SUB_ASSIGN:
				bin_op = GDScript2BinaryOp::OP_SUB;
				break;
			case GDScript2AssignOp::OP_MUL_ASSIGN:
				bin_op = GDScript2BinaryOp::OP_MUL;
				break;
			case GDScript2AssignOp::OP_DIV_ASSIGN:
				bin_op = GDScript2BinaryOp::OP_DIV;
				break;
			case GDScript2AssignOp::OP_MOD_ASSIGN:
				bin_op = GDScript2BinaryOp::OP_MOD;
				break;
			case GDScript2AssignOp::OP_POW_ASSIGN:
				bin_op = GDScript2BinaryOp::OP_POW;
				break;
			default:
				break;
		}
		if (bin_op != GDScript2BinaryOp::OP_NONE) {
			infer_binary_op_type(bin_op, target_type, value_type, p_assign);
		}
	}

	return target_type;
}

GDScript2Type GDScript2SemanticAnalyzer::analyze_call(GDScript2CallNode *p_call) {
	if (!p_call) {
		return GDScript2Type::make_unknown();
	}

	// Analyze callee
	GDScript2Type callee_type = analyze_expression(p_call->callee);

	// Collect argument types
	LocalVector<GDScript2Type> arg_types;
	for (GDScript2ASTNode *arg : p_call->arguments) {
		arg_types.push_back(analyze_expression(arg));
	}

	// Special handling for built-in functions
	if (p_call->callee->type == GDScript2ASTNodeType::NODE_IDENTIFIER) {
		GDScript2IdentifierNode *callee_id = static_cast<GDScript2IdentifierNode *>(p_call->callee);

		// Special case: range() returns iterable of int
		if (callee_id->name == "range") {
			return GDScript2Type::make_array();
		}

		// Special case: len() returns int
		if (callee_id->name == "len") {
			return GDScript2Type::make_int();
		}

		// Special case: str() returns String
		if (callee_id->name == "str") {
			return GDScript2Type::make_string();
		}

		// Special case: typeof() returns int
		if (callee_id->name == "typeof") {
			return GDScript2Type::make_int();
		}
	}

	return infer_call_return_type(callee_type, arg_types, p_call);
}

GDScript2Type GDScript2SemanticAnalyzer::analyze_subscript(GDScript2SubscriptNode *p_subscript) {
	if (!p_subscript) {
		return GDScript2Type::make_unknown();
	}

	GDScript2Type base_type = analyze_expression(p_subscript->base);
	GDScript2Type index_type = analyze_expression(p_subscript->index);

	return infer_subscript_type(base_type, index_type, p_subscript);
}

GDScript2Type GDScript2SemanticAnalyzer::analyze_attribute(GDScript2AttributeNode *p_attribute) {
	if (!p_attribute) {
		return GDScript2Type::make_unknown();
	}

	GDScript2Type base_type = analyze_expression(p_attribute->base);
	return infer_attribute_type(base_type, p_attribute->attribute, p_attribute);
}

GDScript2Type GDScript2SemanticAnalyzer::analyze_array(GDScript2ArrayNode *p_array) {
	if (!p_array) {
		return GDScript2Type::make_unknown();
	}

	GDScript2Type common_element_type;
	bool first = true;

	for (GDScript2ASTNode *elem : p_array->elements) {
		GDScript2Type elem_type = analyze_expression(elem);
		if (first) {
			common_element_type = elem_type;
			first = false;
		} else {
			common_element_type = GDScript2TypeChecker::get_common_type(common_element_type, elem_type);
		}
	}

	if (common_element_type.is_valid() && !common_element_type.is_variant()) {
		return GDScript2Type::make_typed_array(memnew(GDScript2Type(common_element_type)));
	}

	return GDScript2Type::make_array();
}

GDScript2Type GDScript2SemanticAnalyzer::analyze_dictionary(GDScript2DictionaryNode *p_dict) {
	if (!p_dict) {
		return GDScript2Type::make_unknown();
	}

	// Analyze all keys and values
	for (GDScript2ASTNode *key : p_dict->keys) {
		analyze_expression(key);
	}
	for (GDScript2ASTNode *val : p_dict->values) {
		analyze_expression(val);
	}

	return GDScript2Type::make_dictionary();
}

GDScript2Type GDScript2SemanticAnalyzer::analyze_lambda(GDScript2LambdaNode *p_lambda) {
	if (!p_lambda) {
		return GDScript2Type::make_unknown();
	}

	result->symbol_table->push_scope(GDScript2ScopeKind::SCOPE_LAMBDA);

	// Register parameters
	for (GDScript2ParameterNode *param : p_lambda->parameters) {
		analyze_parameter(param);
	}

	// Analyze body
	if (p_lambda->body) {
		if (p_lambda->body->type == GDScript2ASTNodeType::NODE_SUITE) {
			analyze_suite(static_cast<GDScript2SuiteNode *>(p_lambda->body));
		} else {
			analyze_expression(p_lambda->body);
		}
	}

	result->symbol_table->pop_scope();

	return GDScript2Type::make_callable();
}

GDScript2Type GDScript2SemanticAnalyzer::analyze_await(GDScript2AwaitNode *p_await) {
	if (!p_await) {
		return GDScript2Type::make_unknown();
	}

	if (!current_function) {
		push_error(GDScript2DiagnosticCode::ERR_AWAIT_OUTSIDE_FUNCTION,
				"Await can only be used inside a function.", p_await);
		return GDScript2Type::make_unknown();
	}

	// Mark the current function as a coroutine if it uses await
	if (current_function) {
		current_function->is_coroutine = true;
	}

	GDScript2Type expr_type = analyze_expression(p_await->expression);

	// await on Signal returns Variant (signal arguments)
	if (expr_type.kind == GDScript2TypeKind::TYPE_SIGNAL) {
		return GDScript2Type::make_variant();
	}

	// await on coroutine (GDScript2Coroutine) returns its eventual value
	// For now, we return Variant since we don't track coroutine return types yet
	// TODO: Track coroutine return types in type system
	if (expr_type.kind == GDScript2TypeKind::TYPE_OBJECT) {
		// Could be a coroutine object
		return GDScript2Type::make_variant();
	}

	// Invalid await target
	push_warning(GDScript2DiagnosticCode::WARN_UNREACHABLE_CODE,
			vformat("Awaiting value of type '%s' may not work as expected. Expected Signal or Coroutine.",
					expr_type.to_string()),
			p_await);

	return GDScript2Type::make_variant();
}

GDScript2Type GDScript2SemanticAnalyzer::analyze_preload(GDScript2PreloadNode *p_preload) {
	// preload returns Resource
	return GDScript2Type::make_object("Resource");
}

GDScript2Type GDScript2SemanticAnalyzer::analyze_cast(GDScript2CastNode *p_cast) {
	if (!p_cast) {
		return GDScript2Type::make_unknown();
	}

	analyze_expression(p_cast->operand);

	if (p_cast->cast_type && p_cast->cast_type->type == GDScript2ASTNodeType::NODE_TYPE) {
		return resolve_type_node(static_cast<GDScript2TypeNode *>(p_cast->cast_type));
	}

	return GDScript2Type::make_unknown();
}

GDScript2Type GDScript2SemanticAnalyzer::analyze_type_test(GDScript2TypeTestNode *p_test) {
	if (!p_test) {
		return GDScript2Type::make_unknown();
	}

	analyze_expression(p_test->operand);
	// The test type is not analyzed as expression

	return GDScript2Type::make_bool();
}

GDScript2Type GDScript2SemanticAnalyzer::analyze_get_node(GDScript2GetNodeNode *p_get_node) {
	// $NodePath returns Node
	return GDScript2Type::make_object("Node");
}

// ============================================================================
// Type Inference Helpers
// ============================================================================

GDScript2Type GDScript2SemanticAnalyzer::infer_binary_op_type(GDScript2BinaryOp p_op, const GDScript2Type &p_left, const GDScript2Type &p_right, GDScript2ASTNode *p_node) {
	// If either is variant, result is variant
	if (p_left.is_variant() || p_right.is_variant()) {
		return GDScript2Type::make_variant();
	}

	switch (p_op) {
		// Comparison operators always return bool
		case GDScript2BinaryOp::OP_EQ:
		case GDScript2BinaryOp::OP_NE:
		case GDScript2BinaryOp::OP_LT:
		case GDScript2BinaryOp::OP_LE:
		case GDScript2BinaryOp::OP_GT:
		case GDScript2BinaryOp::OP_GE:
		case GDScript2BinaryOp::OP_IN:
			return GDScript2Type::make_bool();

		// Logical operators return bool
		case GDScript2BinaryOp::OP_AND:
		case GDScript2BinaryOp::OP_OR:
			return GDScript2Type::make_bool();

		// Arithmetic operators
		case GDScript2BinaryOp::OP_ADD:
		case GDScript2BinaryOp::OP_SUB:
		case GDScript2BinaryOp::OP_MUL:
			// String + String = String
			if (p_left.kind == GDScript2TypeKind::TYPE_STRING && p_right.kind == GDScript2TypeKind::TYPE_STRING) {
				return GDScript2Type::make_string();
			}
			// Numeric promotion
			if (p_left.is_numeric() && p_right.is_numeric()) {
				if (p_left.kind == GDScript2TypeKind::TYPE_FLOAT || p_right.kind == GDScript2TypeKind::TYPE_FLOAT) {
					return GDScript2Type::make_float();
				}
				return GDScript2Type::make_int();
			}
			// Vector operations
			if (p_left.kind == GDScript2TypeKind::TYPE_VECTOR2 || p_left.kind == GDScript2TypeKind::TYPE_VECTOR3) {
				return p_left;
			}
			break;

		case GDScript2BinaryOp::OP_DIV:
			// Division always produces float for numeric types
			if (p_left.is_numeric() && p_right.is_numeric()) {
				return GDScript2Type::make_float();
			}
			break;

		case GDScript2BinaryOp::OP_MOD:
			// Modulo preserves integer type
			if (p_left.kind == GDScript2TypeKind::TYPE_INT && p_right.kind == GDScript2TypeKind::TYPE_INT) {
				return GDScript2Type::make_int();
			}
			if (p_left.is_numeric() && p_right.is_numeric()) {
				return GDScript2Type::make_float();
			}
			break;

		case GDScript2BinaryOp::OP_POW:
			// Power operator
			if (p_left.is_numeric() && p_right.is_numeric()) {
				return GDScript2Type::make_float();
			}
			break;

		// Bitwise operators return int
		case GDScript2BinaryOp::OP_BIT_AND:
		case GDScript2BinaryOp::OP_BIT_OR:
		case GDScript2BinaryOp::OP_BIT_XOR:
		case GDScript2BinaryOp::OP_BIT_LSH:
		case GDScript2BinaryOp::OP_BIT_RSH:
			return GDScript2Type::make_int();

		case GDScript2BinaryOp::OP_RANGE:
			// Range operator creates an iterable
			return GDScript2Type::make_array();

		default:
			break;
	}

	return GDScript2Type::make_variant();
}

GDScript2Type GDScript2SemanticAnalyzer::infer_unary_op_type(GDScript2UnaryOp p_op, const GDScript2Type &p_operand, GDScript2ASTNode *p_node) {
	switch (p_op) {
		case GDScript2UnaryOp::OP_NEG:
		case GDScript2UnaryOp::OP_POS:
			// Preserve numeric type
			if (p_operand.is_numeric()) {
				return p_operand;
			}
			// Vector types
			if (p_operand.kind == GDScript2TypeKind::TYPE_VECTOR2 ||
					p_operand.kind == GDScript2TypeKind::TYPE_VECTOR3 ||
					p_operand.kind == GDScript2TypeKind::TYPE_VECTOR4) {
				return p_operand;
			}
			break;

		case GDScript2UnaryOp::OP_NOT:
			return GDScript2Type::make_bool();

		case GDScript2UnaryOp::OP_BIT_NOT:
			return GDScript2Type::make_int();

		default:
			break;
	}

	return GDScript2Type::make_variant();
}

GDScript2Type GDScript2SemanticAnalyzer::infer_subscript_type(const GDScript2Type &p_base, const GDScript2Type &p_index, GDScript2ASTNode *p_node) {
	switch (p_base.kind) {
		case GDScript2TypeKind::TYPE_ARRAY:
			if (p_base.element_type) {
				return *p_base.element_type;
			}
			return GDScript2Type::make_variant();

		case GDScript2TypeKind::TYPE_DICTIONARY:
			if (p_base.value_type) {
				return *p_base.value_type;
			}
			return GDScript2Type::make_variant();

		case GDScript2TypeKind::TYPE_STRING:
			return GDScript2Type::make_string();

		case GDScript2TypeKind::TYPE_PACKED_BYTE_ARRAY:
		case GDScript2TypeKind::TYPE_PACKED_INT32_ARRAY:
		case GDScript2TypeKind::TYPE_PACKED_INT64_ARRAY:
			return GDScript2Type::make_int();

		case GDScript2TypeKind::TYPE_PACKED_FLOAT32_ARRAY:
		case GDScript2TypeKind::TYPE_PACKED_FLOAT64_ARRAY:
			return GDScript2Type::make_float();

		case GDScript2TypeKind::TYPE_PACKED_STRING_ARRAY:
			return GDScript2Type::make_string();

		case GDScript2TypeKind::TYPE_PACKED_VECTOR2_ARRAY:
			return GDScript2Type::make_vector2();

		case GDScript2TypeKind::TYPE_PACKED_VECTOR3_ARRAY:
			return GDScript2Type::make_vector3();

		case GDScript2TypeKind::TYPE_PACKED_VECTOR4_ARRAY:
			return GDScript2Type::make_vector4();

		case GDScript2TypeKind::TYPE_PACKED_COLOR_ARRAY:
			return GDScript2Type::make_color();

		case GDScript2TypeKind::TYPE_VECTOR2:
		case GDScript2TypeKind::TYPE_VECTOR3:
		case GDScript2TypeKind::TYPE_VECTOR4:
			return GDScript2Type::make_float();

		case GDScript2TypeKind::TYPE_VECTOR2I:
		case GDScript2TypeKind::TYPE_VECTOR3I:
		case GDScript2TypeKind::TYPE_VECTOR4I:
			return GDScript2Type::make_int();

		case GDScript2TypeKind::TYPE_COLOR:
			return GDScript2Type::make_float();

		default:
			break;
	}

	return GDScript2Type::make_variant();
}

GDScript2Type GDScript2SemanticAnalyzer::infer_attribute_type(const GDScript2Type &p_base, const StringName &p_attribute, GDScript2ASTNode *p_node) {
	// For objects, try to look up property type from ClassDB
	if (p_base.is_object() && p_base.class_name != StringName()) {
		// Check if it's a property
		PropertyInfo prop_info;
		bool found = false;
		StringName class_name = p_base.class_name;

		while (class_name != StringName() && !found) {
			List<PropertyInfo> properties;
			ClassDB::get_property_list(class_name, &properties, true);
			for (const PropertyInfo &prop : properties) {
				if (prop.name == p_attribute) {
					prop_info = prop;
					found = true;
					break;
				}
			}
			if (!found) {
				class_name = ClassDB::get_parent_class(class_name);
			}
		}

		if (found) {
			return GDScript2Type::from_variant_type(prop_info.type);
		}

		// Check if it's a method (return Callable)
		if (ClassDB::has_method(p_base.class_name, p_attribute)) {
			return GDScript2Type::make_callable();
		}
	}

	// Check common properties on built-in types
	switch (p_base.kind) {
		case GDScript2TypeKind::TYPE_VECTOR2:
		case GDScript2TypeKind::TYPE_VECTOR2I:
			if (p_attribute == "x" || p_attribute == "y") {
				return p_base.kind == GDScript2TypeKind::TYPE_VECTOR2I ? GDScript2Type::make_int() : GDScript2Type::make_float();
			}
			break;

		case GDScript2TypeKind::TYPE_VECTOR3:
		case GDScript2TypeKind::TYPE_VECTOR3I:
			if (p_attribute == "x" || p_attribute == "y" || p_attribute == "z") {
				return p_base.kind == GDScript2TypeKind::TYPE_VECTOR3I ? GDScript2Type::make_int() : GDScript2Type::make_float();
			}
			break;

		case GDScript2TypeKind::TYPE_COLOR:
			if (p_attribute == "r" || p_attribute == "g" || p_attribute == "b" || p_attribute == "a") {
				return GDScript2Type::make_float();
			}
			break;

		case GDScript2TypeKind::TYPE_RECT2:
		case GDScript2TypeKind::TYPE_RECT2I:
			if (p_attribute == "position" || p_attribute == "size") {
				return p_base.kind == GDScript2TypeKind::TYPE_RECT2I ? GDScript2Type::make_vector2i() : GDScript2Type::make_vector2();
			}
			break;

		default:
			break;
	}

	return GDScript2Type::make_variant();
}

GDScript2Type GDScript2SemanticAnalyzer::infer_call_return_type(const GDScript2Type &p_callee, const LocalVector<GDScript2Type> &p_arg_types, GDScript2ASTNode *p_node) {
	// Check function signature
	if (p_callee.kind == GDScript2TypeKind::TYPE_FUNC && p_callee.func_signature) {
		if (p_callee.func_signature->return_type) {
			return *p_callee.func_signature->return_type;
		}
	}

	// For metatype (class/type), this is a constructor call
	if (p_callee.kind == GDScript2TypeKind::TYPE_METATYPE) {
		if (p_callee.meta_type) {
			return *p_callee.meta_type;
		}
		if (p_callee.class_name != StringName()) {
			return GDScript2Type::make_object(p_callee.class_name);
		}
	}

	return GDScript2Type::make_variant();
}

// ============================================================================
// Validation Helpers
// ============================================================================

bool GDScript2SemanticAnalyzer::check_type_compatible(const GDScript2Type &p_target, const GDScript2Type &p_source, GDScript2ASTNode *p_node, bool p_allow_implicit) {
	if (GDScript2TypeChecker::is_type_compatible(p_target, p_source, p_allow_implicit)) {
		return true;
	}

	push_error(GDScript2DiagnosticCode::ERR_TYPE_MISMATCH,
			vformat("Cannot assign value of type '%s' to variable of type '%s'.",
					p_source.to_string(), p_target.to_string()),
			p_node);
	return false;
}

bool GDScript2SemanticAnalyzer::validate_assignment_target(GDScript2ASTNode *p_target) {
	if (!p_target) {
		return false;
	}

	switch (p_target->type) {
		case GDScript2ASTNodeType::NODE_IDENTIFIER: {
			GDScript2IdentifierNode *id = static_cast<GDScript2IdentifierNode *>(p_target);
			GDScript2Symbol *sym = result->symbol_table->lookup(id->name);
			if (sym) {
				if (sym->type.is_constant) {
					push_error(GDScript2DiagnosticCode::ERR_CONSTANT_ASSIGNMENT,
							vformat("Cannot assign to constant '%s'.", String(id->name)), p_target);
					return false;
				}
				if (sym->type.is_read_only) {
					push_error(GDScript2DiagnosticCode::ERR_READONLY_ASSIGNMENT,
							vformat("Cannot assign to read-only property '%s'.", String(id->name)), p_target);
					return false;
				}
			}
			return true;
		}

		case GDScript2ASTNodeType::NODE_SUBSCRIPT:
		case GDScript2ASTNodeType::NODE_ATTRIBUTE:
			return true;

		default:
			push_error(GDScript2DiagnosticCode::ERR_INVALID_DECLARATION,
					"Invalid assignment target.", p_target);
			return false;
	}
}

bool GDScript2SemanticAnalyzer::validate_call_arguments(GDScript2FunctionSignature *p_sig, const LocalVector<GDScript2ASTNode *> &p_args, GDScript2ASTNode *p_call) {
	if (!p_sig) {
		return true;
	}

	int required_args = (int)p_sig->parameter_types.size() - p_sig->default_arg_count;

	if ((int)p_args.size() < required_args) {
		push_error(GDScript2DiagnosticCode::ERR_WRONG_ARG_COUNT,
				vformat("Too few arguments: expected at least %d, got %d.", required_args, p_args.size()), p_call);
		return false;
	}

	if (!p_sig->is_vararg && (int)p_args.size() > (int)p_sig->parameter_types.size()) {
		push_error(GDScript2DiagnosticCode::ERR_WRONG_ARG_COUNT,
				vformat("Too many arguments: expected at most %d, got %d.", p_sig->parameter_types.size(), p_args.size()), p_call);
		return false;
	}

	return true;
}

// ============================================================================
// Constant Evaluation
// ============================================================================

bool GDScript2SemanticAnalyzer::try_evaluate_constant(GDScript2ASTNode *p_expr, Variant &r_value) {
	if (!p_expr) {
		return false;
	}

	// Check if we already analyzed this and it has a constant value
	if (result) {
		const GDScript2AnalyzedType *analyzed = result->node_types.getptr(p_expr);
		if (analyzed && analyzed->has_constant_value) {
			r_value = analyzed->constant_value;
			return true;
		}
	}

	switch (p_expr->type) {
		case GDScript2ASTNodeType::NODE_LITERAL: {
			GDScript2LiteralNode *lit = static_cast<GDScript2LiteralNode *>(p_expr);
			r_value = lit->value;
			return true;
		}

		case GDScript2ASTNodeType::NODE_IDENTIFIER: {
			// Check if it's a constant
			GDScript2IdentifierNode *id = static_cast<GDScript2IdentifierNode *>(p_expr);
			GDScript2Symbol *sym = result ? result->symbol_table->lookup(id->name) : nullptr;
			if (sym && sym->kind == GDScript2SymbolKind::SYMBOL_CONSTANT) {
				// TODO: Get actual constant value
				return false;
			}
			if (sym && sym->kind == GDScript2SymbolKind::SYMBOL_ENUM_VALUE) {
				r_value = sym->enum_value;
				return true;
			}
			return false;
		}

		case GDScript2ASTNodeType::NODE_UNARY_OP: {
			GDScript2UnaryOpNode *unary = static_cast<GDScript2UnaryOpNode *>(p_expr);
			Variant operand;
			if (!try_evaluate_constant(unary->operand, operand)) {
				return false;
			}

			bool valid = false;
			switch (unary->op) {
				case GDScript2UnaryOp::OP_NEG:
					Variant::evaluate(Variant::OP_NEGATE, operand, Variant(), r_value, valid);
					break;
				case GDScript2UnaryOp::OP_NOT:
					r_value = !operand.booleanize();
					valid = true;
					break;
				case GDScript2UnaryOp::OP_BIT_NOT:
					Variant::evaluate(Variant::OP_BIT_NEGATE, operand, Variant(), r_value, valid);
					break;
				default:
					break;
			}
			return valid;
		}

		case GDScript2ASTNodeType::NODE_BINARY_OP: {
			GDScript2BinaryOpNode *binary = static_cast<GDScript2BinaryOpNode *>(p_expr);
			Variant left, right;
			if (!try_evaluate_constant(binary->left, left) || !try_evaluate_constant(binary->right, right)) {
				return false;
			}

			bool valid = false;
			Variant::Operator var_op;

			switch (binary->op) {
				case GDScript2BinaryOp::OP_ADD:
					var_op = Variant::OP_ADD;
					break;
				case GDScript2BinaryOp::OP_SUB:
					var_op = Variant::OP_SUBTRACT;
					break;
				case GDScript2BinaryOp::OP_MUL:
					var_op = Variant::OP_MULTIPLY;
					break;
				case GDScript2BinaryOp::OP_DIV:
					var_op = Variant::OP_DIVIDE;
					break;
				case GDScript2BinaryOp::OP_MOD:
					var_op = Variant::OP_MODULE;
					break;
				case GDScript2BinaryOp::OP_POW:
					var_op = Variant::OP_POWER;
					break;
				case GDScript2BinaryOp::OP_EQ:
					var_op = Variant::OP_EQUAL;
					break;
				case GDScript2BinaryOp::OP_NE:
					var_op = Variant::OP_NOT_EQUAL;
					break;
				case GDScript2BinaryOp::OP_LT:
					var_op = Variant::OP_LESS;
					break;
				case GDScript2BinaryOp::OP_LE:
					var_op = Variant::OP_LESS_EQUAL;
					break;
				case GDScript2BinaryOp::OP_GT:
					var_op = Variant::OP_GREATER;
					break;
				case GDScript2BinaryOp::OP_GE:
					var_op = Variant::OP_GREATER_EQUAL;
					break;
				case GDScript2BinaryOp::OP_AND:
					r_value = left.booleanize() && right.booleanize();
					return true;
				case GDScript2BinaryOp::OP_OR:
					r_value = left.booleanize() || right.booleanize();
					return true;
				case GDScript2BinaryOp::OP_BIT_AND:
					var_op = Variant::OP_BIT_AND;
					break;
				case GDScript2BinaryOp::OP_BIT_OR:
					var_op = Variant::OP_BIT_OR;
					break;
				case GDScript2BinaryOp::OP_BIT_XOR:
					var_op = Variant::OP_BIT_XOR;
					break;
				case GDScript2BinaryOp::OP_BIT_LSH:
					var_op = Variant::OP_SHIFT_LEFT;
					break;
				case GDScript2BinaryOp::OP_BIT_RSH:
					var_op = Variant::OP_SHIFT_RIGHT;
					break;
				default:
					return false;
			}

			Variant::evaluate(var_op, left, right, r_value, valid);
			return valid;
		}

		default:
			return false;
	}
}
