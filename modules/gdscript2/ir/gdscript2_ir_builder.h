/**************************************************************************/
/*  gdscript2_ir_builder.h                                                */
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

#include "core/object/ref_counted.h"
#include "gdscript2_ir.h"
#include "modules/gdscript2/front/gdscript2_ast.h"
#include "modules/gdscript2/semantic/gdscript2_symbol_table.h"

class GDScript2IRBuilder : public RefCounted {
	GDCLASS(GDScript2IRBuilder, RefCounted);

	GDScript2IRModule module;
	GDScript2IRFunction *current_func = nullptr;
	GDScript2IRBlock *current_block = nullptr;

	int emit(const GDScript2IRInstr &p_instr);
	int new_block();
	void set_current_block(int p_id);

protected:
	static void _bind_methods() {}

public:
	struct Result {
		GDScript2IRModule module;
		Vector<String> errors;
	};

	Result build(GDScript2ASTNode *p_root, const GDScript2SymbolTable &p_globals);
};
