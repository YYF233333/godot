/**************************************************************************/
/*  test_gdscript2_semantic.cpp                                           */
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
#include "modules/gdscript2/semantic/gdscript2_semantic.h"

namespace {

// Helper function to parse and analyze
GDScript2SemanticAnalyzer::Result parse_and_analyze(const String &p_source) {
	Ref<GDScript2Parser> parser;
	parser.instantiate();
	GDScript2Parser::Result parse_result = parser->parse(p_source, GDScript2Parser::Options());

	Ref<GDScript2SemanticAnalyzer> sema;
	sema.instantiate();
	GDScript2SemanticAnalyzer::Result sema_result = sema->analyze(parse_result.root);

	// Merge parser errors into semantic result
	if (parse_result.has_errors()) {
		for (const GDScript2Parser::Error &err : parse_result.errors) {
			sema_result.diagnostics.report_error(
					GDScript2DiagnosticCode::ERR_UNKNOWN,
					err.message,
					err.line,
					err.column);
		}
	}

	if (parse_result.root) {
		memdelete(parse_result.root);
	}

	return sema_result;
}

// ============================================================================
// Basic Tests
// ============================================================================

void test_gdscript2_semantic_minimal() {
	auto result = parse_and_analyze("");
	CHECK_MESSAGE(!result.has_errors(), "Empty script should not have errors.");
}

void test_gdscript2_semantic_variable_declaration() {
	auto result = parse_and_analyze(R"(
var x = 10
var y: int = 20
var z: String = "hello"
)");
	CHECK_MESSAGE(!result.has_errors(), "Variable declarations should not have errors.");
}

void test_gdscript2_semantic_constant_declaration() {
	auto result = parse_and_analyze(R"(
const PI_VAL = 3.14159
const NAME: String = "Test"
const COUNT: int = 42
)");
	CHECK_MESSAGE(!result.has_errors(), "Constant declarations should not have errors.");
}

void test_gdscript2_semantic_function_declaration() {
	auto result = parse_and_analyze(R"(
func add(a: int, b: int) -> int:
	return a + b

func greet(name: String) -> void:
	print(name)

func no_return():
	pass
)");
	CHECK_MESSAGE(!result.has_errors(), "Function declarations should not have errors.");
}

// ============================================================================
// Type Checking Tests
// ============================================================================

void test_gdscript2_semantic_type_mismatch() {
	auto result = parse_and_analyze(R"(
var x: int = "hello"
)");
	CHECK_MESSAGE(result.has_errors(), "Type mismatch should produce error.");
}

void test_gdscript2_semantic_numeric_promotion() {
	auto result = parse_and_analyze(R"(
var x: float = 10
var y = 1 + 2.5
)");
	CHECK_MESSAGE(!result.has_errors(), "Numeric promotion should work.");
}

void test_gdscript2_semantic_array_type() {
	auto result = parse_and_analyze(R"(
var arr = [1, 2, 3]
var str_arr = ["a", "b", "c"]
)");
	CHECK_MESSAGE(!result.has_errors(), "Array literals should work.");
}

void test_gdscript2_semantic_dictionary_type() {
	auto result = parse_and_analyze(R"(
var dict = {"key": "value", "num": 42}
)");
	CHECK_MESSAGE(!result.has_errors(), "Dictionary literals should work.");
}

// ============================================================================
// Control Flow Tests
// ============================================================================

void test_gdscript2_semantic_if_statement() {
	auto result = parse_and_analyze(R"(
func test():
	var x = 10
	if x > 5:
		print("big")
	elif x > 0:
		print("small")
	else:
		print("zero or negative")
)");
	CHECK_MESSAGE(!result.has_errors(), "If statement should not have errors.");
}

void test_gdscript2_semantic_for_loop() {
	auto result = parse_and_analyze(R"(
func test():
	for i in range(10):
		print(i)

	for item in [1, 2, 3]:
		print(item)
)");
	CHECK_MESSAGE(!result.has_errors(), "For loop should not have errors.");
}

void test_gdscript2_semantic_while_loop() {
	auto result = parse_and_analyze(R"(
func test():
	var x = 10
	while x > 0:
		x -= 1
)");
	CHECK_MESSAGE(!result.has_errors(), "While loop should not have errors.");
}

void test_gdscript2_semantic_break_outside_loop() {
	auto result = parse_and_analyze(R"(
func test():
	break
)");
	CHECK_MESSAGE(result.has_errors(), "Break outside loop should produce error.");
}

void test_gdscript2_semantic_continue_outside_loop() {
	auto result = parse_and_analyze(R"(
func test():
	continue
)");
	CHECK_MESSAGE(result.has_errors(), "Continue outside loop should produce error.");
}

void test_gdscript2_semantic_return_outside_function() {
	auto result = parse_and_analyze(R"(
return 42
)");
	CHECK_MESSAGE(result.has_errors(), "Return outside function should produce error.");
}

// ============================================================================
// Scope Tests
// ============================================================================

void test_gdscript2_semantic_undefined_variable() {
	auto result = parse_and_analyze(R"(
func test():
	print(undefined_var)
)");
	CHECK_MESSAGE(result.has_errors(), "Undefined variable should produce error.");
}

void test_gdscript2_semantic_duplicate_declaration() {
	auto result = parse_and_analyze(R"(
var x = 10
var x = 20
)");
	CHECK_MESSAGE(result.has_errors(), "Duplicate declaration should produce error.");
}

void test_gdscript2_semantic_scope_isolation() {
	auto result = parse_and_analyze(R"(
func test():
	if true:
		var local = 10
	print(local)
)");
	CHECK_MESSAGE(result.has_errors(), "Variable from inner scope should not be accessible.");
}

// ============================================================================
// Class Tests
// ============================================================================

void test_gdscript2_semantic_class_members() {
	auto result = parse_and_analyze(R"(
var member_var = 10
const MEMBER_CONST = 20

signal my_signal(value)

func member_func():
	pass
)");
	CHECK_MESSAGE(!result.has_errors(), "Class members should not have errors.");
}

void test_gdscript2_semantic_enum() {
	auto result = parse_and_analyze(R"(
enum Direction { UP, DOWN, LEFT, RIGHT }
enum { NORTH = 0, SOUTH = 1, EAST = 2, WEST = 3 }
)");
	CHECK_MESSAGE(!result.has_errors(), "Enum declarations should not have errors.");
}

void test_gdscript2_semantic_self_in_static() {
	auto result = parse_and_analyze(R"(
static func static_func():
	print(self)
)");
	CHECK_MESSAGE(result.has_errors(), "Using self in static function should produce error.");
}

// ============================================================================
// Expression Tests
// ============================================================================

void test_gdscript2_semantic_binary_ops() {
	auto result = parse_and_analyze(R"(
func test():
	var a = 1 + 2
	var b = 3 - 4
	var c = 5 * 6
	var d = 7 / 8
	var e = 9 % 10
	var f = 2 ** 3
	var g = true and false
	var h = true or false
	var i = 1 == 2
	var j = 1 != 2
	var k = 1 < 2
	var l = 1 <= 2
	var m = 1 > 2
	var n = 1 >= 2
)");
	CHECK_MESSAGE(!result.has_errors(), "Binary operators should not have errors.");
}

void test_gdscript2_semantic_unary_ops() {
	auto result = parse_and_analyze(R"(
func test():
	var a = -10
	var b = +20
	var c = not true
	var d = ~0xFF
)");
	CHECK_MESSAGE(!result.has_errors(), "Unary operators should not have errors.");
}

void test_gdscript2_semantic_ternary_op() {
	auto result = parse_and_analyze(R"(
func test():
	var x = 10
	var y = 20 if x > 5 else 30
)");
	CHECK_MESSAGE(!result.has_errors(), "Ternary operator should not have errors.");
}

void test_gdscript2_semantic_assignment_to_constant() {
	auto result = parse_and_analyze(R"(
const X = 10

func test():
	X = 20
)");
	CHECK_MESSAGE(result.has_errors(), "Assignment to constant should produce error.");
}

// ============================================================================
// Function Call Tests
// ============================================================================

void test_gdscript2_semantic_function_call() {
	auto result = parse_and_analyze(R"(
func add(a, b):
	return a + b

func test():
	var result = add(1, 2)
)");
	CHECK_MESSAGE(!result.has_errors(), "Function call should not have errors.");
}

void test_gdscript2_semantic_builtin_call() {
	auto result = parse_and_analyze(R"(
func test():
	var s = str(42)
	var l = len([1, 2, 3])
	print("Hello")
)");
	CHECK_MESSAGE(!result.has_errors(), "Built-in function calls should not have errors.");
}

// ============================================================================
// Match Statement Tests
// ============================================================================

void test_gdscript2_semantic_match_statement() {
	auto result = parse_and_analyze(R"(
func test(x):
	match x:
		1:
			print("one")
		2:
			print("two")
		_:
			print("other")
)");
	CHECK_MESSAGE(!result.has_errors(), "Match statement should not have errors.");
}

void test_gdscript2_semantic_match_binding() {
	auto result = parse_and_analyze(R"(
func test(x):
	match x:
		[var first, var rest]:
			print(first)
		{"key": var value}:
			print(value)
)");
	CHECK_MESSAGE(!result.has_errors(), "Match binding patterns should not have errors.");
}

// ============================================================================
// Lambda Tests
// ============================================================================

void test_gdscript2_semantic_lambda() {
	auto result = parse_and_analyze(R"(
func test():
	var add = func(a, b): return a + b
	var result = add.call(1, 2)
)");
	CHECK_MESSAGE(!result.has_errors(), "Lambda should not have errors.");
}

// ============================================================================
// Await Tests
// ============================================================================

void test_gdscript2_semantic_await_in_function() {
	auto result = parse_and_analyze(R"(
signal done

func test():
	await done
)");
	CHECK_MESSAGE(!result.has_errors(), "Await in function should not have errors.");
}

void test_gdscript2_semantic_await_outside_function() {
	auto result = parse_and_analyze(R"(
signal done
await done
)");
	CHECK_MESSAGE(result.has_errors(), "Await outside function should produce error.");
}

// ============================================================================
// Type System Tests
// ============================================================================

void test_gdscript2_semantic_type_from_variant() {
	// Test that types are correctly inferred from literals
	auto result = parse_and_analyze(R"(
func test():
	var i = 42
	var f = 3.14
	var s = "hello"
	var b = true
	var v = Vector2(1, 2)
	var c = Color(1, 0, 0)
)");
	CHECK_MESSAGE(!result.has_errors(), "Type inference from literals should work.");
}

void test_gdscript2_semantic_type_compatibility() {
	// Test type compatibility checks
	auto result = parse_and_analyze(R"(
func test():
	var x: float = 10  # int -> float is ok
	var y: String = "test"
	var z: Object = null  # null -> Object is ok
)");
	CHECK_MESSAGE(!result.has_errors(), "Compatible types should work.");
}

// ============================================================================
// Symbol Table Tests
// ============================================================================

void test_gdscript2_semantic_symbol_lookup() {
	auto result = parse_and_analyze(R"(
var global_var = 10

func test():
	var local_var = 20
	print(global_var)
	print(local_var)
)");
	CHECK_MESSAGE(!result.has_errors(), "Symbol lookup should work for both global and local.");
}

void test_gdscript2_semantic_nested_scope() {
	auto result = parse_and_analyze(R"(
func test():
	var outer = 10
	if true:
		var inner = 20
		print(outer)
		print(inner)
	print(outer)
)");
	CHECK_MESSAGE(!result.has_errors(), "Nested scope should work.");
}

} // namespace

