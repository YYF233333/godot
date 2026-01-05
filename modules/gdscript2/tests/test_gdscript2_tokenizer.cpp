/**************************************************************************/
/*  test_gdscript2_tokenizer.cpp                                          */
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

#include "tests/test_macros.h"

#include "modules/gdscript2/front/gdscript2_tokenizer.h"

namespace {

// Helper to collect all tokens from source
Vector<GDScript2Token> tokenize_all(const String &p_source) {
	GDScript2Tokenizer tokenizer;
	tokenizer.set_source_code(p_source);

	Vector<GDScript2Token> tokens;
	GDScript2Token token;
	do {
		token = tokenizer.scan();
		tokens.push_back(token);
	} while (token.type != GDScript2TokenType::TK_EOF && token.type != GDScript2TokenType::TK_ERROR);

	return tokens;
}

// Test empty source
void test_tokenizer_empty() {
	GDScript2Tokenizer tokenizer;
	tokenizer.set_source_code("");

	GDScript2Token token = tokenizer.scan();
	CHECK_MESSAGE(token.type == GDScript2TokenType::TK_EOF, "Empty source should return EOF token.");
}

// Test basic identifiers
void test_tokenizer_identifier() {
	Vector<GDScript2Token> tokens = tokenize_all("hello world");

	CHECK_MESSAGE(tokens.size() >= 3, "Should have at least 3 tokens (2 identifiers + EOF).");
	CHECK_MESSAGE(tokens[0].type == GDScript2TokenType::TK_IDENTIFIER, "First token should be identifier.");
	CHECK_MESSAGE(StringName(tokens[0].literal) == StringName("hello"), "First identifier should be 'hello'.");
	CHECK_MESSAGE(tokens[1].type == GDScript2TokenType::TK_IDENTIFIER, "Second token should be identifier.");
	CHECK_MESSAGE(StringName(tokens[1].literal) == StringName("world"), "Second identifier should be 'world'.");
}

// Test keywords
void test_tokenizer_keywords() {
	Vector<GDScript2Token> tokens = tokenize_all("func var if else while for return class");

	CHECK_MESSAGE(tokens[0].type == GDScript2TokenType::TK_FUNC, "Should recognize 'func' keyword.");
	CHECK_MESSAGE(tokens[1].type == GDScript2TokenType::TK_VAR, "Should recognize 'var' keyword.");
	CHECK_MESSAGE(tokens[2].type == GDScript2TokenType::TK_IF, "Should recognize 'if' keyword.");
	CHECK_MESSAGE(tokens[3].type == GDScript2TokenType::TK_ELSE, "Should recognize 'else' keyword.");
	CHECK_MESSAGE(tokens[4].type == GDScript2TokenType::TK_WHILE, "Should recognize 'while' keyword.");
	CHECK_MESSAGE(tokens[5].type == GDScript2TokenType::TK_FOR, "Should recognize 'for' keyword.");
	CHECK_MESSAGE(tokens[6].type == GDScript2TokenType::TK_RETURN, "Should recognize 'return' keyword.");
	CHECK_MESSAGE(tokens[7].type == GDScript2TokenType::TK_CLASS, "Should recognize 'class' keyword.");
}

// Test integer literals
void test_tokenizer_integers() {
	Vector<GDScript2Token> tokens = tokenize_all("42 0 123456");

	CHECK_MESSAGE(tokens[0].type == GDScript2TokenType::TK_LITERAL, "Should be literal.");
	CHECK_MESSAGE(int64_t(tokens[0].literal) == 42, "Should parse 42.");

	CHECK_MESSAGE(tokens[1].type == GDScript2TokenType::TK_LITERAL, "Should be literal.");
	CHECK_MESSAGE(int64_t(tokens[1].literal) == 0, "Should parse 0.");

	CHECK_MESSAGE(tokens[2].type == GDScript2TokenType::TK_LITERAL, "Should be literal.");
	CHECK_MESSAGE(int64_t(tokens[2].literal) == 123456, "Should parse 123456.");
}

// Test hex and binary literals
void test_tokenizer_hex_binary() {
	Vector<GDScript2Token> tokens = tokenize_all("0xFF 0b1010");

	CHECK_MESSAGE(tokens[0].type == GDScript2TokenType::TK_LITERAL, "Should be literal.");
	CHECK_MESSAGE(int64_t(tokens[0].literal) == 255, "Should parse 0xFF as 255.");

	CHECK_MESSAGE(tokens[1].type == GDScript2TokenType::TK_LITERAL, "Should be literal.");
	CHECK_MESSAGE(int64_t(tokens[1].literal) == 10, "Should parse 0b1010 as 10.");
}

// Test float literals
void test_tokenizer_floats() {
	Vector<GDScript2Token> tokens = tokenize_all("3.14 0.5 1e10 2.5e-3");

	CHECK_MESSAGE(tokens[0].type == GDScript2TokenType::TK_LITERAL, "Should be literal.");
	CHECK_MESSAGE(Math::is_equal_approx(double(tokens[0].literal), 3.14), "Should parse 3.14.");

	CHECK_MESSAGE(tokens[1].type == GDScript2TokenType::TK_LITERAL, "Should be literal.");
	CHECK_MESSAGE(Math::is_equal_approx(double(tokens[1].literal), 0.5), "Should parse 0.5.");

	CHECK_MESSAGE(tokens[2].type == GDScript2TokenType::TK_LITERAL, "Should be literal.");
	CHECK_MESSAGE(Math::is_equal_approx(double(tokens[2].literal), 1e10), "Should parse 1e10.");

	CHECK_MESSAGE(tokens[3].type == GDScript2TokenType::TK_LITERAL, "Should be literal.");
	CHECK_MESSAGE(Math::is_equal_approx(double(tokens[3].literal), 2.5e-3), "Should parse 2.5e-3.");
}

// Test string literals
void test_tokenizer_strings() {
	Vector<GDScript2Token> tokens = tokenize_all("\"hello\" 'world'");

	CHECK_MESSAGE(tokens[0].type == GDScript2TokenType::TK_LITERAL, "Should be literal.");
	CHECK_MESSAGE(String(tokens[0].literal) == "hello", "Should parse double-quoted string.");

	CHECK_MESSAGE(tokens[1].type == GDScript2TokenType::TK_LITERAL, "Should be literal.");
	CHECK_MESSAGE(String(tokens[1].literal) == "world", "Should parse single-quoted string.");
}

// Test operators
void test_tokenizer_operators() {
	Vector<GDScript2Token> tokens = tokenize_all("+ - * / % == != < > <= >= && || !");

	CHECK_MESSAGE(tokens[0].type == GDScript2TokenType::TK_PLUS, "Should recognize '+'.");
	CHECK_MESSAGE(tokens[1].type == GDScript2TokenType::TK_MINUS, "Should recognize '-'.");
	CHECK_MESSAGE(tokens[2].type == GDScript2TokenType::TK_STAR, "Should recognize '*'.");
	CHECK_MESSAGE(tokens[3].type == GDScript2TokenType::TK_SLASH, "Should recognize '/'.");
	CHECK_MESSAGE(tokens[4].type == GDScript2TokenType::TK_PERCENT, "Should recognize '%'.");
	CHECK_MESSAGE(tokens[5].type == GDScript2TokenType::TK_EQUAL_EQUAL, "Should recognize '=='.");
	CHECK_MESSAGE(tokens[6].type == GDScript2TokenType::TK_BANG_EQUAL, "Should recognize '!='.");
	CHECK_MESSAGE(tokens[7].type == GDScript2TokenType::TK_LESS, "Should recognize '<'.");
	CHECK_MESSAGE(tokens[8].type == GDScript2TokenType::TK_GREATER, "Should recognize '>'.");
	CHECK_MESSAGE(tokens[9].type == GDScript2TokenType::TK_LESS_EQUAL, "Should recognize '<='.");
	CHECK_MESSAGE(tokens[10].type == GDScript2TokenType::TK_GREATER_EQUAL, "Should recognize '>='.");
	CHECK_MESSAGE(tokens[11].type == GDScript2TokenType::TK_AMPERSAND_AMPERSAND, "Should recognize '&&'.");
	CHECK_MESSAGE(tokens[12].type == GDScript2TokenType::TK_PIPE_PIPE, "Should recognize '||'.");
	CHECK_MESSAGE(tokens[13].type == GDScript2TokenType::TK_BANG, "Should recognize '!'.");
}

// Test punctuation
void test_tokenizer_punctuation() {
	Vector<GDScript2Token> tokens = tokenize_all("( ) [ ] { } , : ; . -> @");

	CHECK_MESSAGE(tokens[0].type == GDScript2TokenType::TK_PARENTHESIS_OPEN, "Should recognize '('.");
	CHECK_MESSAGE(tokens[1].type == GDScript2TokenType::TK_PARENTHESIS_CLOSE, "Should recognize ')'.");
	CHECK_MESSAGE(tokens[2].type == GDScript2TokenType::TK_BRACKET_OPEN, "Should recognize '['.");
	CHECK_MESSAGE(tokens[3].type == GDScript2TokenType::TK_BRACKET_CLOSE, "Should recognize ']'.");
	CHECK_MESSAGE(tokens[4].type == GDScript2TokenType::TK_BRACE_OPEN, "Should recognize '{'.");
	CHECK_MESSAGE(tokens[5].type == GDScript2TokenType::TK_BRACE_CLOSE, "Should recognize '}'.");
	CHECK_MESSAGE(tokens[6].type == GDScript2TokenType::TK_COMMA, "Should recognize ','.");
	CHECK_MESSAGE(tokens[7].type == GDScript2TokenType::TK_COLON, "Should recognize ':'.");
	CHECK_MESSAGE(tokens[8].type == GDScript2TokenType::TK_SEMICOLON, "Should recognize ';'.");
	CHECK_MESSAGE(tokens[9].type == GDScript2TokenType::TK_PERIOD, "Should recognize '.'.");
	CHECK_MESSAGE(tokens[10].type == GDScript2TokenType::TK_FORWARD_ARROW, "Should recognize '->'.");
}

// Test indentation
void test_tokenizer_indentation() {
	String source = "if true:\n\tpass\nelse:\n\tpass";
	Vector<GDScript2Token> tokens = tokenize_all(source);

	bool has_indent = false;
	bool has_dedent = false;
	for (const GDScript2Token &t : tokens) {
		if (t.type == GDScript2TokenType::TK_INDENT) {
			has_indent = true;
		}
		if (t.type == GDScript2TokenType::TK_DEDENT) {
			has_dedent = true;
		}
	}

	CHECK_MESSAGE(has_indent, "Should have INDENT token.");
	CHECK_MESSAGE(has_dedent, "Should have DEDENT token.");
}

// Test newlines
void test_tokenizer_newlines() {
	String source = "a\nb\nc";
	Vector<GDScript2Token> tokens = tokenize_all(source);

	int newline_count = 0;
	for (const GDScript2Token &t : tokens) {
		if (t.type == GDScript2TokenType::TK_NEWLINE) {
			newline_count++;
		}
	}

	CHECK_MESSAGE(newline_count >= 2, "Should have at least 2 NEWLINE tokens.");
}

// Test comments (should be skipped)
void test_tokenizer_comments() {
	String source = "a # this is a comment\nb";
	Vector<GDScript2Token> tokens = tokenize_all(source);

	// Comments should be skipped, only identifiers and newlines
	bool has_comment_in_tokens = false;
	for (const GDScript2Token &t : tokens) {
		if (t.source.contains("#")) {
			has_comment_in_tokens = true;
		}
	}

	CHECK_MESSAGE(!has_comment_in_tokens, "Comments should be skipped in token stream.");
}

// Test boolean literals
void test_tokenizer_booleans() {
	Vector<GDScript2Token> tokens = tokenize_all("true false null");

	CHECK_MESSAGE(tokens[0].type == GDScript2TokenType::TK_LITERAL, "true should be literal.");
	CHECK_MESSAGE(bool(tokens[0].literal) == true, "Should parse 'true'.");

	CHECK_MESSAGE(tokens[1].type == GDScript2TokenType::TK_LITERAL, "false should be literal.");
	CHECK_MESSAGE(bool(tokens[1].literal) == false, "Should parse 'false'.");

	CHECK_MESSAGE(tokens[2].type == GDScript2TokenType::TK_LITERAL, "null should be literal.");
	CHECK_MESSAGE(tokens[2].literal.get_type() == Variant::NIL, "Should parse 'null' as NIL.");
}

// Test built-in constants
void test_tokenizer_constants() {
	Vector<GDScript2Token> tokens = tokenize_all("PI TAU INF NAN");

	CHECK_MESSAGE(tokens[0].type == GDScript2TokenType::TK_CONST_PI, "Should recognize PI.");
	CHECK_MESSAGE(tokens[1].type == GDScript2TokenType::TK_CONST_TAU, "Should recognize TAU.");
	CHECK_MESSAGE(tokens[2].type == GDScript2TokenType::TK_CONST_INF, "Should recognize INF.");
	CHECK_MESSAGE(tokens[3].type == GDScript2TokenType::TK_CONST_NAN, "Should recognize NAN.");
}

// Test assignment operators
void test_tokenizer_assignment_ops() {
	Vector<GDScript2Token> tokens = tokenize_all("= += -= *= /= %=");

	CHECK_MESSAGE(tokens[0].type == GDScript2TokenType::TK_EQUAL, "Should recognize '='.");
	CHECK_MESSAGE(tokens[1].type == GDScript2TokenType::TK_PLUS_EQUAL, "Should recognize '+='.");
	CHECK_MESSAGE(tokens[2].type == GDScript2TokenType::TK_MINUS_EQUAL, "Should recognize '-='.");
	CHECK_MESSAGE(tokens[3].type == GDScript2TokenType::TK_STAR_EQUAL, "Should recognize '*='.");
	CHECK_MESSAGE(tokens[4].type == GDScript2TokenType::TK_SLASH_EQUAL, "Should recognize '/='.");
	CHECK_MESSAGE(tokens[5].type == GDScript2TokenType::TK_PERCENT_EQUAL, "Should recognize '%='.");
}

// Test annotation
void test_tokenizer_annotation() {
	Vector<GDScript2Token> tokens = tokenize_all("@export @onready");

	CHECK_MESSAGE(tokens[0].type == GDScript2TokenType::TK_ANNOTATION, "Should recognize annotation.");
	CHECK_MESSAGE(StringName(tokens[0].literal) == StringName("@export"), "Should have correct annotation name.");
}

// Test underscore numbers
void test_tokenizer_underscore_numbers() {
	Vector<GDScript2Token> tokens = tokenize_all("1_000_000 0xFF_FF");

	CHECK_MESSAGE(tokens[0].type == GDScript2TokenType::TK_LITERAL, "Should be literal.");
	CHECK_MESSAGE(int64_t(tokens[0].literal) == 1000000, "Should parse 1_000_000 as 1000000.");

	CHECK_MESSAGE(tokens[1].type == GDScript2TokenType::TK_LITERAL, "Should be literal.");
	CHECK_MESSAGE(int64_t(tokens[1].literal) == 0xFFFF, "Should parse 0xFF_FF as 65535.");
}

// Test multiline string
void test_tokenizer_multiline_string() {
	String source = "\"\"\"hello\nworld\"\"\"";
	Vector<GDScript2Token> tokens = tokenize_all(source);

	CHECK_MESSAGE(tokens[0].type == GDScript2TokenType::TK_LITERAL, "Should be literal.");
	CHECK_MESSAGE(String(tokens[0].literal) == "hello\nworld", "Should parse multiline string.");
}

// Test string escapes
void test_tokenizer_string_escapes() {
	Vector<GDScript2Token> tokens = tokenize_all("\"hello\\nworld\\t!\"");

	CHECK_MESSAGE(tokens[0].type == GDScript2TokenType::TK_LITERAL, "Should be literal.");
	CHECK_MESSAGE(String(tokens[0].literal) == "hello\nworld\t!", "Should handle escape sequences.");
}

// Test line position tracking
void test_tokenizer_line_tracking() {
	String source = "a\nb\nc";
	GDScript2Tokenizer tokenizer;
	tokenizer.set_source_code(source);

	GDScript2Token t1 = tokenizer.scan(); // a
	CHECK_MESSAGE(t1.start_line == 1, "First token should be on line 1.");

	tokenizer.scan(); // newline
	GDScript2Token t2 = tokenizer.scan(); // b
	CHECK_MESSAGE(t2.start_line == 2, "Second identifier should be on line 2.");
}

// Test power operator
void test_tokenizer_power_operator() {
	Vector<GDScript2Token> tokens = tokenize_all("2 ** 3 **= 4");

	CHECK_MESSAGE(tokens[1].type == GDScript2TokenType::TK_STAR_STAR, "Should recognize '**'.");
	CHECK_MESSAGE(tokens[3].type == GDScript2TokenType::TK_STAR_STAR_EQUAL, "Should recognize '**='.");
}

} // namespace

