/**************************************************************************/
/*  test_gdscript2_vm.cpp                                                 */
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

#include "modules/gdscript2/codegen/gdscript2_codegen.h"
#include "modules/gdscript2/front/gdscript2_parser.h"
#include "modules/gdscript2/ir/gdscript2_ir_builder.h"
#include "modules/gdscript2/semantic/gdscript2_semantic.h"
#include "modules/gdscript2/vm/gdscript2_vm.h"

namespace {

// Helper to compile and execute GDScript2 code
struct TestContext {
	GDScript2ASTNode *ast = nullptr;
	GDScript2CompiledModule compiled_module;
	Ref<GDScript2VM> vm;

	bool compile(const String &p_source) {
		// Parse
		Ref<GDScript2Parser> parser;
		parser.instantiate();
		GDScript2Parser::Result parse_result = parser->parse(p_source, GDScript2Parser::Options());
		ast = parse_result.root;

		if (!parse_result.errors.is_empty()) {
			return false;
		}

		// Semantic analysis
		Ref<GDScript2SemanticAnalyzer> sema;
		sema.instantiate();
		GDScript2SemanticAnalyzer::Result sema_result = sema->analyze(ast);

		// Build IR
		Ref<GDScript2IRBuilder> ir_builder;
		ir_builder.instantiate();
		GDScript2IRBuilder::Result ir_result = ir_builder->build(ast);

		// Generate bytecode
		Ref<GDScript2CodeGenerator> codegen;
		codegen.instantiate();
		GDScript2CodeGenerator::Result codegen_result = codegen->generate(ir_result.module);

		if (codegen_result.has_errors()) {
			return false;
		}

		compiled_module = codegen_result.module;

		// Create VM
		vm.instantiate();
		vm->load_module(&compiled_module);

		return true;
	}

	GDScript2ExecutionResult call(const StringName &p_func, const Vector<Variant> &p_args = Vector<Variant>()) {
		if (vm.is_null()) {
			return GDScript2ExecutionResult::make_error(GDScript2ExecutionResult::ERROR_RUNTIME, "VM not initialized");
		}
		return vm->call(p_func, p_args);
	}

	~TestContext() {
		if (ast) {
			memdelete(ast);
		}
	}
};

// ============================================================================
// Basic Execution Tests
// ============================================================================

void test_gdscript2_vm_empty() {
	TestContext ctx;
	CHECK(ctx.compile(""));
	CHECK(ctx.vm.is_valid());
}

void test_gdscript2_vm_return_constant() {
	TestContext ctx;
	String source = R"(
func test():
	return 42
)";
	CHECK(ctx.compile(source));

	GDScript2ExecutionResult result = ctx.call("test");
	CHECK_MESSAGE(result.is_ok(), "Should execute without error");
	CHECK_MESSAGE(result.return_value == Variant(42), "Should return 42");
}

void test_gdscript2_vm_return_string() {
	TestContext ctx;
	String source = R"(
func test():
	return "hello"
)";
	CHECK(ctx.compile(source));

	GDScript2ExecutionResult result = ctx.call("test");
	CHECK_MESSAGE(result.is_ok(), "Should execute without error");
	CHECK_MESSAGE(result.return_value == Variant("hello"), "Should return 'hello'");
}

void test_gdscript2_vm_return_nil() {
	TestContext ctx;
	String source = R"(
func test():
	return null
)";
	CHECK(ctx.compile(source));

	GDScript2ExecutionResult result = ctx.call("test");
	CHECK_MESSAGE(result.is_ok(), "Should execute without error");
	CHECK_MESSAGE(result.return_value.get_type() == Variant::NIL, "Should return null");
}

void test_gdscript2_vm_return_bool() {
	TestContext ctx;
	String source = R"(
func test_true():
	return true

func test_false():
	return false
)";
	CHECK(ctx.compile(source));

	GDScript2ExecutionResult result1 = ctx.call("test_true");
	CHECK_MESSAGE(result1.return_value == Variant(true), "Should return true");

	GDScript2ExecutionResult result2 = ctx.call("test_false");
	CHECK_MESSAGE(result2.return_value == Variant(false), "Should return false");
}

