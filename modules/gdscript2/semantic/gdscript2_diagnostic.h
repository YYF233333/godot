/**************************************************************************/
/*  gdscript2_diagnostic.h                                                */
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

#include "core/string/ustring.h"
#include "core/templates/local_vector.h"

// Forward declaration
struct GDScript2ASTNode;

// Diagnostic severity levels
enum class GDScript2DiagnosticSeverity {
	SEVERITY_ERROR, // Compilation error - prevents execution
	SEVERITY_WARNING, // Warning - compilation succeeds but may indicate issues
	SEVERITY_INFO, // Informational message
	SEVERITY_HINT, // Suggestion for improvement
};

// Diagnostic codes for categorization
enum class GDScript2DiagnosticCode {
	// General errors (1xxx)
	ERR_UNKNOWN = 1000,
	ERR_INTERNAL = 1001,

	// Declaration errors (2xxx)
	ERR_DUPLICATE_DECLARATION = 2001,
	ERR_UNDEFINED_IDENTIFIER = 2002,
	ERR_UNDEFINED_TYPE = 2003,
	ERR_INVALID_DECLARATION = 2004,
	ERR_SHADOWING_BUILTIN = 2005,
	ERR_INVALID_EXTENDS = 2006,
	ERR_CYCLIC_INHERITANCE = 2007,
	ERR_ENUM_VALUE_NOT_CONSTANT = 2008,
	ERR_CONST_NOT_CONSTANT = 2009,

	// Type errors (3xxx)
	ERR_TYPE_MISMATCH = 3001,
	ERR_INCOMPATIBLE_ASSIGNMENT = 3002,
	ERR_CANNOT_INFER_TYPE = 3003,
	ERR_INVALID_OPERAND_TYPE = 3004,
	ERR_INVALID_CAST = 3005,
	ERR_NOT_CALLABLE = 3006,
	ERR_NOT_ITERABLE = 3007,
	ERR_NOT_SUBSCRIPTABLE = 3008,
	ERR_NO_ATTRIBUTE = 3009,
	ERR_WRONG_ARG_COUNT = 3010,
	ERR_WRONG_ARG_TYPE = 3011,
	ERR_RETURN_TYPE_MISMATCH = 3012,
	ERR_VOID_RETURN_VALUE = 3013,
	ERR_MISSING_RETURN = 3014,

	// Control flow errors (4xxx)
	ERR_BREAK_OUTSIDE_LOOP = 4001,
	ERR_CONTINUE_OUTSIDE_LOOP = 4002,
	ERR_RETURN_OUTSIDE_FUNCTION = 4003,
	ERR_UNREACHABLE_CODE = 4004,
	ERR_AWAIT_OUTSIDE_FUNCTION = 4005,
	ERR_YIELD_OUTSIDE_FUNCTION = 4006,

	// Access errors (5xxx)
	ERR_PRIVATE_ACCESS = 5001,
	ERR_STATIC_ACCESS = 5002,
	ERR_SELF_OUTSIDE_CLASS = 5003,
	ERR_SUPER_OUTSIDE_METHOD = 5004,
	ERR_READONLY_ASSIGNMENT = 5005,
	ERR_CONSTANT_ASSIGNMENT = 5006,

	// Warnings (6xxx)
	WARN_UNUSED_VARIABLE = 6001,
	WARN_UNUSED_PARAMETER = 6002,
	WARN_UNUSED_SIGNAL = 6003,
	WARN_SHADOWING_VARIABLE = 6004,
	WARN_NARROWING_CONVERSION = 6005,
	WARN_INTEGER_DIVISION = 6006,
	WARN_UNSAFE_PROPERTY_ACCESS = 6007,
	WARN_UNSAFE_METHOD_ACCESS = 6008,
	WARN_UNSAFE_CAST = 6009,
	WARN_DEPRECATED = 6010,
	WARN_UNREACHABLE_PATTERN = 6011,
	WARN_REDUNDANT_AWAIT = 6012,
	WARN_EMPTY_PATTERN = 6013,
	WARN_STANDALONE_EXPRESSION = 6014,
	WARN_ASSERT_ALWAYS_TRUE = 6015,
	WARN_ASSERT_ALWAYS_FALSE = 6016,
	WARN_RETURN_VALUE_DISCARDED = 6017,
	WARN_UNTYPED_DECLARATION = 6018,
	WARN_INFERRED_DECLARATION = 6019,

