/**************************************************************************/
/*  gdscript2_tokenizer.cpp                                               */
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

#include "gdscript2_tokenizer.h"

#include "core/error/error_macros.h"
#include "core/string/char_utils.h"

#ifdef TOOLS_ENABLED
#include "editor/settings/editor_settings.h"
#endif

// Token name lookup table
static const char *token_names[] = {
	"Empty", // TK_EMPTY
	"Error", // TK_ERROR
	"End of file", // TK_EOF
	// Basic
	"Annotation", // TK_ANNOTATION
	"Identifier", // TK_IDENTIFIER
	"Literal", // TK_LITERAL
	// Comparison
	"<", // TK_LESS
	"<=", // TK_LESS_EQUAL
	">", // TK_GREATER
	">=", // TK_GREATER_EQUAL
	"==", // TK_EQUAL_EQUAL
	"!=", // TK_BANG_EQUAL
	// Logical
	"and", // TK_AND
	"or", // TK_OR
	"not", // TK_NOT
	"&&", // TK_AMPERSAND_AMPERSAND
	"||", // TK_PIPE_PIPE
	"!", // TK_BANG
	// Bitwise
	"&", // TK_AMPERSAND
	"|", // TK_PIPE
	"~", // TK_TILDE
	"^", // TK_CARET
	"<<", // TK_LESS_LESS
	">>", // TK_GREATER_GREATER
	// Math
	"+", // TK_PLUS
	"-", // TK_MINUS
	"*", // TK_STAR
	"**", // TK_STAR_STAR
	"/", // TK_SLASH
	"%", // TK_PERCENT
	// Assignment
	"=", // TK_EQUAL
	"+=", // TK_PLUS_EQUAL
	"-=", // TK_MINUS_EQUAL
	"*=", // TK_STAR_EQUAL
	"**=", // TK_STAR_STAR_EQUAL
	"/=", // TK_SLASH_EQUAL
	"%=", // TK_PERCENT_EQUAL
	"<<=", // TK_LESS_LESS_EQUAL
	">>=", // TK_GREATER_GREATER_EQUAL
	"&=", // TK_AMPERSAND_EQUAL
	"|=", // TK_PIPE_EQUAL
	"^=", // TK_CARET_EQUAL
	// Control flow
	"if", // TK_IF
	"elif", // TK_ELIF
	"else", // TK_ELSE
	"for", // TK_FOR
	"while", // TK_WHILE
	"break", // TK_BREAK
	"continue", // TK_CONTINUE
	"pass", // TK_PASS
	"return", // TK_RETURN
	"match", // TK_MATCH
	"when", // TK_WHEN
	// Keywords
	"as", // TK_AS
	"assert", // TK_ASSERT
	"await", // TK_AWAIT
	"breakpoint", // TK_BREAKPOINT
	"class", // TK_CLASS
	"class_name", // TK_CLASS_NAME
	"const", // TK_CONST
	"enum", // TK_ENUM
	"extends", // TK_EXTENDS
	"func", // TK_FUNC
	"in", // TK_IN
	"is", // TK_IS
	"namespace", // TK_NAMESPACE
	"preload", // TK_PRELOAD
	"self", // TK_SELF
	"signal", // TK_SIGNAL
	"static", // TK_STATIC
	"super", // TK_SUPER
	"trait", // TK_TRAIT
	"var", // TK_VAR
	"void", // TK_VOID
	"yield", // TK_YIELD
	// Punctuation
	"[", // TK_BRACKET_OPEN
	"]", // TK_BRACKET_CLOSE
	"{", // TK_BRACE_OPEN
	"}", // TK_BRACE_CLOSE
	"(", // TK_PARENTHESIS_OPEN
	")", // TK_PARENTHESIS_CLOSE
	",", // TK_COMMA
	";", // TK_SEMICOLON
	".", // TK_PERIOD
	"..", // TK_PERIOD_PERIOD
	"...", // TK_PERIOD_PERIOD_PERIOD
	":", // TK_COLON
	"$", // TK_DOLLAR
	"->", // TK_FORWARD_ARROW
	"_", // TK_UNDERSCORE
	// Whitespace
	"Newline", // TK_NEWLINE
	"Indent", // TK_INDENT
	"Dedent", // TK_DEDENT
	// Constants
	"PI", // TK_CONST_PI
	"TAU", // TK_CONST_TAU
	"INF", // TK_CONST_INF
	"NaN", // TK_CONST_NAN
	// Error hints
	"VCS conflict marker", // TK_VCS_CONFLICT_MARKER
	"`", // TK_BACKTICK
	"?", // TK_QUESTION_MARK
};

static_assert(std_size(token_names) == static_cast<size_t>(GDScript2TokenType::TK_MAX),
		"Token names array size mismatch with enum.");

const char *gdscript2_get_token_name(GDScript2TokenType p_type) {
	ERR_FAIL_INDEX_V_MSG(static_cast<int>(p_type), static_cast<int>(GDScript2TokenType::TK_MAX),
			"<error>", "Token type out of range.");
	return token_names[static_cast<int>(p_type)];
}

const char *GDScript2Token::get_name() const {
	return gdscript2_get_token_name(type);
}

String GDScript2Token::get_debug_name() const {
	switch (type) {
		case GDScript2TokenType::TK_IDENTIFIER:
			return vformat(R"(identifier "%s")", source);
		case GDScript2TokenType::TK_LITERAL:
			return vformat(R"(literal %s)", literal);
		case GDScript2TokenType::TK_ERROR:
			return vformat(R"(error: %s)", literal);
		default:
			return vformat(R"("%s")", get_name());
	}
}

