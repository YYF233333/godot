/**************************************************************************/
/*  gdscript2_type.h                                                      */
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

#include "core/object/ref_counted.h"
#include "core/string/string_name.h"
#include "core/templates/local_vector.h"
#include "core/variant/variant.h"

// Type kind enumeration - covers all GDScript types
enum class GDScript2TypeKind {
	TYPE_UNKNOWN, // Unresolved or error type
	TYPE_VARIANT, // Dynamic type (any)
	TYPE_NIL, // Null/void
	TYPE_BOOL,
	TYPE_INT,
	TYPE_FLOAT,
	TYPE_STRING,
	TYPE_STRING_NAME,
	TYPE_NODE_PATH,

	// Math types
	TYPE_VECTOR2,
	TYPE_VECTOR2I,
	TYPE_VECTOR3,
	TYPE_VECTOR3I,
	TYPE_VECTOR4,
	TYPE_VECTOR4I,
	TYPE_RECT2,
	TYPE_RECT2I,
	TYPE_TRANSFORM2D,
	TYPE_PLANE,
	TYPE_QUATERNION,
	TYPE_AABB,
	TYPE_BASIS,
	TYPE_TRANSFORM3D,
	TYPE_PROJECTION,

	// Misc types
	TYPE_COLOR,
	TYPE_RID,
	TYPE_CALLABLE,
	TYPE_SIGNAL,

	// Container types
	TYPE_ARRAY,
	TYPE_DICTIONARY,
	TYPE_PACKED_BYTE_ARRAY,
	TYPE_PACKED_INT32_ARRAY,
	TYPE_PACKED_INT64_ARRAY,
	TYPE_PACKED_FLOAT32_ARRAY,
	TYPE_PACKED_FLOAT64_ARRAY,
	TYPE_PACKED_STRING_ARRAY,
	TYPE_PACKED_VECTOR2_ARRAY,
	TYPE_PACKED_VECTOR3_ARRAY,
	TYPE_PACKED_VECTOR4_ARRAY,
	TYPE_PACKED_COLOR_ARRAY,

	// Object/Class types
	TYPE_OBJECT, // Base object or specific class

	// Function type (for lambdas and function references)
	TYPE_FUNC,

	// Enum type
	TYPE_ENUM,

	// Script class type (user-defined)
	TYPE_SCRIPT_CLASS,

	// Meta type (type itself, like typeof)
	TYPE_METATYPE,
};

// Forward declaration
struct GDScript2Type;

// Function signature for callable types
struct GDScript2FunctionSignature {
	LocalVector<GDScript2Type *> parameter_types;
	LocalVector<StringName> parameter_names;
	LocalVector<bool> parameter_has_default;
	GDScript2Type *return_type = nullptr;
	int default_arg_count = 0;
	bool is_vararg = false;
	bool is_static = false;
	bool is_coroutine = false;

	~GDScript2FunctionSignature();
};

// Complete type representation
struct GDScript2Type {
	GDScript2TypeKind kind = GDScript2TypeKind::TYPE_UNKNOWN;

	// For TYPE_OBJECT: native class name (e.g., "Node", "Sprite2D")
	// For TYPE_SCRIPT_CLASS: script class name
	// For TYPE_ENUM: enum name
	StringName class_name;

	// For TYPE_SCRIPT_CLASS: script path
	String script_path;

	// For TYPE_ARRAY: element type (typed arrays)
	GDScript2Type *element_type = nullptr;

	// For TYPE_DICTIONARY: key and value types
	GDScript2Type *key_type = nullptr;
	GDScript2Type *value_type = nullptr;

	// For TYPE_FUNC: function signature
	GDScript2FunctionSignature *func_signature = nullptr;

	// For TYPE_METATYPE: the type this metatype represents
	GDScript2Type *meta_type = nullptr;

	// Is this type constant (cannot be reassigned)?
	bool is_constant = false;

	// Is this type read-only (property without setter)?
	bool is_read_only = false;

	// Is this type hard (explicitly annotated) vs inferred?
	bool is_hard_type = false;

	// Constructors
	GDScript2Type() = default;
	GDScript2Type(GDScript2TypeKind p_kind) :
			kind(p_kind) {}
	GDScript2Type(const GDScript2Type &p_other);
	GDScript2Type &operator=(const GDScript2Type &p_other);
	~GDScript2Type();

	// Comparison
	bool operator==(const GDScript2Type &p_other) const;
	bool operator!=(const GDScript2Type &p_other) const { return !(*this == p_other); }

	// Type checks
	bool is_valid() const { return kind != GDScript2TypeKind::TYPE_UNKNOWN; }
	bool is_variant() const { return kind == GDScript2TypeKind::TYPE_VARIANT; }
	bool is_nil() const { return kind == GDScript2TypeKind::TYPE_NIL; }
	bool is_numeric() const { return kind == GDScript2TypeKind::TYPE_INT || kind == GDScript2TypeKind::TYPE_FLOAT; }
	bool is_integral() const { return kind == GDScript2TypeKind::TYPE_INT || kind == GDScript2TypeKind::TYPE_BOOL; }
	bool is_object() const { return kind == GDScript2TypeKind::TYPE_OBJECT || kind == GDScript2TypeKind::TYPE_SCRIPT_CLASS; }
	bool is_container() const;
	bool is_builtin() const;

