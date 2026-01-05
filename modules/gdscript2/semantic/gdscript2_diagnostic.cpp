/**************************************************************************/
/*  gdscript2_diagnostic.cpp                                              */
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

#include "gdscript2_diagnostic.h"

#include "modules/gdscript2/front/gdscript2_ast.h"

// ============================================================================
// GDScript2Diagnostic
// ============================================================================

GDScript2Diagnostic GDScript2Diagnostic::from_node(GDScript2DiagnosticSeverity p_severity, GDScript2DiagnosticCode p_code,
		const String &p_message, GDScript2ASTNode *p_node) {
	GDScript2Diagnostic diag(p_severity, p_code, p_message);
	diag.node = p_node;
	if (p_node) {
		diag.line = p_node->start_line;
		diag.column = p_node->start_column;
		diag.end_line = p_node->end_line;
		diag.end_column = p_node->end_column;
	}
	return diag;
}

String GDScript2Diagnostic::format() const {
	String severity_str;
	switch (severity) {
		case GDScript2DiagnosticSeverity::SEVERITY_ERROR:
			severity_str = "error";
			break;
		case GDScript2DiagnosticSeverity::SEVERITY_WARNING:
			severity_str = "warning";
			break;
		case GDScript2DiagnosticSeverity::SEVERITY_INFO:
			severity_str = "info";
			break;
		case GDScript2DiagnosticSeverity::SEVERITY_HINT:
			severity_str = "hint";
			break;
	}

	String result = vformat("%d:%d: %s: %s", line, column, severity_str, message);

	// Add notes
	for (const String &note : notes) {
		result += "\n  note: " + note;
	}

	// Add suggestions
	for (const String &suggestion : suggestions) {
		result += "\n  suggestion: " + suggestion;
	}

	return result;
}

String GDScript2Diagnostic::format_with_source(const String &p_source) const {
	String result = format();

	// Try to show the source line
	if (line > 0 && !p_source.is_empty()) {
		Vector<String> lines = p_source.split("\n");
		if (line <= lines.size()) {
			String source_line = lines[line - 1];
			result += "\n  " + source_line;

			// Add caret pointing to the error position
			if (column > 0) {
				String caret = "\n  ";
				for (int i = 1; i < column; i++) {
					caret += " ";
				}
				caret += "^";

				// Extend caret for range
				if (end_column > column && end_line == line) {
					for (int i = column; i < end_column - 1; i++) {
						caret += "~";
					}
				}

				result += caret;
			}
		}
	}

	return result;
}

// ============================================================================
// GDScript2DiagnosticReporter
// ============================================================================

GDScript2DiagnosticReporter::GDScript2DiagnosticReporter() {
	// Enable all warnings by default
	// Users can disable specific warnings via configuration
}

void GDScript2DiagnosticReporter::report(const GDScript2Diagnostic &p_diagnostic) {
	// Check if warning is enabled
	if (p_diagnostic.severity == GDScript2DiagnosticSeverity::SEVERITY_WARNING) {
		if (!is_warning_enabled(p_diagnostic.code)) {
			return;
		}

		// Check if warning should be treated as error
		const GDScript2DiagnosticSeverity *as_error = warning_as_error.getptr(p_diagnostic.code);
		if (as_error && *as_error == GDScript2DiagnosticSeverity::SEVERITY_ERROR) {
			GDScript2Diagnostic error_diag = p_diagnostic;
			error_diag.severity = GDScript2DiagnosticSeverity::SEVERITY_ERROR;
			diagnostics.push_back(error_diag);
			error_count++;
			return;
		}
	}

	diagnostics.push_back(p_diagnostic);

	switch (p_diagnostic.severity) {
		case GDScript2DiagnosticSeverity::SEVERITY_ERROR:
			error_count++;
			break;
		case GDScript2DiagnosticSeverity::SEVERITY_WARNING:
			warning_count++;
			break;
		default:
			break;
	}
}

