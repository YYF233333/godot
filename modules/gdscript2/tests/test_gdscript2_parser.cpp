/**************************************************************************/
/*  test_gdscript2_parser.cpp                                             */
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

#include "tests/test_macros.h"

#include "modules/gdscript2/front/gdscript2_parser.h"

namespace {

// Test empty source
void test_parser_empty() {
	Ref<GDScript2Parser> parser;
	parser.instantiate();

	GDScript2Parser::Result result = parser->parse("");

	CHECK_MESSAGE(result.root != nullptr, "Parser should return a root node for empty source.");
	CHECK_MESSAGE(!result.has_errors(), "Empty source should not produce errors.");
	CHECK_MESSAGE(result.root->type == GDScript2ASTNodeType::NODE_CLASS, "Root should be a class node.");

	if (result.root) {
		memdelete(result.root);
	}
}

// Test simple variable declaration
void test_parser_variable() {
	Ref<GDScript2Parser> parser;
	parser.instantiate();

	GDScript2Parser::Result result = parser->parse("var x = 42\n");

	CHECK_MESSAGE(result.root != nullptr, "Parser should return a root node.");
	CHECK_MESSAGE(!result.has_errors(), "Should not produce errors.");

	if (result.root) {
		GDScript2ClassNode *cls = result.root;
		CHECK_MESSAGE(cls->variables.size() == 1, "Should have one variable.");
		if (cls->variables.size() > 0) {
			CHECK_MESSAGE(cls->variables[0]->name == StringName("x"), "Variable name should be 'x'.");
			CHECK_MESSAGE(cls->variables[0]->initializer != nullptr, "Should have initializer.");
		}
		memdelete(result.root);
	}
}

// Test variable with type hint
void test_parser_variable_typed() {
	Ref<GDScript2Parser> parser;
	parser.instantiate();

	GDScript2Parser::Result result = parser->parse("var x: int = 42\n");

	CHECK_MESSAGE(!result.has_errors(), "Should not produce errors.");

	if (result.root) {
		GDScript2ClassNode *cls = result.root;
		CHECK_MESSAGE(cls->variables.size() == 1, "Should have one variable.");
		if (cls->variables.size() > 0) {
			CHECK_MESSAGE(cls->variables[0]->type_hint != nullptr, "Should have type hint.");
			if (cls->variables[0]->type_hint) {
				CHECK_MESSAGE(cls->variables[0]->type_hint->type_name == StringName("int"), "Type should be 'int'.");
			}
		}
		memdelete(result.root);
	}
}

// Test constant declaration
void test_parser_constant() {
	Ref<GDScript2Parser> parser;
	parser.instantiate();

	GDScript2Parser::Result result = parser->parse("const PI_VAL = 3.14\n");

	CHECK_MESSAGE(!result.has_errors(), "Should not produce errors.");

	if (result.root) {
		GDScript2ClassNode *cls = result.root;
		CHECK_MESSAGE(cls->constants.size() == 1, "Should have one constant.");
		if (cls->constants.size() > 0) {
			CHECK_MESSAGE(cls->constants[0]->name == StringName("PI_VAL"), "Constant name should be 'PI_VAL'.");
		}
		memdelete(result.root);
	}
}

// Test function declaration
void test_parser_function() {
	Ref<GDScript2Parser> parser;
	parser.instantiate();

	String source = R"(
func hello():
	pass
)";

	GDScript2Parser::Result result = parser->parse(source);

	CHECK_MESSAGE(!result.has_errors(), "Should not produce errors.");

	if (result.root) {
		GDScript2ClassNode *cls = result.root;
		CHECK_MESSAGE(cls->functions.size() == 1, "Should have one function.");
		if (cls->functions.size() > 0) {
			CHECK_MESSAGE(cls->functions[0]->name == StringName("hello"), "Function name should be 'hello'.");
			CHECK_MESSAGE(cls->functions[0]->body != nullptr, "Should have body.");
		}
		memdelete(result.root);
	}
}