// ============================================================================
// Arithmetic Tests
// ============================================================================

void test_gdscript2_vm_arithmetic_add() {
	TestContext ctx;
	String source = R"(
func test(a, b):
	return a + b
)";
	CHECK(ctx.compile(source));

	Vector<Variant> args;
	args.push_back(10);
	args.push_back(32);

	GDScript2ExecutionResult result = ctx.call("test", args);
	CHECK_MESSAGE(result.is_ok(), "Should execute without error");
	CHECK_MESSAGE(result.return_value == Variant(42), "10 + 32 should equal 42");
}

void test_gdscript2_vm_arithmetic_sub() {
	TestContext ctx;
	String source = R"(
func test(a, b):
	return a - b
)";
	CHECK(ctx.compile(source));

	Vector<Variant> args;
	args.push_back(50);
	args.push_back(8);

	GDScript2ExecutionResult result = ctx.call("test", args);
	CHECK_MESSAGE(result.is_ok(), "Should execute without error");
	CHECK_MESSAGE(result.return_value == Variant(42), "50 - 8 should equal 42");
}

void test_gdscript2_vm_arithmetic_mul() {
	TestContext ctx;
	String source = R"(
func test(a, b):
	return a * b
)";
	CHECK(ctx.compile(source));

	Vector<Variant> args;
	args.push_back(6);
	args.push_back(7);

	GDScript2ExecutionResult result = ctx.call("test", args);
	CHECK_MESSAGE(result.is_ok(), "Should execute without error");
	CHECK_MESSAGE(result.return_value == Variant(42), "6 * 7 should equal 42");
}

void test_gdscript2_vm_arithmetic_div() {
	TestContext ctx;
	String source = R"(
func test(a, b):
	return a / b
)";
	CHECK(ctx.compile(source));

	Vector<Variant> args;
	args.push_back(84);
	args.push_back(2);

	GDScript2ExecutionResult result = ctx.call("test", args);
	CHECK_MESSAGE(result.is_ok(), "Should execute without error");
	CHECK_MESSAGE(result.return_value == Variant(42), "84 / 2 should equal 42");
}

void test_gdscript2_vm_arithmetic_mod() {
	TestContext ctx;
	String source = R"(
func test(a, b):
	return a % b
)";
	CHECK(ctx.compile(source));

	Vector<Variant> args;
	args.push_back(47);
	args.push_back(5);

	GDScript2ExecutionResult result = ctx.call("test", args);
	CHECK_MESSAGE(result.is_ok(), "Should execute without error");
	CHECK_MESSAGE(result.return_value == Variant(2), "47 % 5 should equal 2");
}

void test_gdscript2_vm_arithmetic_complex() {
	TestContext ctx;
	String source = R"(
func test(a, b, c):
	return (a + b) * c
)";
	CHECK(ctx.compile(source));

	Vector<Variant> args;
	args.push_back(3);
	args.push_back(4);
	args.push_back(6);

	GDScript2ExecutionResult result = ctx.call("test", args);
	CHECK_MESSAGE(result.is_ok(), "Should execute without error");
	CHECK_MESSAGE(result.return_value == Variant(42), "(3 + 4) * 6 should equal 42");
}

// ============================================================================
// Comparison Tests
// ============================================================================

void test_gdscript2_vm_comparison_eq() {
	TestContext ctx;
	String source = R"(
func test(a, b):
	return a == b
)";
	CHECK(ctx.compile(source));

	Vector<Variant> args1;
	args1.push_back(42);
	args1.push_back(42);
	CHECK_MESSAGE(ctx.call("test", args1).return_value == Variant(true), "42 == 42 should be true");

	Vector<Variant> args2;
	args2.push_back(42);
	args2.push_back(0);
	CHECK_MESSAGE(ctx.call("test", args2).return_value == Variant(false), "42 == 0 should be false");
}

void test_gdscript2_vm_comparison_lt() {
	TestContext ctx;
	String source = R"(
func test(a, b):
	return a < b
)";
	CHECK(ctx.compile(source));

	Vector<Variant> args1;
	args1.push_back(5);
	args1.push_back(10);
	CHECK_MESSAGE(ctx.call("test", args1).return_value == Variant(true), "5 < 10 should be true");

	Vector<Variant> args2;
	args2.push_back(10);
	args2.push_back(5);
	CHECK_MESSAGE(ctx.call("test", args2).return_value == Variant(false), "10 < 5 should be false");
}

