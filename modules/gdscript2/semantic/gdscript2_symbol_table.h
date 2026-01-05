/**************************************************************************/
/*  gdscript2_symbol_table.h                                              */
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

#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"
#include "gdscript2_type.h"

// Forward declarations
struct GDScript2ASTNode;
struct GDScript2ClassNode;
struct GDScript2FunctionNode;

// Symbol kinds
enum class GDScript2SymbolKind {
	SYMBOL_UNKNOWN,
	SYMBOL_VARIABLE, // Local or member variable
	SYMBOL_CONSTANT, // const declaration
	SYMBOL_PARAMETER, // Function parameter
	SYMBOL_FUNCTION, // Function/method
	SYMBOL_CLASS, // Class definition
	SYMBOL_SIGNAL, // Signal
	SYMBOL_ENUM, // Enum type
	SYMBOL_ENUM_VALUE, // Enum value
	SYMBOL_BUILTIN_TYPE, // Built-in type (int, String, etc.)
	SYMBOL_BUILTIN_FUNC, // Built-in function (print, str, etc.)
	SYMBOL_NATIVE_CLASS, // Native Godot class
	SYMBOL_SINGLETON, // Autoload singleton
};

// Symbol definition
struct GDScript2Symbol {
	StringName name;
	GDScript2SymbolKind kind = GDScript2SymbolKind::SYMBOL_UNKNOWN;
	GDScript2Type type;

	// Source location
	int line = 0;
	int column = 0;

	// AST node that defines this symbol (if any)
	GDScript2ASTNode *definition_node = nullptr;

	// For functions: signature info
	GDScript2FunctionSignature *func_signature = nullptr;

	// For class members
	bool is_static = false;
	bool is_exported = false;

	// For enum values
	int64_t enum_value = 0;
	StringName enum_name; // Parent enum name

	// Usage tracking
	bool is_used = false;

	GDScript2Symbol() = default;
	GDScript2Symbol(const StringName &p_name, GDScript2SymbolKind p_kind, const GDScript2Type &p_type) :
			name(p_name), kind(p_kind), type(p_type) {}

	~GDScript2Symbol() {
		if (func_signature) {
			memdelete(func_signature);
		}
	}

	// Prevent copying due to owning func_signature
	GDScript2Symbol(const GDScript2Symbol &) = delete;
	GDScript2Symbol &operator=(const GDScript2Symbol &) = delete;
};

// Scope kind
enum class GDScript2ScopeKind {
	SCOPE_GLOBAL, // Global/builtin scope
	SCOPE_CLASS, // Class scope (members)
	SCOPE_FUNCTION, // Function scope
	SCOPE_BLOCK, // Block scope (if, for, while, etc.)
	SCOPE_LAMBDA, // Lambda scope
	SCOPE_MATCH, // Match branch scope (bindings)
};

// A single scope in the scope chain
class GDScript2Scope {
	friend class GDScript2SymbolTable;

	GDScript2ScopeKind kind = GDScript2ScopeKind::SCOPE_BLOCK;
	GDScript2Scope *parent = nullptr;
	HashMap<StringName, GDScript2Symbol *> symbols;

	// For class scopes
	GDScript2ClassNode *class_node = nullptr;

	// For function scopes
	GDScript2FunctionNode *function_node = nullptr;

	// Track loop nesting for break/continue validation
	int loop_depth = 0;

public:
	GDScript2Scope(GDScript2ScopeKind p_kind = GDScript2ScopeKind::SCOPE_BLOCK) :
			kind(p_kind) {}

	~GDScript2Scope() {
		for (KeyValue<StringName, GDScript2Symbol *> &kv : symbols) {
			if (kv.value) {
				memdelete(kv.value);
			}
		}
	}

	GDScript2ScopeKind get_kind() const { return kind; }
	GDScript2Scope *get_parent() const { return parent; }
	GDScript2ClassNode *get_class_node() const { return class_node; }
	GDScript2FunctionNode *get_function_node() const { return function_node; }
	int get_loop_depth() const { return loop_depth; }

	// Direct symbol access (current scope only)
	bool has_local(const StringName &p_name) const { return symbols.has(p_name); }

	GDScript2Symbol *get_local(const StringName &p_name) const {
		const GDScript2Symbol *const *found = symbols.getptr(p_name);
		return found ? const_cast<GDScript2Symbol *>(*found) : nullptr;
	}

	void add_symbol(GDScript2Symbol *p_symbol) {
		symbols.insert(p_symbol->name, p_symbol);
	}

	// Iterate all symbols in this scope
	template <typename Fn>
	void for_each_symbol(Fn &&p_func) const {
		for (const KeyValue<StringName, GDScript2Symbol *> &kv : symbols) {
			p_func(kv.value);
		}
	}
};

// Symbol table with nested scopes
class GDScript2SymbolTable {
	// Root global scope (built-ins, singletons)
	GDScript2Scope *global_scope = nullptr;

	// Current scope during analysis
	GDScript2Scope *current_scope = nullptr;

	// All allocated scopes (for cleanup)
	LocalVector<GDScript2Scope *> all_scopes;

	// Class scope stack (for resolving 'self' and member access)
	LocalVector<GDScript2Scope *> class_scope_stack;

	// Function scope stack (for return type checking)
	LocalVector<GDScript2Scope *> function_scope_stack;

public:
	GDScript2SymbolTable();
	~GDScript2SymbolTable();

	// Initialize built-in symbols
	void initialize_builtins();

	// Scope management
	GDScript2Scope *push_scope(GDScript2ScopeKind p_kind);
	void pop_scope();
	GDScript2Scope *get_current_scope() const { return current_scope; }
	GDScript2Scope *get_global_scope() const { return global_scope; }

	// Class scope management
	void enter_class(GDScript2ClassNode *p_class);
	void exit_class();
	GDScript2Scope *get_current_class_scope() const;
	GDScript2ClassNode *get_current_class() const;

	// Function scope management
	void enter_function(GDScript2FunctionNode *p_func);
	void exit_function();
	GDScript2Scope *get_current_function_scope() const;
	GDScript2FunctionNode *get_current_function() const;

	// Loop tracking
	void enter_loop();
	void exit_loop();
	bool is_in_loop() const;

	// Symbol operations
	bool declare(GDScript2Symbol *p_symbol); // Returns false if already declared in current scope
	GDScript2Symbol *lookup(const StringName &p_name) const; // Look up in all scopes
	GDScript2Symbol *lookup_local(const StringName &p_name) const; // Look up in current scope only
	GDScript2Symbol *lookup_in_class(const StringName &p_name) const; // Look up in class scope

	// Check if symbol is declared in current scope
	bool is_declared_local(const StringName &p_name) const;

	// Check if we're in static context
	bool is_static_context() const;

	// Get all symbols in current scope
	template <typename Fn>
	void for_each_in_current_scope(Fn &&p_func) const {
		if (current_scope) {
			current_scope->for_each_symbol(std::forward<Fn>(p_func));
		}
	}

	// Legacy compatibility - simple flat symbol table interface
	bool has(const StringName &p_name) const { return lookup(p_name) != nullptr; }
	void set(const StringName &p_name, const GDScript2Type &p_type);
	bool get(const StringName &p_name, GDScript2Type &r_type) const;
};
