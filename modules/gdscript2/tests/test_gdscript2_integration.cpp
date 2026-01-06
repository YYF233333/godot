/**************************************************************************/
/*  test_gdscript2_integration.cpp                                        */
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

#include "../gdscript2.h"
#include "core/io/file_access.h"

namespace GDScript2Tests {

// Test basic script creation and compilation
static void test_script_creation_and_compilation() {
	String source = R"(
extends Node

var health: int = 100

func get_health() -> int:
	return health

func take_damage(amount: int) -> void:
	health = health - amount
)";

	Ref<GDScript2> script;
	script.instantiate();
	script->set_source_code(source);

	Error err = script->reload();
	CHECK_MESSAGE(err == OK, "Script should compile successfully");
	CHECK_MESSAGE(script->is_valid(), "Script should be valid after compilation");
}

// Test basic method calls
static void test_method_call() {
	String source = R"(
extends Node

func add(a: int, b: int) -> int:
	return a + b

func multiply(x: int, y: int) -> int:
	return x * y
)";

	Ref<GDScript2> script;
	script.instantiate();
	script->set_source_code(source);

	Error err = script->reload();
	CHECK_MESSAGE(err == OK, "Script should compile successfully");

	// Test if script can instantiate (basic check)
	if (err == OK) {
		CHECK(script->can_instantiate());
	}
}

// Test variable access
static void test_variable_access() {
	String source = R"(
extends Node

var name: String = "Test"
var value: int = 42
)";

	Ref<GDScript2> script;
	script.instantiate();
	script->set_source_code(source);

	Error err = script->reload();
	CHECK_MESSAGE(err == OK, "Script should compile successfully");

	// Basic validation - script compiled successfully
	if (err == OK) {
		CHECK(script->is_valid());
	}
}

// Test compilation errors are reported
static void test_compilation_error_handling() {
	String source = R"(
extends Node

func broken_function() -> int:
	return "not an int"
)";

	Ref<GDScript2> script;
	script.instantiate();
	script->set_source_code(source);

	Error err = script->reload();
	CHECK_MESSAGE(err != OK, "Script with type error should fail to compile");
	CHECK_MESSAGE(!script->is_valid(), "Script should not be valid after compilation failure");
}

// Test syntax error handling
static void test_syntax_error_handling() {
	String source = R"(
extends Node

func bad_syntax(
	# Missing closing parenthesis and body
)";

	Ref<GDScript2> script;
	script.instantiate();
	script->set_source_code(source);

	Error err = script->reload();
	CHECK_MESSAGE(err != OK, "Script with syntax error should fail to compile");
}

// Test simple control flow
static void test_control_flow_compilation() {
	String source = R"(
extends Node

func test_if(x: int) -> int:
	if x > 0:
		return 1
	else:
		return -1

func test_while() -> int:
	var sum: int = 0
	var i: int = 0
	while i < 5:
		sum = sum + i
		i = i + 1
	return sum
)";

	Ref<GDScript2> script;
	script.instantiate();
	script->set_source_code(source);

	Error err = script->reload();
	CHECK_MESSAGE(err == OK, "Script with control flow should compile successfully");
}

// Test method info extraction
static void test_method_info() {
	String source = R"(
extends Node

func calculate(a: int, b: int) -> int:
	return a + b
)";

	Ref<GDScript2> script;
	script.instantiate();
	script->set_source_code(source);

	Error err = script->reload();
	CHECK_MESSAGE(err == OK, "Script should compile successfully");

	// Check method exists (basic test)
	if (err == OK) {
		CHECK(script->has_method(StringName("calculate")));
		// Note: Full MethodInfo extraction will be implemented in Phase 2
	}
}

// Main integration test function
void test_integration() {
	test_script_creation_and_compilation();
	test_method_call();
	test_variable_access();
	test_compilation_error_handling();
	test_syntax_error_handling();
	test_control_flow_compilation();
	test_method_info();
}

} // namespace GDScript2Tests
