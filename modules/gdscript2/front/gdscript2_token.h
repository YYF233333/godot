/**************************************************************************/
/*  gdscript2_token.h                                                     */
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
#include "core/variant/variant.h"

// Token types for GDScript2 lexer.
// Organized by category for clarity.
enum class GDScript2TokenType {
	// Special
	TK_EMPTY,
	TK_ERROR,
	TK_EOF,

	// Basic
	TK_ANNOTATION, // @something
	TK_IDENTIFIER,
	TK_LITERAL, // Numbers, strings, etc.

	// Comparison operators
	TK_LESS, // <
	TK_LESS_EQUAL, // <=
	TK_GREATER, // >
	TK_GREATER_EQUAL, // >=
	TK_EQUAL_EQUAL, // ==
	TK_BANG_EQUAL, // !=

	// Logical operators
	TK_AND, // and
	TK_OR, // or
	TK_NOT, // not
	TK_AMPERSAND_AMPERSAND, // &&
	TK_PIPE_PIPE, // ||
	TK_BANG, // !

	// Bitwise operators
	TK_AMPERSAND, // &
	TK_PIPE, // |
	TK_TILDE, // ~
	TK_CARET, // ^
	TK_LESS_LESS, // <<
	TK_GREATER_GREATER, // >>

	// Math operators
	TK_PLUS, // +
	TK_MINUS, // -
	TK_STAR, // *
	TK_STAR_STAR, // **
	TK_SLASH, // /
	TK_PERCENT, // %

	// Assignment operators
	TK_EQUAL, // =
	TK_PLUS_EQUAL, // +=
	TK_MINUS_EQUAL, // -=
	TK_STAR_EQUAL, // *=
	TK_STAR_STAR_EQUAL, // **=
	TK_SLASH_EQUAL, // /=
	TK_PERCENT_EQUAL, // %=
	TK_LESS_LESS_EQUAL, // <<=
	TK_GREATER_GREATER_EQUAL, // >>=
	TK_AMPERSAND_EQUAL, // &=
	TK_PIPE_EQUAL, // |=
	TK_CARET_EQUAL, // ^=

	// Control flow keywords
	TK_IF,
	TK_ELIF,
	TK_ELSE,
	TK_FOR,
	TK_WHILE,
	TK_BREAK,
	TK_CONTINUE,
	TK_PASS,
	TK_RETURN,
	TK_MATCH,
	TK_WHEN,

	// Keywords
	TK_AS,
	TK_ASSERT,
	TK_AWAIT,
	TK_BREAKPOINT,
	TK_CLASS,
	TK_CLASS_NAME,
	TK_CONST,
	TK_ENUM,
	TK_EXTENDS,
	TK_FUNC,
	TK_IN,
	TK_IS,
	TK_NAMESPACE,
	TK_PRELOAD,
	TK_SELF,
	TK_SIGNAL,
	TK_STATIC,
	TK_SUPER,
	TK_TRAIT,
	TK_VAR,
	TK_VOID,
	TK_YIELD,

	// Punctuation
	TK_BRACKET_OPEN, // [
	TK_BRACKET_CLOSE, // ]
	TK_BRACE_OPEN, // {
	TK_BRACE_CLOSE, // }
	TK_PARENTHESIS_OPEN, // (
	TK_PARENTHESIS_CLOSE, // )
	TK_COMMA, // ,
	TK_SEMICOLON, // ;
	TK_PERIOD, // .
	TK_PERIOD_PERIOD, // ..
	TK_PERIOD_PERIOD_PERIOD, // ...
	TK_COLON, // :
	TK_DOLLAR, // $
	TK_FORWARD_ARROW, // ->
	TK_UNDERSCORE, // _ (lone underscore)

	// Whitespace tokens (significant in GDScript)
	TK_NEWLINE,
	TK_INDENT,
	TK_DEDENT,

	// Built-in constants
	TK_CONST_PI,
	TK_CONST_TAU,
	TK_CONST_INF,
	TK_CONST_NAN,

	// Error hints (for better error messages)
	TK_VCS_CONFLICT_MARKER,
	TK_BACKTICK, // `
	TK_QUESTION_MARK, // ?

	TK_MAX
};

// Cursor position relative to token (for IDE integration).
enum class GDScript2CursorPlace {
	CURSOR_NONE,
	CURSOR_BEGINNING,
	CURSOR_MIDDLE,
	CURSOR_END,
};

// Token structure containing all information about a lexed token.
struct GDScript2Token {
	GDScript2TokenType type = GDScript2TokenType::TK_EMPTY;

	// Source location
	int start_line = 0;
	int end_line = 0;
	int start_column = 0;
	int end_column = 0;

	// Source text of the token
	String source;

	// Literal value (for LITERAL, IDENTIFIER, ANNOTATION, ERROR tokens)
	Variant literal;

	// Cursor tracking for IDE features
	int cursor_position = -1;
	GDScript2CursorPlace cursor_place = GDScript2CursorPlace::CURSOR_NONE;

	// Helper methods
	const char *get_name() const;
	String get_debug_name() const;
	bool is_identifier() const;
	bool is_literal() const;
	bool can_precede_bin_op() const;

	StringName get_identifier() const {
		return literal;
	}

	GDScript2Token() = default;
	explicit GDScript2Token(GDScript2TokenType p_type) :
			type(p_type) {}
};

// Get the string name of a token type.
const char *gdscript2_get_token_name(GDScript2TokenType p_type);
