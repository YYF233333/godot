/**************************************************************************/
/*  gdscript2_type.cpp                                                    */
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

#include "gdscript2_type.h"

#include "core/object/class_db.h"

// ============================================================================
// GDScript2FunctionSignature
// ============================================================================

GDScript2FunctionSignature::~GDScript2FunctionSignature() {
	for (GDScript2Type *t : parameter_types) {
		if (t) {
			memdelete(t);
		}
	}
	if (return_type) {
		memdelete(return_type);
	}
}

// ============================================================================
// GDScript2Type - Constructors/Destructor
// ============================================================================

GDScript2Type::GDScript2Type(const GDScript2Type &p_other) {
	kind = p_other.kind;
	class_name = p_other.class_name;
	script_path = p_other.script_path;
	is_constant = p_other.is_constant;
	is_read_only = p_other.is_read_only;
	is_hard_type = p_other.is_hard_type;

	if (p_other.element_type) {
		element_type = memnew(GDScript2Type(*p_other.element_type));
	}
	if (p_other.key_type) {
		key_type = memnew(GDScript2Type(*p_other.key_type));
	}
	if (p_other.value_type) {
		value_type = memnew(GDScript2Type(*p_other.value_type));
	}
	if (p_other.meta_type) {
		meta_type = memnew(GDScript2Type(*p_other.meta_type));
	}
	if (p_other.func_signature) {
		func_signature = memnew(GDScript2FunctionSignature);
		for (GDScript2Type *t : p_other.func_signature->parameter_types) {
			func_signature->parameter_types.push_back(t ? memnew(GDScript2Type(*t)) : nullptr);
		}
		func_signature->parameter_names = p_other.func_signature->parameter_names;
		func_signature->parameter_has_default = p_other.func_signature->parameter_has_default;
		if (p_other.func_signature->return_type) {
			func_signature->return_type = memnew(GDScript2Type(*p_other.func_signature->return_type));
		}
		func_signature->default_arg_count = p_other.func_signature->default_arg_count;
		func_signature->is_vararg = p_other.func_signature->is_vararg;
		func_signature->is_static = p_other.func_signature->is_static;
		func_signature->is_coroutine = p_other.func_signature->is_coroutine;
	}
}

GDScript2Type &GDScript2Type::operator=(const GDScript2Type &p_other) {
	if (this == &p_other) {
		return *this;
	}

	// Clean up existing data
	if (element_type) {
		memdelete(element_type);
		element_type = nullptr;
	}
	if (key_type) {
		memdelete(key_type);
		key_type = nullptr;
	}
	if (value_type) {
		memdelete(value_type);
		value_type = nullptr;
	}
	if (meta_type) {
		memdelete(meta_type);
		meta_type = nullptr;
	}
	if (func_signature) {
		memdelete(func_signature);
		func_signature = nullptr;
	}

	// Copy
	kind = p_other.kind;
	class_name = p_other.class_name;
	script_path = p_other.script_path;
	is_constant = p_other.is_constant;
	is_read_only = p_other.is_read_only;
	is_hard_type = p_other.is_hard_type;

	if (p_other.element_type) {
		element_type = memnew(GDScript2Type(*p_other.element_type));
	}
	if (p_other.key_type) {
		key_type = memnew(GDScript2Type(*p_other.key_type));
	}
	if (p_other.value_type) {
		value_type = memnew(GDScript2Type(*p_other.value_type));
	}
	if (p_other.meta_type) {
		meta_type = memnew(GDScript2Type(*p_other.meta_type));
	}
	if (p_other.func_signature) {
		func_signature = memnew(GDScript2FunctionSignature);
		for (GDScript2Type *t : p_other.func_signature->parameter_types) {
			func_signature->parameter_types.push_back(t ? memnew(GDScript2Type(*t)) : nullptr);
		}
		func_signature->parameter_names = p_other.func_signature->parameter_names;
		func_signature->parameter_has_default = p_other.func_signature->parameter_has_default;
		if (p_other.func_signature->return_type) {
			func_signature->return_type = memnew(GDScript2Type(*p_other.func_signature->return_type));
		}
		func_signature->default_arg_count = p_other.func_signature->default_arg_count;
		func_signature->is_vararg = p_other.func_signature->is_vararg;
		func_signature->is_static = p_other.func_signature->is_static;
		func_signature->is_coroutine = p_other.func_signature->is_coroutine;
	}

	return *this;
}

