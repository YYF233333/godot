/**************************************************************************/
/*  gdscript2.cpp                                                         */
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

#include "gdscript2.h"

#include "gdscript2_language.h"

#include "core/config/engine.h"
#include "core/io/file_access.h"

// ============================================================================
// GDScript2
// ============================================================================

void GDScript2::_bind_methods() {
}

GDScript2::GDScript2() {
	GDScript2BuiltinRegistry::initialize();
}

GDScript2::~GDScript2() {
	_clear();
}

void GDScript2::_clear() {
	if (parsed_class) {
		memdelete(parsed_class);
		parsed_class = nullptr;
	}

	if (ir_module) {
		memdelete(ir_module);
		ir_module = nullptr;
	}

	if (compiled_module) {
		memdelete(compiled_module);
		compiled_module = nullptr;
	}

	if (vm) {
		memdelete(vm);
		vm = nullptr;
	}

	member_functions.clear();
	member_variables.clear();
	constants.clear();
	signals.clear();
	script_properties.clear();
	last_errors.clear();

	valid = false;
}

Error GDScript2::_compile() {
	_clear();

	if (source.is_empty()) {
		ERR_PRINT("GDScript2: Source is empty");
		return FAILED;
	}

	// Step 1 & 2: Parse (Tokenizer is internal to Parser)
	Ref<GDScript2Parser> parser;
	parser.instantiate();
	GDScript2Parser::Result parse_result = parser->parse(source);

	if (parse_result.has_errors()) {
		ERR_PRINT("GDScript2: Parse failed with " + itos(parse_result.errors.size()) + " errors.");
		for (const GDScript2Parser::Error &err : parse_result.errors) {
			ERR_PRINT("  Line " + itos(err.line) + ": " + err.message);
		}
		return ERR_PARSE_ERROR;
	}

	parsed_class = parse_result.root;

	// Step 3: Semantic Analysis
	Ref<GDScript2SemanticAnalyzer> semantic;
	semantic.instantiate();
	GDScript2SemanticAnalyzer::Result semantic_result = semantic->analyze(parsed_class);

	if (semantic_result.has_errors()) {
		ERR_PRINT("GDScript2: Semantic analysis failed with " + itos(semantic_result.diagnostics.get_error_count()) + " errors.");
		const LocalVector<GDScript2Diagnostic> &diags = semantic_result.diagnostics.get_diagnostics();
		for (uint32_t i = 0; i < diags.size(); i++) {
			const GDScript2Diagnostic &diag = diags[i];
			if (diag.severity == GDScript2DiagnosticSeverity::SEVERITY_ERROR) {
				ERR_PRINT("  Line " + itos(diag.line) + ": " + diag.message);
			}
		}
		return ERR_COMPILATION_FAILED;
	}

	// Step 4: Build IR
	Ref<GDScript2IRBuilder> ir_builder;
	ir_builder.instantiate();
	GDScript2IRBuilder::Result ir_result = ir_builder->build(parsed_class);

	if (ir_result.has_errors()) {
		ERR_PRINT("GDScript2: IR build failed.");
		return ERR_COMPILATION_FAILED;
	}

	ir_module = memnew(GDScript2IRModule);
	*ir_module = ir_result.module;

	// Step 5: Generate Bytecode
	Ref<GDScript2CodeGenerator> codegen;
	codegen.instantiate();
	GDScript2CodeGenerator::Result codegen_result = codegen->generate(*ir_module);

	if (codegen_result.has_errors()) {
		ERR_PRINT("GDScript2: Code generation failed.");
		return ERR_COMPILATION_FAILED;
	}

	compiled_module = memnew(GDScript2CompiledModule);
	*compiled_module = codegen_result.module;

	// Step 6: Create VM
	vm = memnew(GDScript2VM);
	vm->load_module(compiled_module);

	// Step 7: Extract metadata
	for (int i = 0; i < compiled_module->functions.size(); i++) {
		const GDScript2CompiledFunction &func = compiled_module->functions[i];
		member_functions[func.name] = const_cast<GDScript2CompiledFunction *>(&func);
	}

	// Extract member variables from parsed AST
	if (parsed_class) {
		int member_index = 0;
		for (GDScript2VariableNode *var : parsed_class->variables) {
			member_variables[var->name] = member_index++;

			// Build PropertyInfo
			PropertyInfo prop;
			prop.name = var->name;
			prop.type = Variant::NIL; // TODO: Map from GDScript2Type
			script_properties.push_back(prop);
		}
	}

	// Check if this is a tool script
	tool = false; // TODO: Detect @tool annotation

	valid = true;
	return OK;
}

