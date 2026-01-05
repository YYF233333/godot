/**************************************************************************/
/*  gdscript2_ast.h                                                       */
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

#include "core/string/string_name.h"
#include "core/templates/local_vector.h"
#include "core/variant/variant.h"

// Forward declarations
struct GDScript2ASTNode;

// AST Node types
enum class GDScript2ASTNodeType {
	// Special
	NODE_NONE,
	NODE_ERROR,

	// Top-level
	NODE_CLASS, // class definition
	NODE_FUNCTION, // func definition
	NODE_VARIABLE, // var declaration
	NODE_CONSTANT, // const declaration
	NODE_SIGNAL, // signal declaration
	NODE_ENUM, // enum declaration
	NODE_ANNOTATION, // @annotation

	// Statements
	NODE_SUITE, // block of statements
	NODE_IF, // if/elif/else
	NODE_FOR, // for loop
	NODE_WHILE, // while loop
	NODE_MATCH, // match statement
	NODE_MATCH_BRANCH, // match branch
	NODE_PATTERN, // match pattern
	NODE_BREAK, // break
	NODE_CONTINUE, // continue
	NODE_PASS, // pass
	NODE_RETURN, // return
	NODE_ASSERT, // assert
	NODE_BREAKPOINT, // breakpoint

	// Expressions
	NODE_LITERAL, // literal value (int, float, string, etc.)
	NODE_IDENTIFIER, // identifier reference
	NODE_SELF, // self keyword
	NODE_BINARY_OP, // binary operation
	NODE_UNARY_OP, // unary operation
	NODE_TERNARY_OP, // ternary (conditional) expression
	NODE_ASSIGNMENT, // assignment
	NODE_CALL, // function call
	NODE_SUBSCRIPT, // array/dict subscript []
	NODE_ATTRIBUTE, // attribute access .
	NODE_ARRAY, // array literal []
	NODE_DICTIONARY, // dictionary literal {}
	NODE_LAMBDA, // lambda function
	NODE_AWAIT, // await expression
	NODE_PRELOAD, // preload() call
	NODE_CAST, // type cast (as)
	NODE_TYPE_TEST, // type test (is)
	NODE_GET_NODE, // $NodePath

	// Type
	NODE_TYPE, // type annotation
	NODE_PARAMETER, // function parameter
};

// Binary operator types
enum class GDScript2BinaryOp {
	OP_NONE,
	// Arithmetic
	OP_ADD, // +
	OP_SUB, // -
	OP_MUL, // *
	OP_DIV, // /
	OP_MOD, // %
	OP_POW, // **
	// Comparison
	OP_EQ, // ==
	OP_NE, // !=
	OP_LT, // <
	OP_LE, // <=
	OP_GT, // >
	OP_GE, // >=
	// Logical
	OP_AND, // and, &&
	OP_OR, // or, ||
	// Bitwise
	OP_BIT_AND, // &
	OP_BIT_OR, // |
	OP_BIT_XOR, // ^
	OP_BIT_LSH, // <<
	OP_BIT_RSH, // >>
	// Other
	OP_IN, // in
	OP_RANGE, // ..
};

// Unary operator types
enum class GDScript2UnaryOp {
	OP_NONE,
	OP_NEG, // -
	OP_POS, // +
	OP_NOT, // not, !
	OP_BIT_NOT, // ~
};

// Assignment operator types
enum class GDScript2AssignOp {
	OP_NONE,
	OP_ASSIGN, // =
	OP_ADD_ASSIGN, // +=
	OP_SUB_ASSIGN, // -=
	OP_MUL_ASSIGN, // *=
	OP_DIV_ASSIGN, // /=
	OP_MOD_ASSIGN, // %=
	OP_POW_ASSIGN, // **=
	OP_LSH_ASSIGN, // <<=
	OP_RSH_ASSIGN, // >>=
	OP_AND_ASSIGN, // &=
	OP_OR_ASSIGN, // |=
	OP_XOR_ASSIGN, // ^=
};