GDScript2Type::~GDScript2Type() {
	if (element_type) {
		memdelete(element_type);
	}
	if (key_type) {
		memdelete(key_type);
	}
	if (value_type) {
		memdelete(value_type);
	}
	if (meta_type) {
		memdelete(meta_type);
	}
	if (func_signature) {
		memdelete(func_signature);
	}
}

// ============================================================================
// GDScript2Type - Comparison
// ============================================================================

bool GDScript2Type::operator==(const GDScript2Type &p_other) const {
	if (kind != p_other.kind) {
		return false;
	}

	switch (kind) {
		case GDScript2TypeKind::TYPE_OBJECT:
		case GDScript2TypeKind::TYPE_SCRIPT_CLASS:
		case GDScript2TypeKind::TYPE_ENUM:
			if (class_name != p_other.class_name) {
				return false;
			}
			if (kind == GDScript2TypeKind::TYPE_SCRIPT_CLASS && script_path != p_other.script_path) {
				return false;
			}
			break;

		case GDScript2TypeKind::TYPE_ARRAY:
			if ((element_type == nullptr) != (p_other.element_type == nullptr)) {
				return false;
			}
			if (element_type && *element_type != *p_other.element_type) {
				return false;
			}
			break;

		case GDScript2TypeKind::TYPE_DICTIONARY:
			if ((key_type == nullptr) != (p_other.key_type == nullptr)) {
				return false;
			}
			if ((value_type == nullptr) != (p_other.value_type == nullptr)) {
				return false;
			}
			if (key_type && *key_type != *p_other.key_type) {
				return false;
			}
			if (value_type && *value_type != *p_other.value_type) {
				return false;
			}
			break;

		default:
			break;
	}

	return true;
}

// ============================================================================
// GDScript2Type - Type Checks
// ============================================================================

bool GDScript2Type::is_container() const {
	switch (kind) {
		case GDScript2TypeKind::TYPE_ARRAY:
		case GDScript2TypeKind::TYPE_DICTIONARY:
		case GDScript2TypeKind::TYPE_PACKED_BYTE_ARRAY:
		case GDScript2TypeKind::TYPE_PACKED_INT32_ARRAY:
		case GDScript2TypeKind::TYPE_PACKED_INT64_ARRAY:
		case GDScript2TypeKind::TYPE_PACKED_FLOAT32_ARRAY:
		case GDScript2TypeKind::TYPE_PACKED_FLOAT64_ARRAY:
		case GDScript2TypeKind::TYPE_PACKED_STRING_ARRAY:
		case GDScript2TypeKind::TYPE_PACKED_VECTOR2_ARRAY:
		case GDScript2TypeKind::TYPE_PACKED_VECTOR3_ARRAY:
		case GDScript2TypeKind::TYPE_PACKED_VECTOR4_ARRAY:
		case GDScript2TypeKind::TYPE_PACKED_COLOR_ARRAY:
			return true;
		default:
			return false;
	}
}

bool GDScript2Type::is_builtin() const {
	switch (kind) {
		case GDScript2TypeKind::TYPE_UNKNOWN:
		case GDScript2TypeKind::TYPE_VARIANT:
		case GDScript2TypeKind::TYPE_OBJECT:
		case GDScript2TypeKind::TYPE_SCRIPT_CLASS:
		case GDScript2TypeKind::TYPE_FUNC:
		case GDScript2TypeKind::TYPE_ENUM:
		case GDScript2TypeKind::TYPE_METATYPE:
			return false;
		default:
			return true;
	}
}

// ============================================================================
// GDScript2Type - String Representation
// ============================================================================