bool GDScript2::can_instantiate() const {
	return valid;
}

bool GDScript2::inherits_script(const Ref<Script> &p_script) const {
	// TODO: Implement inheritance
	return false;
}

StringName GDScript2::get_instance_base_type() const {
	// TODO: Get from extends clause, default to RefCounted or Object
	return StringName("RefCounted");
}

ScriptInstance *GDScript2::instance_create(Object *p_this) {
	if (!valid) {
		ERR_PRINT("GDScript2: Cannot instantiate invalid script.");
		return nullptr;
	}

	// Check if we're in the editor and this is not a tool script
	if (Engine::get_singleton()->is_editor_hint() && !is_tool()) {
		print_line("GDScript2: Creating instance in editor (non-tool script) for ", p_this->get_class());
	}

	GDScript2Instance *instance = memnew(GDScript2Instance);
	instance->set_owner(p_this);
	instance->set_script(Ref<GDScript2>(this));

	// Initialize member variables with default values
	instance->members.resize(member_variables.size());

	// Set initial values from parsed AST
	if (parsed_class) {
		for (GDScript2VariableNode *var : parsed_class->variables) {
			int idx = member_variables[var->name];
			if (var->initializer) {
				// TODO: Evaluate initializer expression
				// For now, just use default Variant
				instance->members.write[idx] = Variant();
			} else {
				instance->members.write[idx] = Variant();
			}
		}
	}

	// Call _init if it exists
	if (member_functions.has("_init")) {
		Callable::CallError error;
		const Variant *null_args = nullptr;
		instance->callp("_init", &null_args, 0, error);
		if (error.error != Callable::CallError::CALL_OK) {
			ERR_PRINT("GDScript2: Failed to call _init");
		}
	}

	return instance;
}

PlaceHolderScriptInstance *GDScript2::placeholder_instance_create(Object *p_this) {
	// TODO: Implement placeholder instances for Editor
	return nullptr;
}

bool GDScript2::instance_has(const Object *p_this) const {
	// TODO: Track instances
	return false;
}

bool GDScript2::has_source_code() const {
	return !source.is_empty();
}

String GDScript2::get_source_code() const {
	return source;
}

void GDScript2::set_source_code(const String &p_code) {
	if (source != p_code) {
		source = p_code;
		valid = false;
	}
}

Error GDScript2::reload(bool p_keep_state) {
	return _compile();
}

bool GDScript2::has_method(const StringName &p_method) const {
	return member_functions.has(p_method);
}

MethodInfo GDScript2::get_method_info(const StringName &p_method) const {
	if (!member_functions.has(p_method)) {
		return MethodInfo();
	}

	//const GDScript2CompiledFunction *func = member_functions[p_method];
	MethodInfo mi;
	mi.name = p_method;
	// TODO: Add argument info from function metadata
	return mi;
}

ScriptLanguage *GDScript2::get_language() const {
	return language;
}

bool GDScript2::has_script_signal(const StringName &p_signal) const {
	return signals.has(p_signal);
}

void GDScript2::get_script_signal_list(List<MethodInfo> *r_signals) const {
	for (const KeyValue<StringName, MethodInfo> &E : signals) {
		r_signals->push_back(E.value);
	}
}

bool GDScript2::get_property_default_value(const StringName &p_property, Variant &r_value) const {
	// TODO: Get default values from initializers
	r_value = Variant();
	return false;
}

void GDScript2::get_script_method_list(List<MethodInfo> *p_list) const {
	for (const KeyValue<StringName, GDScript2CompiledFunction *> &E : member_functions) {
		MethodInfo mi;
		mi.name = E.key;
		p_list->push_back(mi);
	}
}

void GDScript2::get_script_property_list(List<PropertyInfo> *p_list) const {
	for (const PropertyInfo &E : script_properties) {
		p_list->push_back(E);
	}
}