bool GDScript2Token::is_identifier() const {
	switch (type) {
		case GDScript2TokenType::TK_IDENTIFIER:
		case GDScript2TokenType::TK_MATCH: // Used in String.match()
		case GDScript2TokenType::TK_WHEN: // New keyword
		case GDScript2TokenType::TK_CONST_PI:
		case GDScript2TokenType::TK_CONST_INF:
		case GDScript2TokenType::TK_CONST_NAN:
		case GDScript2TokenType::TK_CONST_TAU:
			return true;
		default:
			return false;
	}
}

bool GDScript2Token::is_literal() const {
	return type == GDScript2TokenType::TK_LITERAL;
}

bool GDScript2Token::can_precede_bin_op() const {
	switch (type) {
		case GDScript2TokenType::TK_IDENTIFIER:
		case GDScript2TokenType::TK_LITERAL:
		case GDScript2TokenType::TK_SELF:
		case GDScript2TokenType::TK_BRACKET_CLOSE:
		case GDScript2TokenType::TK_BRACE_CLOSE:
		case GDScript2TokenType::TK_PARENTHESIS_CLOSE:
		case GDScript2TokenType::TK_CONST_PI:
		case GDScript2TokenType::TK_CONST_TAU:
		case GDScript2TokenType::TK_CONST_INF:
		case GDScript2TokenType::TK_CONST_NAN:
			return true;
		default:
			return false;
	}
}

// ============================================================================
// GDScript2Tokenizer Implementation
// ============================================================================

GDScript2Tokenizer::GDScript2Tokenizer() {
#ifdef TOOLS_ENABLED
	if (EditorSettings::get_singleton()) {
		tab_size = EditorSettings::get_singleton()->get_setting("text_editor/behavior/indent/size");
	}
#endif
#ifdef DEBUG_ENABLED
	make_keyword_list();
#endif
}

void GDScript2Tokenizer::set_source_code(const String &p_source_code) {
	source = p_source_code;
	_source = source.get_data();
	_current = _source;
	_start = _source;
	line = 1;
	column = 1;
	length = p_source_code.length();
	position = 0;

	// Reset state
	pending_indents = 0;
	indent_stack.clear();
	indent_stack_stack.clear();
	paren_stack.clear();
	error_stack.clear();
	continuation_lines.clear();
	line_continuation = false;
	multiline_mode = false;
	pending_newline = false;
	indent_char = '\0';
	last_token = GDScript2Token();
	last_newline = GDScript2Token();

#ifdef TOOLS_ENABLED
	comments.clear();
#endif
}

void GDScript2Tokenizer::set_cursor_position(int p_line, int p_column) {
	cursor_line = p_line;
	cursor_column = p_column;
}

bool GDScript2Tokenizer::is_past_cursor() const {
	if (line < cursor_line) {
		return false;
	}
	if (line > cursor_line) {
		return true;
	}
	return column >= cursor_column;
}

void GDScript2Tokenizer::push_expression_indented_block() {
	indent_stack_stack.push_back(indent_stack);
}

void GDScript2Tokenizer::pop_expression_indented_block() {
	ERR_FAIL_COND(indent_stack_stack.is_empty());
	indent_stack = indent_stack_stack.back()->get();
	indent_stack_stack.pop_back();
}

char32_t GDScript2Tokenizer::_advance() {
	if (unlikely(_is_at_end())) {
		return '\0';
	}
	_current++;
	column++;
	position++;
	if (unlikely(_is_at_end())) {
		newline(true);
		check_indent();
	}
	return _peek(-1);
}

void GDScript2Tokenizer::push_paren(char32_t p_char) {
	paren_stack.push_back(p_char);
}

bool GDScript2Tokenizer::pop_paren(char32_t p_expected) {
	if (paren_stack.is_empty()) {
		return false;
	}
	char32_t actual = paren_stack.back()->get();
	paren_stack.pop_back();
	return actual == p_expected;
}

GDScript2Token GDScript2Tokenizer::pop_error() {
	GDScript2Token error = error_stack.back()->get();
	error_stack.pop_back();
	return error;
}

GDScript2Token GDScript2Tokenizer::make_token(GDScript2TokenType p_type) {
	GDScript2Token token(p_type);
	token.start_line = start_line;
	token.end_line = line;
	token.start_column = start_column;
	token.end_column = column;
	token.source = String::utf32(Span(_start, _current - _start));

	// Handle cursor position tracking
	if (p_type != GDScript2TokenType::TK_ERROR && cursor_line > -1) {
		int offset = 0;
		while (_peek(offset) == ' ' || _peek(offset) == '\t') {
			offset++;
		}
		int last_column = column + offset;

		if (start_line == line) {
			// Single line token
			if (cursor_line == start_line && cursor_column >= start_column && cursor_column <= last_column) {
				token.cursor_position = cursor_column - start_column;
				if (cursor_column == start_column) {
					token.cursor_place = GDScript2CursorPlace::CURSOR_BEGINNING;
				} else if (cursor_column < column) {
					token.cursor_place = GDScript2CursorPlace::CURSOR_MIDDLE;
				} else {
					token.cursor_place = GDScript2CursorPlace::CURSOR_END;
				}
			}
		} else {
			// Multi-line token
			if (cursor_line == start_line && cursor_column >= start_column) {
				token.cursor_position = cursor_column - start_column;
				token.cursor_place = (cursor_column == start_column)
						? GDScript2CursorPlace::CURSOR_BEGINNING
						: GDScript2CursorPlace::CURSOR_MIDDLE;
			} else if (cursor_line == line && cursor_column <= last_column) {
				token.cursor_position = cursor_column - start_column;
				token.cursor_place = (cursor_column < column)
						? GDScript2CursorPlace::CURSOR_MIDDLE
						: GDScript2CursorPlace::CURSOR_END;
			} else if (cursor_line > start_line && cursor_line < line) {
				token.cursor_place = GDScript2CursorPlace::CURSOR_MIDDLE;
			}
		}
	}

	last_token = token;
	return token;
}