void GDScript2DiagnosticReporter::report_error(GDScript2DiagnosticCode p_code, const String &p_message, GDScript2ASTNode *p_node) {
	report(GDScript2Diagnostic::from_node(GDScript2DiagnosticSeverity::SEVERITY_ERROR, p_code, p_message, p_node));
}

void GDScript2DiagnosticReporter::report_error(GDScript2DiagnosticCode p_code, const String &p_message, int p_line, int p_column) {
	GDScript2Diagnostic diag(GDScript2DiagnosticSeverity::SEVERITY_ERROR, p_code, p_message);
	diag.line = p_line;
	diag.column = p_column;
	report(diag);
}

void GDScript2DiagnosticReporter::report_warning(GDScript2DiagnosticCode p_code, const String &p_message, GDScript2ASTNode *p_node) {
	report(GDScript2Diagnostic::from_node(GDScript2DiagnosticSeverity::SEVERITY_WARNING, p_code, p_message, p_node));
}

void GDScript2DiagnosticReporter::report_warning(GDScript2DiagnosticCode p_code, const String &p_message, int p_line, int p_column) {
	GDScript2Diagnostic diag(GDScript2DiagnosticSeverity::SEVERITY_WARNING, p_code, p_message);
	diag.line = p_line;
	diag.column = p_column;
	report(diag);
}

void GDScript2DiagnosticReporter::report_info(GDScript2DiagnosticCode p_code, const String &p_message, GDScript2ASTNode *p_node) {
	report(GDScript2Diagnostic::from_node(GDScript2DiagnosticSeverity::SEVERITY_INFO, p_code, p_message, p_node));
}

void GDScript2DiagnosticReporter::set_warning_enabled(GDScript2DiagnosticCode p_code, bool p_enabled) {
	warning_enabled.insert(p_code, p_enabled);
}

void GDScript2DiagnosticReporter::set_warning_as_error(GDScript2DiagnosticCode p_code, bool p_as_error) {
	if (p_as_error) {
		warning_as_error.insert(p_code, GDScript2DiagnosticSeverity::SEVERITY_ERROR);
	} else {
		warning_as_error.erase(p_code);
	}
}

bool GDScript2DiagnosticReporter::is_warning_enabled(GDScript2DiagnosticCode p_code) const {
	const bool *enabled = warning_enabled.getptr(p_code);
	if (enabled) {
		return *enabled;
	}
	// Warnings enabled by default
	return true;
}

LocalVector<GDScript2Diagnostic> GDScript2DiagnosticReporter::get_errors() const {
	LocalVector<GDScript2Diagnostic> errors;
	for (const GDScript2Diagnostic &diag : diagnostics) {
		if (diag.severity == GDScript2DiagnosticSeverity::SEVERITY_ERROR) {
			errors.push_back(diag);
		}
	}
	return errors;
}

LocalVector<GDScript2Diagnostic> GDScript2DiagnosticReporter::get_warnings() const {
	LocalVector<GDScript2Diagnostic> warnings;
	for (const GDScript2Diagnostic &diag : diagnostics) {
		if (diag.severity == GDScript2DiagnosticSeverity::SEVERITY_WARNING) {
			warnings.push_back(diag);
		}
	}
	return warnings;
}

List<String> GDScript2DiagnosticReporter::get_error_strings() const {
	List<String> result;
	for (const GDScript2Diagnostic &diag : diagnostics) {
		if (diag.severity == GDScript2DiagnosticSeverity::SEVERITY_ERROR) {
			result.push_back(diag.format());
		}
	}
	return result;
}

List<String> GDScript2DiagnosticReporter::get_warning_strings() const {
	List<String> result;
	for (const GDScript2Diagnostic &diag : diagnostics) {
		if (diag.severity == GDScript2DiagnosticSeverity::SEVERITY_WARNING) {
			result.push_back(diag.format());
		}
	}
	return result;
}

void GDScript2DiagnosticReporter::clear() {
	diagnostics.clear();
	error_count = 0;
	warning_count = 0;
}
