/**************************************************************************/
/*  test_gdscript2_ir.cpp                                                 */
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
#include "modules/gdscript2/ir/gdscript2_ir_builder.h"
#include "modules/gdscript2/ir/gdscript2_ir_pass.h"
#include "modules/gdscript2/semantic/gdscript2_semantic.h"

namespace {

// Helper: parse and build IR
GDScript2IRBuilder::Result build_ir(const String &p_source) {
	Ref<GDScript2Parser> parser;
	parser.instantiate();
	GDScript2Parser::Result parse_result = parser->parse(p_source, GDScript2Parser::Options());

	Ref<GDScript2IRBuilder> builder;
	builder.instantiate();
	GDScript2IRBuilder::Result ir_result = builder->build(parse_result.root);

	if (parse_result.root) {
		memdelete(parse_result.root);
	}

	return ir_result;
}

// ============================================================================
// IR Building Tests
// ============================================================================

void test_gdscript2_ir_build_empty() {
	auto result = build_ir("");
	CHECK_MESSAGE(!result.has_errors(), "Empty script should build without errors.");
}

void test_gdscript2_ir_build_function() {
	auto result = build_ir(R"(
func test():
	pass
)");
	CHECK_MESSAGE(!result.has_errors(), "Simple function should build without errors.");
	CHECK_MESSAGE(result.module.has_function("test"), "Function 'test' should exist.");
}

void test_gdscript2_ir_build_variables() {
	auto result = build_ir(R"(
func test():
	var x = 10
	var y = 20
	var z = x + y
)");
	CHECK_MESSAGE(!result.has_errors(), "Variable declarations should build without errors.");

	GDScript2IRFunction *func = result.module.get_function("test");
	CHECK_MESSAGE(func != nullptr, "Function should exist.");
	CHECK_MESSAGE(func->has_local("x"), "Local 'x' should exist.");
	CHECK_MESSAGE(func->has_local("y"), "Local 'y' should exist.");
	CHECK_MESSAGE(func->has_local("z"), "Local 'z' should exist.");
}

void test_gdscript2_ir_build_if() {
	auto result = build_ir(R"(
func test(x):
	if x > 0:
		return 1
	else:
		return -1
)");
	CHECK_MESSAGE(!result.has_errors(), "If statement should build without errors.");

	GDScript2IRFunction *func = result.module.get_function("test");
	CHECK_MESSAGE(func != nullptr, "Function should exist.");
	// Should have multiple blocks for if/else
	CHECK_MESSAGE(func->blocks.size() >= 3, "Should have at least 3 blocks for if/else.");
}

void test_gdscript2_ir_build_for() {
	auto result = build_ir(R"(
func test():
	var sum = 0
	for i in range(10):
		sum += i
	return sum
)");
	CHECK_MESSAGE(!result.has_errors(), "For loop should build without errors.");

	GDScript2IRFunction *func = result.module.get_function("test");
	CHECK_MESSAGE(func != nullptr, "Function should exist.");
	CHECK_MESSAGE(func->has_local("i"), "Loop variable 'i' should exist.");
}

void test_gdscript2_ir_build_while() {
	auto result = build_ir(R"(
func test():
	var x = 10
	while x > 0:
		x -= 1
	return x
)");
	CHECK_MESSAGE(!result.has_errors(), "While loop should build without errors.");
}

void test_gdscript2_ir_build_call() {
	auto result = build_ir(R"(
func add(a, b):
	return a + b

func test():
	return add(1, 2)
)");
	CHECK_MESSAGE(!result.has_errors(), "Function call should build without errors.");
	CHECK_MESSAGE(result.module.has_function("add"), "Function 'add' should exist.");
	CHECK_MESSAGE(result.module.has_function("test"), "Function 'test' should exist.");
}

void test_gdscript2_ir_build_array() {
	auto result = build_ir(R"(
func test():
	var arr = [1, 2, 3]
	return arr[0]
)");
	CHECK_MESSAGE(!result.has_errors(), "Array operations should build without errors.");
}

void test_gdscript2_ir_build_dictionary() {
	auto result = build_ir(R"(
func test():
	var dict = {"key": "value"}
	return dict["key"]
)");
	CHECK_MESSAGE(!result.has_errors(), "Dictionary operations should build without errors.");
}

void test_gdscript2_ir_build_binary_ops() {
	auto result = build_ir(R"(
func test():
	var a = 1 + 2
	var b = 3 - 4
	var c = 5 * 6
	var d = 7 / 8
	var e = 9 % 10
	var f = 2 ** 3
	return a + b + c + d + e + f
)");
	CHECK_MESSAGE(!result.has_errors(), "Binary operators should build without errors.");
}

void test_gdscript2_ir_build_comparison() {
	auto result = build_ir(R"(
func test(x, y):
	var a = x == y
	var b = x != y
	var c = x < y
	var d = x <= y
	var e = x > y
	var f = x >= y
	return a or b or c or d or e or f
)");
	CHECK_MESSAGE(!result.has_errors(), "Comparison operators should build without errors.");
}

void test_gdscript2_ir_build_logical_short_circuit() {
	auto result = build_ir(R"(
func test(x, y):
	var a = x and y
	var b = x or y
	return a or b
)");
	CHECK_MESSAGE(!result.has_errors(), "Logical operators should build without errors.");

	// Should have multiple blocks for short-circuit evaluation
	GDScript2IRFunction *func = result.module.get_function("test");
	CHECK_MESSAGE(func != nullptr, "Function should exist.");
	CHECK_MESSAGE(func->blocks.size() >= 3, "Should have blocks for short-circuit.");
}

void test_gdscript2_ir_build_ternary() {
	auto result = build_ir(R"(
func test(x):
	return 1 if x > 0 else -1
)");
	CHECK_MESSAGE(!result.has_errors(), "Ternary operator should build without errors.");
}

void test_gdscript2_ir_build_match() {
	auto result = build_ir(R"(
func test(x):
	match x:
		1:
			return "one"
		2:
			return "two"
		_:
			return "other"
)");
	CHECK_MESSAGE(!result.has_errors(), "Match statement should build without errors.");
}

// ============================================================================
// IR Pass Tests
// ============================================================================

void test_gdscript2_ir_pass_const_fold() {
	auto result = build_ir(R"(
func test():
	var x = 1 + 2
	return x
)");
	CHECK_MESSAGE(!result.has_errors(), "Should build without errors.");

	// Run constant folding
	GDScript2ConstFoldPass pass;
	bool ok = pass.run(result.module);
	CHECK_MESSAGE(ok, "Constant folding should succeed.");
}

void test_gdscript2_ir_pass_dce() {
	auto result = build_ir(R"(
func test():
	var unused = 10
	return 42
)");
	CHECK_MESSAGE(!result.has_errors(), "Should build without errors.");

	// Run DCE
	GDScript2DCEPass pass;
	bool ok = pass.run(result.module);
	CHECK_MESSAGE(ok, "DCE should succeed.");
}

void test_gdscript2_ir_pass_copy_prop() {
	auto result = build_ir(R"(
func test():
	var x = 10
	var y = x
	return y
)");
	CHECK_MESSAGE(!result.has_errors(), "Should build without errors.");

	// Run copy propagation
	GDScript2CopyPropPass pass;
	bool ok = pass.run(result.module);
	CHECK_MESSAGE(ok, "Copy propagation should succeed.");
}

void test_gdscript2_ir_pass_simplify_cfg() {
	auto result = build_ir(R"(
func test(x):
	if x > 0:
		return 1
	return 0
)");
	CHECK_MESSAGE(!result.has_errors(), "Should build without errors.");

	// Run CFG simplification
	GDScript2SimplifyCFGPass pass;
	bool ok = pass.run(result.module);
	CHECK_MESSAGE(ok, "CFG simplification should succeed.");
}

void test_gdscript2_ir_pass_manager() {
	auto result = build_ir(R"(
func factorial(n):
	if n <= 1:
		return 1
	return n * factorial(n - 1)
)");
	CHECK_MESSAGE(!result.has_errors(), "Should build without errors.");

	// Run all standard passes
	GDScript2IRPassManager pm;
	pm.add_standard_passes();
	bool ok = pm.run_all(result.module);
	CHECK_MESSAGE(ok, "Pass manager should succeed.");
}

// ============================================================================
// IR Output Tests
// ============================================================================

void test_gdscript2_ir_to_string() {
	auto result = build_ir(R"(
func add(a, b):
	return a + b
)");
	CHECK_MESSAGE(!result.has_errors(), "Should build without errors.");

	String ir_str = result.module.to_string();
	CHECK_MESSAGE(!ir_str.is_empty(), "IR string should not be empty.");
	CHECK_MESSAGE(ir_str.find("add") >= 0, "IR string should contain function name.");
}

} // namespace

// Main test function
namespace GDScript2Tests {

void test_ir() {
	test_gdscript2_ir_build_empty();
	test_gdscript2_ir_build_function();
	test_gdscript2_ir_build_variables();
	test_gdscript2_ir_build_if();
	test_gdscript2_ir_build_for();
	test_gdscript2_ir_build_while();
	test_gdscript2_ir_build_call();
	test_gdscript2_ir_build_array();
	test_gdscript2_ir_build_dictionary();
	test_gdscript2_ir_build_binary_ops();
	test_gdscript2_ir_build_comparison();
	test_gdscript2_ir_build_logical_short_circuit();
	test_gdscript2_ir_build_ternary();
	test_gdscript2_ir_build_match();
	test_gdscript2_ir_pass_const_fold();
	test_gdscript2_ir_pass_dce();
	test_gdscript2_ir_pass_copy_prop();
	test_gdscript2_ir_pass_simplify_cfg();
	test_gdscript2_ir_pass_manager();
	test_gdscript2_ir_to_string();
}

} // namespace GDScript2Tests