GDScript2Token GDScript2Tokenizer::make_literal(const Variant &p_literal) {
	GDScript2Token token = make_token(GDScript2TokenType::TK_LITERAL);
	token.literal = p_literal;
	return token;
}

GDScript2Token GDScript2Tokenizer::make_identifier(const StringName &p_identifier) {
	GDScript2Token token = make_token(GDScript2TokenType::TK_IDENTIFIER);
	token.literal = p_identifier;
	return token;
}

GDScript2Token GDScript2Tokenizer::make_error(const String &p_message) {
	GDScript2Token error = make_token(GDScript2TokenType::TK_ERROR);
	error.literal = p_message;
	return error;
}

void GDScript2Tokenizer::push_error(const String &p_message) {
	GDScript2Token error = make_error(p_message);
	error_stack.push_back(error);
}

void GDScript2Tokenizer::push_error(const GDScript2Token &p_error) {
	error_stack.push_back(p_error);
}

GDScript2Token GDScript2Tokenizer::make_paren_error(char32_t p_paren) {
	if (paren_stack.is_empty()) {
		return make_error(vformat("Closing \"%c\" doesn't have an opening counterpart.", p_paren));
	}
	GDScript2Token error = make_error(vformat("Closing \"%c\" doesn't match the opening \"%c\".",
			p_paren, paren_stack.back()->get()));
	paren_stack.pop_back();
	return error;
}

GDScript2Token GDScript2Tokenizer::check_vcs_marker(char32_t p_test, GDScript2TokenType p_double_type) {
	const char32_t *next = _current + 1;
	int chars = 2;

	while (*next == p_test) {
		chars++;
		next++;
	}

	if (chars >= 7) {
		// VCS conflict marker
		while (chars > 1) {
			_advance();
			chars--;
		}
		return make_token(GDScript2TokenType::TK_VCS_CONFLICT_MARKER);
	} else {
		_advance();
		return make_token(p_double_type);
	}
}

void GDScript2Tokenizer::newline(bool p_make_token) {
	if (p_make_token && !pending_newline && !line_continuation) {
		GDScript2Token nl(GDScript2TokenType::TK_NEWLINE);
		nl.start_line = line;
		nl.end_line = line;
		nl.start_column = column - 1;
		nl.end_column = column;
		pending_newline = true;
		last_token = nl;
		last_newline = nl;
	}
	line++;
	column = 1;
}

String GDScript2Tokenizer::_get_indent_char_name(char32_t ch) const {
	ERR_FAIL_COND_V(ch != ' ' && ch != '\t', String::chr(ch).c_escape());
	return ch == ' ' ? "space" : "tab";
}

void GDScript2Tokenizer::check_indent() {
	ERR_FAIL_COND_MSG(column != 1, "Checking indentation in the middle of a line.");

	if (_is_at_end()) {
		pending_indents -= indent_level();
		indent_stack.clear();
		return;
	}

	for (;;) {
		char32_t current_indent_char = _peek();
		int indent_count = 0;

		if (current_indent_char != ' ' && current_indent_char != '\t' &&
				current_indent_char != '\r' && current_indent_char != '\n' &&
				current_indent_char != '#') {
			if (line_continuation || multiline_mode) {
				return;
			}
			pending_indents -= indent_level();
			indent_stack.clear();
			return;
		}

		if (_peek() == '\r') {
			_advance();
			if (_peek() != '\n') {
				push_error("Stray carriage return character in source code.");
			}
		}
		if (_peek() == '\n') {
			_advance();
			newline(false);
			continue;
		}

		// Count indentation
		bool mixed = false;
		while (!_is_at_end()) {
			char32_t space = _peek();
			if (space == '\t') {
				column += tab_size - 1;
				indent_count += tab_size;
			} else if (space == ' ') {
				indent_count += 1;
			} else {
				break;
			}
			mixed = mixed || space != current_indent_char;
			_advance();
		}

		if (_is_at_end()) {
			pending_indents -= indent_level();
			indent_stack.clear();
			return;
		}

		if (_peek() == '\r') {
			_advance();
			if (_peek() != '\n') {
				push_error("Stray carriage return character in source code.");
			}
		}
		if (_peek() == '\n') {
			_advance();
			newline(false);
			continue;
		}

		// Handle comments
		if (_peek() == '#') {
#ifdef TOOLS_ENABLED
			String comment;
			while (_peek() != '\n' && !_is_at_end()) {
				comment += _advance();
			}
			comments[line] = CommentData(comment, true);
#else
			while (_peek() != '\n' && !_is_at_end()) {
				_advance();
			}
#endif
			if (_is_at_end()) {
				pending_indents -= indent_level();
				indent_stack.clear();
				return;
			}
			_advance();
			newline(false);
			continue;
		}

		if (mixed && !line_continuation && !multiline_mode) {
			GDScript2Token error = make_error("Mixed use of tabs and spaces for indentation.");
			error.start_line = line;
			error.start_column = 1;
			push_error(error);
		}

		if (line_continuation || multiline_mode) {
			return;
		}

		// Check indent character consistency
		if (indent_char == '\0') {
			indent_char = current_indent_char;
		} else if (current_indent_char != indent_char) {
			GDScript2Token error = make_error(vformat(
					"Used %s character for indentation instead of %s as used before in the file.",
					_get_indent_char_name(current_indent_char), _get_indent_char_name(indent_char)));
			error.start_line = line;
			error.start_column = 1;
			push_error(error);
		}

		// Calculate indent/dedent
		int previous_indent = 0;
		if (indent_level() > 0) {
			previous_indent = indent_stack.back()->get();
		}

		if (indent_count == previous_indent) {
			return;
		}

		if (indent_count > previous_indent) {
			indent_stack.push_back(indent_count);
			pending_indents++;
		} else {
			if (indent_level() == 0) {
				push_error("Tokenizer bug: trying to dedent without previous indent.");
				return;
			}
			while (indent_level() > 0 && indent_stack.back()->get() > indent_count) {
				indent_stack.pop_back();
				pending_indents--;
			}
			if ((indent_level() > 0 && indent_stack.back()->get() != indent_count) ||
					(indent_level() == 0 && indent_count != 0)) {
				GDScript2Token error = make_error("Unindent doesn't match the previous indentation level.");
				error.start_line = line;
				error.start_column = 1;
				error.end_column = column + 1;
				push_error(error);
				indent_stack.push_back(indent_count);
			}
		}
		break;
	}
}

