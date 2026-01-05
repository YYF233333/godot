/**************************************************************************/
/*  test_gdscript2_coroutine.cpp                                          */
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
#include "modules/gdscript2/vm/gdscript2_coroutine.h"
#include "modules/gdscript2/vm/gdscript2_vm.h"

// Helper function to compile and run a script
GDScript2ExecutionResult compile_and_run(const String &p_source, GDScript2VM *p_vm) {
	// Parse
	GDScript2Parser parser;
	GDScript2Parser::Result parse_result = parser.parse(p_source);
	if (!parse_result.root || parse_result.has_errors()) {
		return GDScript2ExecutionResult::make_error(
				GDScript2ExecutionResult::ERROR_RUNTIME,
				"Parse error");
	}

	// Semantic analysis
	GDScript2SemanticAnalyzer analyzer;
	GDScript2SemanticAnalyzer::Result semantic_result = analyzer.analyze(parse_result.root);
	if (semantic_result.has_errors()) {
		return GDScript2ExecutionResult::make_error(
				GDScript2ExecutionResult::ERROR_RUNTIME,
				"Semantic error");
	}

	// Build IR
	GDScript2IRBuilder ir_builder;
	GDScript2IRBuilder::Result ir_result = ir_builder.build(parse_result.root);
	if (ir_result.has_errors()) {
		return GDScript2ExecutionResult::make_error(
				GDScript2ExecutionResult::ERROR_RUNTIME,
				"IR build error");
	}

	// Generate bytecode
	GDScript2CodeGenerator codegen;
	GDScript2CodeGenerator::Result codegen_result = codegen.generate(ir_result.module);
	if (codegen_result.has_errors()) {
		return GDScript2ExecutionResult::make_error(
				GDScript2ExecutionResult::ERROR_RUNTIME,
				"Codegen error");
	}

	// Execute
	p_vm->load_module(&codegen_result.module);
	return p_vm->call("test");
}

void test_coroutine_basic_creation() {
	Ref<GDScript2Coroutine> coro;
	coro.instantiate();

	CHECK(coro->get_state() == GDScript2Coroutine::STATE_SUSPENDED);
	CHECK(coro->is_valid());
	CHECK(!coro->is_completed());
}

void test_coroutine_state_transitions() {
	Ref<GDScript2Coroutine> coro;
	coro.instantiate();

	// Initial state
	CHECK(coro->is_suspended());

	// Complete
	coro->complete(42);
	CHECK(coro->is_completed());
	CHECK(coro->get_state() == GDScript2Coroutine::STATE_COMPLETED);
	CHECK((int)coro->get_resume_value() == 42);
}

void test_coroutine_cancel() {
	Ref<GDScript2Coroutine> coro;
	coro.instantiate();

	coro->cancel();
	CHECK(coro->get_state() == GDScript2Coroutine::STATE_CANCELLED);
	CHECK(coro->is_completed());
	CHECK(!coro->is_valid());
}

void test_coroutine_error_state() {
	Ref<GDScript2Coroutine> coro;
	coro.instantiate();

	coro->set_error("Test error", 42);
	CHECK(coro->get_state() == GDScript2Coroutine::STATE_ERROR);
	CHECK(coro->is_completed());
	CHECK(coro->get_error_message() == "Test error");
	CHECK(coro->get_error_line() == 42);
}

void test_coroutine_manager_creation() {
	Ref<GDScript2CoroutineManager> manager;
	manager.instantiate();

	Ref<GDScript2Coroutine> coro = manager->create_coroutine();
	CHECK(coro.is_valid());
	CHECK(manager->get_active_count() == 1);
}

void test_coroutine_manager_cleanup() {
	Ref<GDScript2CoroutineManager> manager;
	manager.instantiate();

	Ref<GDScript2Coroutine> coro1 = manager->create_coroutine();
	Ref<GDScript2Coroutine> coro2 = manager->create_coroutine();
	Ref<GDScript2Coroutine> coro3 = manager->create_coroutine();

	CHECK(manager->get_active_count() == 3);

	// Complete one
	coro2->complete();
	manager->cleanup_completed();

	CHECK(manager->get_active_count() == 2);
}

void test_coroutine_manager_cancel_all() {
	Ref<GDScript2CoroutineManager> manager;
	manager.instantiate();

	Ref<GDScript2Coroutine> coro1 = manager->create_coroutine();
	Ref<GDScript2Coroutine> coro2 = manager->create_coroutine();

	manager->cancel_all();

	CHECK(manager->get_active_count() == 0);
	CHECK(coro1->is_completed());
	CHECK(coro2->is_completed());
}