// Pattern types for match
enum class GDScript2PatternType {
	PATTERN_LITERAL, // literal value
	PATTERN_BIND, // var name
	PATTERN_EXPRESSION, // expression
	PATTERN_ARRAY, // [pattern, ...]
	PATTERN_DICTIONARY, // {key: pattern, ...}
	PATTERN_REST, // ..
};

// Base AST Node structure
struct GDScript2ASTNode {
	GDScript2ASTNodeType type = GDScript2ASTNodeType::NODE_NONE;
	int start_line = 0;
	int end_line = 0;
	int start_column = 0;
	int end_column = 0;

	virtual ~GDScript2ASTNode() = default;
};

// ============================================================================
// Expression Nodes
// ============================================================================

struct GDScript2LiteralNode : public GDScript2ASTNode {
	Variant value;

	GDScript2LiteralNode() { type = GDScript2ASTNodeType::NODE_LITERAL; }
};

struct GDScript2IdentifierNode : public GDScript2ASTNode {
	StringName name;

	GDScript2IdentifierNode() { type = GDScript2ASTNodeType::NODE_IDENTIFIER; }
};

struct GDScript2SelfNode : public GDScript2ASTNode {
	GDScript2SelfNode() { type = GDScript2ASTNodeType::NODE_SELF; }
};

struct GDScript2BinaryOpNode : public GDScript2ASTNode {
	GDScript2BinaryOp op = GDScript2BinaryOp::OP_NONE;
	GDScript2ASTNode *left = nullptr;
	GDScript2ASTNode *right = nullptr;

	GDScript2BinaryOpNode() { type = GDScript2ASTNodeType::NODE_BINARY_OP; }
	~GDScript2BinaryOpNode() {
		if (left) {
			memdelete(left);
		}
		if (right) {
			memdelete(right);
		}
	}
};

struct GDScript2UnaryOpNode : public GDScript2ASTNode {
	GDScript2UnaryOp op = GDScript2UnaryOp::OP_NONE;
	GDScript2ASTNode *operand = nullptr;

	GDScript2UnaryOpNode() { type = GDScript2ASTNodeType::NODE_UNARY_OP; }
	~GDScript2UnaryOpNode() {
		if (operand) {
			memdelete(operand);
		}
	}
};

struct GDScript2TernaryOpNode : public GDScript2ASTNode {
	GDScript2ASTNode *condition = nullptr;
	GDScript2ASTNode *true_expr = nullptr;
	GDScript2ASTNode *false_expr = nullptr;

	GDScript2TernaryOpNode() { type = GDScript2ASTNodeType::NODE_TERNARY_OP; }
	~GDScript2TernaryOpNode() {
		if (condition) {
			memdelete(condition);
		}
		if (true_expr) {
			memdelete(true_expr);
		}
		if (false_expr) {
			memdelete(false_expr);
		}
	}
};

struct GDScript2AssignmentNode : public GDScript2ASTNode {
	GDScript2AssignOp op = GDScript2AssignOp::OP_ASSIGN;
	GDScript2ASTNode *target = nullptr;
	GDScript2ASTNode *value = nullptr;

	GDScript2AssignmentNode() { type = GDScript2ASTNodeType::NODE_ASSIGNMENT; }
	~GDScript2AssignmentNode() {
		if (target) {
			memdelete(target);
		}
		if (value) {
			memdelete(value);
		}
	}
};

struct GDScript2CallNode : public GDScript2ASTNode {
	GDScript2ASTNode *callee = nullptr;
	LocalVector<GDScript2ASTNode *> arguments;
	bool is_super = false;

	GDScript2CallNode() { type = GDScript2ASTNodeType::NODE_CALL; }
	~GDScript2CallNode() {
		if (callee) {
			memdelete(callee);
		}
		for (GDScript2ASTNode *arg : arguments) {
			if (arg) {
				memdelete(arg);
			}
		}
	}
};