// ============================================================================
// GDScript2Instance
// ============================================================================

GDScript2Instance::~GDScript2Instance() {
}

bool GDScript2Instance::set(const StringName &p_name, const Variant &p_value) {
	const HashMap<StringName, int> &member_vars = script->get_member_variables();

	if (member_vars.has(p_name)) {
		int idx = member_vars[p_name];
		members.write[idx] = p_value;
		return true;
	}

	return false;
}

bool GDScript2Instance::get(const StringName &p_name, Variant &r_ret) const {
	const HashMap<StringName, int> &member_vars = script->get_member_variables();

	if (member_vars.has(p_name)) {
		int idx = member_vars[p_name];
		r_ret = members[idx];
		return true;
	}

	return false;
}

void GDScript2Instance::get_property_list(List<PropertyInfo> *p_properties) const {
	script->get_script_property_list(p_properties);
}

Variant::Type GDScript2Instance::get_property_type(const StringName &p_name, bool *r_is_valid) const {
	if (r_is_valid) {
		*r_is_valid = script->get_member_variables().has(p_name);
	}
	return Variant::NIL; // TODO: Return actual type
}

bool GDScript2Instance::property_can_revert(const StringName &p_name) const {
	return false; // TODO: Check default values
}

bool GDScript2Instance::property_get_revert(const StringName &p_name, Variant &r_ret) const {
	return script->get_property_default_value(p_name, r_ret);
}

void GDScript2Instance::get_method_list(List<MethodInfo> *p_list) const {
	script->get_script_method_list(p_list);
}

bool GDScript2Instance::has_method(const StringName &p_method) const {
	return script->has_method(p_method);
}

int GDScript2Instance::get_method_argument_count(const StringName &p_method, bool *r_is_valid) const {
	if (script->has_method(p_method)) {
		if (r_is_valid) {
			*r_is_valid = true;
		}
		// TODO: Return actual argument count
		return 0;
	}

	if (r_is_valid) {
		*r_is_valid = false;
	}
	return 0;
}

Variant GDScript2Instance::callp(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
	// Check if this method exists in the script
	if (!script->has_method(p_method)) {
		// Method not in script - let the owner handle it (fallback to base class)
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
		return Variant();
	}

	// Don't call lifecycle methods in editor for non-tool scripts
	if (Engine::get_singleton()->is_editor_hint() && !script->is_tool()) {
		// Block lifecycle methods
		if (p_method == "_ready" || p_method == "_enter_tree" || p_method == "_exit_tree" ||
				p_method == "_process" || p_method == "_physics_process" || p_method == "_input" ||
				p_method == "_unhandled_input" || p_method == "_draw") {
			r_error.error = Callable::CallError::CALL_OK;
			return Variant();
		}
	}

	GDScript2VM *vm = script->get_vm();
	if (!vm) {
		ERR_PRINT("GDScript2: VM is null!");
		r_error.error = Callable::CallError::CALL_ERROR_INSTANCE_IS_NULL;
		return Variant();
	}

	// Build arguments vector
	Vector<Variant> args;
	args.resize(p_argcount);
	for (int i = 0; i < p_argcount; i++) {
		args.write[i] = *p_args[i];
	}

	// Call through VM (using call_with_self to provide owner context)
	GDScript2ExecutionResult result = vm->call_with_self(p_method, Variant(owner), args);

	if (result.has_error()) {
		ERR_PRINT("GDScript2: Call to " + String(p_method) + " failed: " + result.error_message);
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
		return Variant();
	}

	r_error.error = Callable::CallError::CALL_OK;
	return result.return_value;
}

void GDScript2Instance::notification(int p_notification, bool p_reversed) {
	// Don't process notifications in editor for non-tool scripts
	if (Engine::get_singleton()->is_editor_hint() && !script->is_tool()) {
		return;
	}

	// Call _notification if it exists
	if (script->has_method("_notification")) {
		Variant notification_var = p_notification;
		const Variant *args[1] = { &notification_var };
		Callable::CallError error;
		callp("_notification", args, 1, error);
	}
}

Ref<Script> GDScript2Instance::get_script() const {
	return script;
}

ScriptLanguage *GDScript2Instance::get_language() {
	return script->get_language();
}