String GDScript2Type::to_string() const {
	switch (kind) {
		case GDScript2TypeKind::TYPE_UNKNOWN:
			return "<unknown>";
		case GDScript2TypeKind::TYPE_VARIANT:
			return "Variant";
		case GDScript2TypeKind::TYPE_NIL:
			return "null";
		case GDScript2TypeKind::TYPE_BOOL:
			return "bool";
		case GDScript2TypeKind::TYPE_INT:
			return "int";
		case GDScript2TypeKind::TYPE_FLOAT:
			return "float";
		case GDScript2TypeKind::TYPE_STRING:
			return "String";
		case GDScript2TypeKind::TYPE_STRING_NAME:
			return "StringName";
		case GDScript2TypeKind::TYPE_NODE_PATH:
			return "NodePath";
		case GDScript2TypeKind::TYPE_VECTOR2:
			return "Vector2";
		case GDScript2TypeKind::TYPE_VECTOR2I:
			return "Vector2i";
		case GDScript2TypeKind::TYPE_VECTOR3:
			return "Vector3";
		case GDScript2TypeKind::TYPE_VECTOR3I:
			return "Vector3i";
		case GDScript2TypeKind::TYPE_VECTOR4:
			return "Vector4";
		case GDScript2TypeKind::TYPE_VECTOR4I:
			return "Vector4i";
		case GDScript2TypeKind::TYPE_RECT2:
			return "Rect2";
		case GDScript2TypeKind::TYPE_RECT2I:
			return "Rect2i";
		case GDScript2TypeKind::TYPE_TRANSFORM2D:
			return "Transform2D";
		case GDScript2TypeKind::TYPE_PLANE:
			return "Plane";
		case GDScript2TypeKind::TYPE_QUATERNION:
			return "Quaternion";
		case GDScript2TypeKind::TYPE_AABB:
			return "AABB";
		case GDScript2TypeKind::TYPE_BASIS:
			return "Basis";
		case GDScript2TypeKind::TYPE_TRANSFORM3D:
			return "Transform3D";
		case GDScript2TypeKind::TYPE_PROJECTION:
			return "Projection";
		case GDScript2TypeKind::TYPE_COLOR:
			return "Color";
		case GDScript2TypeKind::TYPE_RID:
			return "RID";
		case GDScript2TypeKind::TYPE_CALLABLE:
			return "Callable";
		case GDScript2TypeKind::TYPE_SIGNAL:
			return "Signal";
		case GDScript2TypeKind::TYPE_ARRAY:
			if (element_type) {
				return "Array[" + element_type->to_string() + "]";
			}
			return "Array";
		case GDScript2TypeKind::TYPE_DICTIONARY:
			if (key_type && value_type) {
				return "Dictionary[" + key_type->to_string() + ", " + value_type->to_string() + "]";
			}
			return "Dictionary";
		case GDScript2TypeKind::TYPE_PACKED_BYTE_ARRAY:
			return "PackedByteArray";
		case GDScript2TypeKind::TYPE_PACKED_INT32_ARRAY:
			return "PackedInt32Array";
		case GDScript2TypeKind::TYPE_PACKED_INT64_ARRAY:
			return "PackedInt64Array";
		case GDScript2TypeKind::TYPE_PACKED_FLOAT32_ARRAY:
			return "PackedFloat32Array";
		case GDScript2TypeKind::TYPE_PACKED_FLOAT64_ARRAY:
			return "PackedFloat64Array";
		case GDScript2TypeKind::TYPE_PACKED_STRING_ARRAY:
			return "PackedStringArray";
		case GDScript2TypeKind::TYPE_PACKED_VECTOR2_ARRAY:
			return "PackedVector2Array";
		case GDScript2TypeKind::TYPE_PACKED_VECTOR3_ARRAY:
			return "PackedVector3Array";
		case GDScript2TypeKind::TYPE_PACKED_VECTOR4_ARRAY:
			return "PackedVector4Array";
		case GDScript2TypeKind::TYPE_PACKED_COLOR_ARRAY:
			return "PackedColorArray";
		case GDScript2TypeKind::TYPE_OBJECT:
			if (class_name != StringName()) {
				return String(class_name);
			}
			return "Object";
		case GDScript2TypeKind::TYPE_SCRIPT_CLASS:
			if (class_name != StringName()) {
				return String(class_name);
			}
			if (!script_path.is_empty()) {
				return script_path.get_file();
			}
			return "<script class>";
		case GDScript2TypeKind::TYPE_FUNC:
			return "Callable"; // Functions are represented as Callable in GDScript
		case GDScript2TypeKind::TYPE_ENUM:
			if (class_name != StringName()) {
				return String(class_name);
			}
			return "<enum>";
		case GDScript2TypeKind::TYPE_METATYPE:
			if (meta_type) {
				return "typeof(" + meta_type->to_string() + ")";
			}
			return "<metatype>";
	}
	return "<invalid>";
}