// ============================================================================
// Control Flow Tests
// ============================================================================

void test_gdscript2_vm_if_statement() {
	TestContext ctx;
	String source = R"(
func test(x):
	if x > 0:
		return 1
	else:
		return -1
)";
	CHECK(ctx.compile(source));

	Vector<Variant> args1;
	args1.push_back(10);
	CHECK_MESSAGE(ctx.call("test", args1).return_value == Variant(1), "test(10) should return 1");

	Vector<Variant> args2;
	args2.push_back(-5);
	CHECK_MESSAGE(ctx.call("test", args2).return_value == Variant(-1), "test(-5) should return -1");
}

void test_gdscript2_vm_while_loop() {
	TestContext ctx;
	String source = R"(
func test(n):
	var sum = 0
	var i = 1
	while i <= n:
		sum = sum + i
		i = i + 1
	return sum
)";
	CHECK(ctx.compile(source));

	Vector<Variant> args;
	args.push_back(10);
	GDScript2ExecutionResult result = ctx.call("test", args);
	CHECK_MESSAGE(result.is_ok(), "Should execute without error");
	CHECK_MESSAGE(result.return_value == Variant(55), "Sum 1..10 should be 55");
}

void test_gdscript2_vm_nested_if() {
	TestContext ctx;
	String source = R"(
func test(x, y):
	if x > 0:
		if y > 0:
			return 1
		else:
			return 2
	else:
		if y > 0:
			return 3
		else:
			return 4
)";
	CHECK(ctx.compile(source));

	Vector<Variant> args1;
	args1.push_back(1);
	args1.push_back(1);
	CHECK_MESSAGE(ctx.call("test", args1).return_value == Variant(1), "test(1,1) should return 1");

	Vector<Variant> args2;
	args2.push_back(1);
	args2.push_back(-1);
	CHECK_MESSAGE(ctx.call("test", args2).return_value == Variant(2), "test(1,-1) should return 2");

	Vector<Variant> args3;
	args3.push_back(-1);
	args3.push_back(1);
	CHECK_MESSAGE(ctx.call("test", args3).return_value == Variant(3), "test(-1,1) should return 3");

	Vector<Variant> args4;
	args4.push_back(-1);
	args4.push_back(-1);
	CHECK_MESSAGE(ctx.call("test", args4).return_value == Variant(4), "test(-1,-1) should return 4");
}

// ============================================================================
// Variable Tests
// ============================================================================

void test_gdscript2_vm_local_variables() {
	TestContext ctx;
	String source = R"(
func test():
	var x = 10
	var y = 20
	var z = x + y
	return z
)";
	CHECK(ctx.compile(source));

	GDScript2ExecutionResult result = ctx.call("test");
	CHECK_MESSAGE(result.is_ok(), "Should execute without error");
	CHECK_MESSAGE(result.return_value == Variant(30), "x + y should equal 30");
}

void test_gdscript2_vm_variable_reassign() {
	TestContext ctx;
	String source = R"(
func test():
	var x = 10
	x = x + 5
	x = x * 2
	return x
)";
	CHECK(ctx.compile(source));

	GDScript2ExecutionResult result = ctx.call("test");
	CHECK_MESSAGE(result.is_ok(), "Should execute without error");
	CHECK_MESSAGE(result.return_value == Variant(30), "(10 + 5) * 2 should equal 30");
}

// ============================================================================
// Array Tests
// ============================================================================

void test_gdscript2_vm_array_create() {
	TestContext ctx;
	String source = R"(
func test():
	var arr = [1, 2, 3]
	return arr
)";
	CHECK(ctx.compile(source));

	GDScript2ExecutionResult result = ctx.call("test");
	CHECK_MESSAGE(result.is_ok(), "Should execute without error");
	CHECK_MESSAGE(result.return_value.get_type() == Variant::ARRAY, "Should return array");

	Array arr = result.return_value;
	CHECK_MESSAGE(arr.size() == 3, "Array should have 3 elements");
	CHECK_MESSAGE(arr[0] == Variant(1), "arr[0] should be 1");
	CHECK_MESSAGE(arr[1] == Variant(2), "arr[1] should be 2");
	CHECK_MESSAGE(arr[2] == Variant(3), "arr[2] should be 3");
}