	// Type string representation
	String to_string() const;

	// Factory methods for common types
	static GDScript2Type make_unknown() { return GDScript2Type(GDScript2TypeKind::TYPE_UNKNOWN); }
	static GDScript2Type make_variant() { return GDScript2Type(GDScript2TypeKind::TYPE_VARIANT); }
	static GDScript2Type make_nil() { return GDScript2Type(GDScript2TypeKind::TYPE_NIL); }
	static GDScript2Type make_bool() { return GDScript2Type(GDScript2TypeKind::TYPE_BOOL); }
	static GDScript2Type make_int() { return GDScript2Type(GDScript2TypeKind::TYPE_INT); }
	static GDScript2Type make_float() { return GDScript2Type(GDScript2TypeKind::TYPE_FLOAT); }
	static GDScript2Type make_string() { return GDScript2Type(GDScript2TypeKind::TYPE_STRING); }
	static GDScript2Type make_string_name() { return GDScript2Type(GDScript2TypeKind::TYPE_STRING_NAME); }
	static GDScript2Type make_node_path() { return GDScript2Type(GDScript2TypeKind::TYPE_NODE_PATH); }
	static GDScript2Type make_vector2() { return GDScript2Type(GDScript2TypeKind::TYPE_VECTOR2); }
	static GDScript2Type make_vector2i() { return GDScript2Type(GDScript2TypeKind::TYPE_VECTOR2I); }
	static GDScript2Type make_vector3() { return GDScript2Type(GDScript2TypeKind::TYPE_VECTOR3); }
	static GDScript2Type make_vector3i() { return GDScript2Type(GDScript2TypeKind::TYPE_VECTOR3I); }
	static GDScript2Type make_vector4() { return GDScript2Type(GDScript2TypeKind::TYPE_VECTOR4); }
	static GDScript2Type make_vector4i() { return GDScript2Type(GDScript2TypeKind::TYPE_VECTOR4I); }
	static GDScript2Type make_rect2() { return GDScript2Type(GDScript2TypeKind::TYPE_RECT2); }
	static GDScript2Type make_rect2i() { return GDScript2Type(GDScript2TypeKind::TYPE_RECT2I); }
	static GDScript2Type make_transform2d() { return GDScript2Type(GDScript2TypeKind::TYPE_TRANSFORM2D); }
	static GDScript2Type make_plane() { return GDScript2Type(GDScript2TypeKind::TYPE_PLANE); }
	static GDScript2Type make_quaternion() { return GDScript2Type(GDScript2TypeKind::TYPE_QUATERNION); }
	static GDScript2Type make_aabb() { return GDScript2Type(GDScript2TypeKind::TYPE_AABB); }
	static GDScript2Type make_basis() { return GDScript2Type(GDScript2TypeKind::TYPE_BASIS); }
	static GDScript2Type make_transform3d() { return GDScript2Type(GDScript2TypeKind::TYPE_TRANSFORM3D); }
	static GDScript2Type make_projection() { return GDScript2Type(GDScript2TypeKind::TYPE_PROJECTION); }
	static GDScript2Type make_color() { return GDScript2Type(GDScript2TypeKind::TYPE_COLOR); }
	static GDScript2Type make_rid() { return GDScript2Type(GDScript2TypeKind::TYPE_RID); }
	static GDScript2Type make_callable() { return GDScript2Type(GDScript2TypeKind::TYPE_CALLABLE); }
	static GDScript2Type make_signal() { return GDScript2Type(GDScript2TypeKind::TYPE_SIGNAL); }
	static GDScript2Type make_dictionary();
	static GDScript2Type make_array();

	static GDScript2Type make_object(const StringName &p_class = StringName());
	static GDScript2Type make_script_class(const StringName &p_class_name, const String &p_script_path = String());
	static GDScript2Type make_enum(const StringName &p_enum_name);
	static GDScript2Type make_typed_array(GDScript2Type *p_element_type);
	static GDScript2Type make_typed_dictionary(GDScript2Type *p_key_type, GDScript2Type *p_value_type);
	static GDScript2Type make_func(GDScript2FunctionSignature *p_signature);
	static GDScript2Type make_metatype(GDScript2Type *p_type);

	// Convert from Variant type
	static GDScript2Type from_variant_type(Variant::Type p_type);
	static GDScript2Type from_variant(const Variant &p_value);

	// Convert from type name string
	static GDScript2Type from_type_name(const StringName &p_name);

	// Convert to Variant type (if applicable)
	Variant::Type to_variant_type() const;
};

// Type compatibility checking utilities
class GDScript2TypeChecker {
public:
	// Check if p_source can be assigned to p_target
	static bool is_type_compatible(const GDScript2Type &p_target, const GDScript2Type &p_source, bool p_allow_implicit_conversion = false);

	// Check if types are strictly equal
	static bool types_equal(const GDScript2Type &p_a, const GDScript2Type &p_b);

	// Get common type for two types (for type inference in expressions)
	static GDScript2Type get_common_type(const GDScript2Type &p_a, const GDScript2Type &p_b);

	// Check if a class inherits from another
	static bool is_class_inherited(const StringName &p_derived, const StringName &p_base);

	// Check if type can be used in boolean context
	static bool can_be_boolean(const GDScript2Type &p_type);
};
