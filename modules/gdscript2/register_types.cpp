/**************************************************************************/
/*  register_types.cpp                                                    */
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

#include "register_types.h"

#include "gdscript2_language.h"
#include "runtime/gdscript2_builtin.h"
#include "vm/gdscript2_coroutine.h"

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/script_language.h"

static GDScript2Language *gdscript2_language = nullptr;
static Ref<GDScript2ResourceLoader> gdscript2_loader;
static Ref<GDScript2ResourceSaver> gdscript2_saver;

void initialize_gdscript2_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SERVERS) {
		return;
	}

	// Register coroutine classes
	GDREGISTER_CLASS(GDScript2Coroutine);
	GDREGISTER_CLASS(GDScript2CoroutineManager);

	// Initialize builtin functions registry
	GDScript2BuiltinRegistry::initialize();

	gdscript2_language = memnew(GDScript2Language);
	ScriptServer::register_language(gdscript2_language);

	gdscript2_loader = gdscript2_language->get_loader();
	if (gdscript2_loader.is_valid()) {
		ResourceLoader::add_resource_format_loader(gdscript2_loader);
	}

	gdscript2_saver = gdscript2_language->get_saver();
	if (gdscript2_saver.is_valid()) {
		ResourceSaver::add_resource_format_saver(gdscript2_saver);
	}
}

void uninitialize_gdscript2_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SERVERS) {
		return;
	}

	if (gdscript2_saver.is_valid()) {
		ResourceSaver::remove_resource_format_saver(gdscript2_saver);
		gdscript2_saver.unref();
	}

	if (gdscript2_loader.is_valid()) {
		ResourceLoader::remove_resource_format_loader(gdscript2_loader);
		gdscript2_loader.unref();
	}

	if (gdscript2_language) {
		ScriptServer::unregister_language(gdscript2_language);
		memdelete(gdscript2_language);
		gdscript2_language = nullptr;
	}

	// Finalize builtin functions registry
	GDScript2BuiltinRegistry::finalize();
}