// ============================================================================
// GDScript2Type - Factory Methods
// ============================================================================

GDScript2Type GDScript2Type::make_dictionary() {
	return GDScript2Type(GDScript2TypeKind::TYPE_DICTIONARY);
}

GDScript2Type GDScript2Type::make_array() {
	return GDScript2Type(GDScript2TypeKind::TYPE_ARRAY);
}

GDScript2Type GDScript2Type::make_object(const StringName &p_class) {
	GDScript2Type type(GDScript2TypeKind::TYPE_OBJECT);
	type.class_name = p_class;
	return type;
}

GDScript2Type GDScript2Type::make_script_class(const StringName &p_class_name, const String &p_script_path) {
	GDScript2Type type(GDScript2TypeKind::TYPE_SCRIPT_CLASS);
	type.class_name = p_class_name;
	type.script_path = p_script_path;
	return type;
}

GDScript2Type GDScript2Type::make_enum(const StringName &p_enum_name) {
	GDScript2Type type(GDScript2TypeKind::TYPE_ENUM);
	type.class_name = p_enum_name;
	return type;
}

GDScript2Type GDScript2Type::make_typed_array(GDScript2Type *p_element_type) {
	GDScript2Type type(GDScript2TypeKind::TYPE_ARRAY);
	type.element_type = p_element_type;
	return type;
}

GDScript2Type GDScript2Type::make_typed_dictionary(GDScript2Type *p_key_type, GDScript2Type *p_value_type) {
	GDScript2Type type(GDScript2TypeKind::TYPE_DICTIONARY);
	type.key_type = p_key_type;
	type.value_type = p_value_type;
	return type;
}

GDScript2Type GDScript2Type::make_func(GDScript2FunctionSignature *p_signature) {
	GDScript2Type type(GDScript2TypeKind::TYPE_FUNC);
	type.func_signature = p_signature;
	return type;
}

GDScript2Type GDScript2Type::make_metatype(GDScript2Type *p_type) {
	GDScript2Type type(GDScript2TypeKind::TYPE_METATYPE);
	type.meta_type = p_type;
	return type;
}

// ============================================================================
// GDScript2Type - Conversion from Variant
// ============================================================================

