/**************************************************************************/
/*  gdscript2_language.h                                                  */
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

#include "gdscript2.h"

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/script_language.h"

class GDScript2Language;

class GDScript2ResourceLoader : public ResourceFormatLoader {
	GDCLASS(GDScript2ResourceLoader, ResourceFormatLoader);

protected:
	static void _bind_methods() {}

public:
	virtual void get_recognized_extensions(List<String> *p_extensions) const override;
	virtual bool handles_type(const String &p_type) const override;
	virtual String get_resource_type(const String &p_path) const override;
	virtual Ref<Resource> load(const String &p_path, const String &p_original_path = "", Error *r_error = nullptr, bool p_use_sub_threads = false, float *r_progress = nullptr, CacheMode p_cache_mode = CacheMode::CACHE_MODE_REUSE) override;
};

class GDScript2ResourceSaver : public ResourceFormatSaver {
	GDCLASS(GDScript2ResourceSaver, ResourceFormatSaver);

protected:
	static void _bind_methods() {}

public:
	virtual Error save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags = 0) override;
	virtual void get_recognized_extensions(const Ref<Resource> &p_resource, List<String> *p_extensions) const override;
	virtual bool recognize(const Ref<Resource> &p_resource) const override;
};

class GDScript2Language : public ScriptLanguage {
	GDCLASS(GDScript2Language, ScriptLanguage);

	static GDScript2Language *singleton;

	Ref<GDScript2ResourceLoader> loader;
	Ref<GDScript2ResourceSaver> saver;

protected:
	static void _bind_methods() {}

public:
	GDScript2Language();
	~GDScript2Language() override;

	_FORCE_INLINE_ static GDScript2Language *get_singleton() { return singleton; }

	Ref<GDScript2ResourceLoader> get_loader() const { return loader; }
	Ref<GDScript2ResourceSaver> get_saver() const { return saver; }

	// ScriptLanguage interface (stubbed for bootstrap).
	virtual String get_name() const override { return "GDScript2"; }
	virtual void init() override {}
	virtual String get_type() const override { return "GDScript2"; }
	virtual String get_extension() const override { return "gd2"; }
	virtual void finish() override {}

	virtual Vector<String> get_reserved_words() const override { return Vector<String>(); }
	virtual bool is_control_flow_keyword(const String &p_string) const override {
		(void)p_string;
		return false;
	}
	virtual Vector<String> get_comment_delimiters() const override { return Vector<String>(); }
	virtual Vector<String> get_doc_comment_delimiters() const override { return Vector<String>(); }
	virtual Vector<String> get_string_delimiters() const override { return Vector<String>(); }

	virtual bool validate(const String &p_script, const String &p_path = "", List<String> *r_functions = nullptr, List<ScriptError> *r_errors = nullptr, List<Warning> *r_warnings = nullptr, HashSet<int> *r_safe_lines = nullptr) const override {
		(void)p_script;
		(void)p_path;
		if (r_functions) {
			r_functions->clear();
		}
		if (r_errors) {
			r_errors->clear();
		}
		if (r_warnings) {
			r_warnings->clear();
		}
		if (r_safe_lines) {
			r_safe_lines->clear();
		}
		return true;
	}

	virtual Script *create_script() const override;
	virtual Ref<Script> make_template(const String &p_template, const String &p_class_name, const String &p_base_class_name) const override;
	virtual Vector<ScriptTemplate> get_built_in_templates(const StringName &p_object) override;
	virtual bool is_using_templates() override { return true; }
	virtual bool supports_builtin_mode() const override { return true; }
	virtual int find_function(const String &p_function, const String &p_code) const override {
		(void)p_function;
		(void)p_code;
		return -1;
	}
	virtual String make_function(const String &p_class, const String &p_name, const PackedStringArray &p_args) const override {
		(void)p_class;
		(void)p_name;
		(void)p_args;
		return String();
	}

	virtual void auto_indent_code(String &p_code, int p_from_line, int p_to_line) const override {
		(void)p_code;
		(void)p_from_line;
		(void)p_to_line;
	}

	virtual void add_global_constant(const StringName &p_variable, const Variant &p_value) override {
		(void)p_variable;
		(void)p_value;
	}

	virtual String debug_get_error() const override { return String(); }
	virtual int debug_get_stack_level_count() const override { return 0; }
	virtual int debug_get_stack_level_line(int p_level) const override {
		(void)p_level;
		return 0;
	}
	virtual String debug_get_stack_level_function(int p_level) const override {
		(void)p_level;
		return String();
	}
	virtual String debug_get_stack_level_source(int p_level) const override {
		(void)p_level;
		return String();
	}
	virtual void debug_get_stack_level_locals(int p_level, List<String> *p_locals, List<Variant> *p_values, int p_max_subitems = -1, int p_max_depth = -1) override {
		(void)p_level;
		(void)p_locals;
		(void)p_values;
		(void)p_max_subitems;
		(void)p_max_depth;
	}
	virtual void debug_get_stack_level_members(int p_level, List<String> *p_members, List<Variant> *p_values, int p_max_subitems = -1, int p_max_depth = -1) override {
		(void)p_level;
		(void)p_members;
		(void)p_values;
		(void)p_max_subitems;
		(void)p_max_depth;
	}
	virtual void debug_get_globals(List<String> *p_globals, List<Variant> *p_values, int p_max_subitems = -1, int p_max_depth = -1) override {
		(void)p_globals;
		(void)p_values;
		(void)p_max_subitems;
		(void)p_max_depth;
	}
	virtual String debug_parse_stack_level_expression(int p_level, const String &p_expression, int p_max_subitems = -1, int p_max_depth = -1) override {
		(void)p_level;
		(void)p_expression;
		(void)p_max_subitems;
		(void)p_max_depth;
		return String();
	}

	virtual void reload_all_scripts() override {}
	virtual void reload_scripts(const Array &p_scripts, bool p_soft_reload) override {
		(void)p_scripts;
		(void)p_soft_reload;
	}
	virtual void reload_tool_script(const Ref<Script> &p_script, bool p_soft_reload) override {
		(void)p_script;
		(void)p_soft_reload;
	}

	virtual void get_recognized_extensions(List<String> *p_extensions) const override {
		if (p_extensions) {
			p_extensions->push_back("gd2");
		}
	}
	virtual void get_public_functions(List<MethodInfo> *p_functions) const override { (void)p_functions; }
	virtual void get_public_constants(List<Pair<String, Variant>> *p_constants) const override { (void)p_constants; }
	virtual void get_public_annotations(List<MethodInfo> *p_annotations) const override { (void)p_annotations; }

	virtual void profiling_start() override {}
	virtual void profiling_stop() override {}
	virtual void profiling_set_save_native_calls(bool p_enable) override { (void)p_enable; }
	virtual int profiling_get_accumulated_data(ProfilingInfo *p_info_arr, int p_info_max) override {
		(void)p_info_arr;
		(void)p_info_max;
		return 0;
	}
	virtual int profiling_get_frame_data(ProfilingInfo *p_info_arr, int p_info_max) override {
		(void)p_info_arr;
		(void)p_info_max;
		return 0;
	}
};
