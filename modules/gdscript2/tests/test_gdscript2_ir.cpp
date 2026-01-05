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

void test_gdscript2_ir_build_minimal() {
	// Parse empty script.
	Ref<GDScript2Parser> parser;
	parser.instantiate();
	GDScript2Parser::Result parse_result = parser->parse("", GDScript2Parser::Options());

	// Semantic analysis.
	Ref<GDScript2SemanticAnalyzer> sema;
	sema.instantiate();
	GDScript2SemanticAnalyzer::Result sema_result = sema->analyze(parse_result.root);

	// Build IR.
	Ref<GDScript2IRBuilder> builder;
	builder.instantiate();
	GDScript2IRBuilder::Result ir_result = builder->build(parse_result.root, sema_result.globals);

	CHECK_MESSAGE(ir_result.errors.is_empty(), "IR builder should not report errors for stub AST.");
	CHECK_MESSAGE(ir_result.module.functions.size() == 1, "IR module should have one function.");
	CHECK_MESSAGE(ir_result.module.functions[0].blocks.size() == 1, "Function should have one block.");

	if (parse_result.root) {
		memdelete(parse_result.root);
	}
}

void test_gdscript2_ir_passes() {
	// Build minimal IR.
	Ref<GDScript2Parser> parser;
	parser.instantiate();
	GDScript2Parser::Result parse_result = parser->parse("", GDScript2Parser::Options());

	Ref<GDScript2SemanticAnalyzer> sema;
	sema.instantiate();
	GDScript2SemanticAnalyzer::Result sema_result = sema->analyze(parse_result.root);

	Ref<GDScript2IRBuilder> builder;
	builder.instantiate();
	GDScript2IRBuilder::Result ir_result = builder->build(parse_result.root, sema_result.globals);

	// Run passes.
	GDScript2IRPassManager pm;
	pm.add_pass(memnew(GDScript2ConstFoldPass));
	pm.add_pass(memnew(GDScript2DCEPass));
	bool ok = pm.run_all(ir_result.module);

	CHECK_MESSAGE(ok, "Pass manager should succeed on stub IR.");

	if (parse_result.root) {
		memdelete(parse_result.root);
	}
}

} // namespace

REGISTER_TEST_COMMAND("gdscript2-ir-build", &test_gdscript2_ir_build_minimal);
REGISTER_TEST_COMMAND("gdscript2-ir-passes", &test_gdscript2_ir_passes);