	// Info/Hints (7xxx)
	INFO_COULD_BE_CONST = 7001,
	INFO_COULD_BE_STATIC = 7002,
};

// A single diagnostic message
struct GDScript2Diagnostic {
	GDScript2DiagnosticSeverity severity = GDScript2DiagnosticSeverity::SEVERITY_ERROR;
	GDScript2DiagnosticCode code = GDScript2DiagnosticCode::ERR_UNKNOWN;
	String message;

	// Source location
	int line = 0;
	int column = 0;
	int end_line = 0;
	int end_column = 0;

	// Related AST node (if applicable)
	GDScript2ASTNode *node = nullptr;

	// Additional context/suggestions
	LocalVector<String> notes;
	LocalVector<String> suggestions;

	GDScript2Diagnostic() = default;
	GDScript2Diagnostic(GDScript2DiagnosticSeverity p_severity, GDScript2DiagnosticCode p_code, const String &p_message) :
			severity(p_severity), code(p_code), message(p_message) {}

	// Create from AST node
	static GDScript2Diagnostic from_node(GDScript2DiagnosticSeverity p_severity, GDScript2DiagnosticCode p_code,
			const String &p_message, GDScript2ASTNode *p_node);

	// Format for display
	String format() const;
	String format_with_source(const String &p_source) const;
};

// Diagnostic collector/reporter
class GDScript2DiagnosticReporter {
	LocalVector<GDScript2Diagnostic> diagnostics;
	int error_count = 0;
	int warning_count = 0;

	// Warning configuration
	HashMap<GDScript2DiagnosticCode, bool> warning_enabled;
	HashMap<GDScript2DiagnosticCode, GDScript2DiagnosticSeverity> warning_as_error;

public:
	GDScript2DiagnosticReporter();

	// Report diagnostics
	void report(const GDScript2Diagnostic &p_diagnostic);
	void report_error(GDScript2DiagnosticCode p_code, const String &p_message, GDScript2ASTNode *p_node = nullptr);
	void report_error(GDScript2DiagnosticCode p_code, const String &p_message, int p_line, int p_column);
	void report_warning(GDScript2DiagnosticCode p_code, const String &p_message, GDScript2ASTNode *p_node = nullptr);
	void report_warning(GDScript2DiagnosticCode p_code, const String &p_message, int p_line, int p_column);
	void report_info(GDScript2DiagnosticCode p_code, const String &p_message, GDScript2ASTNode *p_node = nullptr);

	// Warning configuration
	void set_warning_enabled(GDScript2DiagnosticCode p_code, bool p_enabled);
	void set_warning_as_error(GDScript2DiagnosticCode p_code, bool p_as_error);
	bool is_warning_enabled(GDScript2DiagnosticCode p_code) const;

	// Query diagnostics
	bool has_errors() const { return error_count > 0; }
	bool has_warnings() const { return warning_count > 0; }
	int get_error_count() const { return error_count; }
	int get_warning_count() const { return warning_count; }

	const LocalVector<GDScript2Diagnostic> &get_diagnostics() const { return diagnostics; }

	// Get only errors or warnings
	LocalVector<GDScript2Diagnostic> get_errors() const;
	LocalVector<GDScript2Diagnostic> get_warnings() const;

	// Convert to string list (legacy compatibility)
	List<String> get_error_strings() const;
	List<String> get_warning_strings() const;

	// Clear all diagnostics
	void clear();
};
