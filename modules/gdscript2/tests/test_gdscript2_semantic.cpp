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

void test_gdscript2_semantic_minimal() {
	Ref<GDScript2Parser> parser;
	parser.instantiate();
	GDScript2Parser::Result parse_result = parser->parse("", GDScript2Parser::Options());

	Ref<GDScript2SemanticAnalyzer> sema;
	sema.instantiate();
	GDScript2SemanticAnalyzer::Result sema_result = sema->analyze(parse_result.root);

	CHECK_MESSAGE(sema_result.errors.is_empty(), "Semantic analyzer should not report errors for empty AST stub.");
	GDScript2Type dummy;
	CHECK_MESSAGE(sema_result.globals.get(StringName("_dummy"), dummy), "Semantic analyzer should seed dummy global.");

	if (parse_result.root) {
		memdelete(parse_result.root);
	}
}

} // namespace

REGISTER_TEST_COMMAND("gdscript2-semantic-minimal", &test_gdscript2_semantic_minimal);
