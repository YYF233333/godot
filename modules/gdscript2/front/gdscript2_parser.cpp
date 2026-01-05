/**************************************************************************/
/*  gdscript2_parser.cpp                                                  */
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

#include "gdscript2_parser.h"

#include "core/math/math_funcs.h"

// ============================================================================
// Token Handling
// ============================================================================

void GDScript2Parser::advance() {
	previous = current;
	for (;;) {
		current = tokenizer.scan();
		if (current.type != GDScript2TokenType::TK_ERROR) {
			break;
		}
		error_at_current(String(current.literal));
	}
}

bool GDScript2Parser::check(GDScript2TokenType p_type) const {
	return current.type == p_type;
}

bool GDScript2Parser::match(GDScript2TokenType p_type) {
	if (!check(p_type)) {
		return false;
	}
	advance();
	return true;
}

void GDScript2Parser::consume(GDScript2TokenType p_type, const String &p_message) {
	if (current.type == p_type) {
		advance();
		return;
	}
	error_at_current(p_message);
}

bool GDScript2Parser::is_at_end() const {
	return current.type == GDScript2TokenType::TK_EOF;
}

// ============================================================================
// Error Handling
// ============================================================================

void GDScript2Parser::error(const String &p_message) {
	Error err;
	err.message = p_message;
	err.line = previous.end_line;
	err.column = previous.end_column;
	errors.push_back(err);
	had_error = true;
	panic_mode = true;
}

void GDScript2Parser::error_at_current(const String &p_message) {
	if (panic_mode) {
		return;
	}
	Error err;
	err.message = p_message;
	err.line = current.start_line;
	err.column = current.start_column;
	errors.push_back(err);
	had_error = true;
	panic_mode = true;
}

void GDScript2Parser::synchronize() {
	panic_mode = false;

	while (!is_at_end()) {
		if (previous.type == GDScript2TokenType::TK_NEWLINE) {
			return;
		}

		switch (current.type) {
			case GDScript2TokenType::TK_CLASS:
			case GDScript2TokenType::TK_FUNC:
			case GDScript2TokenType::TK_VAR:
			case GDScript2TokenType::TK_CONST:
			case GDScript2TokenType::TK_SIGNAL:
			case GDScript2TokenType::TK_ENUM:
			case GDScript2TokenType::TK_IF:
			case GDScript2TokenType::TK_FOR:
			case GDScript2TokenType::TK_WHILE:
			case GDScript2TokenType::TK_MATCH:
			case GDScript2TokenType::TK_RETURN:
				return;
			default:
				break;
		}

		advance();
	}
}

// ============================================================================
// Helper Methods
// ============================================================================

void GDScript2Parser::set_node_start(GDScript2ASTNode *p_node, const GDScript2Token &p_token) {
	p_node->start_line = p_token.start_line;
	p_node->start_column = p_token.start_column;
}

void GDScript2Parser::set_node_end(GDScript2ASTNode *p_node, const GDScript2Token &p_token) {
	p_node->end_line = p_token.end_line;
	p_node->end_column = p_token.end_column;
}

// ============================================================================
// Main Parse Function
// ============================================================================

GDScript2Parser::Result GDScript2Parser::parse(const String &p_source, const Options &p_options) {
	(void)p_options;

	Result result;
	errors.clear();
	panic_mode = false;
	had_error = false;

	tokenizer.set_source_code(p_source);
	advance();

	result.root = parse_class(false);
	result.errors = errors;

	return result;
}

// ============================================================================
// Top Level Parsing
// ============================================================================

GDScript2ClassNode *GDScript2Parser::parse_class(bool p_is_inner) {
	GDScript2ClassNode *cls = memnew(GDScript2ClassNode);
	cls->is_inner_class = p_is_inner;
	set_node_start(cls, current);

	if (p_is_inner) {
		// Inner class: class ClassName:
		consume(GDScript2TokenType::TK_CLASS, "Expected 'class'.");
		consume(GDScript2TokenType::TK_IDENTIFIER, "Expected class name.");
		cls->name = previous.literal;

		if (match(GDScript2TokenType::TK_EXTENDS)) {
			consume(GDScript2TokenType::TK_IDENTIFIER, "Expected base class name.");
			cls->extends_path = previous.literal;
		}

		consume(GDScript2TokenType::TK_COLON, "Expected ':' after class declaration.");
		consume(GDScript2TokenType::TK_NEWLINE, "Expected newline after ':'.");
		consume(GDScript2TokenType::TK_INDENT, "Expected indented block.");

		parse_class_body(cls);

		consume(GDScript2TokenType::TK_DEDENT, "Expected end of class body.");
	} else {
		// Top-level class (script file)
		parse_class_body(cls);
	}

	set_node_end(cls, previous);
	return cls;
}

void GDScript2Parser::parse_class_body(GDScript2ClassNode *p_class) {
	LocalVector<GDScript2AnnotationNode *> pending_annotations;

	int iteration_count = 0;
	const int MAX_ITERATIONS = 10000;
	GDScript2Token last_token = current;

	while (!is_at_end()) {
		iteration_count++;
		if (iteration_count > MAX_ITERATIONS) {
			error("Too many iterations in parse_class_body, possible infinite loop.");
			break;
		}

		// Check for dedent (end of inner class)
		if (p_class->is_inner_class && check(GDScript2TokenType::TK_DEDENT)) {
			break;
		}

		// Skip newlines
		while (match(GDScript2TokenType::TK_NEWLINE)) {
			// Skip empty lines
		}

		if (is_at_end()) {
			break;
		}
		if (p_class->is_inner_class && check(GDScript2TokenType::TK_DEDENT)) {
			break;
		}

		// Safety: track token position to detect stuck parsing
		GDScript2Token before_parse = current;

		// Parse annotations
		if (check(GDScript2TokenType::TK_ANNOTATION)) {
			GDScript2AnnotationNode *annotation = parse_annotation();
			if (annotation) {
				pending_annotations.push_back(annotation);
			}
			continue;
		}

		// Parse class-level declarations
		if (match(GDScript2TokenType::TK_CLASS_NAME)) {
			consume(GDScript2TokenType::TK_IDENTIFIER, "Expected class name after 'class_name'.");
			p_class->class_name = previous.literal;
			match(GDScript2TokenType::TK_NEWLINE);
			continue;
		}

		if (match(GDScript2TokenType::TK_EXTENDS)) {
			if (check(GDScript2TokenType::TK_IDENTIFIER)) {
				advance();
				p_class->extends_path = previous.literal;
			} else if (check(GDScript2TokenType::TK_LITERAL)) {
				advance();
				p_class->extends_path = String(previous.literal);
			} else {
				error_at_current("Expected class name or path after 'extends'.");
			}
			match(GDScript2TokenType::TK_NEWLINE);
			continue;
		}

		if (check(GDScript2TokenType::TK_CLASS)) {
			GDScript2ClassNode *inner = parse_class(true);
			if (inner) {
				p_class->inner_classes.push_back(inner);
			}
			continue;
		}

		bool is_static = match(GDScript2TokenType::TK_STATIC);

		if (check(GDScript2TokenType::TK_FUNC)) {
			GDScript2FunctionNode *func = parse_function(is_static);
			if (func) {
				for (GDScript2AnnotationNode *a : pending_annotations) {
					func->annotations.push_back(a);
				}
				pending_annotations.clear();
				p_class->functions.push_back(func);
			}
			continue;
		}

		if (check(GDScript2TokenType::TK_VAR)) {
			GDScript2VariableNode *var = parse_variable(is_static);
			if (var) {
				for (GDScript2AnnotationNode *a : pending_annotations) {
					var->annotations.push_back(a);
				}
				pending_annotations.clear();
				p_class->variables.push_back(var);
			}
			continue;
		}

		if (is_static) {
			error("'static' can only be used with 'func' or 'var'.");
			// Force advance to prevent infinite loop
			if (!is_at_end()) {
				advance();
			}
			continue;
		}

		if (check(GDScript2TokenType::TK_CONST)) {
			GDScript2ConstantNode *constant = parse_constant();
			if (constant) {
				p_class->constants.push_back(constant);
			}
			// Clear any pending annotations (constants don't use them typically)
			for (GDScript2AnnotationNode *a : pending_annotations) {
				memdelete(a);
			}
			pending_annotations.clear();
			continue;
		}

		if (check(GDScript2TokenType::TK_SIGNAL)) {
			GDScript2SignalNode *sig = parse_signal();
			if (sig) {
				p_class->signals.push_back(sig);
			}
			for (GDScript2AnnotationNode *a : pending_annotations) {
				memdelete(a);
			}
			pending_annotations.clear();
			continue;
		}

		if (check(GDScript2TokenType::TK_ENUM)) {
			GDScript2EnumNode *en = parse_enum();
			if (en) {
				p_class->enums.push_back(en);
			}
			for (GDScript2AnnotationNode *a : pending_annotations) {
				memdelete(a);
			}
			pending_annotations.clear();
			continue;
		}

		// Unexpected token
		// Special handling for indent/dedent tokens - they indicate structure issues
		if (current.type == GDScript2TokenType::TK_INDENT ||
				current.type == GDScript2TokenType::TK_DEDENT) {
			// Skip these tokens as they're likely leftover from failed parse
			advance();
			continue;
		}

		error_at_current("Expected class member declaration.");
		synchronize();

		// Safety: if token didn't advance, force progress to avoid infinite loop
		if (before_parse.type == current.type &&
				before_parse.start_line == current.start_line &&
				before_parse.start_column == current.start_column) {
			// Still stuck - force one token advance
			advance();
		}
	}

	// Clean up any leftover annotations
	for (GDScript2AnnotationNode *a : pending_annotations) {
		memdelete(a);
	}
}