GDScript2Type GDScript2Type::from_variant_type(Variant::Type p_type) {
	switch (p_type) {
		case Variant::NIL:
			return make_nil();
		case Variant::BOOL:
			return make_bool();
		case Variant::INT:
			return make_int();
		case Variant::FLOAT:
			return make_float();
		case Variant::STRING:
			return make_string();
		case Variant::STRING_NAME:
			return make_string_name();
		case Variant::NODE_PATH:
			return make_node_path();
		case Variant::VECTOR2:
			return make_vector2();
		case Variant::VECTOR2I:
			return make_vector2i();
		case Variant::VECTOR3:
			return make_vector3();
		case Variant::VECTOR3I:
			return make_vector3i();
		case Variant::VECTOR4:
			return make_vector4();
		case Variant::VECTOR4I:
			return make_vector4i();
		case Variant::RECT2:
			return make_rect2();
		case Variant::RECT2I:
			return make_rect2i();
		case Variant::TRANSFORM2D:
			return make_transform2d();
		case Variant::PLANE:
			return make_plane();
		case Variant::QUATERNION:
			return make_quaternion();
		case Variant::AABB:
			return GDScript2Type(GDScript2TypeKind::TYPE_AABB);
		case Variant::BASIS:
			return make_basis();
		case Variant::TRANSFORM3D:
			return make_transform3d();
		case Variant::PROJECTION:
			return make_projection();
		case Variant::COLOR:
			return make_color();
		case Variant::RID:
			return make_rid();
		case Variant::CALLABLE:
			return make_callable();
		case Variant::SIGNAL:
			return make_signal();
		case Variant::ARRAY:
			return make_array();
		case Variant::DICTIONARY:
			return make_dictionary();
		case Variant::PACKED_BYTE_ARRAY:
			return GDScript2Type(GDScript2TypeKind::TYPE_PACKED_BYTE_ARRAY);
		case Variant::PACKED_INT32_ARRAY:
			return GDScript2Type(GDScript2TypeKind::TYPE_PACKED_INT32_ARRAY);
		case Variant::PACKED_INT64_ARRAY:
			return GDScript2Type(GDScript2TypeKind::TYPE_PACKED_INT64_ARRAY);
		case Variant::PACKED_FLOAT32_ARRAY:
			return GDScript2Type(GDScript2TypeKind::TYPE_PACKED_FLOAT32_ARRAY);
		case Variant::PACKED_FLOAT64_ARRAY:
			return GDScript2Type(GDScript2TypeKind::TYPE_PACKED_FLOAT64_ARRAY);
		case Variant::PACKED_STRING_ARRAY:
			return GDScript2Type(GDScript2TypeKind::TYPE_PACKED_STRING_ARRAY);
		case Variant::PACKED_VECTOR2_ARRAY:
			return GDScript2Type(GDScript2TypeKind::TYPE_PACKED_VECTOR2_ARRAY);
		case Variant::PACKED_VECTOR3_ARRAY:
			return GDScript2Type(GDScript2TypeKind::TYPE_PACKED_VECTOR3_ARRAY);
		case Variant::PACKED_VECTOR4_ARRAY:
			return GDScript2Type(GDScript2TypeKind::TYPE_PACKED_VECTOR4_ARRAY);
		case Variant::PACKED_COLOR_ARRAY:
			return GDScript2Type(GDScript2TypeKind::TYPE_PACKED_COLOR_ARRAY);
		case Variant::OBJECT:
			return make_object();
		default:
			return make_unknown();
	}
}

GDScript2Type GDScript2Type::from_variant(const Variant &p_value) {
	GDScript2Type type = from_variant_type(p_value.get_type());

	// For objects, try to get the class name
	if (p_value.get_type() == Variant::OBJECT) {
		Object *obj = p_value;
		if (obj) {
			type.class_name = obj->get_class_name();
		}
	}

	return type;
}

// ============================================================================
// GDScript2Type - Conversion from Type Name
// ============================================================================