void GDScript2Tokenizer::_skip_whitespace() {
	if (pending_indents != 0) {
		return;
	}

	bool is_bol = column == 1;

	if (is_bol) {
		check_indent();
		return;
	}

	for (;;) {
		char32_t c = _peek();
		switch (c) {
			case ' ':
				_advance();
				break;
			case '\t':
				_advance();
				column += tab_size - 1;
				break;
			case '\r':
				_advance();
				if (_peek() != '\n') {
					push_error("Stray carriage return character in source code.");
					return;
				}
				break;
			case '\n':
				_advance();
				newline(!is_bol);
				check_indent();
				break;
			case '#': {
#ifdef TOOLS_ENABLED
				String comment;
				while (_peek() != '\n' && !_is_at_end()) {
					comment += _advance();
				}
				comments[line] = CommentData(comment, is_bol);
#else
				while (_peek() != '\n' && !_is_at_end()) {
					_advance();
				}
#endif
				if (_is_at_end()) {
					return;
				}
				_advance();
				newline(!is_bol);
				check_indent();
			} break;
			default:
				return;
		}
	}
}

// Keyword matching macros
#define KEYWORDS(KEYWORD_GROUP, KEYWORD)                     \
	KEYWORD_GROUP('a')                                       \
	KEYWORD("as", GDScript2TokenType::TK_AS)                 \
	KEYWORD("and", GDScript2TokenType::TK_AND)               \
	KEYWORD("assert", GDScript2TokenType::TK_ASSERT)         \
	KEYWORD("await", GDScript2TokenType::TK_AWAIT)           \
	KEYWORD_GROUP('b')                                       \
	KEYWORD("break", GDScript2TokenType::TK_BREAK)           \
	KEYWORD("breakpoint", GDScript2TokenType::TK_BREAKPOINT) \
	KEYWORD_GROUP('c')                                       \
	KEYWORD("class", GDScript2TokenType::TK_CLASS)           \
	KEYWORD("class_name", GDScript2TokenType::TK_CLASS_NAME) \
	KEYWORD("const", GDScript2TokenType::TK_CONST)           \
	KEYWORD("continue", GDScript2TokenType::TK_CONTINUE)     \
	KEYWORD_GROUP('e')                                       \
	KEYWORD("elif", GDScript2TokenType::TK_ELIF)             \
	KEYWORD("else", GDScript2TokenType::TK_ELSE)             \
	KEYWORD("enum", GDScript2TokenType::TK_ENUM)             \
	KEYWORD("extends", GDScript2TokenType::TK_EXTENDS)       \
	KEYWORD_GROUP('f')                                       \
	KEYWORD("for", GDScript2TokenType::TK_FOR)               \
	KEYWORD("func", GDScript2TokenType::TK_FUNC)             \
	KEYWORD_GROUP('i')                                       \
	KEYWORD("if", GDScript2TokenType::TK_IF)                 \
	KEYWORD("in", GDScript2TokenType::TK_IN)                 \
	KEYWORD("is", GDScript2TokenType::TK_IS)                 \
	KEYWORD_GROUP('m')                                       \
	KEYWORD("match", GDScript2TokenType::TK_MATCH)           \
	KEYWORD_GROUP('n')                                       \
	KEYWORD("namespace", GDScript2TokenType::TK_NAMESPACE)   \
	KEYWORD("not", GDScript2TokenType::TK_NOT)               \
	KEYWORD_GROUP('o')                                       \
	KEYWORD("or", GDScript2TokenType::TK_OR)                 \
	KEYWORD_GROUP('p')                                       \
	KEYWORD("pass", GDScript2TokenType::TK_PASS)             \
	KEYWORD("preload", GDScript2TokenType::TK_PRELOAD)       \
	KEYWORD_GROUP('r')                                       \
	KEYWORD("return", GDScript2TokenType::TK_RETURN)         \
	KEYWORD_GROUP('s')                                       \
	KEYWORD("self", GDScript2TokenType::TK_SELF)             \
	KEYWORD("signal", GDScript2TokenType::TK_SIGNAL)         \
	KEYWORD("static", GDScript2TokenType::TK_STATIC)         \
	KEYWORD("super", GDScript2TokenType::TK_SUPER)           \
	KEYWORD_GROUP('t')                                       \
	KEYWORD("trait", GDScript2TokenType::TK_TRAIT)           \
	KEYWORD_GROUP('v')                                       \
	KEYWORD("var", GDScript2TokenType::TK_VAR)               \
	KEYWORD("void", GDScript2TokenType::TK_VOID)             \
	KEYWORD_GROUP('w')                                       \
	KEYWORD("while", GDScript2TokenType::TK_WHILE)           \
	KEYWORD("when", GDScript2TokenType::TK_WHEN)             \
	KEYWORD_GROUP('y')                                       \
	KEYWORD("yield", GDScript2TokenType::TK_YIELD)           \
	KEYWORD_GROUP('I')                                       \
	KEYWORD("INF", GDScript2TokenType::TK_CONST_INF)         \
	KEYWORD_GROUP('N')                                       \
	KEYWORD("NAN", GDScript2TokenType::TK_CONST_NAN)         \
	KEYWORD_GROUP('P')                                       \
	KEYWORD("PI", GDScript2TokenType::TK_CONST_PI)           \
	KEYWORD_GROUP('T')                                       \
	KEYWORD("TAU", GDScript2TokenType::TK_CONST_TAU)

