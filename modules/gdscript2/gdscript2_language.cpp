/**************************************************************************/
/*  gdscript2_language.cpp                                                */
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

#include "gdscript2_language.h"

#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"

GDScript2Language *GDScript2Language::singleton = nullptr;

void GDScript2ResourceLoader::get_recognized_extensions(List<String> *p_extensions) const {
	if (p_extensions) {
		p_extensions->push_back("gd2");
	}
}

bool GDScript2ResourceLoader::handles_type(const String &p_type) const {
	return p_type == "Script" || p_type == "GDScript2";
}

String GDScript2ResourceLoader::get_resource_type(const String &p_path) const {
	if (p_path.get_extension().to_lower() == "gd2") {
		return "GDScript2";
	}
	return "";
}

Ref<Resource> GDScript2ResourceLoader::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	(void)p_use_sub_threads;
	(void)r_progress;
	(void)p_cache_mode;

	Error err;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ, &err);

	if (err != OK) {
		if (r_error) {
			*r_error = err;
		}
		ERR_PRINT("GDScript2: Cannot open file: " + p_path);
		return Ref<Resource>();
	}

	String source = f->get_as_utf8_string();
	f.unref();

	Ref<GDScript2> script;
	script.instantiate();
	script->set_language(GDScript2Language::get_singleton());
	script->set_path(p_original_path.is_empty() ? p_path : p_original_path, true);
	script->set_source_code(source);

	Error reload_err = script->reload();
	if (reload_err != OK) {
		if (r_error) {
			*r_error = reload_err;
		}
		ERR_PRINT("GDScript2: Failed to compile script: " + p_path);
		return Ref<Resource>();
	}

	if (r_error) {
		*r_error = OK;
	}

	return script;
}

Error GDScript2ResourceSaver::save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags) {
	Ref<GDScript2> script = p_resource;

	if (script.is_null()) {
		return ERR_INVALID_PARAMETER;
	}

	String source = script->get_source_code();

	Error err;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::WRITE, &err);

	if (err != OK) {
		ERR_PRINT("GDScript2: Cannot save file: " + p_path);
		return err;
	}

	f->store_string(source);

	if (f->get_error() != OK && f->get_error() != ERR_FILE_EOF) {
		return ERR_CANT_CREATE;
	}

	return OK;
}

void GDScript2ResourceSaver::get_recognized_extensions(const Ref<Resource> &p_resource, List<String> *p_extensions) const {
	if (Object::cast_to<GDScript2>(*p_resource)) {
		if (p_extensions) {
			p_extensions->push_back("gd2");
		}
	}
}

bool GDScript2ResourceSaver::recognize(const Ref<Resource> &p_resource) const {
	return Object::cast_to<GDScript2>(*p_resource) != nullptr;
}

GDScript2Language::GDScript2Language() {
	singleton = this;
	loader.instantiate();
	saver.instantiate();
}

GDScript2Language::~GDScript2Language() {
	singleton = nullptr;
	loader.unref();
	saver.unref();
}

Script *GDScript2Language::create_script() const {
	GDScript2 *script = memnew(GDScript2);
	script->set_language(const_cast<GDScript2Language *>(this));
	return script;
}

Ref<Script> GDScript2Language::make_template(const String &p_template, const String &p_class_name, const String &p_base_class_name) const {
	Ref<GDScript2> script;
	script.instantiate();
	script->set_language(const_cast<GDScript2Language *>(this));

	// Replace template placeholders
	String source = p_template;
	source = source.replace("_BASE_", p_base_class_name);
	source = source.replace("_CLASS_", p_class_name);
	source = source.replace("_TS_", "\t"); // Tab character

	script->set_source_code(source);

	return script;
}

Vector<ScriptLanguage::ScriptTemplate> GDScript2Language::get_built_in_templates(const StringName &p_object) {
	Vector<ScriptLanguage::ScriptTemplate> templates;

	// Provide a basic default template for Node
	if (p_object == StringName("Node")) {
		ScriptLanguage::ScriptTemplate t;
		t.id = 0;
		t.name = "Node: Default";
		t.inherit = "Node";
		t.content = "extends _BASE_\n\n\nfunc _ready() -> void:\n_TS_pass\n";
		templates.append(t);
	}
	// Provide a basic default template for Object
	else if (p_object == StringName("Object")) {
		ScriptLanguage::ScriptTemplate t;
		t.id = 0;
		t.name = "Object: Empty";
		t.inherit = "Object";
		t.content = "extends _BASE_\n\n\nfunc _init() -> void:\n_TS_pass\n";
		templates.append(t);
	}
	// Default template for any other type
	else {
		ScriptLanguage::ScriptTemplate t;
		t.id = 0;
		t.name = "Default";
		t.inherit = p_object;
		t.content = "extends _BASE_\n\n\n";
		templates.append(t);
	}

	return templates;
}