// Test function with parameters
void test_parser_function_params() {
	Ref<GDScript2Parser> parser;
	parser.instantiate();

	String source = R"(
func add(a: int, b: int) -> int:
	return a + b
)";

	GDScript2Parser::Result result = parser->parse(source);

	CHECK_MESSAGE(!result.has_errors(), "Should not produce errors.");

	if (result.root) {
		GDScript2ClassNode *cls = result.root;
		CHECK_MESSAGE(cls->functions.size() == 1, "Should have one function.");
		if (cls->functions.size() > 0) {
			GDScript2FunctionNode *func = cls->functions[0];
			CHECK_MESSAGE(func->name == StringName("add"), "Function name should be 'add'.");
			CHECK_MESSAGE(func->parameters.size() == 2, "Should have 2 parameters.");
			CHECK_MESSAGE(func->return_type != nullptr, "Should have return type.");
		}
		memdelete(result.root);
	}
}

// Test if statement
void test_parser_if() {
	Ref<GDScript2Parser> parser;
	parser.instantiate();

	String source = R"(
func test():
	if x > 0:
		pass
)";

	GDScript2Parser::Result result = parser->parse(source);

	CHECK_MESSAGE(!result.has_errors(), "Should not produce errors.");

	if (result.root) {
		GDScript2ClassNode *cls = result.root;
		CHECK_MESSAGE(cls->functions.size() == 1, "Should have one function.");
		if (cls->functions.size() > 0 && cls->functions[0]->body) {
			CHECK_MESSAGE(cls->functions[0]->body->statements.size() > 0, "Should have statements.");
			if (cls->functions[0]->body->statements.size() > 0) {
				CHECK_MESSAGE(cls->functions[0]->body->statements[0]->type == GDScript2ASTNodeType::NODE_IF,
						"First statement should be if.");
			}
		}
		memdelete(result.root);
	}
}

// Test if-else statement
void test_parser_if_else() {
	Ref<GDScript2Parser> parser;
	parser.instantiate();

	String source = R"(
func test():
	if x > 0:
		pass
	else:
		pass
)";

	GDScript2Parser::Result result = parser->parse(source);

	CHECK_MESSAGE(!result.has_errors(), "Should not produce errors.");

	if (result.root) {
		GDScript2ClassNode *cls = result.root;
		if (cls->functions.size() > 0 && cls->functions[0]->body &&
				cls->functions[0]->body->statements.size() > 0) {
			GDScript2ASTNode *stmt = cls->functions[0]->body->statements[0];
			CHECK_MESSAGE(stmt->type == GDScript2ASTNodeType::NODE_IF, "Should be if node.");
			GDScript2IfNode *if_node = static_cast<GDScript2IfNode *>(stmt);
			CHECK_MESSAGE(if_node->false_block != nullptr, "Should have else block.");
		}
		memdelete(result.root);
	}
}

// Test for loop
void test_parser_for() {
	Ref<GDScript2Parser> parser;
	parser.instantiate();

	String source = R"(
func test():
	for i in range(10):
		pass
)";

	GDScript2Parser::Result result = parser->parse(source);

	CHECK_MESSAGE(!result.has_errors(), "Should not produce errors.");

	if (result.root) {
		GDScript2ClassNode *cls = result.root;
		if (cls->functions.size() > 0 && cls->functions[0]->body &&
				cls->functions[0]->body->statements.size() > 0) {
			GDScript2ASTNode *stmt = cls->functions[0]->body->statements[0];
			CHECK_MESSAGE(stmt->type == GDScript2ASTNodeType::NODE_FOR, "Should be for node.");
			GDScript2ForNode *for_node = static_cast<GDScript2ForNode *>(stmt);
			CHECK_MESSAGE(for_node->variable == StringName("i"), "Loop variable should be 'i'.");
		}
		memdelete(result.root);
	}
}

// Test while loop
void test_parser_while() {
	Ref<GDScript2Parser> parser;
	parser.instantiate();

	String source = R"(
func test():
	while x > 0:
		pass
)";

	GDScript2Parser::Result result = parser->parse(source);

	CHECK_MESSAGE(!result.has_errors(), "Should not produce errors.");

	if (result.root) {
		GDScript2ClassNode *cls = result.root;
		if (cls->functions.size() > 0 && cls->functions[0]->body &&
				cls->functions[0]->body->statements.size() > 0) {
			GDScript2ASTNode *stmt = cls->functions[0]->body->statements[0];
			CHECK_MESSAGE(stmt->type == GDScript2ASTNodeType::NODE_WHILE, "Should be while node.");
		}
		memdelete(result.root);
	}
}

// Test binary expressions
void test_parser_binary_expr() {
	Ref<GDScript2Parser> parser;
	parser.instantiate();

	String source = R"(
func test():
	var x = 1 + 2 * 3
)";

	GDScript2Parser::Result result = parser->parse(source);

	CHECK_MESSAGE(!result.has_errors(), "Should not produce errors.");

	if (result.root) {
		memdelete(result.root);
	}
}