GDScript2AnnotationNode *GDScript2Parser::parse_annotation() {
	GDScript2AnnotationNode *annotation = memnew(GDScript2AnnotationNode);
	set_node_start(annotation, current);

	advance(); // consume @annotation
	annotation->name = previous.literal;

	// Parse arguments if present
	if (match(GDScript2TokenType::TK_PARENTHESIS_OPEN)) {
		tokenizer.set_multiline_mode(true);

		if (!check(GDScript2TokenType::TK_PARENTHESIS_CLOSE)) {
			do {
				GDScript2ASTNode *arg = parse_expression();
				if (arg) {
					annotation->arguments.push_back(arg);
				}
			} while (match(GDScript2TokenType::TK_COMMA));
		}

		tokenizer.set_multiline_mode(false);
		consume(GDScript2TokenType::TK_PARENTHESIS_CLOSE, "Expected ')' after annotation arguments.");
	}

	match(GDScript2TokenType::TK_NEWLINE);
	set_node_end(annotation, previous);
	return annotation;
}

GDScript2FunctionNode *GDScript2Parser::parse_function(bool p_is_static) {
	GDScript2FunctionNode *func = memnew(GDScript2FunctionNode);
	func->is_static = p_is_static;
	set_node_start(func, current);

	consume(GDScript2TokenType::TK_FUNC, "Expected 'func'.");
	consume(GDScript2TokenType::TK_IDENTIFIER, "Expected function name.");
	func->name = previous.literal;

	// Parameters
	consume(GDScript2TokenType::TK_PARENTHESIS_OPEN, "Expected '(' after function name.");
	func->parameters = parse_parameter_list();
	consume(GDScript2TokenType::TK_PARENTHESIS_CLOSE, "Expected ')' after parameters.");

	// Return type
	if (match(GDScript2TokenType::TK_FORWARD_ARROW)) {
		func->return_type = parse_type();
	}

	consume(GDScript2TokenType::TK_COLON, "Expected ':' after function signature.");
	match(GDScript2TokenType::TK_NEWLINE);

	// Body
	func->body = parse_suite();

	set_node_end(func, previous);
	return func;
}

GDScript2VariableNode *GDScript2Parser::parse_variable(bool p_is_static) {
	GDScript2VariableNode *var = memnew(GDScript2VariableNode);
	var->is_static = p_is_static;
	set_node_start(var, current);

	consume(GDScript2TokenType::TK_VAR, "Expected 'var'.");
	consume(GDScript2TokenType::TK_IDENTIFIER, "Expected variable name.");
	var->name = previous.literal;

	// Type hint
	if (match(GDScript2TokenType::TK_COLON)) {
		if (!check(GDScript2TokenType::TK_EQUAL)) {
			var->type_hint = parse_type();
		}
	}

	// Initializer
	if (match(GDScript2TokenType::TK_EQUAL)) {
		var->initializer = parse_expression();
	}

	match(GDScript2TokenType::TK_NEWLINE);
	set_node_end(var, previous);
	return var;
}

GDScript2ConstantNode *GDScript2Parser::parse_constant() {
	GDScript2ConstantNode *constant = memnew(GDScript2ConstantNode);
	set_node_start(constant, current);

	consume(GDScript2TokenType::TK_CONST, "Expected 'const'.");
	consume(GDScript2TokenType::TK_IDENTIFIER, "Expected constant name.");
	constant->name = previous.literal;

	// Type hint
	if (match(GDScript2TokenType::TK_COLON)) {
		if (!check(GDScript2TokenType::TK_EQUAL)) {
			constant->type_hint = parse_type();
		}
	}

	// Initializer (required for constants)
	consume(GDScript2TokenType::TK_EQUAL, "Expected '=' after constant name.");
	constant->initializer = parse_expression();

	match(GDScript2TokenType::TK_NEWLINE);
	set_node_end(constant, previous);
	return constant;
}

GDScript2SignalNode *GDScript2Parser::parse_signal() {
	GDScript2SignalNode *sig = memnew(GDScript2SignalNode);
	set_node_start(sig, current);

	consume(GDScript2TokenType::TK_SIGNAL, "Expected 'signal'.");
	consume(GDScript2TokenType::TK_IDENTIFIER, "Expected signal name.");
	sig->name = previous.literal;

	// Parameters (optional)
	if (match(GDScript2TokenType::TK_PARENTHESIS_OPEN)) {
		sig->parameters = parse_parameter_list();
		consume(GDScript2TokenType::TK_PARENTHESIS_CLOSE, "Expected ')' after signal parameters.");
	}

	match(GDScript2TokenType::TK_NEWLINE);
	set_node_end(sig, previous);
	return sig;
}

