/**************************************************************************/
/*  gdscript2.h                                                           */
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

#include "codegen/gdscript2_codegen.h"
#include "front/gdscript2_ast.h"
#include "front/gdscript2_parser.h"
#include "front/gdscript2_tokenizer.h"
#include "ir/gdscript2_ir.h"
#include "ir/gdscript2_ir_builder.h"
#include "runtime/gdscript2_builtin.h"
#include "semantic/gdscript2_semantic.h"
#include "vm/gdscript2_vm.h"

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/object/script_language.h"

class GDScript2Language;
class GDScript2Instance;

class GDScript2 : public Script {
	GDCLASS(GDScript2, Script);

	friend class GDScript2Instance;
	friend class GDScript2Language;

private:
	bool valid = false;
	bool tool = false;

	String source;
	String path;

	// Compilation artifacts
	GDScript2ClassNode *parsed_class = nullptr;
	GDScript2IRModule *ir_module = nullptr;
	GDScript2CompiledModule *compiled_module = nullptr;
	GDScript2VM *vm = nullptr;

	// Script metadata
	HashMap<StringName, GDScript2CompiledFunction *> member_functions;
	HashMap<StringName, int> member_variables; // Name -> index
	HashMap<StringName, Variant> constants;
	HashMap<StringName, MethodInfo> signals;
	Vector<PropertyInfo> script_properties;

	GDScript2Language *language = nullptr;

	// Compilation error tracking
	List<GDScript2Diagnostic> last_errors;

	void _clear();
	Error _compile();

protected:
	static void _bind_methods();

public:
	GDScript2();
	virtual ~GDScript2() override;

	void set_language(GDScript2Language *p_language) { language = p_language; }

	// Script interface
	virtual bool can_instantiate() const override;
	virtual Ref<Script> get_base_script() const override { return Ref<Script>(); }
	virtual StringName get_global_name() const override { return StringName(); }
	virtual bool inherits_script(const Ref<Script> &p_script) const override;

	virtual StringName get_instance_base_type() const override;
	virtual ScriptInstance *instance_create(Object *p_this) override;
	virtual PlaceHolderScriptInstance *placeholder_instance_create(Object *p_this) override;
	virtual bool instance_has(const Object *p_this) const override;

	virtual bool has_source_code() const override;
	virtual String get_source_code() const override;
	virtual void set_source_code(const String &p_code) override;
	virtual Error reload(bool p_keep_state = false) override;

#ifdef TOOLS_ENABLED
	virtual StringName get_doc_class_name() const override { return StringName(); }
	virtual Vector<DocData::ClassDoc> get_documentation() const override { return Vector<DocData::ClassDoc>(); }
	virtual String get_class_icon_path() const override { return String(); }
#endif

	virtual bool has_method(const StringName &p_method) const override;
	virtual MethodInfo get_method_info(const StringName &p_method) const override;

	virtual bool is_tool() const override { return tool; }
	virtual bool is_valid() const override { return valid; }
	virtual bool is_abstract() const override { return false; }

	virtual ScriptLanguage *get_language() const override;

	virtual bool has_script_signal(const StringName &p_signal) const override;
	virtual void get_script_signal_list(List<MethodInfo> *r_signals) const override;

	virtual bool get_property_default_value(const StringName &p_property, Variant &r_value) const override;

	virtual void get_script_method_list(List<MethodInfo> *p_list) const override;
	virtual void get_script_property_list(List<PropertyInfo> *p_list) const override;

	virtual const Variant get_rpc_config() const override { return Variant(); }

	// Internal API
	GDScript2VM *get_vm() const { return vm; }
	GDScript2CompiledModule *get_compiled_module() const { return compiled_module; }
	const HashMap<StringName, int> &get_member_variables() const { return member_variables; }
	int get_member_count() const { return member_variables.size(); }
};

class GDScript2Instance : public ScriptInstance {
	friend class GDScript2;

	ObjectID owner_id;
	Object *owner = nullptr;
	Ref<GDScript2> script;
	Vector<Variant> members; // Member variable values

public:
	GDScript2Instance() {}
	virtual ~GDScript2Instance() override;

	void set_owner(Object *p_owner) { owner = p_owner; }
	void set_script(const Ref<GDScript2> &p_script) { script = p_script; }

	// ScriptInstance interface
	virtual Object *get_owner() override { return owner; }

	virtual bool set(const StringName &p_name, const Variant &p_value) override;
	virtual bool get(const StringName &p_name, Variant &r_ret) const override;
	virtual void get_property_list(List<PropertyInfo> *p_properties) const override;
	virtual Variant::Type get_property_type(const StringName &p_name, bool *r_is_valid = nullptr) const override;
	virtual void validate_property(PropertyInfo &p_property) const override {}

	virtual bool property_can_revert(const StringName &p_name) const override;
	virtual bool property_get_revert(const StringName &p_name, Variant &r_ret) const override;

	virtual void get_method_list(List<MethodInfo> *p_list) const override;
	virtual bool has_method(const StringName &p_method) const override;

	virtual int get_method_argument_count(const StringName &p_method, bool *r_is_valid = nullptr) const override;

	virtual Variant callp(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) override;

	virtual void notification(int p_notification, bool p_reversed = false) override;

	virtual Ref<Script> get_script() const override;
	virtual ScriptLanguage *get_language() override;

	virtual const Variant get_rpc_config() const override { return Variant(); }
};
