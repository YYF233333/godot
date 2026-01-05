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

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"

ScriptLanguage *GDScript2Script::get_language() const {
	return language;
}

void GDScript2ResourceLoader::get_recognized_extensions(List<String> *p_extensions) const {
	if (p_extensions) {
		p_extensions->push_back("gd");
	}
}

bool GDScript2ResourceLoader::handles_type(const String &p_type) const {
	return p_type == "Script" || p_type == "GDScript2Script";
}

String GDScript2ResourceLoader::get_resource_type(const String &p_path) const {
	if (p_path.get_extension().to_lower() == "gd2") {
		return "Script";
	}
	return "";
}

Ref<Resource> GDScript2ResourceLoader::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	(void)p_path;
	(void)p_original_path;
	(void)p_use_sub_threads;
	(void)r_progress;
	(void)p_cache_mode;
	if (r_error) {
		*r_error = ERR_UNAVAILABLE;
	}
	return Ref<Resource>();
}

Error GDScript2ResourceSaver::save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags) {
	(void)p_resource;
	(void)p_path;
	(void)p_flags;
	return ERR_UNAVAILABLE;
}

void GDScript2ResourceSaver::get_recognized_extensions(const Ref<Resource> &p_resource, List<String> *p_extensions) const {
	(void)p_resource;
	if (p_extensions) {
		p_extensions->push_back("gd");
	}
}

bool GDScript2ResourceSaver::recognize(const Ref<Resource> &p_resource) const {
	return p_resource.is_valid() && (p_resource->get_class_name() == "GDScript2Script" || p_resource->is_class("Script"));
}

GDScript2Language::GDScript2Language() {
	loader.instantiate();
	saver.instantiate();
}

GDScript2Language::~GDScript2Language() {
	loader.unref();
	saver.unref();
}

Script *GDScript2Language::create_script() const {
	GDScript2Script *script = memnew(GDScript2Script);
	script->set_language(const_cast<GDScript2Language *>(this));
	return script;
}