// Register all tests
REGISTER_TEST_COMMAND("gdscript2-tokenizer-empty", &test_tokenizer_empty);
REGISTER_TEST_COMMAND("gdscript2-tokenizer-identifier", &test_tokenizer_identifier);
REGISTER_TEST_COMMAND("gdscript2-tokenizer-keywords", &test_tokenizer_keywords);
REGISTER_TEST_COMMAND("gdscript2-tokenizer-integers", &test_tokenizer_integers);
REGISTER_TEST_COMMAND("gdscript2-tokenizer-hex-binary", &test_tokenizer_hex_binary);
REGISTER_TEST_COMMAND("gdscript2-tokenizer-floats", &test_tokenizer_floats);
REGISTER_TEST_COMMAND("gdscript2-tokenizer-strings", &test_tokenizer_strings);
REGISTER_TEST_COMMAND("gdscript2-tokenizer-operators", &test_tokenizer_operators);
REGISTER_TEST_COMMAND("gdscript2-tokenizer-punctuation", &test_tokenizer_punctuation);
REGISTER_TEST_COMMAND("gdscript2-tokenizer-indentation", &test_tokenizer_indentation);
REGISTER_TEST_COMMAND("gdscript2-tokenizer-newlines", &test_tokenizer_newlines);
REGISTER_TEST_COMMAND("gdscript2-tokenizer-comments", &test_tokenizer_comments);
REGISTER_TEST_COMMAND("gdscript2-tokenizer-booleans", &test_tokenizer_booleans);
REGISTER_TEST_COMMAND("gdscript2-tokenizer-constants", &test_tokenizer_constants);
REGISTER_TEST_COMMAND("gdscript2-tokenizer-assignment-ops", &test_tokenizer_assignment_ops);
REGISTER_TEST_COMMAND("gdscript2-tokenizer-annotation", &test_tokenizer_annotation);
REGISTER_TEST_COMMAND("gdscript2-tokenizer-underscore-numbers", &test_tokenizer_underscore_numbers);
REGISTER_TEST_COMMAND("gdscript2-tokenizer-multiline-string", &test_tokenizer_multiline_string);
REGISTER_TEST_COMMAND("gdscript2-tokenizer-string-escapes", &test_tokenizer_string_escapes);
REGISTER_TEST_COMMAND("gdscript2-tokenizer-line-tracking", &test_tokenizer_line_tracking);
REGISTER_TEST_COMMAND("gdscript2-tokenizer-power-operator", &test_tokenizer_power_operator);
