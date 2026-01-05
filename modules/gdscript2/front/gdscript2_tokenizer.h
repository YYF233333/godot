/**************************************************************************/
/*  gdscript2_tokenizer.h                                                 */
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

#include "gdscript2_token.h"

#include "core/templates/hash_map.h"
#include "core/templates/list.h"
#include "core/templates/vector.h"

// GDScript2 Tokenizer - Lexical analyzer for GDScript2 source code.
// Converts source text into a stream of tokens for the parser.
class GDScript2Tokenizer {
public:
#ifdef TOOLS_ENABLED
	// Comment data for documentation extraction.
	struct CommentData {
		String comment;
		bool new_line = false; // True if comment starts at beginning of line.

		CommentData() = default;
		CommentData(const String &p_comment, bool p_new_line) :
				comment(p_comment), new_line(p_new_line) {}
	};
#endif

private:
	// Source code storage
	String source;
	const char32_t *_source = nullptr;
	const char32_t *_current = nullptr;
	int length = 0;
	int position = 0;

	// Position tracking
	int line = 1;
	int column = 1;
	const char32_t *_start = nullptr;
	int start_line = 0;
	int start_column = 0;

	// Cursor tracking (for IDE integration)
	int cursor_line = -1;
	int cursor_column = -1;

	// Indentation handling
	int tab_size = 4;
	char32_t indent_char = '\0';
	int pending_indents = 0;
	List<int> indent_stack;
	List<List<int>> indent_stack_stack; // For lambdas

	// State tracking
	bool line_continuation = false;
	bool multiline_mode = false;
	bool pending_newline = false;
	GDScript2Token last_token;
	GDScript2Token last_newline;
	List<GDScript2Token> error_stack;
	List<char32_t> paren_stack;
	Vector<int> continuation_lines;

#ifdef TOOLS_ENABLED
	HashMap<int, CommentData> comments;
#endif

#ifdef DEBUG_ENABLED
	Vector<String> keyword_list;
	void make_keyword_list();
#endif

	// Helper methods
	_FORCE_INLINE_ bool _is_at_end() const { return position >= length; }
	_FORCE_INLINE_ char32_t _peek(int p_offset = 0) const {
		int pos = position + p_offset;
		return (pos >= 0 && pos < length) ? _current[p_offset] : '\0';
	}
	_FORCE_INLINE_ int indent_level() const { return indent_stack.size(); }
	_FORCE_INLINE_ bool has_error() const { return !error_stack.is_empty(); }

	char32_t _advance();
	String _get_indent_char_name(char32_t ch) const;
	void _skip_whitespace();
	void check_indent();

	// Token creation
	GDScript2Token make_token(GDScript2TokenType p_type);
	GDScript2Token make_literal(const Variant &p_literal);
	GDScript2Token make_identifier(const StringName &p_identifier);
	GDScript2Token make_error(const String &p_message);
	GDScript2Token make_paren_error(char32_t p_paren);
	GDScript2Token check_vcs_marker(char32_t p_test, GDScript2TokenType p_double_type);

	void push_error(const String &p_message);
	void push_error(const GDScript2Token &p_error);
	GDScript2Token pop_error();

	// Parenthesis tracking
	void push_paren(char32_t p_char);
	bool pop_paren(char32_t p_expected);

	// Newline handling
	void newline(bool p_make_token);

	// Token scanners
	GDScript2Token scan_number();
	GDScript2Token scan_string();
	GDScript2Token scan_identifier();
	GDScript2Token scan_annotation();

public:
	// Initialize with source code
	void set_source_code(const String &p_source_code);

	// Main scanning function - returns next token
	GDScript2Token scan();

	// Cursor position (for IDE integration)
	int get_cursor_line() const { return cursor_line; }
	int get_cursor_column() const { return cursor_column; }
	void set_cursor_position(int p_line, int p_column);
	bool is_past_cursor() const;

	// Multiline mode (for expressions inside parentheses)
	void set_multiline_mode(bool p_state) { multiline_mode = p_state; }
	bool is_multiline_mode() const { return multiline_mode; }

	// Lambda/expression block indentation
	void push_expression_indented_block();
	void pop_expression_indented_block();

	// Continuation lines (for line continuation with \)
	const Vector<int> &get_continuation_lines() const { return continuation_lines; }

#ifdef TOOLS_ENABLED
	const HashMap<int, CommentData> &get_comments() const { return comments; }
	int get_current_position() const { return position; }
	const String &get_source_code() const { return source; }
#endif

	GDScript2Tokenizer();
};