GDScript2Type GDScript2Type::from_type_name(const StringName &p_name) {
	// Check built-in type names
	if (p_name == "Variant") {
		return make_variant();
	}
	if (p_name == "bool") {
		return make_bool();
	}
	if (p_name == "int") {
		return make_int();
	}
	if (p_name == "float") {
		return make_float();
	}
	if (p_name == "String") {
		return make_string();
	}
	if (p_name == "StringName") {
		return make_string_name();
	}
	if (p_name == "NodePath") {
		return make_node_path();
	}
	if (p_name == "Vector2") {
		return make_vector2();
	}
	if (p_name == "Vector2i") {
		return make_vector2i();
	}
	if (p_name == "Vector3") {
		return make_vector3();
	}
	if (p_name == "Vector3i") {
		return make_vector3i();
	}
	if (p_name == "Vector4") {
		return make_vector4();
	}
	if (p_name == "Vector4i") {
		return make_vector4i();
	}
	if (p_name == "Rect2") {
		return make_rect2();
	}
	if (p_name == "Rect2i") {
		return make_rect2i();
	}
	if (p_name == "Transform2D") {
		return make_transform2d();
	}
	if (p_name == "Plane") {
		return make_plane();
	}
	if (p_name == "Quaternion") {
		return make_quaternion();
	}
	if (p_name == "AABB") {
		return GDScript2Type(GDScript2TypeKind::TYPE_AABB);
	}
	if (p_name == "Basis") {
		return make_basis();
	}
	if (p_name == "Transform3D") {
		return make_transform3d();
	}
	if (p_name == "Projection") {
		return make_projection();
	}
	if (p_name == "Color") {
		return make_color();
	}
	if (p_name == "RID") {
		return make_rid();
	}
	if (p_name == "Callable") {
		return make_callable();
	}
	if (p_name == "Signal") {
		return make_signal();
	}
	if (p_name == "Array") {
		return make_array();
	}
	if (p_name == "Dictionary") {
		return make_dictionary();
	}
	if (p_name == "PackedByteArray") {
		return GDScript2Type(GDScript2TypeKind::TYPE_PACKED_BYTE_ARRAY);
	}
	if (p_name == "PackedInt32Array") {
		return GDScript2Type(GDScript2TypeKind::TYPE_PACKED_INT32_ARRAY);
	}
	if (p_name == "PackedInt64Array") {
		return GDScript2Type(GDScript2TypeKind::TYPE_PACKED_INT64_ARRAY);
	}
	if (p_name == "PackedFloat32Array") {
		return GDScript2Type(GDScript2TypeKind::TYPE_PACKED_FLOAT32_ARRAY);
	}
	if (p_name == "PackedFloat64Array") {
		return GDScript2Type(GDScript2TypeKind::TYPE_PACKED_FLOAT64_ARRAY);
	}
	if (p_name == "PackedStringArray") {
		return GDScript2Type(GDScript2TypeKind::TYPE_PACKED_STRING_ARRAY);
	}
	if (p_name == "PackedVector2Array") {
		return GDScript2Type(GDScript2TypeKind::TYPE_PACKED_VECTOR2_ARRAY);
	}
	if (p_name == "PackedVector3Array") {
		return GDScript2Type(GDScript2TypeKind::TYPE_PACKED_VECTOR3_ARRAY);
	}
	if (p_name == "PackedVector4Array") {
		return GDScript2Type(GDScript2TypeKind::TYPE_PACKED_VECTOR4_ARRAY);
	}
	if (p_name == "PackedColorArray") {
		return GDScript2Type(GDScript2TypeKind::TYPE_PACKED_COLOR_ARRAY);
	}
	if (p_name == "Object") {
		return make_object();
	}

	// Check if it's a native class
	if (ClassDB::class_exists(p_name)) {
		return make_object(p_name);
	}

	// Unknown type - might be a script class, resolved later
	return make_unknown();
}

// ============================================================================
// GDScript2Type - Convert to Variant Type
// ============================================================================