#define MIN_KEYWORD_LENGTH 2
#define MAX_KEYWORD_LENGTH 10

#ifdef DEBUG_ENABLED
void GDScript2Tokenizer::make_keyword_list() {
#define KEYWORD_LINE(keyword, token_type) keyword,
#define KEYWORD_GROUP_IGNORE(group)
	keyword_list = {
		KEYWORDS(KEYWORD_GROUP_IGNORE, KEYWORD_LINE)
	};
#undef KEYWORD_LINE
#undef KEYWORD_GROUP_IGNORE
}
#endif

GDScript2Token GDScript2Tokenizer::scan_identifier() {
	bool only_ascii = _peek(-1) < 128;

	while (is_unicode_identifier_continue(_peek())) {
		char32_t c = _advance();
		only_ascii = only_ascii && c < 128;
	}

	int len = _current - _start;

	if (len == 1 && _peek(-1) == '_') {
		GDScript2Token token = make_token(GDScript2TokenType::TK_UNDERSCORE);
		token.literal = "_";
		return token;
	}

	String name = String::utf32(Span(_start, len));

	if (len < MIN_KEYWORD_LENGTH || len > MAX_KEYWORD_LENGTH) {
		return make_identifier(name);
	}

	if (!only_ascii) {
#ifdef DEBUG_ENABLED
		// Check for confusable identifiers (if TextServer available)
		// This would require TextServer integration
#endif
		return make_identifier(name);
	}

	// Keyword matching
#define KEYWORD_GROUP_CASE(ch) \
	break;                     \
	case ch:
#define KEYWORD(keyword, token_type)                    \
	{                                                   \
		const int keyword_length = sizeof(keyword) - 1; \
		if (keyword_length == len && name == keyword) { \
			GDScript2Token kw = make_token(token_type); \
			kw.literal = name;                          \
			return kw;                                  \
		}                                               \
	}

	switch (_start[0]) {
		default:
			KEYWORDS(KEYWORD_GROUP_CASE, KEYWORD)
			break;
	}

#undef KEYWORD_GROUP_CASE
#undef KEYWORD

	// Check special literals
	if (len == 4) {
		if (name == "true") {
			return make_literal(true);
		} else if (name == "null") {
			return make_literal(Variant());
		}
	} else if (len == 5) {
		if (name == "false") {
			return make_literal(false);
		}
	}

	return make_identifier(name);
}

#undef MAX_KEYWORD_LENGTH
#undef MIN_KEYWORD_LENGTH
#undef KEYWORDS

GDScript2Token GDScript2Tokenizer::scan_number() {
	int base = 10;
	bool has_decimal = false;
	bool has_exponent = false;
	bool has_error = false;
	bool need_digits = false;
	bool (*digit_check_func)(char32_t) = is_digit;

	if ((_peek(-1) == '+' || _peek(-1) == '-') && _peek() == '0') {
		_advance();
	}

	if (_peek(-1) == '.') {
		has_decimal = true;
	} else if (_peek(-1) == '0') {
		if (_peek() == 'x' || _peek() == 'X') {
			base = 16;
			digit_check_func = is_hex_digit;
			need_digits = true;
			_advance();
		} else if (_peek() == 'b' || _peek() == 'B') {
			base = 2;
			digit_check_func = is_binary_digit;
			need_digits = true;
			_advance();
		}
	}

	if (base != 10 && is_underscore(_peek())) {
		GDScript2Token error = make_error(vformat(R"(Unexpected underscore after "0%c".)", _peek(-1)));
		error.start_column = column;
		error.end_column = column + 1;
		push_error(error);
		has_error = true;
	}

	bool previous_was_underscore = false;
	while (digit_check_func(_peek()) || is_underscore(_peek())) {
		if (is_underscore(_peek())) {
			if (previous_was_underscore) {
				GDScript2Token error = make_error(R"(Multiple underscores cannot be adjacent in a numeric literal.)");
				error.start_column = column;
				error.end_column = column + 1;
				push_error(error);
			}
			previous_was_underscore = true;
		} else {
			need_digits = false;
			previous_was_underscore = false;
		}
		_advance();
	}

	// Handle decimal point
	if (_peek() == '.' && _peek(1) != '.') {
		if (base == 10 && !has_decimal) {
			has_decimal = true;
		} else if (base == 10) {
			GDScript2Token error = make_error("Cannot use a decimal point twice in a number.");
			error.start_column = column;
			error.end_column = column + 1;
			push_error(error);
			has_error = true;
		} else if (base == 16) {
			GDScript2Token error = make_error("Cannot use a decimal point in a hexadecimal number.");
			error.start_column = column;
			error.end_column = column + 1;
			push_error(error);
			has_error = true;
		} else {
			GDScript2Token error = make_error("Cannot use a decimal point in a binary number.");
			error.start_column = column;
			error.end_column = column + 1;
			push_error(error);
			has_error = true;
		}

		if (!has_error) {
			_advance();

			if (is_underscore(_peek())) {
				GDScript2Token error = make_error(R"(Unexpected underscore after decimal point.)");
				error.start_column = column;
				error.end_column = column + 1;
				push_error(error);
				has_error = true;
			}

			previous_was_underscore = false;
			while (is_digit(_peek()) || is_underscore(_peek())) {
				if (is_underscore(_peek())) {
					if (previous_was_underscore) {
						GDScript2Token error = make_error(R"(Multiple underscores cannot be adjacent in a numeric literal.)");
						error.start_column = column;
						error.end_column = column + 1;
						push_error(error);
					}
					previous_was_underscore = true;
				} else {
					previous_was_underscore = false;
				}
				_advance();
			}
		}
	}

	// Handle exponent
	if (base == 10 && (_peek() == 'e' || _peek() == 'E')) {
		has_exponent = true;
		_advance();
		if (_peek() == '+' || _peek() == '-') {
			_advance();
		}
		if (!is_digit(_peek())) {
			GDScript2Token error = make_error(R"(Expected exponent value after "e".)");
			error.start_column = column;
			error.end_column = column + 1;
			push_error(error);
		}
		previous_was_underscore = false;
		while (is_digit(_peek()) || is_underscore(_peek())) {
			if (is_underscore(_peek())) {
				if (previous_was_underscore) {
					GDScript2Token error = make_error(R"(Multiple underscores cannot be adjacent in a numeric literal.)");
					error.start_column = column;
					error.end_column = column + 1;
					push_error(error);
				}
				previous_was_underscore = true;
			} else {
				previous_was_underscore = false;
			}
			_advance();
		}
	}

	if (need_digits) {
		GDScript2Token error = make_error(vformat(R"(Expected %s digit after "0%c".)",
				(base == 16 ? "hexadecimal" : "binary"), (base == 16 ? 'x' : 'b')));
		error.start_column = column;
		error.end_column = column + 1;
		return error;
	}

	if (!has_error && has_decimal && _peek() == '.' && _peek(1) != '.') {
		GDScript2Token error = make_error("Cannot use a decimal point twice in a number.");
		error.start_column = column;
		error.end_column = column + 1;
		push_error(error);
	} else if (is_unicode_identifier_start(_peek()) || is_unicode_identifier_continue(_peek())) {
		push_error("Invalid numeric notation.");
	}

	int len = _current - _start;
	String number = String::utf32(Span(_start, len)).remove_char('_');

	if (base == 16) {
		int64_t value = number.hex_to_int();
		return make_literal(value);
	} else if (base == 2) {
		int64_t value = number.bin_to_int();
		return make_literal(value);
	} else if (has_decimal || has_exponent) {
		double value = number.to_float();
		return make_literal(value);
	} else {
		int64_t value = number.to_int();
		return make_literal(value);
	}
}

