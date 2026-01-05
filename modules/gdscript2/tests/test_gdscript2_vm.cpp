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

void test_gdscript2_vm_execute_minimal() {
	// Parse empty script.
	Ref<GDScript2Parser> parser;
	parser.instantiate();
	GDScript2Parser::Result parse_result = parser->parse("", GDScript2Parser::Options());

	// Semantic analysis.
	Ref<GDScript2SemanticAnalyzer> sema;
	sema.instantiate();
	GDScript2SemanticAnalyzer::Result sema_result = sema->analyze(parse_result.root);

	// Build IR.
	Ref<GDScript2IRBuilder> ir_builder;
	ir_builder.instantiate();
	GDScript2IRBuilder::Result ir_result = ir_builder->build(parse_result.root, sema_result.globals);

	// Generate bytecode.
	Ref<GDScript2CodeGenerator> codegen;
	codegen.instantiate();
	GDScript2CodeGenerator::Result codegen_result = codegen->generate(ir_result.module);

	// Execute.
	Ref<GDScript2VM> vm;
	vm.instantiate();
	vm->load_module(&codegen_result.module);
	GDScript2ExecutionResult exec_result = vm->execute();

	CHECK_MESSAGE(exec_result.status == GDScript2ExecutionResult::OK, "VM execution should succeed for stub bytecode.");

	if (parse_result.root) {
		memdelete(parse_result.root);
	}
}

void test_gdscript2_vm_function_not_found() {
	// Create empty module.
	GDScript2CompiledModule empty_module;

	Ref<GDScript2VM> vm;
	vm.instantiate();
	vm->load_module(&empty_module);

	GDScript2ExecutionResult result = vm->call(StringName("nonexistent"), Vector<Variant>());
	CHECK_MESSAGE(result.status == GDScript2ExecutionResult::ERROR_RUNTIME, "VM should return error for nonexistent function.");
}

} // namespace

REGISTER_TEST_COMMAND("gdscript2-vm-execute", &test_gdscript2_vm_execute_minimal);
REGISTER_TEST_COMMAND("gdscript2-vm-not-found", &test_gdscript2_vm_function_not_found);