struct GDScript2SubscriptNode : public GDScript2ASTNode {
	GDScript2ASTNode *base = nullptr;
	GDScript2ASTNode *index = nullptr;

	GDScript2SubscriptNode() { type = GDScript2ASTNodeType::NODE_SUBSCRIPT; }
	~GDScript2SubscriptNode() {
		if (base) {
			memdelete(base);
		}
		if (index) {
			memdelete(index);
		}
	}
};

struct GDScript2AttributeNode : public GDScript2ASTNode {
	GDScript2ASTNode *base = nullptr;
	StringName attribute;

	GDScript2AttributeNode() { type = GDScript2ASTNodeType::NODE_ATTRIBUTE; }
	~GDScript2AttributeNode() {
		if (base) {
			memdelete(base);
		}
	}
};

struct GDScript2ArrayNode : public GDScript2ASTNode {
	LocalVector<GDScript2ASTNode *> elements;

	GDScript2ArrayNode() { type = GDScript2ASTNodeType::NODE_ARRAY; }
	~GDScript2ArrayNode() {
		for (GDScript2ASTNode *elem : elements) {
			if (elem) {
				memdelete(elem);
			}
		}
	}
};

struct GDScript2DictionaryNode : public GDScript2ASTNode {
	LocalVector<GDScript2ASTNode *> keys;
	LocalVector<GDScript2ASTNode *> values;

	GDScript2DictionaryNode() { type = GDScript2ASTNodeType::NODE_DICTIONARY; }
	~GDScript2DictionaryNode() {
		for (GDScript2ASTNode *key : keys) {
			if (key) {
				memdelete(key);
			}
		}
		for (GDScript2ASTNode *val : values) {
			if (val) {
				memdelete(val);
			}
		}
	}
};

struct GDScript2AwaitNode : public GDScript2ASTNode {
	GDScript2ASTNode *expression = nullptr;

	GDScript2AwaitNode() { type = GDScript2ASTNodeType::NODE_AWAIT; }
	~GDScript2AwaitNode() {
		if (expression) {
			memdelete(expression);
		}
	}
};

struct GDScript2PreloadNode : public GDScript2ASTNode {
	String path;

	GDScript2PreloadNode() { type = GDScript2ASTNodeType::NODE_PRELOAD; }
};

struct GDScript2CastNode : public GDScript2ASTNode {
	GDScript2ASTNode *operand = nullptr;
	GDScript2ASTNode *cast_type = nullptr;

	GDScript2CastNode() { type = GDScript2ASTNodeType::NODE_CAST; }
	~GDScript2CastNode() {
		if (operand) {
			memdelete(operand);
		}
		if (cast_type) {
			memdelete(cast_type);
		}
	}
};

struct GDScript2TypeTestNode : public GDScript2ASTNode {
	GDScript2ASTNode *operand = nullptr;
	GDScript2ASTNode *test_type = nullptr;

	GDScript2TypeTestNode() { type = GDScript2ASTNodeType::NODE_TYPE_TEST; }
	~GDScript2TypeTestNode() {
		if (operand) {
			memdelete(operand);
		}
		if (test_type) {
			memdelete(test_type);
		}
	}
};

struct GDScript2GetNodeNode : public GDScript2ASTNode {
	String path;

	GDScript2GetNodeNode() { type = GDScript2ASTNodeType::NODE_GET_NODE; }
};

// ============================================================================
// Type Nodes
// ============================================================================

struct GDScript2TypeNode : public GDScript2ASTNode {
	StringName type_name;
	LocalVector<GDScript2TypeNode *> container_types; // For Array[T], Dictionary[K,V]

	GDScript2TypeNode() { type = GDScript2ASTNodeType::NODE_TYPE; }
	~GDScript2TypeNode() {
		for (GDScript2TypeNode *t : container_types) {
			if (t) {
				memdelete(t);
			}
		}
	}
};