void test_gdscript2_vm_array_access() {
	TestContext ctx;
	String source = R"(
func test():
	var arr = [10, 20, 30]
	return arr[1]
)";
	CHECK(ctx.compile(source));

	GDScript2ExecutionResult result = ctx.call("test");
	CHECK_MESSAGE(result.is_ok(), "Should execute without error");
	CHECK_MESSAGE(result.return_value == Variant(20), "arr[1] should be 20");
}

void test_gdscript2_vm_array_modify() {
	TestContext ctx;
	String source = R"(
func test():
	var arr = [1, 2, 3]
	arr[1] = 42
	return arr[1]
)";
	CHECK(ctx.compile(source));

	GDScript2ExecutionResult result = ctx.call("test");
	CHECK_MESSAGE(result.is_ok(), "Should execute without error");
	CHECK_MESSAGE(result.return_value == Variant(42), "arr[1] should be 42 after modification");
}

// ============================================================================
// Dictionary Tests
// ============================================================================

void test_gdscript2_vm_dictionary_create() {
	TestContext ctx;
	String source = R"(
func test():
	var dict = {"a": 1, "b": 2}
	return dict
)";
	CHECK(ctx.compile(source));

	GDScript2ExecutionResult result = ctx.call("test");
	CHECK_MESSAGE(result.is_ok(), "Should execute without error");
	CHECK_MESSAGE(result.return_value.get_type() == Variant::DICTIONARY, "Should return dictionary");

	Dictionary dict = result.return_value;
	CHECK_MESSAGE(dict.size() == 2, "Dict should have 2 entries");
}

void test_gdscript2_vm_dictionary_access() {
	TestContext ctx;
	String source = R"(
func test():
	var dict = {"x": 10, "y": 20}
	return dict["y"]
)";
	CHECK(ctx.compile(source));

	GDScript2ExecutionResult result = ctx.call("test");
	CHECK_MESSAGE(result.is_ok(), "Should execute without error");
	CHECK_MESSAGE(result.return_value == Variant(20), "dict['y'] should be 20");
}

// ============================================================================
// Function Call Tests
// ============================================================================

void test_gdscript2_vm_function_call() {
	TestContext ctx;
	String source = R"(
func helper(x):
	return x * 2

func test():
	return helper(21)
)";
	CHECK(ctx.compile(source));

	GDScript2ExecutionResult result = ctx.call("test");
	CHECK_MESSAGE(result.is_ok(), "Should execute without error");
	CHECK_MESSAGE(result.return_value == Variant(42), "helper(21) should return 42");
}

void test_gdscript2_vm_recursive_function() {
	TestContext ctx;
	String source = R"(
func factorial(n):
	if n <= 1:
		return 1
	return n * factorial(n - 1)

func test():
	return factorial(5)
)";
	CHECK(ctx.compile(source));

	GDScript2ExecutionResult result = ctx.call("test");
	CHECK_MESSAGE(result.is_ok(), "Should execute without error");
	CHECK_MESSAGE(result.return_value == Variant(120), "factorial(5) should be 120");
}

void test_gdscript2_vm_fibonacci() {
	TestContext ctx;
	String source = R"(
func fib(n):
	if n <= 1:
		return n
	return fib(n - 1) + fib(n - 2)

func test():
	return fib(10)
)";
	CHECK(ctx.compile(source));

	GDScript2ExecutionResult result = ctx.call("test");
	CHECK_MESSAGE(result.is_ok(), "Should execute without error");
	CHECK_MESSAGE(result.return_value == Variant(55), "fib(10) should be 55");
}

// ============================================================================
// Unary Operator Tests
// ============================================================================

void test_gdscript2_vm_unary_neg() {
	TestContext ctx;
	String source = R"(
func test(x):
	return -x
)";
	CHECK(ctx.compile(source));

	Vector<Variant> args;
	args.push_back(42);

	GDScript2ExecutionResult result = ctx.call("test", args);
	CHECK_MESSAGE(result.is_ok(), "Should execute without error");
	CHECK_MESSAGE(result.return_value == Variant(-42), "-42 should equal -42");
}