GDScript2Token GDScript2Tokenizer::scan_string() {
	enum StringType {
		STRING_REGULAR,
		STRING_NAME,
		STRING_NODEPATH,
	};

	bool is_raw = false;
	bool is_multiline = false;
	StringType type = STRING_REGULAR;

	if (_peek(-1) == 'r') {
		is_raw = true;
		_advance();
	} else if (_peek(-1) == '&') {
		type = STRING_NAME;
		_advance();
	} else if (_peek(-1) == '^') {
		type = STRING_NODEPATH;
		_advance();
	}

	char32_t quote_char = _peek(-1);

	if (_peek() == quote_char && _peek(1) == quote_char) {
		is_multiline = true;
		_advance();
		_advance();
	}

	String result;
	char32_t prev = 0;
	int prev_pos = 0;

	for (;;) {
		if (_is_at_end()) {
			return make_error("Unterminated string.");
		}

		char32_t ch = _peek();

		// Check for invisible direction control characters
		if (ch == 0x200E || ch == 0x200F || (ch >= 0x202A && ch <= 0x202E) || (ch >= 0x2066 && ch <= 0x2069)) {
			GDScript2Token error;
			if (is_raw) {
				error = make_error("Invisible text direction control character present in the string, use regular string literal instead of r-string.");
			} else {
				error = make_error("Invisible text direction control character present in the string, escape it (\"\\u" + String::num_int64(ch, 16) + "\") to avoid confusion.");
			}
			error.start_column = column;
			error.end_column = column + 1;
			push_error(error);
		}

		if (ch == '\\') {
			_advance();
			if (_is_at_end()) {
				return make_error("Unterminated string.");
			}

			if (is_raw) {
				if (_peek() == quote_char) {
					_advance();
					if (_is_at_end()) {
						return make_error("Unterminated string.");
					}
					result += '\\';
					result += quote_char;
				} else if (_peek() == '\\') {
					_advance();
					if (_is_at_end()) {
						return make_error("Unterminated string.");
					}
					result += '\\';
					result += '\\';
				} else {
					result += '\\';
				}
			} else {
				char32_t code = _peek();
				_advance();
				if (_is_at_end()) {
					return make_error("Unterminated string.");
				}

				char32_t escaped = 0;
				bool valid_escape = true;

				switch (code) {
					case 'a':
						escaped = '\a';
						break;
					case 'b':
						escaped = '\b';
						break;
					case 'f':
						escaped = '\f';
						break;
					case 'n':
						escaped = '\n';
						break;
					case 'r':
						escaped = '\r';
						break;
					case 't':
						escaped = '\t';
						break;
					case 'v':
						escaped = '\v';
						break;
					case '\'':
						escaped = '\'';
						break;
					case '\"':
						escaped = '\"';
						break;
					case '\\':
						escaped = '\\';
						break;
					case 'U':
					case 'u': {
						int hex_len = (code == 'U') ? 6 : 4;
						for (int j = 0; j < hex_len; j++) {
							if (_is_at_end()) {
								return make_error("Unterminated string.");
							}
							char32_t digit = _peek();
							char32_t value = 0;
							if (is_digit(digit)) {
								value = digit - '0';
							} else if (digit >= 'a' && digit <= 'f') {
								value = digit - 'a' + 10;
							} else if (digit >= 'A' && digit <= 'F') {
								value = digit - 'A' + 10;
							} else {
								GDScript2Token error = make_error("Invalid hexadecimal digit in unicode escape sequence.");
								error.start_column = column;
								error.end_column = column + 1;
								push_error(error);
								valid_escape = false;
								break;
							}
							escaped <<= 4;
							escaped |= value;
							_advance();
						}
					} break;
					case '\r':
						if (_peek() != '\n') {
							result += ch;
							_advance();
							break;
						}
						[[fallthrough]];
					case '\n':
						newline(false);
						valid_escape = false;
						break;
					default:
						GDScript2Token error = make_error("Invalid escape in string.");
						error.start_column = column - 2;
						push_error(error);
						valid_escape = false;
						break;
				}

				// Handle UTF-16 surrogate pairs
				if (valid_escape) {
					if ((escaped & 0xfffffc00) == 0xd800) {
						if (prev == 0) {
							prev = escaped;
							prev_pos = column - 2;
							continue;
						} else {
							GDScript2Token error = make_error("Invalid UTF-16 sequence in string, unpaired lead surrogate.");
							error.start_column = column - 2;
							push_error(error);
							valid_escape = false;
							prev = 0;
						}
					} else if ((escaped & 0xfffffc00) == 0xdc00) {
						if (prev == 0) {
							GDScript2Token error = make_error("Invalid UTF-16 sequence in string, unpaired trail surrogate.");
							error.start_column = column - 2;
							push_error(error);
							valid_escape = false;
						} else {
							escaped = (prev << 10UL) + escaped - ((0xd800 << 10UL) + 0xdc00 - 0x10000);
							prev = 0;
						}
					}
					if (prev != 0) {
						GDScript2Token error = make_error("Invalid UTF-16 sequence in string, unpaired lead surrogate.");
						error.start_column = prev_pos;
						push_error(error);
						prev = 0;
					}
				}

				if (valid_escape) {
					result += escaped;
				}
			}
		} else if (ch == quote_char) {
			if (prev != 0) {
				GDScript2Token error = make_error("Invalid UTF-16 sequence in string, unpaired lead surrogate");
				error.start_column = prev_pos;
				push_error(error);
				prev = 0;
			}
			_advance();
			if (is_multiline) {
				if (_peek() == quote_char && _peek(1) == quote_char) {
					_advance();
					_advance();
					break;
				} else {
					result += quote_char;
				}
			} else {
				break;
			}
		} else {
			if (prev != 0) {
				GDScript2Token error = make_error("Invalid UTF-16 sequence in string, unpaired lead surrogate");
				error.start_column = prev_pos;
				push_error(error);
				prev = 0;
			}
			result += ch;
			_advance();
			if (ch == '\n') {
				newline(false);
			}
		}
	}

	if (prev != 0) {
		GDScript2Token error = make_error("Invalid UTF-16 sequence in string, unpaired lead surrogate");
		error.start_column = prev_pos;
		push_error(error);
	}

	Variant string_value;
	switch (type) {
		case STRING_NAME:
			string_value = StringName(result);
			break;
		case STRING_NODEPATH:
			string_value = NodePath(result);
			break;
		case STRING_REGULAR:
			string_value = result;
			break;
	}

	return make_literal(string_value);
}