Variant::Type GDScript2Type::to_variant_type() const {
	switch (kind) {
		case GDScript2TypeKind::TYPE_NIL:
			return Variant::NIL;
		case GDScript2TypeKind::TYPE_BOOL:
			return Variant::BOOL;
		case GDScript2TypeKind::TYPE_INT:
			return Variant::INT;
		case GDScript2TypeKind::TYPE_FLOAT:
			return Variant::FLOAT;
		case GDScript2TypeKind::TYPE_STRING:
			return Variant::STRING;
		case GDScript2TypeKind::TYPE_STRING_NAME:
			return Variant::STRING_NAME;
		case GDScript2TypeKind::TYPE_NODE_PATH:
			return Variant::NODE_PATH;
		case GDScript2TypeKind::TYPE_VECTOR2:
			return Variant::VECTOR2;
		case GDScript2TypeKind::TYPE_VECTOR2I:
			return Variant::VECTOR2I;
		case GDScript2TypeKind::TYPE_VECTOR3:
			return Variant::VECTOR3;
		case GDScript2TypeKind::TYPE_VECTOR3I:
			return Variant::VECTOR3I;
		case GDScript2TypeKind::TYPE_VECTOR4:
			return Variant::VECTOR4;
		case GDScript2TypeKind::TYPE_VECTOR4I:
			return Variant::VECTOR4I;
		case GDScript2TypeKind::TYPE_RECT2:
			return Variant::RECT2;
		case GDScript2TypeKind::TYPE_RECT2I:
			return Variant::RECT2I;
		case GDScript2TypeKind::TYPE_TRANSFORM2D:
			return Variant::TRANSFORM2D;
		case GDScript2TypeKind::TYPE_PLANE:
			return Variant::PLANE;
		case GDScript2TypeKind::TYPE_QUATERNION:
			return Variant::QUATERNION;
		case GDScript2TypeKind::TYPE_AABB:
			return Variant::AABB;
		case GDScript2TypeKind::TYPE_BASIS:
			return Variant::BASIS;
		case GDScript2TypeKind::TYPE_TRANSFORM3D:
			return Variant::TRANSFORM3D;
		case GDScript2TypeKind::TYPE_PROJECTION:
			return Variant::PROJECTION;
		case GDScript2TypeKind::TYPE_COLOR:
			return Variant::COLOR;
		case GDScript2TypeKind::TYPE_RID:
			return Variant::RID;
		case GDScript2TypeKind::TYPE_CALLABLE:
		case GDScript2TypeKind::TYPE_FUNC:
			return Variant::CALLABLE;
		case GDScript2TypeKind::TYPE_SIGNAL:
			return Variant::SIGNAL;
		case GDScript2TypeKind::TYPE_ARRAY:
			return Variant::ARRAY;
		case GDScript2TypeKind::TYPE_DICTIONARY:
			return Variant::DICTIONARY;
		case GDScript2TypeKind::TYPE_PACKED_BYTE_ARRAY:
			return Variant::PACKED_BYTE_ARRAY;
		case GDScript2TypeKind::TYPE_PACKED_INT32_ARRAY:
			return Variant::PACKED_INT32_ARRAY;
		case GDScript2TypeKind::TYPE_PACKED_INT64_ARRAY:
			return Variant::PACKED_INT64_ARRAY;
		case GDScript2TypeKind::TYPE_PACKED_FLOAT32_ARRAY:
			return Variant::PACKED_FLOAT32_ARRAY;
		case GDScript2TypeKind::TYPE_PACKED_FLOAT64_ARRAY:
			return Variant::PACKED_FLOAT64_ARRAY;
		case GDScript2TypeKind::TYPE_PACKED_STRING_ARRAY:
			return Variant::PACKED_STRING_ARRAY;
		case GDScript2TypeKind::TYPE_PACKED_VECTOR2_ARRAY:
			return Variant::PACKED_VECTOR2_ARRAY;
		case GDScript2TypeKind::TYPE_PACKED_VECTOR3_ARRAY:
			return Variant::PACKED_VECTOR3_ARRAY;
		case GDScript2TypeKind::TYPE_PACKED_VECTOR4_ARRAY:
			return Variant::PACKED_VECTOR4_ARRAY;
		case GDScript2TypeKind::TYPE_PACKED_COLOR_ARRAY:
			return Variant::PACKED_COLOR_ARRAY;
		case GDScript2TypeKind::TYPE_OBJECT:
		case GDScript2TypeKind::TYPE_SCRIPT_CLASS:
			return Variant::OBJECT;
		default:
			return Variant::NIL;
	}
}

// ============================================================================
// GDScript2TypeChecker
// ============================================================================

