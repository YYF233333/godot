/**************************************************************************/
/*  gdscript2_test_suite.h                                                */
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

#include "tests/test_macros.h"

// Forward declarations of test functions from cpp files
namespace GDScript2Tests {
void test_tokenizer();
void test_parser();
void test_semantic();
void test_ir();
void test_codegen();
void test_vm();
void test_runtime();
void test_front();
void test_coroutine();
void test_signal();
} //namespace GDScript2Tests

// Register tests with doctest
TEST_SUITE("[Modules][GDScript2]") {
	TEST_CASE("[GDScript2] Tokenizer") {
		GDScript2Tests::test_tokenizer();
	}

	TEST_CASE("[GDScript2] Parser") {
		GDScript2Tests::test_parser();
	}

	TEST_CASE("[GDScript2] Semantic Analyzer") {
		GDScript2Tests::test_semantic();
	}

	TEST_CASE("[GDScript2] IR Builder") {
		GDScript2Tests::test_ir();
	}

	TEST_CASE("[GDScript2] Codegen") {
		GDScript2Tests::test_codegen();
	}

	TEST_CASE("[GDScript2] Virtual Machine") {
		GDScript2Tests::test_vm();
	}

	TEST_CASE("[GDScript2] Runtime") {
		GDScript2Tests::test_runtime();
	}

	TEST_CASE("[GDScript2] Frontend Integration") {
		GDScript2Tests::test_front();
	}

	TEST_CASE("[GDScript2] Coroutine System") {
		GDScript2Tests::test_coroutine();
	}

	TEST_CASE("[GDScript2] Signal System") {
		GDScript2Tests::test_signal();
	}
}