GDScript2EnumNode *GDScript2Parser::parse_enum() {
	GDScript2EnumNode *en = memnew(GDScript2EnumNode);
	set_node_start(en, current);

	consume(GDScript2TokenType::TK_ENUM, "Expected 'enum'.");

	// Named enum (optional)
	if (check(GDScript2TokenType::TK_IDENTIFIER)) {
		advance();
		en->name = previous.literal;
	}

	consume(GDScript2TokenType::TK_BRACE_OPEN, "Expected '{' after enum name.");
	tokenizer.set_multiline_mode(true);

	// Parse enum values
	while (!check(GDScript2TokenType::TK_BRACE_CLOSE) && !is_at_end()) {
		consume(GDScript2TokenType::TK_IDENTIFIER, "Expected enum value name.");
		en->value_names.push_back(previous.literal);

		GDScript2ASTNode *value_expr = nullptr;
		if (match(GDScript2TokenType::TK_EQUAL)) {
			value_expr = parse_expression();
		}
		en->value_expressions.push_back(value_expr);

		if (!check(GDScript2TokenType::TK_BRACE_CLOSE)) {
			consume(GDScript2TokenType::TK_COMMA, "Expected ',' between enum values.");
		}
	}

	tokenizer.set_multiline_mode(false);
	consume(GDScript2TokenType::TK_BRACE_CLOSE, "Expected '}' after enum values.");
	match(GDScript2TokenType::TK_NEWLINE);

	set_node_end(en, previous);
	return en;
}

// ============================================================================
// Types and Parameters
// ============================================================================

GDScript2TypeNode *GDScript2Parser::parse_type() {
	GDScript2TypeNode *type_node = memnew(GDScript2TypeNode);
	set_node_start(type_node, current);

	// Handle void keyword
	if (match(GDScript2TokenType::TK_VOID)) {
		type_node->type_name = "void";
		set_node_end(type_node, previous);
		return type_node;
	}

	if (!check(GDScript2TokenType::TK_IDENTIFIER)) {
		error_at_current("Expected type name.");
		// Return a dummy type node to avoid null pointer issues
		type_node->type_name = "Variant";
		set_node_end(type_node, current);
		return type_node;
	}

	consume(GDScript2TokenType::TK_IDENTIFIER, "Expected type name.");
	type_node->type_name = previous.literal;

	// Container types: Array[T], Dictionary[K, V]
	if (match(GDScript2TokenType::TK_BRACKET_OPEN)) {
		do {
			GDScript2TypeNode *elem_type = parse_type();
			if (elem_type) {
				type_node->container_types.push_back(elem_type);
			}
		} while (match(GDScript2TokenType::TK_COMMA));

		consume(GDScript2TokenType::TK_BRACKET_CLOSE, "Expected ']' after type parameters.");
	}

	set_node_end(type_node, previous);
	return type_node;
}

GDScript2ParameterNode *GDScript2Parser::parse_parameter() {
	GDScript2ParameterNode *param = memnew(GDScript2ParameterNode);
	set_node_start(param, current);

	consume(GDScript2TokenType::TK_IDENTIFIER, "Expected parameter name.");
	param->name = previous.literal;

	// Type hint
	if (match(GDScript2TokenType::TK_COLON)) {
		param->type_hint = parse_type();
	}

	// Default value
	if (match(GDScript2TokenType::TK_EQUAL)) {
		param->default_value = parse_expression();
	}

	set_node_end(param, previous);
	return param;
}

LocalVector<GDScript2ParameterNode *> GDScript2Parser::parse_parameter_list() {
	LocalVector<GDScript2ParameterNode *> params;

	if (check(GDScript2TokenType::TK_PARENTHESIS_CLOSE)) {
		return params;
	}

	do {
		GDScript2ParameterNode *param = parse_parameter();
		if (param) {
			params.push_back(param);
		}
	} while (match(GDScript2TokenType::TK_COMMA));

	return params;
}

// ============================================================================
// Statements
// ============================================================================

GDScript2SuiteNode *GDScript2Parser::parse_suite() {
	GDScript2SuiteNode *suite = memnew(GDScript2SuiteNode);
	set_node_start(suite, current);

	if (match(GDScript2TokenType::TK_INDENT)) {
		int iteration = 0;
		const int MAX_ITERATIONS = 10000;

		while (!check(GDScript2TokenType::TK_DEDENT) && !is_at_end()) {
			iteration++;
			if (iteration > MAX_ITERATIONS) {
				error("Too many iterations in parse_suite, possible infinite loop.");
				break;
			}

			// Skip empty lines
			while (match(GDScript2TokenType::TK_NEWLINE)) {
			}

			if (check(GDScript2TokenType::TK_DEDENT) || is_at_end()) {
				break;
			}

			GDScript2ASTNode *stmt = parse_statement();
			if (stmt) {
				suite->statements.push_back(stmt);
			}

			if (panic_mode) {
				synchronize();
			}
		}
		consume(GDScript2TokenType::TK_DEDENT, "Expected dedent.");
	} else {
		// Single-line suite (e.g., if x: pass)
		GDScript2ASTNode *stmt = parse_statement();
		if (stmt) {
			suite->statements.push_back(stmt);
		}
	}

	set_node_end(suite, previous);
	return suite;
}

GDScript2ASTNode *GDScript2Parser::parse_statement() {
	// Control flow statements
	if (check(GDScript2TokenType::TK_IF)) {
		return parse_if();
	}
	if (check(GDScript2TokenType::TK_FOR)) {
		return parse_for();
	}
	if (check(GDScript2TokenType::TK_WHILE)) {
		return parse_while();
	}
	if (check(GDScript2TokenType::TK_MATCH)) {
		return parse_match();
	}

	// Simple statements
	if (match(GDScript2TokenType::TK_PASS)) {
		GDScript2PassNode *pass = memnew(GDScript2PassNode);
		set_node_start(pass, previous);
		set_node_end(pass, previous);
		match(GDScript2TokenType::TK_NEWLINE);
		return pass;
	}

	if (match(GDScript2TokenType::TK_BREAK)) {
		GDScript2BreakNode *brk = memnew(GDScript2BreakNode);
		set_node_start(brk, previous);
		set_node_end(brk, previous);
		match(GDScript2TokenType::TK_NEWLINE);
		return brk;
	}

	if (match(GDScript2TokenType::TK_CONTINUE)) {
		GDScript2ContinueNode *cont = memnew(GDScript2ContinueNode);
		set_node_start(cont, previous);
		set_node_end(cont, previous);
		match(GDScript2TokenType::TK_NEWLINE);
		return cont;
	}

	if (check(GDScript2TokenType::TK_RETURN)) {
		return parse_return();
	}

	if (check(GDScript2TokenType::TK_ASSERT)) {
		return parse_assert();
	}

	if (match(GDScript2TokenType::TK_BREAKPOINT)) {
		GDScript2BreakpointNode *bp = memnew(GDScript2BreakpointNode);
		set_node_start(bp, previous);
		set_node_end(bp, previous);
		match(GDScript2TokenType::TK_NEWLINE);
		return bp;
	}

	// Variable declaration in local scope
	if (check(GDScript2TokenType::TK_VAR)) {
		return parse_variable(false);
	}

	// Expression statement (including assignment)
	GDScript2ASTNode *expr = parse_expression();
	match(GDScript2TokenType::TK_NEWLINE);
	return expr;
}