struct GDScript2ParameterNode : public GDScript2ASTNode {
	StringName name;
	GDScript2TypeNode *type_hint = nullptr;
	GDScript2ASTNode *default_value = nullptr;

	GDScript2ParameterNode() { type = GDScript2ASTNodeType::NODE_PARAMETER; }
	~GDScript2ParameterNode() {
		if (type_hint) {
			memdelete(type_hint);
		}
		if (default_value) {
			memdelete(default_value);
		}
	}
};

// ============================================================================
// Statement Nodes
// ============================================================================

struct GDScript2SuiteNode : public GDScript2ASTNode {
	LocalVector<GDScript2ASTNode *> statements;

	GDScript2SuiteNode() { type = GDScript2ASTNodeType::NODE_SUITE; }
	~GDScript2SuiteNode() {
		for (GDScript2ASTNode *stmt : statements) {
			if (stmt) {
				memdelete(stmt);
			}
		}
	}
};

struct GDScript2IfNode : public GDScript2ASTNode {
	GDScript2ASTNode *condition = nullptr;
	GDScript2SuiteNode *true_block = nullptr;
	GDScript2ASTNode *false_block = nullptr; // Can be SuiteNode or another IfNode (elif)

	GDScript2IfNode() { type = GDScript2ASTNodeType::NODE_IF; }
	~GDScript2IfNode() {
		if (condition) {
			memdelete(condition);
		}
		if (true_block) {
			memdelete(true_block);
		}
		if (false_block) {
			memdelete(false_block);
		}
	}
};

struct GDScript2ForNode : public GDScript2ASTNode {
	StringName variable;
	GDScript2ASTNode *iterable = nullptr;
	GDScript2SuiteNode *body = nullptr;

	GDScript2ForNode() { type = GDScript2ASTNodeType::NODE_FOR; }
	~GDScript2ForNode() {
		if (iterable) {
			memdelete(iterable);
		}
		if (body) {
			memdelete(body);
		}
	}
};

struct GDScript2WhileNode : public GDScript2ASTNode {
	GDScript2ASTNode *condition = nullptr;
	GDScript2SuiteNode *body = nullptr;

	GDScript2WhileNode() { type = GDScript2ASTNodeType::NODE_WHILE; }
	~GDScript2WhileNode() {
		if (condition) {
			memdelete(condition);
		}
		if (body) {
			memdelete(body);
		}
	}
};

struct GDScript2PatternNode : public GDScript2ASTNode {
	GDScript2PatternType pattern_type = GDScript2PatternType::PATTERN_LITERAL;
	Variant literal; // For PATTERN_LITERAL
	StringName bind_name; // For PATTERN_BIND
	GDScript2ASTNode *expression = nullptr; // For PATTERN_EXPRESSION
	LocalVector<GDScript2PatternNode *> array_patterns; // For PATTERN_ARRAY
	LocalVector<GDScript2ASTNode *> dictionary_keys; // For PATTERN_DICTIONARY
	LocalVector<GDScript2PatternNode *> dictionary_patterns; // For PATTERN_DICTIONARY
	bool has_rest = false; // For PATTERN_REST in arrays

	GDScript2PatternNode() { type = GDScript2ASTNodeType::NODE_PATTERN; }
	~GDScript2PatternNode() {
		if (expression) {
			memdelete(expression);
		}
		for (GDScript2PatternNode *p : array_patterns) {
			if (p) {
				memdelete(p);
			}
		}
		for (GDScript2ASTNode *k : dictionary_keys) {
			if (k) {
				memdelete(k);
			}
		}
		for (GDScript2PatternNode *p : dictionary_patterns) {
			if (p) {
				memdelete(p);
			}
		}
	}
};

struct GDScript2MatchBranchNode : public GDScript2ASTNode {
	LocalVector<GDScript2PatternNode *> patterns;
	GDScript2ASTNode *guard = nullptr; // when clause
	GDScript2SuiteNode *body = nullptr;