void test_coroutine_signal_waiting() {
	Ref<GDScript2Coroutine> coro;
	coro.instantiate();

	// Create a test object with a signal
	Object *test_obj = memnew(Object);
	test_obj->add_user_signal(MethodInfo("test_signal"));

	coro->wait_for_signal(test_obj, "test_signal");
	CHECK(coro->is_waiting_for_signal());
	CHECK(coro->get_signal_target() == test_obj);
	CHECK(coro->get_signal_name() == "test_signal");

	// Cleanup
	coro->cancel_signal_wait();
	CHECK(!coro->is_waiting_for_signal());

	memdelete(test_obj);
}

void test_coroutine_await_marking() {
	String source = R"(
func test():
	var sig = Signal()
	await sig
	return 42
)";

	GDScript2Parser parser;
	GDScript2Parser::Result parse_result = parser.parse(source);
	CHECK(parse_result.root != nullptr);
	CHECK(!parse_result.has_errors());

	GDScript2SemanticAnalyzer analyzer;
	GDScript2SemanticAnalyzer::Result semantic_result = analyzer.analyze(parse_result.root);

	// Check that the function was marked as coroutine
	if (parse_result.root && parse_result.root->type == GDScript2ASTNodeType::NODE_CLASS) {
		GDScript2ClassNode *class_node = static_cast<GDScript2ClassNode *>(parse_result.root);
		if (!class_node->functions.is_empty()) {
			GDScript2FunctionNode *func = class_node->functions[0];
			CHECK(func->is_coroutine);
		}
	}
}

void test_coroutine_await_ir() {
	String source = R"(
func test():
	var sig = Signal()
	var result = await sig
	return result
)";

	GDScript2Parser parser;
	GDScript2Parser::Result parse_result = parser.parse(source);
	CHECK(parse_result.root != nullptr);

	GDScript2SemanticAnalyzer analyzer;
	GDScript2SemanticAnalyzer::Result semantic_result = analyzer.analyze(parse_result.root);

	GDScript2IRBuilder ir_builder;
	GDScript2IRBuilder::Result ir_result = ir_builder.build(parse_result.root);
	CHECK(!ir_result.has_errors());

	// Check that IR contains OP_AWAIT
	bool found_await = false;
	for (const GDScript2IRFunction &func : ir_result.module.functions) {
		for (const GDScript2IRBlock &block : func.blocks) {
			for (const GDScript2IRInstr &instr : block.instructions) {
				if (instr.op == GDScript2IROp::OP_AWAIT) {
					found_await = true;
					break;
				}
			}
		}
	}
	CHECK(found_await);
}

void test_coroutine_await_bytecode() {
	String source = R"(
func test():
	var sig = Signal()
	var result = await sig
	return result
)";

	GDScript2Parser parser;
	GDScript2Parser::Result parse_result = parser.parse(source);
	CHECK(parse_result.root != nullptr);

	GDScript2SemanticAnalyzer analyzer;
	GDScript2SemanticAnalyzer::Result semantic_result = analyzer.analyze(parse_result.root);

	GDScript2IRBuilder ir_builder;
	GDScript2IRBuilder::Result ir_result = ir_builder.build(parse_result.root);
	CHECK(!ir_result.has_errors());

	GDScript2CodeGenerator codegen;
	GDScript2CodeGenerator::Result codegen_result = codegen.generate(ir_result.module);
	CHECK(!codegen_result.has_errors());

	// Check that bytecode contains OP_AWAIT
	bool found_await = false;
	for (const GDScript2CompiledFunction &func : codegen_result.module.functions) {
		for (const GDScript2BytecodeInstr &instr : func.code) {
			if (instr.op == GDScript2Opcode::OP_AWAIT) {
				found_await = true;
				break;
			}
		}
	}
	CHECK(found_await);
}

void test_coroutine_vm_integration() {
	Ref<GDScript2VM> vm;
	vm.instantiate();

	Ref<GDScript2CoroutineManager> manager;
	manager.instantiate();
	manager->set_vm(vm.ptr());
	vm->set_coroutine_manager(manager.ptr());

	CHECK(vm->get_coroutine_manager() == manager.ptr());
	CHECK(!vm->is_in_coroutine());
}

// Main test function for coroutine tests
namespace GDScript2Tests {

void test_coroutine() {
	test_coroutine_basic_creation();
	test_coroutine_state_transitions();
	test_coroutine_cancel();
	test_coroutine_error_state();
	test_coroutine_manager_creation();
	test_coroutine_manager_cleanup();
	test_coroutine_manager_cancel_all();
	test_coroutine_signal_waiting();
	test_coroutine_await_marking();
	test_coroutine_await_ir();
	test_coroutine_await_bytecode();
	test_coroutine_vm_integration();
}

} // namespace GDScript2Tests