GDScript2IfNode *GDScript2Parser::parse_if() {
	GDScript2IfNode *if_node = memnew(GDScript2IfNode);
	set_node_start(if_node, current);

	consume(GDScript2TokenType::TK_IF, "Expected 'if'.");
	if_node->condition = parse_expression();
	consume(GDScript2TokenType::TK_COLON, "Expected ':' after if condition.");
	match(GDScript2TokenType::TK_NEWLINE);

	if_node->true_block = parse_suite();

	// elif / else
	if (match(GDScript2TokenType::TK_ELIF)) {
		// Create nested if for elif
		GDScript2IfNode *elif_node = memnew(GDScript2IfNode);
		set_node_start(elif_node, previous);

		elif_node->condition = parse_expression();
		consume(GDScript2TokenType::TK_COLON, "Expected ':' after elif condition.");
		match(GDScript2TokenType::TK_NEWLINE);
		elif_node->true_block = parse_suite();

		// Continue chain
		if (check(GDScript2TokenType::TK_ELIF) || check(GDScript2TokenType::TK_ELSE)) {
			if (match(GDScript2TokenType::TK_ELIF)) {
				// Recursively handle elif chain - put token back conceptually
				// Actually we need to handle this differently
				// For now, create a simpler version
			}
			if (match(GDScript2TokenType::TK_ELSE)) {
				consume(GDScript2TokenType::TK_COLON, "Expected ':' after else.");
				match(GDScript2TokenType::TK_NEWLINE);
				elif_node->false_block = parse_suite();
			}
		}

		set_node_end(elif_node, previous);
		if_node->false_block = elif_node;
	} else if (match(GDScript2TokenType::TK_ELSE)) {
		consume(GDScript2TokenType::TK_COLON, "Expected ':' after else.");
		match(GDScript2TokenType::TK_NEWLINE);
		if_node->false_block = parse_suite();
	}

	set_node_end(if_node, previous);
	return if_node;
}

GDScript2ForNode *GDScript2Parser::parse_for() {
	GDScript2ForNode *for_node = memnew(GDScript2ForNode);
	set_node_start(for_node, current);

	consume(GDScript2TokenType::TK_FOR, "Expected 'for'.");
	consume(GDScript2TokenType::TK_IDENTIFIER, "Expected loop variable.");
	for_node->variable = previous.literal;

	consume(GDScript2TokenType::TK_IN, "Expected 'in' after loop variable.");
	for_node->iterable = parse_expression();

	consume(GDScript2TokenType::TK_COLON, "Expected ':' after for expression.");
	match(GDScript2TokenType::TK_NEWLINE);

	for_node->body = parse_suite();

	set_node_end(for_node, previous);
	return for_node;
}

GDScript2WhileNode *GDScript2Parser::parse_while() {
	GDScript2WhileNode *while_node = memnew(GDScript2WhileNode);
	set_node_start(while_node, current);

	consume(GDScript2TokenType::TK_WHILE, "Expected 'while'.");
	while_node->condition = parse_expression();

	consume(GDScript2TokenType::TK_COLON, "Expected ':' after while condition.");
	match(GDScript2TokenType::TK_NEWLINE);

	while_node->body = parse_suite();

	set_node_end(while_node, previous);
	return while_node;
}

GDScript2MatchNode *GDScript2Parser::parse_match() {
	GDScript2MatchNode *match_node = memnew(GDScript2MatchNode);
	set_node_start(match_node, current);

	consume(GDScript2TokenType::TK_MATCH, "Expected 'match'.");
	match_node->test_value = parse_expression();

	consume(GDScript2TokenType::TK_COLON, "Expected ':' after match value.");
	consume(GDScript2TokenType::TK_NEWLINE, "Expected newline after match.");
	consume(GDScript2TokenType::TK_INDENT, "Expected indented block for match branches.");

	int iteration = 0;
	while (!check(GDScript2TokenType::TK_DEDENT) && !is_at_end()) {
		iteration++;
		if (iteration > 100) {
			error("Too many iterations in match parsing, possible infinite loop.");
			break;
		}

		while (match(GDScript2TokenType::TK_NEWLINE)) {
		}
		if (check(GDScript2TokenType::TK_DEDENT)) {
			break;
		}

		// Safety: track position before parsing branch
		GDScript2Token before_branch = current;

		GDScript2MatchBranchNode *branch = parse_match_branch();
		if (branch) {
			match_node->branches.push_back(branch);

			// Safety: if no progress was made, break to avoid infinite loop
			if (before_branch.type == current.type &&
					before_branch.source == current.source &&
					before_branch.start_line == current.start_line &&
					before_branch.start_column == current.start_column) {
				error("No progress in match branch parsing, aborting.");
				break;
			}
		} else {
			error("Failed to parse match branch.");
			break;
		}
	}

	consume(GDScript2TokenType::TK_DEDENT, "Expected end of match block.");

	set_node_end(match_node, previous);
	return match_node;
}

GDScript2MatchBranchNode *GDScript2Parser::parse_match_branch() {
	GDScript2MatchBranchNode *branch = memnew(GDScript2MatchBranchNode);
	set_node_start(branch, current);

	// Parse patterns (comma-separated)
	do {
		GDScript2Token before_pattern = current;
		GDScript2PatternNode *pattern = parse_pattern();
		if (pattern) {
			branch->patterns.push_back(pattern);
		}
		// Safety check to prevent infinite loop
		if (before_pattern.type == current.type && before_pattern.source == current.source && !pattern) {
			error("Failed to parse pattern, no progress made.");
			break;
		}
	} while (match(GDScript2TokenType::TK_COMMA));

	// Optional guard (when clause)
	if (match(GDScript2TokenType::TK_WHEN)) {
		branch->guard = parse_expression();
	}

	consume(GDScript2TokenType::TK_COLON, "Expected ':' after match pattern.");
	match(GDScript2TokenType::TK_NEWLINE);

	branch->body = parse_suite();

	set_node_end(branch, previous);
	return branch;
}