	GDScript2MatchBranchNode() { type = GDScript2ASTNodeType::NODE_MATCH_BRANCH; }
	~GDScript2MatchBranchNode() {
		for (GDScript2PatternNode *p : patterns) {
			if (p) {
				memdelete(p);
			}
		}
		if (guard) {
			memdelete(guard);
		}
		if (body) {
			memdelete(body);
		}
	}
};

struct GDScript2MatchNode : public GDScript2ASTNode {
	GDScript2ASTNode *test_value = nullptr;
	LocalVector<GDScript2MatchBranchNode *> branches;

	GDScript2MatchNode() { type = GDScript2ASTNodeType::NODE_MATCH; }
	~GDScript2MatchNode() {
		if (test_value) {
			memdelete(test_value);
		}
		for (GDScript2MatchBranchNode *b : branches) {
			if (b) {
				memdelete(b);
			}
		}
	}
};

struct GDScript2BreakNode : public GDScript2ASTNode {
	GDScript2BreakNode() { type = GDScript2ASTNodeType::NODE_BREAK; }
};

struct GDScript2ContinueNode : public GDScript2ASTNode {
	GDScript2ContinueNode() { type = GDScript2ASTNodeType::NODE_CONTINUE; }
};

struct GDScript2PassNode : public GDScript2ASTNode {
	GDScript2PassNode() { type = GDScript2ASTNodeType::NODE_PASS; }
};

struct GDScript2ReturnNode : public GDScript2ASTNode {
	GDScript2ASTNode *value = nullptr;

	GDScript2ReturnNode() { type = GDScript2ASTNodeType::NODE_RETURN; }
	~GDScript2ReturnNode() {
		if (value) {
			memdelete(value);
		}
	}
};

struct GDScript2AssertNode : public GDScript2ASTNode {
	GDScript2ASTNode *condition = nullptr;
	GDScript2ASTNode *message = nullptr;

	GDScript2AssertNode() { type = GDScript2ASTNodeType::NODE_ASSERT; }
	~GDScript2AssertNode() {
		if (condition) {
			memdelete(condition);
		}
		if (message) {
			memdelete(message);
		}
	}
};

struct GDScript2BreakpointNode : public GDScript2ASTNode {
	GDScript2BreakpointNode() { type = GDScript2ASTNodeType::NODE_BREAKPOINT; }
};

// ============================================================================
// Declaration Nodes
// ============================================================================

struct GDScript2AnnotationNode : public GDScript2ASTNode {
	StringName name;
	LocalVector<GDScript2ASTNode *> arguments;

	GDScript2AnnotationNode() { type = GDScript2ASTNodeType::NODE_ANNOTATION; }
	~GDScript2AnnotationNode() {
		for (GDScript2ASTNode *arg : arguments) {
			if (arg) {
				memdelete(arg);
			}
		}
	}
};

struct GDScript2VariableNode : public GDScript2ASTNode {
	StringName name;
	GDScript2TypeNode *type_hint = nullptr;
	GDScript2ASTNode *initializer = nullptr;
	LocalVector<GDScript2AnnotationNode *> annotations;
	bool is_static = false;

	GDScript2VariableNode() { type = GDScript2ASTNodeType::NODE_VARIABLE; }
	~GDScript2VariableNode() {
		if (type_hint) {
			memdelete(type_hint);
		}
		if (initializer) {
			memdelete(initializer);
		}
		for (GDScript2AnnotationNode *a : annotations) {
			if (a) {
				memdelete(a);
			}
		}
	}
};

struct GDScript2ConstantNode : public GDScript2ASTNode {
	StringName name;
	GDScript2TypeNode *type_hint = nullptr;
	GDScript2ASTNode *initializer = nullptr;

	GDScript2ConstantNode() { type = GDScript2ASTNodeType::NODE_CONSTANT; }
	~GDScript2ConstantNode() {
		if (type_hint) {
			memdelete(type_hint);
		}
		if (initializer) {
			memdelete(initializer);
		}
	}
};

