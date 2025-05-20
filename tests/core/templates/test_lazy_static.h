/**************************************************************************/
/*  test_lazy_static.h                                                    */
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

#include "core/string/ustring.h"
#include "core/templates/lazy_static.h"

#include "tests/test_macros.h"

namespace TestLazyStatic {

static LazyStatic<String> lazy_string([] {
	static int init_count = 0;
	init_count++;
	if (init_count == 1) {
		return String("hello");
	}
	return String("multiple initializations");
});

TEST_CASE("[LazyStatic] Basic initialization and access") {
	// First access should initialize
	CHECK(lazy_string == "hello");

	// Second access should not re-initialize
	CHECK(lazy_string == "hello");

	// Check implicit conversion works in expressions
	String &value = lazy_string;
	value.append_ascii(" world");
	CHECK(lazy_string == "hello world");

	lazy_string->append_ascii(" again");
	CHECK(lazy_string == "hello world again");

	lazy_string.get() = "new value";
	CHECK(lazy_string == "new value");
}

} // namespace TestLazyStatic