GDScript2PatternNode *GDScript2Parser::parse_pattern() {
	GDScript2PatternNode *pattern = memnew(GDScript2PatternNode);
	set_node_start(pattern, current);

	// Rest pattern (..)
	if (match(GDScript2TokenType::TK_PERIOD_PERIOD)) {
		pattern->pattern_type = GDScript2PatternType::PATTERN_REST;
		set_node_end(pattern, previous);
		return pattern;
	}

	// Wildcard pattern (_)
	if (match(GDScript2TokenType::TK_UNDERSCORE)) {
		pattern->pattern_type = GDScript2PatternType::PATTERN_EXPRESSION;
		// Create a simple identifier node for wildcard
		GDScript2IdentifierNode *wildcard = memnew(GDScript2IdentifierNode);
		wildcard->name = "_";
		pattern->expression = wildcard;
		set_node_end(pattern, previous);
		return pattern;
	}

	// Binding pattern (var name)
	if (match(GDScript2TokenType::TK_VAR)) {
		consume(GDScript2TokenType::TK_IDENTIFIER, "Expected identifier after 'var' in pattern.");
		pattern->pattern_type = GDScript2PatternType::PATTERN_BIND;
		pattern->bind_name = previous.literal;
		set_node_end(pattern, previous);
		return pattern;
	}

	// Array pattern
	if (check(GDScript2TokenType::TK_BRACKET_OPEN)) {
		advance();
		pattern->pattern_type = GDScript2PatternType::PATTERN_ARRAY;

		if (!check(GDScript2TokenType::TK_BRACKET_CLOSE)) {
			do {
				// Safety check: ensure we're making progress
				GDScript2Token before_parse = current;
				GDScript2PatternNode *elem = parse_pattern();
				if (elem) {
					pattern->array_patterns.push_back(elem);
				}
				// If no progress was made, break to avoid infinite loop
				if (before_parse.type == current.type && before_parse.source == current.source) {
					error("Failed to parse array pattern element.");
					break;
				}
			} while (match(GDScript2TokenType::TK_COMMA) && !check(GDScript2TokenType::TK_BRACKET_CLOSE));
		}

		consume(GDScript2TokenType::TK_BRACKET_CLOSE, "Expected ']' after array pattern.");
		set_node_end(pattern, previous);
		return pattern;
	}

	// Dictionary pattern
	if (check(GDScript2TokenType::TK_BRACE_OPEN)) {
		advance();
		pattern->pattern_type = GDScript2PatternType::PATTERN_DICTIONARY;

		if (!check(GDScript2TokenType::TK_BRACE_CLOSE)) {
			do {
				// Safety check: ensure we're making progress
				GDScript2Token before_parse = current;

				GDScript2ASTNode *key = parse_expression();
				pattern->dictionary_keys.push_back(key);

				consume(GDScript2TokenType::TK_COLON, "Expected ':' in dictionary pattern.");

				GDScript2PatternNode *value_pattern = parse_pattern();
				pattern->dictionary_patterns.push_back(value_pattern);

				// If no progress was made, break to avoid infinite loop
				if (before_parse.type == current.type && before_parse.source == current.source) {
					error("Failed to parse dictionary pattern element.");
					break;
				}
			} while (match(GDScript2TokenType::TK_COMMA) && !check(GDScript2TokenType::TK_BRACE_CLOSE));
		}

		consume(GDScript2TokenType::TK_BRACE_CLOSE, "Expected '}' after dictionary pattern.");
		set_node_end(pattern, previous);
		return pattern;
	}

	// Literal or expression pattern
	if (check(GDScript2TokenType::TK_LITERAL)) {
		advance();
		pattern->pattern_type = GDScript2PatternType::PATTERN_LITERAL;
		pattern->literal = previous.literal;
		set_node_end(pattern, previous);
		return pattern;
	}

	// Expression pattern (identifier, etc.)
	// Safety check: if we can't parse anything, advance at least one token to avoid infinite loop
	if (is_at_end()) {
		error("Unexpected end of file in pattern.");
		memdelete(pattern);
		return nullptr;
	}

	pattern->pattern_type = GDScript2PatternType::PATTERN_EXPRESSION;
	pattern->expression = parse_expression();

	// If parse_expression returned null, create a dummy expression to avoid null pointer issues
	if (!pattern->expression) {
		pattern->expression = memnew(GDScript2IdentifierNode);
		static_cast<GDScript2IdentifierNode *>(pattern->expression)->name = "_";
	}

	set_node_end(pattern, previous);
	return pattern;
}

GDScript2ReturnNode *GDScript2Parser::parse_return() {
	GDScript2ReturnNode *ret = memnew(GDScript2ReturnNode);
	set_node_start(ret, current);

	consume(GDScript2TokenType::TK_RETURN, "Expected 'return'.");

	if (!check(GDScript2TokenType::TK_NEWLINE) && !is_at_end()) {
		ret->value = parse_expression();
	}

	match(GDScript2TokenType::TK_NEWLINE);
	set_node_end(ret, previous);
	return ret;
}

GDScript2AssertNode *GDScript2Parser::parse_assert() {
	GDScript2AssertNode *assert_node = memnew(GDScript2AssertNode);
	set_node_start(assert_node, current);

	consume(GDScript2TokenType::TK_ASSERT, "Expected 'assert'.");
	consume(GDScript2TokenType::TK_PARENTHESIS_OPEN, "Expected '(' after 'assert'.");

	assert_node->condition = parse_expression();

	if (match(GDScript2TokenType::TK_COMMA)) {
		assert_node->message = parse_expression();
	}

	consume(GDScript2TokenType::TK_PARENTHESIS_CLOSE, "Expected ')' after assert.");
	match(GDScript2TokenType::TK_NEWLINE);

	set_node_end(assert_node, previous);
	return assert_node;
}

// ============================================================================
// Expressions (Precedence Climbing)
// ============================================================================

GDScript2ASTNode *GDScript2Parser::parse_expression() {
	return parse_assignment();
}

GDScript2ASTNode *GDScript2Parser::parse_assignment() {
	GDScript2ASTNode *expr = parse_ternary();

	if (check(GDScript2TokenType::TK_EQUAL) ||
			check(GDScript2TokenType::TK_PLUS_EQUAL) ||
			check(GDScript2TokenType::TK_MINUS_EQUAL) ||
			check(GDScript2TokenType::TK_STAR_EQUAL) ||
			check(GDScript2TokenType::TK_SLASH_EQUAL) ||
			check(GDScript2TokenType::TK_PERCENT_EQUAL) ||
			check(GDScript2TokenType::TK_STAR_STAR_EQUAL) ||
			check(GDScript2TokenType::TK_LESS_LESS_EQUAL) ||
			check(GDScript2TokenType::TK_GREATER_GREATER_EQUAL) ||
			check(GDScript2TokenType::TK_AMPERSAND_EQUAL) ||
			check(GDScript2TokenType::TK_PIPE_EQUAL) ||
			check(GDScript2TokenType::TK_CARET_EQUAL)) {
		GDScript2Token op_token = current;
		advance();

		GDScript2AssignmentNode *assign = memnew(GDScript2AssignmentNode);
		assign->start_line = expr->start_line;
		assign->start_column = expr->start_column;
		assign->target = expr;

		switch (op_token.type) {
			case GDScript2TokenType::TK_EQUAL:
				assign->op = GDScript2AssignOp::OP_ASSIGN;
				break;
			case GDScript2TokenType::TK_PLUS_EQUAL:
				assign->op = GDScript2AssignOp::OP_ADD_ASSIGN;
				break;
			case GDScript2TokenType::TK_MINUS_EQUAL:
				assign->op = GDScript2AssignOp::OP_SUB_ASSIGN;
				break;
			case GDScript2TokenType::TK_STAR_EQUAL:
				assign->op = GDScript2AssignOp::OP_MUL_ASSIGN;
				break;
			case GDScript2TokenType::TK_SLASH_EQUAL:
				assign->op = GDScript2AssignOp::OP_DIV_ASSIGN;
				break;
			case GDScript2TokenType::TK_PERCENT_EQUAL:
				assign->op = GDScript2AssignOp::OP_MOD_ASSIGN;
				break;
			case GDScript2TokenType::TK_STAR_STAR_EQUAL:
				assign->op = GDScript2AssignOp::OP_POW_ASSIGN;
				break;
			case GDScript2TokenType::TK_LESS_LESS_EQUAL:
				assign->op = GDScript2AssignOp::OP_LSH_ASSIGN;
				break;
			case GDScript2TokenType::TK_GREATER_GREATER_EQUAL:
				assign->op = GDScript2AssignOp::OP_RSH_ASSIGN;
				break;
			case GDScript2TokenType::TK_AMPERSAND_EQUAL:
				assign->op = GDScript2AssignOp::OP_AND_ASSIGN;
				break;
			case GDScript2TokenType::TK_PIPE_EQUAL:
				assign->op = GDScript2AssignOp::OP_OR_ASSIGN;
				break;
			case GDScript2TokenType::TK_CARET_EQUAL:
				assign->op = GDScript2AssignOp::OP_XOR_ASSIGN;
				break;
			default:
				break;
		}

		assign->value = parse_assignment(); // Right-associative
		set_node_end(assign, previous);
		return assign;
	}

	return expr;
}

GDScript2ASTNode *GDScript2Parser::parse_ternary() {
	GDScript2ASTNode *expr = parse_or();

	if (match(GDScript2TokenType::TK_IF)) {
		// Ternary: value if condition else other
		GDScript2TernaryOpNode *ternary = memnew(GDScript2TernaryOpNode);
		ternary->start_line = expr->start_line;
		ternary->start_column = expr->start_column;
		ternary->true_expr = expr;
		ternary->condition = parse_or();
		consume(GDScript2TokenType::TK_ELSE, "Expected 'else' in ternary expression.");
		ternary->false_expr = parse_ternary();
		set_node_end(ternary, previous);
		return ternary;
	}

	return expr;
}

