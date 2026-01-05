/**************************************************************************/
/*  gdscript2_parser.h                                                    */
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

#include "gdscript2_ast.h"
#include "gdscript2_tokenizer.h"

#include "core/object/ref_counted.h"
#include "core/templates/list.h"

// GDScript2 Parser - Recursive descent parser that builds AST from tokens
class GDScript2Parser : public RefCounted {
	GDCLASS(GDScript2Parser, RefCounted);

public:
	// Parser options
	struct Options {
		bool keep_comments = false;
		bool strict_indentation = true;
	};

	// Parser error
	struct Error {
		String message;
		int line = 0;
		int column = 0;
	};

	// Parser result
	struct Result {
		GDScript2ClassNode *root = nullptr;
		List<Error> errors;

		bool has_errors() const { return !errors.is_empty(); }
	};

protected:
	static void _bind_methods() {}

private:
	GDScript2Tokenizer tokenizer;
	GDScript2Token current;
	GDScript2Token previous;
	List<Error> errors;
	bool panic_mode = false;
	bool had_error = false;

	// Token handling
	void advance();
	bool check(GDScript2TokenType p_type) const;
	bool match(GDScript2TokenType p_type);
	void consume(GDScript2TokenType p_type, const String &p_message);
	bool is_at_end() const;

	// Error handling
	void error(const String &p_message);
	void error_at_current(const String &p_message);
	void synchronize();

	// Helper to set node position from token
	void set_node_start(GDScript2ASTNode *p_node, const GDScript2Token &p_token);
	void set_node_end(GDScript2ASTNode *p_node, const GDScript2Token &p_token);

	// Parsing methods - Top level
	GDScript2ClassNode *parse_class(bool p_is_inner = false);
	void parse_class_body(GDScript2ClassNode *p_class);
	GDScript2AnnotationNode *parse_annotation();
	GDScript2FunctionNode *parse_function(bool p_is_static = false);
	GDScript2VariableNode *parse_variable(bool p_is_static = false);
	GDScript2ConstantNode *parse_constant();
	GDScript2SignalNode *parse_signal();
	GDScript2EnumNode *parse_enum();

	// Parsing methods - Types and parameters
	GDScript2TypeNode *parse_type();
	GDScript2ParameterNode *parse_parameter();
	LocalVector<GDScript2ParameterNode *> parse_parameter_list();

	// Parsing methods - Statements
	GDScript2SuiteNode *parse_suite();
	GDScript2ASTNode *parse_statement();
	GDScript2IfNode *parse_if();
	GDScript2ForNode *parse_for();
	GDScript2WhileNode *parse_while();
	GDScript2MatchNode *parse_match();
	GDScript2MatchBranchNode *parse_match_branch();
	GDScript2PatternNode *parse_pattern();
	GDScript2ReturnNode *parse_return();
	GDScript2AssertNode *parse_assert();

	// Parsing methods - Expressions (precedence climbing)
	GDScript2ASTNode *parse_expression();
	GDScript2ASTNode *parse_assignment();
	GDScript2ASTNode *parse_ternary();
	GDScript2ASTNode *parse_or();
	GDScript2ASTNode *parse_and();
	GDScript2ASTNode *parse_not();
	GDScript2ASTNode *parse_comparison();
	GDScript2ASTNode *parse_bit_or();
	GDScript2ASTNode *parse_bit_xor();
	GDScript2ASTNode *parse_bit_and();
	GDScript2ASTNode *parse_shift();
	GDScript2ASTNode *parse_range();
	GDScript2ASTNode *parse_addition();
	GDScript2ASTNode *parse_multiplication();
	GDScript2ASTNode *parse_unary();
	GDScript2ASTNode *parse_power();
	GDScript2ASTNode *parse_call();
	GDScript2ASTNode *parse_primary();

	// Parsing methods - Literals and constructors
	GDScript2ArrayNode *parse_array();
	GDScript2DictionaryNode *parse_dictionary();
	GDScript2LambdaNode *parse_lambda();

public:
	// Main parse function
	Result parse(const String &p_source, const Options &p_options = Options());

	GDScript2Parser() = default;
};
