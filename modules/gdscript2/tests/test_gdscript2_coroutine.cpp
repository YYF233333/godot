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

namespace TestGDScript2Coroutine {

// Helper function to compile and run a script
GDScript2ExecutionResult compile_and_run(const String &p_source, GDScript2VM *p_vm) {
	// Parse
	GDScript2Parser parser;
	GDScript2ParseResult *parse_result = parser.parse(p_source, "test.gd2");
	if (!parse_result || !parse_result->root || parse_result->has_errors()) {
		return GDScript2ExecutionResult::make_error(
				GDScript2ExecutionResult::ERROR_RUNTIME,
				"Parse error");
	}

	// Semantic analysis
	GDScript2SemanticAnalyzer analyzer;
	GDScript2SemanticResult *semantic_result = analyzer.analyze(parse_result);
	if (!semantic_result || semantic_result->has_errors()) {
		return GDScript2ExecutionResult::make_error(
				GDScript2ExecutionResult::ERROR_RUNTIME,
				"Semantic error");
	}

	// Build IR
	GDScript2IRBuilder ir_builder;
	GDScript2IRModule *ir_module = ir_builder.build(parse_result->root);
	if (!ir_module || ir_builder.has_errors()) {
		return GDScript2ExecutionResult::make_error(
				GDScript2ExecutionResult::ERROR_RUNTIME,
				"IR build error");
	}

	// Generate bytecode
	GDScript2CodeGenerator codegen;
	GDScript2CompiledModule *module = codegen.generate(ir_module);
	if (!module) {
		return GDScript2ExecutionResult::make_error(
				GDScript2ExecutionResult::ERROR_RUNTIME,
				"Codegen error");
	}

	// Execute
	p_vm->load_module(module);
	return p_vm->call("test");
}

TEST_CASE("[Modules][GDScript2] Coroutine - Basic creation and state") {
	Ref<GDScript2Coroutine> coro;
	coro.instantiate();

	CHECK(coro->get_state() == GDScript2Coroutine::STATE_SUSPENDED);
	CHECK(coro->is_valid());
	CHECK(!coro->is_completed());
}

TEST_CASE("[Modules][GDScript2] Coroutine - State transitions") {
	Ref<GDScript2Coroutine> coro;
	coro.instantiate();

	// Initial state
	CHECK(coro->is_suspended());

	// Complete
	coro->complete(42);
	CHECK(coro->is_completed());
	CHECK(coro->get_state() == GDScript2Coroutine::STATE_COMPLETED);
	CHECK(coro->get_resume_value() == 42);
}

TEST_CASE("[Modules][GDScript2] Coroutine - Cancel") {
	Ref<GDScript2Coroutine> coro;
	coro.instantiate();

	coro->cancel();
	CHECK(coro->get_state() == GDScript2Coroutine::STATE_CANCELLED);
	CHECK(coro->is_completed());
	CHECK(!coro->is_valid());
}

TEST_CASE("[Modules][GDScript2] Coroutine - Error state") {
	Ref<GDScript2Coroutine> coro;
	coro.instantiate();

	coro->set_error("Test error", 42);
	CHECK(coro->get_state() == GDScript2Coroutine::STATE_ERROR);
	CHECK(coro->is_completed());
	CHECK(coro->get_error_message() == "Test error");
	CHECK(coro->get_error_line() == 42);
}

TEST_CASE("[Modules][GDScript2] Coroutine Manager - Creation") {
	Ref<GDScript2CoroutineManager> manager;
	manager.instantiate();

	Ref<GDScript2Coroutine> coro = manager->create_coroutine();
	CHECK(coro.is_valid());
	CHECK(manager->get_active_count() == 1);
}

TEST_CASE("[Modules][GDScript2] Coroutine Manager - Cleanup completed") {
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

TEST_CASE("[Modules][GDScript2] Coroutine Manager - Cancel all") {
	Ref<GDScript2CoroutineManager> manager;
	manager.instantiate();

	Ref<GDScript2Coroutine> coro1 = manager->create_coroutine();
	Ref<GDScript2Coroutine> coro2 = manager->create_coroutine();

	manager->cancel_all();

	CHECK(manager->get_active_count() == 0);
	CHECK(coro1->is_completed());
	CHECK(coro2->is_completed());
}

TEST_CASE("[Modules][GDScript2] Coroutine - Signal waiting setup") {
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

TEST_CASE("[Modules][GDScript2] Coroutine - Await marks function as coroutine") {
	String source = R"(
func test():
	var sig = Signal()
	await sig
	return 42
)";

	GDScript2Parser parser;
	GDScript2ParseResult *parse_result = parser.parse(source, "test.gd2");
	CHECK(parse_result != nullptr);
	CHECK(!parse_result->has_errors());

	GDScript2SemanticAnalyzer analyzer;
	GDScript2SemanticResult *semantic_result = analyzer.analyze(parse_result);
	CHECK(semantic_result != nullptr);

	// Check that the function was marked as coroutine
	if (parse_result->root && parse_result->root->type == GDScript2ASTNodeType::NODE_CLASS) {
		GDScript2ClassNode *class_node = static_cast<GDScript2ClassNode *>(parse_result->root);
		if (!class_node->functions.is_empty()) {
			GDScript2FunctionNode *func = class_node->functions[0];
			CHECK(func->is_coroutine);
		}
	}
}

TEST_CASE("[Modules][GDScript2] Coroutine - Simple await in IR") {
	String source = R"(
func test():
	var sig = Signal()
	var result = await sig
	return result
)";

	GDScript2Parser parser;
	GDScript2ParseResult *parse_result = parser.parse(source, "test.gd2");
	CHECK(parse_result != nullptr);

	GDScript2SemanticAnalyzer analyzer;
	GDScript2SemanticResult *semantic_result = analyzer.analyze(parse_result);
	CHECK(semantic_result != nullptr);

	GDScript2IRBuilder ir_builder;
	GDScript2IRModule *ir_module = ir_builder.build(parse_result->root);
	CHECK(ir_module != nullptr);
	CHECK(!ir_builder.has_errors());

	// Check that IR contains OP_AWAIT
	bool found_await = false;
	for (const GDScript2IRFunction &func : ir_module->functions) {
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

TEST_CASE("[Modules][GDScript2] Coroutine - Await in bytecode") {
	String source = R"(
func test():
	var sig = Signal()
	var result = await sig
	return result
)";

	GDScript2Parser parser;
	GDScript2ParseResult *parse_result = parser.parse(source, "test.gd2");
	CHECK(parse_result != nullptr);

	GDScript2SemanticAnalyzer analyzer;
	GDScript2SemanticResult *semantic_result = analyzer.analyze(parse_result);
	CHECK(semantic_result != nullptr);

	GDScript2IRBuilder ir_builder;
	GDScript2IRModule *ir_module = ir_builder.build(parse_result->root);
	CHECK(ir_module != nullptr);

	GDScript2CodeGenerator codegen;
	GDScript2CompiledModule *module = codegen.generate(ir_module);
	CHECK(module != nullptr);

	// Check that bytecode contains OP_AWAIT
	bool found_await = false;
	for (const GDScript2CompiledFunction &func : module->functions) {
		for (const GDScript2BytecodeInstr &instr : func.bytecode) {
			if (instr.opcode == GDScript2Opcode::OP_AWAIT) {
				found_await = true;
				break;
			}
		}
	}
	CHECK(found_await);
}

TEST_CASE("[Modules][GDScript2] Coroutine - VM integration basic") {
	Ref<GDScript2VM> vm;
	vm.instantiate();

	Ref<GDScript2CoroutineManager> manager;
	manager.instantiate();
	manager->set_vm(vm.ptr());
	vm->set_coroutine_manager(manager.ptr());

	CHECK(vm->get_coroutine_manager() == manager.ptr());
	CHECK(!vm->is_in_coroutine());
}

} // namespace TestGDScript2Coroutine
