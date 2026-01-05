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

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/script_language.h"

class GDScript2Language;

class GDScript2Script : public Script {
	GDCLASS(GDScript2Script, Script);

	GDScript2Language *language = nullptr;

protected:
	static void _bind_methods() {}

public:
	void set_language(GDScript2Language *p_language) { language = p_language; }

	// Script interface (stubbed for bootstrap).
	virtual bool can_instantiate() const override { return false; }
	virtual Ref<Script> get_base_script() const override { return Ref<Script>(); }
	virtual StringName get_global_name() const override { return StringName(); }
	virtual bool inherits_script(const Ref<Script> &p_script) const override { return false; }

	virtual StringName get_instance_base_type() const override { return StringName(); }
	virtual ScriptInstance *instance_create(Object *p_this) override { return nullptr; }
	virtual bool instance_has(const Object *p_this) const override { return false; }

	virtual bool has_source_code() const override { return false; }
	virtual String get_source_code() const override { return String(); }
	virtual void set_source_code(const String &p_code) override { (void)p_code; }
	virtual Error reload(bool p_keep_state = false) override {
		(void)p_keep_state;
		return ERR_UNAVAILABLE;
	}

#ifdef TOOLS_ENABLED
	virtual StringName get_doc_class_name() const override { return StringName(); }
	virtual Vector<DocData::ClassDoc> get_documentation() const override { return Vector<DocData::ClassDoc>(); }
	virtual String get_class_icon_path() const override { return String(); }
#endif

	virtual bool has_method(const StringName &p_method) const override {
		(void)p_method;
		return false;
	}
	virtual MethodInfo get_method_info(const StringName &p_method) const override {
		(void)p_method;
		return MethodInfo();
	}

	virtual bool is_tool() const override { return false; }
	virtual bool is_valid() const override { return false; }
	virtual bool is_abstract() const override { return true; }

	virtual ScriptLanguage *get_language() const override;

	virtual bool has_script_signal(const StringName &p_signal) const override {
		(void)p_signal;
		return false;
	}
	virtual void get_script_signal_list(List<MethodInfo> *r_signals) const override { (void)r_signals; }

	virtual bool get_property_default_value(const StringName &p_property, Variant &r_value) const override {
		(void)p_property;
		(void)r_value;
		return false;
	}

	virtual void get_script_method_list(List<MethodInfo> *p_list) const override { (void)p_list; }
	virtual void get_script_property_list(List<PropertyInfo> *p_list) const override { (void)p_list; }

	virtual const Variant get_rpc_config() const override { return Variant(); }
};

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

	Ref<GDScript2ResourceLoader> loader;
	Ref<GDScript2ResourceSaver> saver;

protected:
	static void _bind_methods() {}

public:
	GDScript2Language();
	~GDScript2Language() override;

	Ref<GDScript2ResourceLoader> get_loader() const { return loader; }
	Ref<GDScript2ResourceSaver> get_saver() const { return saver; }

	// ScriptLanguage interface (stubbed for bootstrap).
	virtual String get_name() const override { return "GDScript2"; }
	virtual void init() override {}
	virtual String get_type() const override { return "GDScript2"; }
	virtual String get_extension() const override { return "gd"; }
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
			p_extensions->push_back("gd");
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