void test_gdscript2_vm_unary_not() {
	TestContext ctx;
	String source = R"(
func test(x):
	return not x
)";
	CHECK(ctx.compile(source));

	Vector<Variant> args1;
	args1.push_back(true);
	CHECK_MESSAGE(ctx.call("test", args1).return_value == Variant(false), "not true should be false");

	Vector<Variant> args2;
	args2.push_back(false);
	CHECK_MESSAGE(ctx.call("test", args2).return_value == Variant(true), "not false should be true");
}

// ============================================================================
// Global Variable Tests
// ============================================================================

void test_gdscript2_vm_globals() {
	TestContext ctx;
	String source = R"(
func test():
	return 1
)";
	CHECK(ctx.compile(source));

	// Set global
	ctx.vm->set_global("my_global", 42);

	CHECK_MESSAGE(ctx.vm->has_global("my_global"), "Should have global");
	CHECK_MESSAGE(ctx.vm->get_global("my_global") == Variant(42), "Global should be 42");
}

// ============================================================================
// Error Handling Tests
// ============================================================================

void test_gdscript2_vm_function_not_found() {
	TestContext ctx;
	String source = R"(
func test():
	return 1
)";
	CHECK(ctx.compile(source));

	GDScript2ExecutionResult result = ctx.call("nonexistent");
	CHECK_MESSAGE(result.has_error(), "Should report error for nonexistent function");
	CHECK_MESSAGE(result.status == GDScript2ExecutionResult::ERROR_RUNTIME, "Should be runtime error");
}

void test_gdscript2_vm_no_module() {
	Ref<GDScript2VM> vm;
	vm.instantiate();

	GDScript2ExecutionResult result = vm->call("test");
	CHECK_MESSAGE(result.has_error(), "Should report error when no module loaded");
}

// ============================================================================
// String Operations Tests
// ============================================================================

void test_gdscript2_vm_string_concat() {
	TestContext ctx;
	String source = R"(
func test():
	var a = "Hello"
	var b = " World"
	return a + b
)";
	CHECK(ctx.compile(source));

	GDScript2ExecutionResult result = ctx.call("test");
	CHECK_MESSAGE(result.is_ok(), "Should execute without error");
	CHECK_MESSAGE(result.return_value == Variant("Hello World"), "Should concatenate strings");
}

} // namespace

// Main test function
namespace GDScript2Tests {

void test_vm() {
	test_gdscript2_vm_empty();
	test_gdscript2_vm_return_constant();
	test_gdscript2_vm_return_string();
	test_gdscript2_vm_return_nil();
	test_gdscript2_vm_return_bool();
	test_gdscript2_vm_arithmetic_add();
	test_gdscript2_vm_arithmetic_sub();
	test_gdscript2_vm_arithmetic_mul();
	test_gdscript2_vm_arithmetic_div();
	test_gdscript2_vm_arithmetic_mod();
	test_gdscript2_vm_arithmetic_complex();
	test_gdscript2_vm_comparison_eq();
	test_gdscript2_vm_comparison_lt();
	test_gdscript2_vm_if_statement();
	test_gdscript2_vm_while_loop();
	test_gdscript2_vm_nested_if();
	test_gdscript2_vm_local_variables();
	test_gdscript2_vm_variable_reassign();
	test_gdscript2_vm_array_create();
	test_gdscript2_vm_array_access();
	test_gdscript2_vm_array_modify();
	test_gdscript2_vm_dictionary_create();
	test_gdscript2_vm_dictionary_access();
	test_gdscript2_vm_function_call();
	test_gdscript2_vm_recursive_function();
	test_gdscript2_vm_fibonacci();
	test_gdscript2_vm_unary_neg();
	test_gdscript2_vm_unary_not();
	test_gdscript2_vm_globals();
	test_gdscript2_vm_function_not_found();
	test_gdscript2_vm_no_module();
	test_gdscript2_vm_string_concat();
}

} // namespace GDScript2Tests