GDScript2ASTNode *GDScript2Parser::parse_or() {
	GDScript2ASTNode *expr = parse_and();

	while (match(GDScript2TokenType::TK_OR) || match(GDScript2TokenType::TK_PIPE_PIPE)) {
		GDScript2BinaryOpNode *binary = memnew(GDScript2BinaryOpNode);
		binary->start_line = expr->start_line;
		binary->start_column = expr->start_column;
		binary->op = GDScript2BinaryOp::OP_OR;
		binary->left = expr;
		binary->right = parse_and();
		set_node_end(binary, previous);
		expr = binary;
	}

	return expr;
}

GDScript2ASTNode *GDScript2Parser::parse_and() {
	GDScript2ASTNode *expr = parse_not();

	while (match(GDScript2TokenType::TK_AND) || match(GDScript2TokenType::TK_AMPERSAND_AMPERSAND)) {
		GDScript2BinaryOpNode *binary = memnew(GDScript2BinaryOpNode);
		binary->start_line = expr->start_line;
		binary->start_column = expr->start_column;
		binary->op = GDScript2BinaryOp::OP_AND;
		binary->left = expr;
		binary->right = parse_not();
		set_node_end(binary, previous);
		expr = binary;
	}

	return expr;
}

GDScript2ASTNode *GDScript2Parser::parse_not() {
	if (match(GDScript2TokenType::TK_NOT) || match(GDScript2TokenType::TK_BANG)) {
		GDScript2UnaryOpNode *unary = memnew(GDScript2UnaryOpNode);
		set_node_start(unary, previous);
		unary->op = GDScript2UnaryOp::OP_NOT;
		unary->operand = parse_not();
		set_node_end(unary, previous);
		return unary;
	}

	return parse_comparison();
}

GDScript2ASTNode *GDScript2Parser::parse_comparison() {
	GDScript2ASTNode *expr = parse_bit_or();

	while (check(GDScript2TokenType::TK_LESS) ||
			check(GDScript2TokenType::TK_LESS_EQUAL) ||
			check(GDScript2TokenType::TK_GREATER) ||
			check(GDScript2TokenType::TK_GREATER_EQUAL) ||
			check(GDScript2TokenType::TK_EQUAL_EQUAL) ||
			check(GDScript2TokenType::TK_BANG_EQUAL) ||
			check(GDScript2TokenType::TK_IN) ||
			check(GDScript2TokenType::TK_IS)) {
		GDScript2Token op_token = current;
		advance();

		// Handle 'is' as type test
		if (op_token.type == GDScript2TokenType::TK_IS) {
			GDScript2TypeTestNode *type_test = memnew(GDScript2TypeTestNode);
			type_test->start_line = expr->start_line;
			type_test->start_column = expr->start_column;
			type_test->operand = expr;
			type_test->test_type = parse_bit_or(); // Usually an identifier
			set_node_end(type_test, previous);
			return type_test;
		}

		GDScript2BinaryOpNode *binary = memnew(GDScript2BinaryOpNode);
		binary->start_line = expr->start_line;
		binary->start_column = expr->start_column;
		binary->left = expr;

		switch (op_token.type) {
			case GDScript2TokenType::TK_LESS:
				binary->op = GDScript2BinaryOp::OP_LT;
				break;
			case GDScript2TokenType::TK_LESS_EQUAL:
				binary->op = GDScript2BinaryOp::OP_LE;
				break;
			case GDScript2TokenType::TK_GREATER:
				binary->op = GDScript2BinaryOp::OP_GT;
				break;
			case GDScript2TokenType::TK_GREATER_EQUAL:
				binary->op = GDScript2BinaryOp::OP_GE;
				break;
			case GDScript2TokenType::TK_EQUAL_EQUAL:
				binary->op = GDScript2BinaryOp::OP_EQ;
				break;
			case GDScript2TokenType::TK_BANG_EQUAL:
				binary->op = GDScript2BinaryOp::OP_NE;
				break;
			case GDScript2TokenType::TK_IN:
				binary->op = GDScript2BinaryOp::OP_IN;
				break;
			default:
				break;
		}

		binary->right = parse_bit_or();
		set_node_end(binary, previous);
		expr = binary;
	}

	return expr;
}

GDScript2ASTNode *GDScript2Parser::parse_bit_or() {
	GDScript2ASTNode *expr = parse_bit_xor();

	while (match(GDScript2TokenType::TK_PIPE)) {
		GDScript2BinaryOpNode *binary = memnew(GDScript2BinaryOpNode);
		binary->start_line = expr->start_line;
		binary->start_column = expr->start_column;
		binary->op = GDScript2BinaryOp::OP_BIT_OR;
		binary->left = expr;
		binary->right = parse_bit_xor();
		set_node_end(binary, previous);
		expr = binary;
	}

	return expr;
}

GDScript2ASTNode *GDScript2Parser::parse_bit_xor() {
	GDScript2ASTNode *expr = parse_bit_and();

	while (match(GDScript2TokenType::TK_CARET)) {
		GDScript2BinaryOpNode *binary = memnew(GDScript2BinaryOpNode);
		binary->start_line = expr->start_line;
		binary->start_column = expr->start_column;
		binary->op = GDScript2BinaryOp::OP_BIT_XOR;
		binary->left = expr;
		binary->right = parse_bit_and();
		set_node_end(binary, previous);
		expr = binary;
	}

	return expr;
}

GDScript2ASTNode *GDScript2Parser::parse_bit_and() {
	GDScript2ASTNode *expr = parse_shift();

	while (match(GDScript2TokenType::TK_AMPERSAND)) {
		GDScript2BinaryOpNode *binary = memnew(GDScript2BinaryOpNode);
		binary->start_line = expr->start_line;
		binary->start_column = expr->start_column;
		binary->op = GDScript2BinaryOp::OP_BIT_AND;
		binary->left = expr;
		binary->right = parse_shift();
		set_node_end(binary, previous);
		expr = binary;
	}

	return expr;
}

GDScript2ASTNode *GDScript2Parser::parse_shift() {
	GDScript2ASTNode *expr = parse_range();

	while (check(GDScript2TokenType::TK_LESS_LESS) || check(GDScript2TokenType::TK_GREATER_GREATER)) {
		GDScript2Token op_token = current;
		advance();

		GDScript2BinaryOpNode *binary = memnew(GDScript2BinaryOpNode);
		binary->start_line = expr->start_line;
		binary->start_column = expr->start_column;
		binary->op = (op_token.type == GDScript2TokenType::TK_LESS_LESS)
				? GDScript2BinaryOp::OP_BIT_LSH
				: GDScript2BinaryOp::OP_BIT_RSH;
		binary->left = expr;
		binary->right = parse_range();
		set_node_end(binary, previous);
		expr = binary;
	}

	return expr;
}

GDScript2ASTNode *GDScript2Parser::parse_range() {
	GDScript2ASTNode *expr = parse_addition();

	// Note: range (..) is typically used differently in GDScript
	// For now, we skip this as it's mainly used in for loops

	return expr;
}