// Test call expression
void test_parser_call() {
	Ref<GDScript2Parser> parser;
	parser.instantiate();

	String source = R"(
func test():
	print("hello", 42)
)";

	GDScript2Parser::Result result = parser->parse(source);

	CHECK_MESSAGE(!result.has_errors(), "Should not produce errors.");

	if (result.root) {
		memdelete(result.root);
	}
}

// Test array literal
void test_parser_array() {
	Ref<GDScript2Parser> parser;
	parser.instantiate();

	String source = R"(
func test():
	var arr = [1, 2, 3]
)";

	GDScript2Parser::Result result = parser->parse(source);

	CHECK_MESSAGE(!result.has_errors(), "Should not produce errors.");

	if (result.root) {
		memdelete(result.root);
	}
}

// Test dictionary literal
void test_parser_dictionary() {
	Ref<GDScript2Parser> parser;
	parser.instantiate();

	String source = R"(
func test():
	var dict = {"a": 1, "b": 2}
)";

	GDScript2Parser::Result result = parser->parse(source);

	CHECK_MESSAGE(!result.has_errors(), "Should not produce errors.");

	if (result.root) {
		memdelete(result.root);
	}
}

// Test signal declaration
void test_parser_signal() {
	Ref<GDScript2Parser> parser;
	parser.instantiate();

	String source = R"(
signal my_signal(value: int)
)";

	GDScript2Parser::Result result = parser->parse(source);

	CHECK_MESSAGE(!result.has_errors(), "Should not produce errors.");

	if (result.root) {
		GDScript2ClassNode *cls = result.root;
		CHECK_MESSAGE(cls->signals.size() == 1, "Should have one signal.");
		if (cls->signals.size() > 0) {
			CHECK_MESSAGE(cls->signals[0]->name == StringName("my_signal"), "Signal name should be 'my_signal'.");
		}
		memdelete(result.root);
	}
}

// Test enum declaration
void test_parser_enum() {
	Ref<GDScript2Parser> parser;
	parser.instantiate();

	String source = R"(
enum State { IDLE, WALKING, RUNNING }
)";

	GDScript2Parser::Result result = parser->parse(source);

	CHECK_MESSAGE(!result.has_errors(), "Should not produce errors.");

	if (result.root) {
		GDScript2ClassNode *cls = result.root;
		CHECK_MESSAGE(cls->enums.size() == 1, "Should have one enum.");
		if (cls->enums.size() > 0) {
			CHECK_MESSAGE(cls->enums[0]->name == StringName("State"), "Enum name should be 'State'.");
			CHECK_MESSAGE(cls->enums[0]->value_names.size() == 3, "Should have 3 enum values.");
		}
		memdelete(result.root);
	}
}

// Test annotation
void test_parser_annotation() {
	Ref<GDScript2Parser> parser;
	parser.instantiate();

	String source = R"(
@export
var health: int = 100
)";

	GDScript2Parser::Result result = parser->parse(source);

	CHECK_MESSAGE(!result.has_errors(), "Should not produce errors.");

	if (result.root) {
		GDScript2ClassNode *cls = result.root;
		CHECK_MESSAGE(cls->variables.size() == 1, "Should have one variable.");
		if (cls->variables.size() > 0) {
			CHECK_MESSAGE(cls->variables[0]->annotations.size() == 1, "Should have one annotation.");
		}
		memdelete(result.root);
	}
}

// Test class with extends
void test_parser_extends() {
	Ref<GDScript2Parser> parser;
	parser.instantiate();

	String source = R"(
extends Node2D

func _ready():
	pass
)";

	GDScript2Parser::Result result = parser->parse(source);

	CHECK_MESSAGE(!result.has_errors(), "Should not produce errors.");

	if (result.root) {
		GDScript2ClassNode *cls = result.root;
		CHECK_MESSAGE(cls->extends_path == StringName("Node2D"), "Should extend Node2D.");
		memdelete(result.root);
	}
}