bool GDScript2TypeChecker::is_type_compatible(const GDScript2Type &p_target, const GDScript2Type &p_source, bool p_allow_implicit_conversion) {
	// Variant accepts anything
	if (p_target.is_variant()) {
		return true;
	}

	// Unknown target accepts anything (error recovery)
	if (!p_target.is_valid()) {
		return true;
	}

	// Unknown source is always compatible (error recovery)
	if (!p_source.is_valid()) {
		return true;
	}

	// Null can be assigned to any object type
	if (p_source.is_nil()) {
		return p_target.is_object() || p_target.kind == GDScript2TypeKind::TYPE_CALLABLE ||
				p_target.kind == GDScript2TypeKind::TYPE_SIGNAL;
	}

	// Same kind
	if (p_target.kind == p_source.kind) {
		switch (p_target.kind) {
			case GDScript2TypeKind::TYPE_OBJECT:
			case GDScript2TypeKind::TYPE_SCRIPT_CLASS:
				// Check class inheritance
				if (p_target.class_name == StringName() || p_source.class_name == StringName()) {
					return true; // Unspecified object type
				}
				return is_class_inherited(p_source.class_name, p_target.class_name);

			case GDScript2TypeKind::TYPE_ARRAY:
				// Check element type compatibility
				if (!p_target.element_type) {
					return true; // Untyped array accepts any array
				}
				if (!p_source.element_type) {
					return false; // Typed array doesn't accept untyped
				}
				return is_type_compatible(*p_target.element_type, *p_source.element_type, p_allow_implicit_conversion);

			case GDScript2TypeKind::TYPE_DICTIONARY:
				// Check key/value type compatibility
				if (!p_target.key_type && !p_target.value_type) {
					return true; // Untyped dict accepts any dict
				}
				if (!p_source.key_type || !p_source.value_type) {
					return false; // Typed dict doesn't accept untyped
				}
				return is_type_compatible(*p_target.key_type, *p_source.key_type, p_allow_implicit_conversion) &&
						is_type_compatible(*p_target.value_type, *p_source.value_type, p_allow_implicit_conversion);

			default:
				return true;
		}
	}

	// Implicit conversions
	if (p_allow_implicit_conversion) {
		// int -> float
		if (p_target.kind == GDScript2TypeKind::TYPE_FLOAT && p_source.kind == GDScript2TypeKind::TYPE_INT) {
			return true;
		}

		// StringName <-> String
		if ((p_target.kind == GDScript2TypeKind::TYPE_STRING && p_source.kind == GDScript2TypeKind::TYPE_STRING_NAME) ||
				(p_target.kind == GDScript2TypeKind::TYPE_STRING_NAME && p_source.kind == GDScript2TypeKind::TYPE_STRING)) {
			return true;
		}

		// NodePath <-> String
		if ((p_target.kind == GDScript2TypeKind::TYPE_NODE_PATH && p_source.kind == GDScript2TypeKind::TYPE_STRING) ||
				(p_target.kind == GDScript2TypeKind::TYPE_STRING && p_source.kind == GDScript2TypeKind::TYPE_NODE_PATH)) {
			return true;
		}
	}

	return false;
}

bool GDScript2TypeChecker::types_equal(const GDScript2Type &p_a, const GDScript2Type &p_b) {
	return p_a == p_b;
}

GDScript2Type GDScript2TypeChecker::get_common_type(const GDScript2Type &p_a, const GDScript2Type &p_b) {
	// If either is variant, result is variant
	if (p_a.is_variant() || p_b.is_variant()) {
		return GDScript2Type::make_variant();
	}

	// If either is unknown, use the other
	if (!p_a.is_valid()) {
		return p_b;
	}
	if (!p_b.is_valid()) {
		return p_a;
	}

	// Same type
	if (p_a == p_b) {
		return p_a;
	}

	// Numeric promotion: int + float = float
	if (p_a.is_numeric() && p_b.is_numeric()) {
		if (p_a.kind == GDScript2TypeKind::TYPE_FLOAT || p_b.kind == GDScript2TypeKind::TYPE_FLOAT) {
			return GDScript2Type::make_float();
		}
		return GDScript2Type::make_int();
	}

	// Object types: find common base class
	if (p_a.is_object() && p_b.is_object()) {
		// For now, return generic Object
		// TODO: Find actual common base class
		return GDScript2Type::make_object();
	}

	// Default to variant
	return GDScript2Type::make_variant();
}

bool GDScript2TypeChecker::is_class_inherited(const StringName &p_derived, const StringName &p_base) {
	if (p_derived == p_base) {
		return true;
	}

	// Check native class inheritance
	if (ClassDB::class_exists(p_derived) && ClassDB::class_exists(p_base)) {
		return ClassDB::is_parent_class(p_derived, p_base);
	}

	// TODO: Check script class inheritance
	return false;
}

bool GDScript2TypeChecker::can_be_boolean(const GDScript2Type &p_type) {
	// In GDScript, any value can be used in boolean context
	// But we can provide warnings for certain types
	return true;
}