// Main test function that calls all semantic tests
namespace GDScript2Tests {

void test_semantic() {
	test_gdscript2_semantic_minimal();
	test_gdscript2_semantic_variable_declaration();
	test_gdscript2_semantic_constant_declaration();
	test_gdscript2_semantic_function_declaration();
	test_gdscript2_semantic_type_mismatch();
	test_gdscript2_semantic_numeric_promotion();
	test_gdscript2_semantic_array_type();
	test_gdscript2_semantic_dictionary_type();
	test_gdscript2_semantic_if_statement();
	test_gdscript2_semantic_for_loop();
	test_gdscript2_semantic_while_loop();
	test_gdscript2_semantic_break_outside_loop();
	test_gdscript2_semantic_continue_outside_loop();
	test_gdscript2_semantic_return_outside_function();
	test_gdscript2_semantic_undefined_variable();
	test_gdscript2_semantic_duplicate_declaration();
	test_gdscript2_semantic_scope_isolation();
	test_gdscript2_semantic_class_members();
	test_gdscript2_semantic_enum();
	test_gdscript2_semantic_self_in_static();
	test_gdscript2_semantic_binary_ops();
	test_gdscript2_semantic_unary_ops();
	test_gdscript2_semantic_ternary_op();
	test_gdscript2_semantic_assignment_to_constant();
	test_gdscript2_semantic_function_call();
	test_gdscript2_semantic_builtin_call();
	test_gdscript2_semantic_match_statement();
	test_gdscript2_semantic_match_binding();
	test_gdscript2_semantic_lambda();
	test_gdscript2_semantic_await_in_function();
	test_gdscript2_semantic_await_outside_function();
	test_gdscript2_semantic_type_from_variant();
	test_gdscript2_semantic_type_compatibility();
	test_gdscript2_semantic_symbol_lookup();
	test_gdscript2_semantic_nested_scope();
}

} // namespace GDScript2Tests