GDScript2ASTNode *GDScript2Parser::parse_addition() {
	GDScript2ASTNode *expr = parse_multiplication();

	while (check(GDScript2TokenType::TK_PLUS) || check(GDScript2TokenType::TK_MINUS)) {
		GDScript2Token op_token = current;
		advance();

		GDScript2BinaryOpNode *binary = memnew(GDScript2BinaryOpNode);
		binary->start_line = expr->start_line;
		binary->start_column = expr->start_column;
		binary->op = (op_token.type == GDScript2TokenType::TK_PLUS)
				? GDScript2BinaryOp::OP_ADD
				: GDScript2BinaryOp::OP_SUB;
		binary->left = expr;
		binary->right = parse_multiplication();
		set_node_end(binary, previous);
		expr = binary;
	}

	return expr;
}

GDScript2ASTNode *GDScript2Parser::parse_multiplication() {
	GDScript2ASTNode *expr = parse_unary();

	while (check(GDScript2TokenType::TK_STAR) ||
			check(GDScript2TokenType::TK_SLASH) ||
			check(GDScript2TokenType::TK_PERCENT)) {
		GDScript2Token op_token = current;
		advance();

		GDScript2BinaryOpNode *binary = memnew(GDScript2BinaryOpNode);
		binary->start_line = expr->start_line;
		binary->start_column = expr->start_column;

		switch (op_token.type) {
			case GDScript2TokenType::TK_STAR:
				binary->op = GDScript2BinaryOp::OP_MUL;
				break;
			case GDScript2TokenType::TK_SLASH:
				binary->op = GDScript2BinaryOp::OP_DIV;
				break;
			case GDScript2TokenType::TK_PERCENT:
				binary->op = GDScript2BinaryOp::OP_MOD;
				break;
			default:
				break;
		}

		binary->left = expr;
		binary->right = parse_unary();
		set_node_end(binary, previous);
		expr = binary;
	}

	return expr;
}

GDScript2ASTNode *GDScript2Parser::parse_unary() {
	if (match(GDScript2TokenType::TK_MINUS)) {
		GDScript2UnaryOpNode *unary = memnew(GDScript2UnaryOpNode);
		set_node_start(unary, previous);
		unary->op = GDScript2UnaryOp::OP_NEG;
		unary->operand = parse_unary();
		set_node_end(unary, previous);
		return unary;
	}

	if (match(GDScript2TokenType::TK_PLUS)) {
		GDScript2UnaryOpNode *unary = memnew(GDScript2UnaryOpNode);
		set_node_start(unary, previous);
		unary->op = GDScript2UnaryOp::OP_POS;
		unary->operand = parse_unary();
		set_node_end(unary, previous);
		return unary;
	}

	if (match(GDScript2TokenType::TK_TILDE)) {
		GDScript2UnaryOpNode *unary = memnew(GDScript2UnaryOpNode);
		set_node_start(unary, previous);
		unary->op = GDScript2UnaryOp::OP_BIT_NOT;
		unary->operand = parse_unary();
		set_node_end(unary, previous);
		return unary;
	}

	return parse_power();
}

GDScript2ASTNode *GDScript2Parser::parse_power() {
	GDScript2ASTNode *expr = parse_call();

	if (match(GDScript2TokenType::TK_STAR_STAR)) {
		GDScript2BinaryOpNode *binary = memnew(GDScript2BinaryOpNode);
		binary->start_line = expr->start_line;
		binary->start_column = expr->start_column;
		binary->op = GDScript2BinaryOp::OP_POW;
		binary->left = expr;
		binary->right = parse_power(); // Right-associative
		set_node_end(binary, previous);
		return binary;
	}

	return expr;
}

GDScript2ASTNode *GDScript2Parser::parse_call() {
	GDScript2ASTNode *expr = parse_primary();

	for (;;) {
		if (match(GDScript2TokenType::TK_PARENTHESIS_OPEN)) {
			// Function call
			GDScript2CallNode *call = memnew(GDScript2CallNode);
			call->start_line = expr->start_line;
			call->start_column = expr->start_column;
			call->callee = expr;

			tokenizer.set_multiline_mode(true);

			if (!check(GDScript2TokenType::TK_PARENTHESIS_CLOSE)) {
				do {
					GDScript2ASTNode *arg = parse_expression();
					if (arg) {
						call->arguments.push_back(arg);
					}
				} while (match(GDScript2TokenType::TK_COMMA));
			}

			tokenizer.set_multiline_mode(false);
			consume(GDScript2TokenType::TK_PARENTHESIS_CLOSE, "Expected ')' after arguments.");
			set_node_end(call, previous);
			expr = call;
		} else if (match(GDScript2TokenType::TK_BRACKET_OPEN)) {
			// Subscript
			GDScript2SubscriptNode *subscript = memnew(GDScript2SubscriptNode);
			subscript->start_line = expr->start_line;
			subscript->start_column = expr->start_column;
			subscript->base = expr;

			tokenizer.set_multiline_mode(true);
			subscript->index = parse_expression();
			tokenizer.set_multiline_mode(false);

			consume(GDScript2TokenType::TK_BRACKET_CLOSE, "Expected ']' after subscript.");
			set_node_end(subscript, previous);
			expr = subscript;
		} else if (match(GDScript2TokenType::TK_PERIOD)) {
			// Attribute access
			consume(GDScript2TokenType::TK_IDENTIFIER, "Expected attribute name after '.'.");

			GDScript2AttributeNode *attr = memnew(GDScript2AttributeNode);
			attr->start_line = expr->start_line;
			attr->start_column = expr->start_column;
			attr->base = expr;
			attr->attribute = previous.literal;
			set_node_end(attr, previous);
			expr = attr;
		} else {
			break;
		}
	}

	// Handle 'as' for type cast
	if (match(GDScript2TokenType::TK_AS)) {
		GDScript2CastNode *cast = memnew(GDScript2CastNode);
		cast->start_line = expr->start_line;
		cast->start_column = expr->start_column;
		cast->operand = expr;
		cast->cast_type = parse_primary(); // Type name
		set_node_end(cast, previous);
		return cast;
	}

	return expr;
}

