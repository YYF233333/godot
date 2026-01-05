/**************************************************************************/
/*  gdscript2_ir_pass.h                                                   */
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

#include "gdscript2_ir.h"

// Abstract base for IR passes.
class GDScript2IRPass {
public:
	virtual ~GDScript2IRPass() = default;
	virtual const char *get_name() const = 0;
	virtual bool run(GDScript2IRModule &p_module) = 0;
};

// Pass manager: runs a sequence of passes.
class GDScript2IRPassManager {
	Vector<GDScript2IRPass *> passes;

public:
	void add_pass(GDScript2IRPass *p_pass) { passes.push_back(p_pass); }
	bool run_all(GDScript2IRModule &p_module) {
		for (GDScript2IRPass *pass : passes) {
			if (!pass->run(p_module)) {
				return false;
			}
		}
		return true;
	}
	~GDScript2IRPassManager() {
		for (GDScript2IRPass *pass : passes) {
			memdelete(pass);
		}
	}
};

// Example pass: constant folding stub.
class GDScript2ConstFoldPass : public GDScript2IRPass {
public:
	const char *get_name() const override { return "ConstFold"; }
	bool run(GDScript2IRModule &p_module) override;
};

// Example pass: dead code elimination stub.
class GDScript2DCEPass : public GDScript2IRPass {
public:
	const char *get_name() const override { return "DCE"; }
	bool run(GDScript2IRModule &p_module) override;
};