// Test inner class
void test_parser_inner_class() {
	Ref<GDScript2Parser> parser;
	parser.instantiate();

	String source = R"(
class InnerClass:
	var value = 0
)";

	GDScript2Parser::Result result = parser->parse(source);

	CHECK_MESSAGE(!result.has_errors(), "Should not produce errors.");

	if (result.root) {
		GDScript2ClassNode *cls = result.root;
		CHECK_MESSAGE(cls->inner_classes.size() == 1, "Should have one inner class.");
		if (cls->inner_classes.size() > 0) {
			CHECK_MESSAGE(cls->inner_classes[0]->name == StringName("InnerClass"), "Inner class name should be 'InnerClass'.");
		}
		memdelete(result.root);
	}
}

// Test match statement
void test_parser_match() {
	Ref<GDScript2Parser> parser;
	parser.instantiate();

	String source = R"(
func test(x):
	match x:
		1:
			pass
		2:
			pass
		_:
			pass
)";

	GDScript2Parser::Result result = parser->parse(source);

	CHECK_MESSAGE(!result.has_errors(), "Should not produce errors.");

	if (result.root) {
		GDScript2ClassNode *cls = result.root;
		if (cls->functions.size() > 0 && cls->functions[0]->body &&
				cls->functions[0]->body->statements.size() > 0) {
			GDScript2ASTNode *stmt = cls->functions[0]->body->statements[0];
			CHECK_MESSAGE(stmt->type == GDScript2ASTNodeType::NODE_MATCH, "Should be match node.");
		}
		memdelete(result.root);
	}
}

// Test return statement
void test_parser_return() {
	Ref<GDScript2Parser> parser;
	parser.instantiate();

	String source = R"(
func add(a, b):
	return a + b
)";

	GDScript2Parser::Result result = parser->parse(source);

	CHECK_MESSAGE(!result.has_errors(), "Should not produce errors.");

	if (result.root) {
		GDScript2ClassNode *cls = result.root;
		if (cls->functions.size() > 0 && cls->functions[0]->body &&
				cls->functions[0]->body->statements.size() > 0) {
			GDScript2ASTNode *stmt = cls->functions[0]->body->statements[0];
			CHECK_MESSAGE(stmt->type == GDScript2ASTNodeType::NODE_RETURN, "Should be return node.");
			GDScript2ReturnNode *ret = static_cast<GDScript2ReturnNode *>(stmt);
			CHECK_MESSAGE(ret->value != nullptr, "Should have return value.");
		}
		memdelete(result.root);
	}
}

// Test static members
void test_parser_static() {
	Ref<GDScript2Parser> parser;
	parser.instantiate();

	String source = R"(
static var count = 0

static func get_count():
	return count
)";

	GDScript2Parser::Result result = parser->parse(source);

	CHECK_MESSAGE(!result.has_errors(), "Should not produce errors.");

	if (result.root) {
		GDScript2ClassNode *cls = result.root;
		CHECK_MESSAGE(cls->variables.size() == 1, "Should have one variable.");
		CHECK_MESSAGE(cls->functions.size() == 1, "Should have one function.");
		if (cls->variables.size() > 0) {
			CHECK_MESSAGE(cls->variables[0]->is_static, "Variable should be static.");
		}
		if (cls->functions.size() > 0) {
			CHECK_MESSAGE(cls->functions[0]->is_static, "Function should be static.");
		}
		memdelete(result.root);
	}
}

// Test error recovery
void test_parser_error_recovery() {
	Ref<GDScript2Parser> parser;
	parser.instantiate();

	String source = R"(
var x =
var y = 10
)";

	GDScript2Parser::Result result = parser->parse(source);

	// Should have errors but still produce some AST
	CHECK_MESSAGE(result.root != nullptr, "Should still produce root node.");
	CHECK_MESSAGE(result.has_errors(), "Should have errors.");

	if (result.root) {
		memdelete(result.root);
	}
}

} // namespace

// Main test function that calls all parser tests
namespace GDScript2Tests {

void test_parser() {
	test_parser_empty();
	test_parser_variable();
	test_parser_variable_typed();
	test_parser_constant();
	test_parser_function();
	test_parser_function_params();
	test_parser_if();
	test_parser_if_else();
	test_parser_for();
	test_parser_while();
	test_parser_binary_expr();
	test_parser_call();
	test_parser_array();
	test_parser_dictionary();
	test_parser_signal();
	test_parser_enum();
	test_parser_annotation();
	test_parser_extends();
	test_parser_inner_class();
	test_parser_match();
	test_parser_return();
	test_parser_static();
	test_parser_error_recovery();
}

} // namespace GDScript2Tests
