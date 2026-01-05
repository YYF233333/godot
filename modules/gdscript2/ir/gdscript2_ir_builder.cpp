/**************************************************************************/
/*  gdscript2_ir_builder.cpp                                              */
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

#include "gdscript2_ir_builder.h"

int GDScript2IRBuilder::emit(const GDScript2IRInstr &p_instr) {
	if (current_block == nullptr) {
		return -1;
	}
	current_block->instructions.push_back(p_instr);
	return current_block->instructions.size() - 1;
}

int GDScript2IRBuilder::new_block() {
	if (current_func == nullptr) {
		return -1;
	}
	GDScript2IRBlock block;
	block.id = current_func->blocks.size();
	current_func->blocks.push_back(block);
	return block.id;
}

void GDScript2IRBuilder::set_current_block(int p_id) {
	if (current_func == nullptr || p_id < 0 || p_id >= current_func->blocks.size()) {
		current_block = nullptr;
		return;
	}
	current_block = &current_func->blocks.write[p_id];
}

GDScript2IRBuilder::Result GDScript2IRBuilder::build(GDScript2ASTNode *p_root, const GDScript2SymbolTable &p_globals) {
	Result result;
	module = GDScript2IRModule();

	if (p_root == nullptr) {
		result.errors.push_back("AST root is null; cannot build IR.");
		result.module = module;
		return result;
	}

	// Stub: create a single function with an empty entry block.
	GDScript2IRFunction main_func;
	main_func.name = StringName("_main");
	module.functions.push_back(main_func);
	current_func = &module.functions.write[0];

	int entry = new_block();
	set_current_block(entry);
	current_func->entry_block = entry;

	// Emit a single NOP as placeholder.
	emit(GDScript2IRInstr(GDScript2IROp::NOP));

	// Emit implicit return.
	emit(GDScript2IRInstr(GDScript2IROp::RETURN));

	result.module = module;
	return result;
}