GDScript2ASTNode *GDScript2Parser::parse_primary() {
	// Literal
	if (match(GDScript2TokenType::TK_LITERAL)) {
		GDScript2LiteralNode *literal = memnew(GDScript2LiteralNode);
		set_node_start(literal, previous);
		set_node_end(literal, previous);
		literal->value = previous.literal;
		return literal;
	}

	// Builtin constants
	if (match(GDScript2TokenType::TK_CONST_PI)) {
		GDScript2LiteralNode *literal = memnew(GDScript2LiteralNode);
		set_node_start(literal, previous);
		set_node_end(literal, previous);
		literal->value = Math::PI;
		return literal;
	}
	if (match(GDScript2TokenType::TK_CONST_TAU)) {
		GDScript2LiteralNode *literal = memnew(GDScript2LiteralNode);
		set_node_start(literal, previous);
		set_node_end(literal, previous);
		literal->value = Math::TAU;
		return literal;
	}
	if (match(GDScript2TokenType::TK_CONST_INF)) {
		GDScript2LiteralNode *literal = memnew(GDScript2LiteralNode);
		set_node_start(literal, previous);
		set_node_end(literal, previous);
		literal->value = Math::INF;
		return literal;
	}
	if (match(GDScript2TokenType::TK_CONST_NAN)) {
		GDScript2LiteralNode *literal = memnew(GDScript2LiteralNode);
		set_node_start(literal, previous);
		set_node_end(literal, previous);
		literal->value = Math::NaN;
		return literal;
	}

	// Self
	if (match(GDScript2TokenType::TK_SELF)) {
		GDScript2SelfNode *self = memnew(GDScript2SelfNode);
		set_node_start(self, previous);
		set_node_end(self, previous);
		return self;
	}

	// Identifier
	if (match(GDScript2TokenType::TK_IDENTIFIER)) {
		GDScript2IdentifierNode *id = memnew(GDScript2IdentifierNode);
		set_node_start(id, previous);
		set_node_end(id, previous);
		id->name = previous.literal;
		return id;
	}

	// Grouped expression
	if (match(GDScript2TokenType::TK_PARENTHESIS_OPEN)) {
		tokenizer.set_multiline_mode(true);
		GDScript2ASTNode *expr = parse_expression();
		tokenizer.set_multiline_mode(false);
		consume(GDScript2TokenType::TK_PARENTHESIS_CLOSE, "Expected ')' after expression.");
		return expr;
	}

	// Array literal
	if (check(GDScript2TokenType::TK_BRACKET_OPEN)) {
		return parse_array();
	}

	// Dictionary literal
	if (check(GDScript2TokenType::TK_BRACE_OPEN)) {
		return parse_dictionary();
	}

	// Lambda
	if (check(GDScript2TokenType::TK_FUNC)) {
		return parse_lambda();
	}

	// await
	if (match(GDScript2TokenType::TK_AWAIT)) {
		GDScript2AwaitNode *await_node = memnew(GDScript2AwaitNode);
		set_node_start(await_node, previous);
		await_node->expression = parse_expression();
		set_node_end(await_node, previous);
		return await_node;
	}

	// preload
	if (match(GDScript2TokenType::TK_PRELOAD)) {
		GDScript2PreloadNode *preload = memnew(GDScript2PreloadNode);
		set_node_start(preload, previous);
		consume(GDScript2TokenType::TK_PARENTHESIS_OPEN, "Expected '(' after 'preload'.");
		consume(GDScript2TokenType::TK_LITERAL, "Expected string path in preload.");
		preload->path = String(previous.literal);
		consume(GDScript2TokenType::TK_PARENTHESIS_CLOSE, "Expected ')' after preload path.");
		set_node_end(preload, previous);
		return preload;
	}

	// $NodePath
	if (match(GDScript2TokenType::TK_DOLLAR)) {
		GDScript2GetNodeNode *get_node = memnew(GDScript2GetNodeNode);
		set_node_start(get_node, previous);

		// Parse node path (simplified)
		String path;
		while (check(GDScript2TokenType::TK_IDENTIFIER) ||
				check(GDScript2TokenType::TK_SLASH) ||
				check(GDScript2TokenType::TK_PERIOD)) {
			path += String(current.source);
			advance();
		}
		get_node->path = path;
		set_node_end(get_node, previous);
		return get_node;
	}

	// super
	if (match(GDScript2TokenType::TK_SUPER)) {
		GDScript2CallNode *super_call = memnew(GDScript2CallNode);
		set_node_start(super_call, previous);
		super_call->is_super = true;

		// super() or super.method()
		if (match(GDScript2TokenType::TK_PERIOD)) {
			consume(GDScript2TokenType::TK_IDENTIFIER, "Expected method name after 'super.'.");
			GDScript2IdentifierNode *method = memnew(GDScript2IdentifierNode);
			method->name = previous.literal;
			super_call->callee = method;
		}

		if (match(GDScript2TokenType::TK_PARENTHESIS_OPEN)) {
			tokenizer.set_multiline_mode(true);
			if (!check(GDScript2TokenType::TK_PARENTHESIS_CLOSE)) {
				do {
					GDScript2ASTNode *arg = parse_expression();
					if (arg) {
						super_call->arguments.push_back(arg);
					}
				} while (match(GDScript2TokenType::TK_COMMA));
			}
			tokenizer.set_multiline_mode(false);
			consume(GDScript2TokenType::TK_PARENTHESIS_CLOSE, "Expected ')' after super arguments.");
		}

		set_node_end(super_call, previous);
		return super_call;
	}

	error_at_current("Expected expression.");
	return nullptr;
}

GDScript2ArrayNode *GDScript2Parser::parse_array() {
	GDScript2ArrayNode *array = memnew(GDScript2ArrayNode);
	set_node_start(array, current);

	consume(GDScript2TokenType::TK_BRACKET_OPEN, "Expected '['.");
	tokenizer.set_multiline_mode(true);

	if (!check(GDScript2TokenType::TK_BRACKET_CLOSE)) {
		do {
			GDScript2ASTNode *elem = parse_expression();
			if (elem) {
				array->elements.push_back(elem);
			}
		} while (match(GDScript2TokenType::TK_COMMA) && !check(GDScript2TokenType::TK_BRACKET_CLOSE));
	}

	tokenizer.set_multiline_mode(false);
	consume(GDScript2TokenType::TK_BRACKET_CLOSE, "Expected ']' after array elements.");
	set_node_end(array, previous);
	return array;
}

GDScript2DictionaryNode *GDScript2Parser::parse_dictionary() {
	GDScript2DictionaryNode *dict = memnew(GDScript2DictionaryNode);
	set_node_start(dict, current);

	consume(GDScript2TokenType::TK_BRACE_OPEN, "Expected '{'.");
	tokenizer.set_multiline_mode(true);

	if (!check(GDScript2TokenType::TK_BRACE_CLOSE)) {
		do {
			GDScript2ASTNode *key = parse_expression();
			dict->keys.push_back(key);

			// Support both : and = for dictionary entries
			if (!match(GDScript2TokenType::TK_COLON)) {
				consume(GDScript2TokenType::TK_EQUAL, "Expected ':' or '=' after dictionary key.");
			}

			GDScript2ASTNode *value = parse_expression();
			dict->values.push_back(value);
		} while (match(GDScript2TokenType::TK_COMMA) && !check(GDScript2TokenType::TK_BRACE_CLOSE));
	}

	tokenizer.set_multiline_mode(false);
	consume(GDScript2TokenType::TK_BRACE_CLOSE, "Expected '}' after dictionary entries.");
	set_node_end(dict, previous);
	return dict;
}

GDScript2LambdaNode *GDScript2Parser::parse_lambda() {
	GDScript2LambdaNode *lambda = memnew(GDScript2LambdaNode);
	set_node_start(lambda, current);

	consume(GDScript2TokenType::TK_FUNC, "Expected 'func'.");

	// Optional name (ignored for lambdas)
	match(GDScript2TokenType::TK_IDENTIFIER);

	// Parameters
	consume(GDScript2TokenType::TK_PARENTHESIS_OPEN, "Expected '(' in lambda.");
	lambda->parameters = parse_parameter_list();
	consume(GDScript2TokenType::TK_PARENTHESIS_CLOSE, "Expected ')' after lambda parameters.");

	consume(GDScript2TokenType::TK_COLON, "Expected ':' after lambda signature.");

	// Lambda body - can be expression or suite
	if (check(GDScript2TokenType::TK_NEWLINE)) {
		match(GDScript2TokenType::TK_NEWLINE);
		tokenizer.push_expression_indented_block();
		lambda->body = parse_suite();
		tokenizer.pop_expression_indented_block();
	} else {
		// Single line lambda - can be return statement or expression
		if (check(GDScript2TokenType::TK_RETURN)) {
			lambda->body = parse_return();
		} else {
			lambda->body = parse_expression();
		}
	}

	set_node_end(lambda, previous);
	return lambda;
}