GDScript2Token GDScript2Tokenizer::scan_annotation() {
	if (is_unicode_identifier_start(_peek())) {
		_advance();
	} else {
		push_error("Expected annotation identifier after \"@\".");
	}

	while (is_unicode_identifier_continue(_peek())) {
		_advance();
	}

	GDScript2Token annotation = make_token(GDScript2TokenType::TK_ANNOTATION);
	annotation.literal = StringName(annotation.source);
	return annotation;
}

GDScript2Token GDScript2Tokenizer::scan() {
	if (has_error()) {
		return pop_error();
	}

	_skip_whitespace();

	if (pending_newline) {
		pending_newline = false;
		if (!multiline_mode) {
			return last_newline;
		}
	}

	if (has_error()) {
		return pop_error();
	}

	_start = _current;
	start_line = line;
	start_column = column;

	// Handle pending indents/dedents
	if (pending_indents != 0) {
		_start -= start_column - 1;
		start_column = 1;
		if (pending_indents > 0) {
			pending_indents--;
			return make_token(GDScript2TokenType::TK_INDENT);
		} else {
			pending_indents++;
			GDScript2Token dedent = make_token(GDScript2TokenType::TK_DEDENT);
			dedent.end_column += 1;
			return dedent;
		}
	}

	if (_is_at_end()) {
		return make_token(GDScript2TokenType::TK_EOF);
	}

	const char32_t c = _advance();

	// Line continuation
	if (c == '\\') {
		if (_peek() == '\r') {
			if (_peek(1) != '\n') {
				return make_error("Unexpected carriage return character.");
			}
			_advance();
		}
		if (_peek() != '\n') {
			return make_error("Expected new line after \"\\\".");
		}
		_advance();
		newline(false);
		line_continuation = true;
		_skip_whitespace();
		continuation_lines.push_back(line);
		return scan();
	}

	line_continuation = false;

	// Numbers
	if (is_digit(c)) {
		return scan_number();
	} else if (c == 'r' && (_peek() == '"' || _peek() == '\'')) {
		return scan_string();
	} else if (is_unicode_identifier_start(c)) {
		return scan_identifier();
	}

	switch (c) {
		// Strings
		case '"':
		case '\'':
			return scan_string();

		// Annotation
		case '@':
			return scan_annotation();

		// Single characters
		case '~':
			return make_token(GDScript2TokenType::TK_TILDE);
		case ',':
			return make_token(GDScript2TokenType::TK_COMMA);
		case ':':
			return make_token(GDScript2TokenType::TK_COLON);
		case ';':
			return make_token(GDScript2TokenType::TK_SEMICOLON);
		case '$':
			return make_token(GDScript2TokenType::TK_DOLLAR);
		case '?':
			return make_token(GDScript2TokenType::TK_QUESTION_MARK);
		case '`':
			return make_token(GDScript2TokenType::TK_BACKTICK);

		// Parentheses
		case '(':
			push_paren('(');
			return make_token(GDScript2TokenType::TK_PARENTHESIS_OPEN);
		case '[':
			push_paren('[');
			return make_token(GDScript2TokenType::TK_BRACKET_OPEN);
		case '{':
			push_paren('{');
			return make_token(GDScript2TokenType::TK_BRACE_OPEN);
		case ')':
			if (!pop_paren('(')) {
				return make_paren_error(c);
			}
			return make_token(GDScript2TokenType::TK_PARENTHESIS_CLOSE);
		case ']':
			if (!pop_paren('[')) {
				return make_paren_error(c);
			}
			return make_token(GDScript2TokenType::TK_BRACKET_CLOSE);
		case '}':
			if (!pop_paren('{')) {
				return make_paren_error(c);
			}
			return make_token(GDScript2TokenType::TK_BRACE_CLOSE);

		// Double characters
		case '!':
			if (_peek() == '=') {
				_advance();
				return make_token(GDScript2TokenType::TK_BANG_EQUAL);
			}
			return make_token(GDScript2TokenType::TK_BANG);

		case '.':
			if (_peek() == '.') {
				_advance();
				if (_peek() == '.') {
					_advance();
					return make_token(GDScript2TokenType::TK_PERIOD_PERIOD_PERIOD);
				}
				return make_token(GDScript2TokenType::TK_PERIOD_PERIOD);
			} else if (is_digit(_peek())) {
				return scan_number();
			}
			return make_token(GDScript2TokenType::TK_PERIOD);

		case '+':
			if (_peek() == '=') {
				_advance();
				return make_token(GDScript2TokenType::TK_PLUS_EQUAL);
			} else if (is_digit(_peek()) && !last_token.can_precede_bin_op()) {
				return scan_number();
			}
			return make_token(GDScript2TokenType::TK_PLUS);

		case '-':
			if (_peek() == '=') {
				_advance();
				return make_token(GDScript2TokenType::TK_MINUS_EQUAL);
			} else if (is_digit(_peek()) && !last_token.can_precede_bin_op()) {
				return scan_number();
			} else if (_peek() == '>') {
				_advance();
				return make_token(GDScript2TokenType::TK_FORWARD_ARROW);
			}
			return make_token(GDScript2TokenType::TK_MINUS);

		case '*':
			if (_peek() == '=') {
				_advance();
				return make_token(GDScript2TokenType::TK_STAR_EQUAL);
			} else if (_peek() == '*') {
				if (_peek(1) == '=') {
					_advance();
					_advance();
					return make_token(GDScript2TokenType::TK_STAR_STAR_EQUAL);
				}
				_advance();
				return make_token(GDScript2TokenType::TK_STAR_STAR);
			}
			return make_token(GDScript2TokenType::TK_STAR);

		case '/':
			if (_peek() == '=') {
				_advance();
				return make_token(GDScript2TokenType::TK_SLASH_EQUAL);
			}
			return make_token(GDScript2TokenType::TK_SLASH);

		case '%':
			if (_peek() == '=') {
				_advance();
				return make_token(GDScript2TokenType::TK_PERCENT_EQUAL);
			}
			return make_token(GDScript2TokenType::TK_PERCENT);

		case '^':
			if (_peek() == '=') {
				_advance();
				return make_token(GDScript2TokenType::TK_CARET_EQUAL);
			} else if (_peek() == '"' || _peek() == '\'') {
				return scan_string();
			}
			return make_token(GDScript2TokenType::TK_CARET);

		case '&':
			if (_peek() == '&') {
				_advance();
				return make_token(GDScript2TokenType::TK_AMPERSAND_AMPERSAND);
			} else if (_peek() == '=') {
				_advance();
				return make_token(GDScript2TokenType::TK_AMPERSAND_EQUAL);
			} else if (_peek() == '"' || _peek() == '\'') {
				return scan_string();
			}
			return make_token(GDScript2TokenType::TK_AMPERSAND);

		case '|':
			if (_peek() == '|') {
				_advance();
				return make_token(GDScript2TokenType::TK_PIPE_PIPE);
			} else if (_peek() == '=') {
				_advance();
				return make_token(GDScript2TokenType::TK_PIPE_EQUAL);
			}
			return make_token(GDScript2TokenType::TK_PIPE);

		// Potential VCS markers
		case '=':
			if (_peek() == '=') {
				return check_vcs_marker('=', GDScript2TokenType::TK_EQUAL_EQUAL);
			}
			return make_token(GDScript2TokenType::TK_EQUAL);

		case '<':
			if (_peek() == '=') {
				_advance();
				return make_token(GDScript2TokenType::TK_LESS_EQUAL);
			} else if (_peek() == '<') {
				if (_peek(1) == '=') {
					_advance();
					_advance();
					return make_token(GDScript2TokenType::TK_LESS_LESS_EQUAL);
				}
				return check_vcs_marker('<', GDScript2TokenType::TK_LESS_LESS);
			}
			return make_token(GDScript2TokenType::TK_LESS);

		case '>':
			if (_peek() == '=') {
				_advance();
				return make_token(GDScript2TokenType::TK_GREATER_EQUAL);
			} else if (_peek() == '>') {
				if (_peek(1) == '=') {
					_advance();
					_advance();
					return make_token(GDScript2TokenType::TK_GREATER_GREATER_EQUAL);
				}
				return check_vcs_marker('>', GDScript2TokenType::TK_GREATER_GREATER);
			}
			return make_token(GDScript2TokenType::TK_GREATER);

		default:
			if (is_whitespace(c)) {
				return make_error(vformat(R"(Invalid white space character U+%04X.)", static_cast<int32_t>(c)));
			}
			return make_error(vformat(R"(Invalid character "%c" (U+%04X).)", c, static_cast<int32_t>(c)));
	}
}