struct GDScript2SignalNode : public GDScript2ASTNode {
	StringName name;
	LocalVector<GDScript2ParameterNode *> parameters;

	GDScript2SignalNode() { type = GDScript2ASTNodeType::NODE_SIGNAL; }
	~GDScript2SignalNode() {
		for (GDScript2ParameterNode *p : parameters) {
			if (p) {
				memdelete(p);
			}
		}
	}
};

struct GDScript2EnumNode : public GDScript2ASTNode {
	StringName name; // Empty for anonymous enums
	LocalVector<StringName> value_names;
	LocalVector<GDScript2ASTNode *> value_expressions; // Can be null for auto-increment

	GDScript2EnumNode() { type = GDScript2ASTNodeType::NODE_ENUM; }
	~GDScript2EnumNode() {
		for (GDScript2ASTNode *expr : value_expressions) {
			if (expr) {
				memdelete(expr);
			}
		}
	}
};

struct GDScript2FunctionNode : public GDScript2ASTNode {
	StringName name;
	LocalVector<GDScript2ParameterNode *> parameters;
	GDScript2TypeNode *return_type = nullptr;
	GDScript2SuiteNode *body = nullptr;
	LocalVector<GDScript2AnnotationNode *> annotations;
	bool is_static = false;
	bool is_coroutine = false; // async function

	GDScript2FunctionNode() { type = GDScript2ASTNodeType::NODE_FUNCTION; }
	~GDScript2FunctionNode() {
		for (GDScript2ParameterNode *p : parameters) {
			if (p) {
				memdelete(p);
			}
		}
		if (return_type) {
			memdelete(return_type);
		}
		if (body) {
			memdelete(body);
		}
		for (GDScript2AnnotationNode *a : annotations) {
			if (a) {
				memdelete(a);
			}
		}
	}
};

struct GDScript2LambdaNode : public GDScript2ASTNode {
	LocalVector<GDScript2ParameterNode *> parameters;
	GDScript2ASTNode *body = nullptr; // Can be expression or suite

	GDScript2LambdaNode() { type = GDScript2ASTNodeType::NODE_LAMBDA; }
	~GDScript2LambdaNode() {
		for (GDScript2ParameterNode *p : parameters) {
			if (p) {
				memdelete(p);
			}
		}
		if (body) {
			memdelete(body);
		}
	}
};

struct GDScript2ClassNode : public GDScript2ASTNode {
	StringName name; // Class name (can be empty for main script class)
	StringName extends_path; // extends "res://..." or class name
	StringName class_name; // class_name declaration
	bool is_inner_class = false;

	LocalVector<GDScript2AnnotationNode *> annotations;
	LocalVector<GDScript2VariableNode *> variables;
	LocalVector<GDScript2ConstantNode *> constants;
	LocalVector<GDScript2SignalNode *> signals;
	LocalVector<GDScript2EnumNode *> enums;
	LocalVector<GDScript2FunctionNode *> functions;
	LocalVector<GDScript2ClassNode *> inner_classes;

	GDScript2ClassNode() { type = GDScript2ASTNodeType::NODE_CLASS; }
	~GDScript2ClassNode() {
		for (GDScript2AnnotationNode *a : annotations) {
			if (a) {
				memdelete(a);
			}
		}
		for (GDScript2VariableNode *v : variables) {
			if (v) {
				memdelete(v);
			}
		}
		for (GDScript2ConstantNode *c : constants) {
			if (c) {
				memdelete(c);
			}
		}
		for (GDScript2SignalNode *s : signals) {
			if (s) {
				memdelete(s);
			}
		}
		for (GDScript2EnumNode *e : enums) {
			if (e) {
				memdelete(e);
			}
		}
		for (GDScript2FunctionNode *f : functions) {
			if (f) {
				memdelete(f);
			}
		}
		for (GDScript2ClassNode *c : inner_classes) {
			if (c) {
				memdelete(c);
			}
		}
	}
};
