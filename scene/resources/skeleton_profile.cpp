/**************************************************************************/
/*  skeleton_profile.cpp                                                  */
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

#include "skeleton_profile.h"

#include "core/config/engine.h"

bool SkeletonProfile::_set(const StringName &p_path, const Variant &p_value) {
	ERR_FAIL_COND_V(is_read_only, false);
	String path = p_path;

	if (path.begins_with("groups/")) {
		int which = path.get_slicec('/', 1).to_int();
		String what = path.get_slicec('/', 2);
		ERR_FAIL_INDEX_V(which, groups.size(), false);

		if (what == "group_name") {
			set_group_name(which, p_value);
			return true;
		} else if (what == "texture") {
			set_texture(which, p_value);
			return true;
		}
	}

	if (path.begins_with("bones/")) {
		int which = path.get_slicec('/', 1).to_int();
		String what = path.get_slicec('/', 2);
		ERR_FAIL_INDEX_V(which, bones.size(), false);

		if (what == "bone_name") {
			set_bone_name(which, p_value);
			return true;
		} else if (what == "bone_parent") {
			set_bone_parent(which, p_value);
			return true;
		} else if (what == "tail_direction") {
			set_tail_direction(which, static_cast<TailDirection>((int)p_value));
			return true;
		} else if (what == "bone_tail") {
			set_bone_tail(which, p_value);
			return true;
		} else if (what == "reference_pose") {
			set_reference_pose(which, p_value);
			return true;
		} else if (what == "handle_offset") {
			set_handle_offset(which, p_value);
			return true;
		} else if (what == "group") {
			set_group(which, p_value);
			return true;
		} else if (what == "require") {
			set_required(which, p_value);
			return true;
		}
	}
	return false;
}

bool SkeletonProfile::_get(const StringName &p_path, Variant &r_ret) const {
	String path = p_path;

	if (path.begins_with("groups/")) {
		int which = path.get_slicec('/', 1).to_int();
		String what = path.get_slicec('/', 2);
		ERR_FAIL_INDEX_V(which, groups.size(), false);

		if (what == "group_name") {
			r_ret = get_group_name(which);
			return true;
		} else if (what == "texture") {
			r_ret = get_texture(which);
			return true;
		}
	} else if (path.begins_with("bones/")) {
		int which = path.get_slicec('/', 1).to_int();
		String what = path.get_slicec('/', 2);
		ERR_FAIL_INDEX_V(which, bones.size(), false);

		if (what == "bone_name") {
			r_ret = get_bone_name(which);
			return true;
		} else if (what == "bone_parent") {
			r_ret = get_bone_parent(which);
			return true;
		} else if (what == "tail_direction") {
			r_ret = get_tail_direction(which);
			return true;
		} else if (what == "bone_tail") {
			r_ret = get_bone_tail(which);
			return true;
		} else if (what == "reference_pose") {
			r_ret = get_reference_pose(which);
			return true;
		} else if (what == "handle_offset") {
			r_ret = get_handle_offset(which);
			return true;
		} else if (what == "group") {
			r_ret = get_group(which);
			return true;
		} else if (what == "require") {
			r_ret = is_required(which);
			return true;
		}
	}

	return false;
}

void SkeletonProfile::_validate_property(PropertyInfo &p_property) const {
	if (!Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	if (is_read_only) {
		if (p_property.name == ("group_size") || p_property.name == ("bone_size") || p_property.name == ("root_bone") || p_property.name == ("scale_base_bone")) {
			p_property.usage = PROPERTY_USAGE_NO_EDITOR;
			return;
		}
	}

	if (p_property.name == ("root_bone") || p_property.name == ("scale_base_bone")) {
		String hint = "";
		for (int i = 0; i < bones.size(); i++) {
			if (i > 0) {
				hint += ",";
			}
			hint += String(bones[i].bone_name);
		}
		p_property.hint_string = hint;
	}
}

void SkeletonProfile::_get_property_list(List<PropertyInfo> *p_list) const {
	if (is_read_only) {
		return;
	}
	String group_names = "";
	for (int i = 0; i < groups.size(); i++) {
		String path = "groups/" + itos(i) + "/";
		p_list->push_back(PropertyInfo(Variant::STRING_NAME, path + "group_name"));
		p_list->push_back(PropertyInfo(Variant::OBJECT, path + "texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"));
		if (i > 0) {
			group_names = group_names + ",";
		}
		group_names = group_names + groups[i].group_name;
	}
	for (int i = 0; i < bones.size(); i++) {
		String path = "bones/" + itos(i) + "/";
		int bone_tail_usage = (get_tail_direction(i) != TAIL_DIRECTION_SPECIFIC_CHILD) ? PROPERTY_USAGE_NONE : PROPERTY_USAGE_DEFAULT;

		p_list->push_back(PropertyInfo(Variant::STRING_NAME, path + "bone_name"));
		p_list->push_back(PropertyInfo(Variant::STRING_NAME, path + "bone_parent"));
		p_list->push_back(PropertyInfo(Variant::INT, path + "tail_direction", PROPERTY_HINT_ENUM, "AverageChildren,SpecificChild,End"));
		p_list->push_back(PropertyInfo(Variant::STRING_NAME, path + "bone_tail", PROPERTY_HINT_NONE, "", bone_tail_usage));
		p_list->push_back(PropertyInfo(Variant::TRANSFORM3D, path + "reference_pose"));
		p_list->push_back(PropertyInfo(Variant::VECTOR2, path + "handle_offset"));
		p_list->push_back(PropertyInfo(Variant::STRING_NAME, path + "group", PROPERTY_HINT_ENUM, group_names));
		p_list->push_back(PropertyInfo(Variant::BOOL, path + "require"));
	}
}

StringName SkeletonProfile::get_root_bone() {
	return root_bone;
}

void SkeletonProfile::set_root_bone(const StringName &p_bone_name) {
	if (is_read_only) {
		return;
	}
	root_bone = p_bone_name;
}

StringName SkeletonProfile::get_scale_base_bone() {
	return scale_base_bone;
}

void SkeletonProfile::set_scale_base_bone(const StringName &p_bone_name) {
	if (is_read_only) {
		return;
	}
	scale_base_bone = p_bone_name;
}

int SkeletonProfile::get_group_size() {
	return groups.size();
}

void SkeletonProfile::set_group_size(int p_size) {
	if (is_read_only) {
		return;
	}
	ERR_FAIL_COND(p_size < 0);
	groups.resize(p_size);
	emit_signal("profile_updated");
	notify_property_list_changed();
}

StringName SkeletonProfile::get_group_name(int p_group_idx) const {
	ERR_FAIL_INDEX_V(p_group_idx, groups.size(), StringName());
	return groups[p_group_idx].group_name;
}

void SkeletonProfile::set_group_name(int p_group_idx, const StringName &p_group_name) {
	if (is_read_only) {
		return;
	}
	ERR_FAIL_INDEX(p_group_idx, groups.size());
	groups.ptrw()[p_group_idx].group_name = p_group_name;
	emit_signal("profile_updated");
}

Ref<Texture2D> SkeletonProfile::get_texture(int p_group_idx) const {
	ERR_FAIL_INDEX_V(p_group_idx, groups.size(), Ref<Texture2D>());
	return groups[p_group_idx].texture;
}

void SkeletonProfile::set_texture(int p_group_idx, const Ref<Texture2D> &p_texture) {
	if (is_read_only) {
		return;
	}
	ERR_FAIL_INDEX(p_group_idx, groups.size());
	groups.ptrw()[p_group_idx].texture = p_texture;
	emit_signal("profile_updated");
}

int SkeletonProfile::get_bone_size() {
	return bones.size();
}

void SkeletonProfile::set_bone_size(int p_size) {
	if (is_read_only) {
		return;
	}
	ERR_FAIL_COND(p_size < 0);
	bones.resize(p_size);
	emit_signal("profile_updated");
	notify_property_list_changed();
}

int SkeletonProfile::find_bone(const StringName &p_bone_name) const {
	if (p_bone_name == StringName()) {
		return -1;
	}
	for (int i = 0; i < bones.size(); i++) {
		if (bones[i].bone_name == p_bone_name) {
			return i;
		}
	}
	return -1;
}

PackedStringArray SkeletonProfile::get_bone_names() {
	PackedStringArray s;
	for (const SkeletonProfileBone &bone : bones) {
		s.push_back(bone.bone_name);
	}
	return s;
}

StringName SkeletonProfile::get_bone_name(int p_bone_idx) const {
	ERR_FAIL_INDEX_V(p_bone_idx, bones.size(), StringName());
	return bones[p_bone_idx].bone_name;
}

void SkeletonProfile::set_bone_name(int p_bone_idx, const StringName &p_bone_name) {
	if (is_read_only) {
		return;
	}
	ERR_FAIL_INDEX(p_bone_idx, bones.size());
	bones.ptrw()[p_bone_idx].bone_name = p_bone_name;
	emit_signal("profile_updated");
}

StringName SkeletonProfile::get_bone_parent(int p_bone_idx) const {
	ERR_FAIL_INDEX_V(p_bone_idx, bones.size(), StringName());
	return bones[p_bone_idx].bone_parent;
}

void SkeletonProfile::set_bone_parent(int p_bone_idx, const StringName &p_bone_parent) {
	if (is_read_only) {
		return;
	}
	ERR_FAIL_INDEX(p_bone_idx, bones.size());
	bones.ptrw()[p_bone_idx].bone_parent = p_bone_parent;
	emit_signal("profile_updated");
}

SkeletonProfile::TailDirection SkeletonProfile::get_tail_direction(int p_bone_idx) const {
	ERR_FAIL_INDEX_V(p_bone_idx, bones.size(), TAIL_DIRECTION_AVERAGE_CHILDREN);
	return bones[p_bone_idx].tail_direction;
}

void SkeletonProfile::set_tail_direction(int p_bone_idx, TailDirection p_tail_direction) {
	if (is_read_only) {
		return;
	}
	ERR_FAIL_INDEX(p_bone_idx, bones.size());
	bones.ptrw()[p_bone_idx].tail_direction = p_tail_direction;
	emit_signal("profile_updated");
	notify_property_list_changed();
}

StringName SkeletonProfile::get_bone_tail(int p_bone_idx) const {
	ERR_FAIL_INDEX_V(p_bone_idx, bones.size(), StringName());
	return bones[p_bone_idx].bone_tail;
}

void SkeletonProfile::set_bone_tail(int p_bone_idx, const StringName &p_bone_tail) {
	if (is_read_only) {
		return;
	}
	ERR_FAIL_INDEX(p_bone_idx, bones.size());
	bones.ptrw()[p_bone_idx].bone_tail = p_bone_tail;
	emit_signal("profile_updated");
}

Transform3D SkeletonProfile::get_reference_pose(int p_bone_idx) const {
	ERR_FAIL_INDEX_V(p_bone_idx, bones.size(), Transform3D());
	return bones[p_bone_idx].reference_pose;
}

void SkeletonProfile::set_reference_pose(int p_bone_idx, const Transform3D &p_reference_pose) {
	if (is_read_only) {
		return;
	}
	ERR_FAIL_INDEX(p_bone_idx, bones.size());
	bones.ptrw()[p_bone_idx].reference_pose = p_reference_pose;
	emit_signal("profile_updated");
}

Vector2 SkeletonProfile::get_handle_offset(int p_bone_idx) const {
	ERR_FAIL_INDEX_V(p_bone_idx, bones.size(), Vector2());
	return bones[p_bone_idx].handle_offset;
}

void SkeletonProfile::set_handle_offset(int p_bone_idx, const Vector2 &p_handle_offset) {
	if (is_read_only) {
		return;
	}
	ERR_FAIL_INDEX(p_bone_idx, bones.size());
	bones.ptrw()[p_bone_idx].handle_offset = p_handle_offset;
	emit_signal("profile_updated");
}

StringName SkeletonProfile::get_group(int p_bone_idx) const {
	ERR_FAIL_INDEX_V(p_bone_idx, bones.size(), StringName());
	return bones[p_bone_idx].group;
}

void SkeletonProfile::set_group(int p_bone_idx, const StringName &p_group) {
	if (is_read_only) {
		return;
	}
	ERR_FAIL_INDEX(p_bone_idx, bones.size());
	bones.ptrw()[p_bone_idx].group = p_group;
	emit_signal("profile_updated");
}

bool SkeletonProfile::is_required(int p_bone_idx) const {
	ERR_FAIL_INDEX_V(p_bone_idx, bones.size(), false);
	return bones[p_bone_idx].required;
}

void SkeletonProfile::set_required(int p_bone_idx, bool p_required) {
	if (is_read_only) {
		return;
	}
	ERR_FAIL_INDEX(p_bone_idx, bones.size());
	bones.ptrw()[p_bone_idx].required = p_required;
	emit_signal("profile_updated");
}

bool SkeletonProfile::has_bone(const StringName &p_bone_name) {
	bool is_found = false;
	for (int i = 0; i < bones.size(); i++) {
		if (bones[i].bone_name == p_bone_name) {
			is_found = true;
			break;
		}
	}
	return is_found;
}

void SkeletonProfile::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_root_bone", "bone_name"), &SkeletonProfile::set_root_bone);
	ClassDB::bind_method(D_METHOD("get_root_bone"), &SkeletonProfile::get_root_bone);

	ClassDB::bind_method(D_METHOD("set_scale_base_bone", "bone_name"), &SkeletonProfile::set_scale_base_bone);
	ClassDB::bind_method(D_METHOD("get_scale_base_bone"), &SkeletonProfile::get_scale_base_bone);

	ClassDB::bind_method(D_METHOD("set_group_size", "size"), &SkeletonProfile::set_group_size);
	ClassDB::bind_method(D_METHOD("get_group_size"), &SkeletonProfile::get_group_size);

	ClassDB::bind_method(D_METHOD("get_group_name", "group_idx"), &SkeletonProfile::get_group_name);
	ClassDB::bind_method(D_METHOD("set_group_name", "group_idx", "group_name"), &SkeletonProfile::set_group_name);

	ClassDB::bind_method(D_METHOD("get_texture", "group_idx"), &SkeletonProfile::get_texture);
	ClassDB::bind_method(D_METHOD("set_texture", "group_idx", "texture"), &SkeletonProfile::set_texture);

	ClassDB::bind_method(D_METHOD("set_bone_size", "size"), &SkeletonProfile::set_bone_size);
	ClassDB::bind_method(D_METHOD("get_bone_size"), &SkeletonProfile::get_bone_size);

	ClassDB::bind_method(D_METHOD("find_bone", "bone_name"), &SkeletonProfile::find_bone);

	ClassDB::bind_method(D_METHOD("get_bone_name", "bone_idx"), &SkeletonProfile::get_bone_name);
	ClassDB::bind_method(D_METHOD("set_bone_name", "bone_idx", "bone_name"), &SkeletonProfile::set_bone_name);

	ClassDB::bind_method(D_METHOD("get_bone_parent", "bone_idx"), &SkeletonProfile::get_bone_parent);
	ClassDB::bind_method(D_METHOD("set_bone_parent", "bone_idx", "bone_parent"), &SkeletonProfile::set_bone_parent);

	ClassDB::bind_method(D_METHOD("get_tail_direction", "bone_idx"), &SkeletonProfile::get_tail_direction);
	ClassDB::bind_method(D_METHOD("set_tail_direction", "bone_idx", "tail_direction"), &SkeletonProfile::set_tail_direction);

	ClassDB::bind_method(D_METHOD("get_bone_tail", "bone_idx"), &SkeletonProfile::get_bone_tail);
	ClassDB::bind_method(D_METHOD("set_bone_tail", "bone_idx", "bone_tail"), &SkeletonProfile::set_bone_tail);

	ClassDB::bind_method(D_METHOD("get_reference_pose", "bone_idx"), &SkeletonProfile::get_reference_pose);
	ClassDB::bind_method(D_METHOD("set_reference_pose", "bone_idx", "bone_name"), &SkeletonProfile::set_reference_pose);

	ClassDB::bind_method(D_METHOD("get_handle_offset", "bone_idx"), &SkeletonProfile::get_handle_offset);
	ClassDB::bind_method(D_METHOD("set_handle_offset", "bone_idx", "handle_offset"), &SkeletonProfile::set_handle_offset);

	ClassDB::bind_method(D_METHOD("get_group", "bone_idx"), &SkeletonProfile::get_group);
	ClassDB::bind_method(D_METHOD("set_group", "bone_idx", "group"), &SkeletonProfile::set_group);

	ClassDB::bind_method(D_METHOD("is_required", "bone_idx"), &SkeletonProfile::is_required);
	ClassDB::bind_method(D_METHOD("set_required", "bone_idx", "required"), &SkeletonProfile::set_required);

	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "root_bone", PROPERTY_HINT_ENUM_SUGGESTION, ""), "set_root_bone", "get_root_bone");
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "scale_base_bone", PROPERTY_HINT_ENUM_SUGGESTION, ""), "set_scale_base_bone", "get_scale_base_bone");

	ADD_PROPERTY(PropertyInfo(Variant::INT, "group_size", PROPERTY_HINT_RANGE, "0,100,1", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_ARRAY, "Groups,groups/"), "set_group_size", "get_group_size");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "bone_size", PROPERTY_HINT_RANGE, "0,100,1", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_ARRAY, "Bones,bones/"), "set_bone_size", "get_bone_size");

	ADD_SIGNAL(MethodInfo("profile_updated"));

	BIND_ENUM_CONSTANT(TAIL_DIRECTION_AVERAGE_CHILDREN);
	BIND_ENUM_CONSTANT(TAIL_DIRECTION_SPECIFIC_CHILD);
	BIND_ENUM_CONSTANT(TAIL_DIRECTION_END);
}

SkeletonProfile::SkeletonProfile() {
}

SkeletonProfile::~SkeletonProfile() {
}

SkeletonProfileHumanoid::SkeletonProfileHumanoid() {
	is_read_only = true;

	root_bone = "Root";
	scale_base_bone = "Hips";

	groups.resize(4);

	groups.ptrw()[0].group_name = "Body";
	groups.ptrw()[1].group_name = "Face";
	groups.ptrw()[2].group_name = "LeftHand";
	groups.ptrw()[3].group_name = "RightHand";

	bones.resize(56);

	bones.ptrw()[0].bone_name = "Root";
	bones.ptrw()[0].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0);
	bones.ptrw()[0].handle_offset = Vector2(0.5, 0.91);
	bones.ptrw()[0].group = "Body";

	bones.ptrw()[1].bone_name = "Hips";
	bones.ptrw()[1].bone_parent = "Root";
	bones.ptrw()[1].tail_direction = TAIL_DIRECTION_SPECIFIC_CHILD;
	bones.ptrw()[1].bone_tail = "Spine";
	bones.ptrw()[1].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.75, 0);
	bones.ptrw()[1].handle_offset = Vector2(0.5, 0.5);
	bones.ptrw()[1].group = "Body";
	bones.ptrw()[1].required = true;

	bones.ptrw()[2].bone_name = "Spine";
	bones.ptrw()[2].bone_parent = "Hips";
	bones.ptrw()[2].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.1, 0);
	bones.ptrw()[2].handle_offset = Vector2(0.5, 0.43);
	bones.ptrw()[2].group = "Body";
	bones.ptrw()[2].required = true;

	bones.ptrw()[3].bone_name = "Chest";
	bones.ptrw()[3].bone_parent = "Spine";
	bones.ptrw()[3].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.1, 0);
	bones.ptrw()[3].handle_offset = Vector2(0.5, 0.36);
	bones.ptrw()[3].group = "Body";

	bones.ptrw()[4].bone_name = "UpperChest";
	bones.ptrw()[4].bone_parent = "Chest";
	bones.ptrw()[4].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.1, 0);
	bones.ptrw()[4].handle_offset = Vector2(0.5, 0.29);
	bones.ptrw()[4].group = "Body";

	bones.ptrw()[5].bone_name = "Neck";
	bones.ptrw()[5].bone_parent = "UpperChest";
	bones.ptrw()[5].tail_direction = TAIL_DIRECTION_SPECIFIC_CHILD;
	bones.ptrw()[5].bone_tail = "Head";
	bones.ptrw()[5].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.1, 0);
	bones.ptrw()[5].handle_offset = Vector2(0.5, 0.23);
	bones.ptrw()[5].group = "Body";
	bones.ptrw()[5].required = false;

	bones.ptrw()[6].bone_name = "Head";
	bones.ptrw()[6].bone_parent = "Neck";
	bones.ptrw()[6].tail_direction = TAIL_DIRECTION_END;
	bones.ptrw()[6].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.1, 0);
	bones.ptrw()[6].handle_offset = Vector2(0.5, 0.18);
	bones.ptrw()[6].group = "Body";
	bones.ptrw()[6].required = true;

	bones.ptrw()[7].bone_name = "LeftEye";
	bones.ptrw()[7].bone_parent = "Head";
	bones.ptrw()[7].reference_pose = Transform3D(1, 0, 0, 0, 0, -1, 0, 1, 0, 0.05, 0.15, 0);
	bones.ptrw()[7].handle_offset = Vector2(0.6, 0.46);
	bones.ptrw()[7].group = "Face";

	bones.ptrw()[8].bone_name = "RightEye";
	bones.ptrw()[8].bone_parent = "Head";
	bones.ptrw()[8].reference_pose = Transform3D(1, 0, 0, 0, 0, -1, 0, 1, 0, -0.05, 0.15, 0);
	bones.ptrw()[8].handle_offset = Vector2(0.37, 0.46);
	bones.ptrw()[8].group = "Face";

	bones.ptrw()[9].bone_name = "Jaw";
	bones.ptrw()[9].bone_parent = "Head";
	bones.ptrw()[9].reference_pose = Transform3D(-1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0.05, 0.05);
	bones.ptrw()[9].handle_offset = Vector2(0.46, 0.75);
	bones.ptrw()[9].group = "Face";

	bones.ptrw()[10].bone_name = "LeftShoulder";
	bones.ptrw()[10].bone_parent = "UpperChest";
	bones.ptrw()[10].reference_pose = Transform3D(0, 1, 0, 0, 0, 1, 1, 0, 0, 0.05, 0.1, 0);
	bones.ptrw()[10].handle_offset = Vector2(0.55, 0.235);
	bones.ptrw()[10].group = "Body";
	bones.ptrw()[10].required = true;

	bones.ptrw()[11].bone_name = "LeftUpperArm";
	bones.ptrw()[11].bone_parent = "LeftShoulder";
	bones.ptrw()[11].reference_pose = Transform3D(-1, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0.05, 0);
	bones.ptrw()[11].handle_offset = Vector2(0.6, 0.24);
	bones.ptrw()[11].group = "Body";
	bones.ptrw()[11].required = true;

	bones.ptrw()[12].bone_name = "LeftLowerArm";
	bones.ptrw()[12].bone_parent = "LeftUpperArm";
	bones.ptrw()[12].reference_pose = Transform3D(0, 0, -1, 0, 1, 0, 1, 0, 0, 0, 0.25, 0);
	bones.ptrw()[12].handle_offset = Vector2(0.7, 0.24);
	bones.ptrw()[12].group = "Body";
	bones.ptrw()[12].required = true;

	bones.ptrw()[13].bone_name = "LeftHand";
	bones.ptrw()[13].bone_parent = "LeftLowerArm";
	bones.ptrw()[13].tail_direction = TAIL_DIRECTION_SPECIFIC_CHILD;
	bones.ptrw()[13].bone_tail = "LeftMiddleProximal";
	bones.ptrw()[13].reference_pose = Transform3D(0, 0, 1, 0, 1, 0, -1, 0, 0, 0, 0.25, 0);
	bones.ptrw()[13].handle_offset = Vector2(0.82, 0.235);
	bones.ptrw()[13].group = "Body";
	bones.ptrw()[13].required = true;

	bones.ptrw()[14].bone_name = "LeftThumbMetacarpal";
	bones.ptrw()[14].bone_parent = "LeftHand";
	bones.ptrw()[14].reference_pose = Transform3D(0, -0.577, 0.816, 0, 0.816, 0.577, -1, 0, 0, -0.025, 0.025, 0);
	bones.ptrw()[14].handle_offset = Vector2(0.4, 0.8);
	bones.ptrw()[14].group = "LeftHand";

	bones.ptrw()[15].bone_name = "LeftThumbProximal";
	bones.ptrw()[15].bone_parent = "LeftThumbMetacarpal";
	bones.ptrw()[15].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.043, 0);
	bones.ptrw()[15].handle_offset = Vector2(0.3, 0.69);
	bones.ptrw()[15].group = "LeftHand";

	bones.ptrw()[16].bone_name = "LeftThumbDistal";
	bones.ptrw()[16].bone_parent = "LeftThumbProximal";
	bones.ptrw()[16].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.043, 0);
	bones.ptrw()[16].handle_offset = Vector2(0.23, 0.555);
	bones.ptrw()[16].group = "LeftHand";

	bones.ptrw()[17].bone_name = "LeftIndexProximal";
	bones.ptrw()[17].bone_parent = "LeftHand";
	bones.ptrw()[17].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -0.025, 0.075, 0);
	bones.ptrw()[17].handle_offset = Vector2(0.413, 0.52);
	bones.ptrw()[17].group = "LeftHand";

	bones.ptrw()[18].bone_name = "LeftIndexIntermediate";
	bones.ptrw()[18].bone_parent = "LeftIndexProximal";
	bones.ptrw()[18].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.05, 0);
	bones.ptrw()[18].handle_offset = Vector2(0.403, 0.36);
	bones.ptrw()[18].group = "LeftHand";

	bones.ptrw()[19].bone_name = "LeftIndexDistal";
	bones.ptrw()[19].bone_parent = "LeftIndexIntermediate";
	bones.ptrw()[19].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.025, 0);
	bones.ptrw()[19].handle_offset = Vector2(0.403, 0.255);
	bones.ptrw()[19].group = "LeftHand";

	bones.ptrw()[20].bone_name = "LeftMiddleProximal";
	bones.ptrw()[20].bone_parent = "LeftHand";
	bones.ptrw()[20].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.075, 0);
	bones.ptrw()[20].handle_offset = Vector2(0.5, 0.51);
	bones.ptrw()[20].group = "LeftHand";

	bones.ptrw()[21].bone_name = "LeftMiddleIntermediate";
	bones.ptrw()[21].bone_parent = "LeftMiddleProximal";
	bones.ptrw()[21].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.075, 0);
	bones.ptrw()[21].handle_offset = Vector2(0.5, 0.345);
	bones.ptrw()[21].group = "LeftHand";

	bones.ptrw()[22].bone_name = "LeftMiddleDistal";
	bones.ptrw()[22].bone_parent = "LeftMiddleIntermediate";
	bones.ptrw()[22].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.025, 0);
	bones.ptrw()[22].handle_offset = Vector2(0.5, 0.22);
	bones.ptrw()[22].group = "LeftHand";

	bones.ptrw()[23].bone_name = "LeftRingProximal";
	bones.ptrw()[23].bone_parent = "LeftHand";
	bones.ptrw()[23].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0.025, 0.075, 0);
	bones.ptrw()[23].handle_offset = Vector2(0.586, 0.52);
	bones.ptrw()[23].group = "LeftHand";

	bones.ptrw()[24].bone_name = "LeftRingIntermediate";
	bones.ptrw()[24].bone_parent = "LeftRingProximal";
	bones.ptrw()[24].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.05, 0);
	bones.ptrw()[24].handle_offset = Vector2(0.59, 0.36);
	bones.ptrw()[24].group = "LeftHand";

	bones.ptrw()[25].bone_name = "LeftRingDistal";
	bones.ptrw()[25].bone_parent = "LeftRingIntermediate";
	bones.ptrw()[25].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.025, 0);
	bones.ptrw()[25].handle_offset = Vector2(0.591, 0.25);
	bones.ptrw()[25].group = "LeftHand";

	bones.ptrw()[26].bone_name = "LeftLittleProximal";
	bones.ptrw()[26].bone_parent = "LeftHand";
	bones.ptrw()[26].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0.05, 0.05, 0);
	bones.ptrw()[26].handle_offset = Vector2(0.663, 0.543);
	bones.ptrw()[26].group = "LeftHand";

	bones.ptrw()[27].bone_name = "LeftLittleIntermediate";
	bones.ptrw()[27].bone_parent = "LeftLittleProximal";
	bones.ptrw()[27].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.05, 0);
	bones.ptrw()[27].handle_offset = Vector2(0.672, 0.415);
	bones.ptrw()[27].group = "LeftHand";

	bones.ptrw()[28].bone_name = "LeftLittleDistal";
	bones.ptrw()[28].bone_parent = "LeftLittleIntermediate";
	bones.ptrw()[28].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.025, 0);
	bones.ptrw()[28].handle_offset = Vector2(0.672, 0.32);
	bones.ptrw()[28].group = "LeftHand";

	bones.ptrw()[29].bone_name = "RightShoulder";
	bones.ptrw()[29].bone_parent = "UpperChest";
	bones.ptrw()[29].reference_pose = Transform3D(0, -1, 0, 0, 0, 1, -1, 0, 0, -0.05, 0.1, 0);
	bones.ptrw()[29].handle_offset = Vector2(0.45, 0.235);
	bones.ptrw()[29].group = "Body";
	bones.ptrw()[29].required = true;

	bones.ptrw()[30].bone_name = "RightUpperArm";
	bones.ptrw()[30].bone_parent = "RightShoulder";
	bones.ptrw()[30].reference_pose = Transform3D(-1, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0.05, 0);
	bones.ptrw()[30].handle_offset = Vector2(0.4, 0.24);
	bones.ptrw()[30].group = "Body";
	bones.ptrw()[30].required = true;

	bones.ptrw()[31].bone_name = "RightLowerArm";
	bones.ptrw()[31].bone_parent = "RightUpperArm";
	bones.ptrw()[31].reference_pose = Transform3D(0, 0, 1, 0, 1, 0, -1, 0, 0, 0, 0.25, 0);
	bones.ptrw()[31].handle_offset = Vector2(0.3, 0.24);
	bones.ptrw()[31].group = "Body";
	bones.ptrw()[31].required = true;

	bones.ptrw()[32].bone_name = "RightHand";
	bones.ptrw()[32].bone_parent = "RightLowerArm";
	bones.ptrw()[32].tail_direction = TAIL_DIRECTION_SPECIFIC_CHILD;
	bones.ptrw()[32].bone_tail = "RightMiddleProximal";
	bones.ptrw()[32].reference_pose = Transform3D(0, 0, -1, 0, 1, 0, 1, 0, 0, 0, 0.25, 0);
	bones.ptrw()[32].handle_offset = Vector2(0.18, 0.235);
	bones.ptrw()[32].group = "Body";
	bones.ptrw()[32].required = true;

	bones.ptrw()[33].bone_name = "RightThumbMetacarpal";
	bones.ptrw()[33].bone_parent = "RightHand";
	bones.ptrw()[33].reference_pose = Transform3D(0, 0.577, -0.816, 0, 0.816, 0.577, 1, 0, 0, 0.025, 0.025, 0);
	bones.ptrw()[33].handle_offset = Vector2(0.6, 0.8);
	bones.ptrw()[33].group = "RightHand";

	bones.ptrw()[34].bone_name = "RightThumbProximal";
	bones.ptrw()[34].bone_parent = "RightThumbMetacarpal";
	bones.ptrw()[34].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.043, 0);
	bones.ptrw()[34].handle_offset = Vector2(0.7, 0.69);
	bones.ptrw()[34].group = "RightHand";

	bones.ptrw()[35].bone_name = "RightThumbDistal";
	bones.ptrw()[35].bone_parent = "RightThumbProximal";
	bones.ptrw()[35].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.043, 0);
	bones.ptrw()[35].handle_offset = Vector2(0.77, 0.555);
	bones.ptrw()[35].group = "RightHand";

	bones.ptrw()[36].bone_name = "RightIndexProximal";
	bones.ptrw()[36].bone_parent = "RightHand";
	bones.ptrw()[36].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0.025, 0.075, 0);
	bones.ptrw()[36].handle_offset = Vector2(0.587, 0.52);
	bones.ptrw()[36].group = "RightHand";

	bones.ptrw()[37].bone_name = "RightIndexIntermediate";
	bones.ptrw()[37].bone_parent = "RightIndexProximal";
	bones.ptrw()[37].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.05, 0);
	bones.ptrw()[37].handle_offset = Vector2(0.597, 0.36);
	bones.ptrw()[37].group = "RightHand";

	bones.ptrw()[38].bone_name = "RightIndexDistal";
	bones.ptrw()[38].bone_parent = "RightIndexIntermediate";
	bones.ptrw()[38].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.025, 0);
	bones.ptrw()[38].handle_offset = Vector2(0.597, 0.255);
	bones.ptrw()[38].group = "RightHand";

	bones.ptrw()[39].bone_name = "RightMiddleProximal";
	bones.ptrw()[39].bone_parent = "RightHand";
	bones.ptrw()[39].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.075, 0);
	bones.ptrw()[39].handle_offset = Vector2(0.5, 0.51);
	bones.ptrw()[39].group = "RightHand";

	bones.ptrw()[40].bone_name = "RightMiddleIntermediate";
	bones.ptrw()[40].bone_parent = "RightMiddleProximal";
	bones.ptrw()[40].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.075, 0);
	bones.ptrw()[40].handle_offset = Vector2(0.5, 0.345);
	bones.ptrw()[40].group = "RightHand";

	bones.ptrw()[41].bone_name = "RightMiddleDistal";
	bones.ptrw()[41].bone_parent = "RightMiddleIntermediate";
	bones.ptrw()[41].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.025, 0);
	bones.ptrw()[41].handle_offset = Vector2(0.5, 0.22);
	bones.ptrw()[41].group = "RightHand";

	bones.ptrw()[42].bone_name = "RightRingProximal";
	bones.ptrw()[42].bone_parent = "RightHand";
	bones.ptrw()[42].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -0.025, 0.075, 0);
	bones.ptrw()[42].handle_offset = Vector2(0.414, 0.52);
	bones.ptrw()[42].group = "RightHand";

	bones.ptrw()[43].bone_name = "RightRingIntermediate";
	bones.ptrw()[43].bone_parent = "RightRingProximal";
	bones.ptrw()[43].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.05, 0);
	bones.ptrw()[43].handle_offset = Vector2(0.41, 0.36);
	bones.ptrw()[43].group = "RightHand";

	bones.ptrw()[44].bone_name = "RightRingDistal";
	bones.ptrw()[44].bone_parent = "RightRingIntermediate";
	bones.ptrw()[44].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.025, 0);
	bones.ptrw()[44].handle_offset = Vector2(0.409, 0.25);
	bones.ptrw()[44].group = "RightHand";

	bones.ptrw()[45].bone_name = "RightLittleProximal";
	bones.ptrw()[45].bone_parent = "RightHand";
	bones.ptrw()[45].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, -0.05, 0.05, 0);
	bones.ptrw()[45].handle_offset = Vector2(0.337, 0.543);
	bones.ptrw()[45].group = "RightHand";

	bones.ptrw()[46].bone_name = "RightLittleIntermediate";
	bones.ptrw()[46].bone_parent = "RightLittleProximal";
	bones.ptrw()[46].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.05, 0);
	bones.ptrw()[46].handle_offset = Vector2(0.328, 0.415);
	bones.ptrw()[46].group = "RightHand";

	bones.ptrw()[47].bone_name = "RightLittleDistal";
	bones.ptrw()[47].bone_parent = "RightLittleIntermediate";
	bones.ptrw()[47].reference_pose = Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0.025, 0);
	bones.ptrw()[47].handle_offset = Vector2(0.328, 0.32);
	bones.ptrw()[47].group = "RightHand";

	bones.ptrw()[48].bone_name = "LeftUpperLeg";
	bones.ptrw()[48].bone_parent = "Hips";
	bones.ptrw()[48].reference_pose = Transform3D(-1, 0, 0, 0, -1, 0, 0, 0, 1, 0.1, 0, 0);
	bones.ptrw()[48].handle_offset = Vector2(0.549, 0.49);
	bones.ptrw()[48].group = "Body";
	bones.ptrw()[48].required = true;

	bones.ptrw()[49].bone_name = "LeftLowerLeg";
	bones.ptrw()[49].bone_parent = "LeftUpperLeg";
	bones.ptrw()[49].reference_pose = Transform3D(-1, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0.375, 0);
	bones.ptrw()[49].handle_offset = Vector2(0.548, 0.683);
	bones.ptrw()[49].group = "Body";
	bones.ptrw()[49].required = true;

	bones.ptrw()[50].bone_name = "LeftFoot";
	bones.ptrw()[50].bone_parent = "LeftLowerLeg";
	bones.ptrw()[50].reference_pose = Transform3D(-1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0.375, 0);
	bones.ptrw()[50].handle_offset = Vector2(0.545, 0.9);
	bones.ptrw()[50].group = "Body";
	bones.ptrw()[50].required = true;

	bones.ptrw()[51].bone_name = "LeftToes";
	bones.ptrw()[51].bone_parent = "LeftFoot";
	bones.ptrw()[51].reference_pose = Transform3D(-1, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0.15, 0);
	bones.ptrw()[51].handle_offset = Vector2(0.545, 0.95);
	bones.ptrw()[51].group = "Body";

	bones.ptrw()[52].bone_name = "RightUpperLeg";
	bones.ptrw()[52].bone_parent = "Hips";
	bones.ptrw()[52].reference_pose = Transform3D(-1, 0, 0, 0, -1, 0, 0, 0, 1, -0.1, 0, 0);
	bones.ptrw()[52].handle_offset = Vector2(0.451, 0.49);
	bones.ptrw()[52].group = "Body";
	bones.ptrw()[52].required = true;

	bones.ptrw()[53].bone_name = "RightLowerLeg";
	bones.ptrw()[53].bone_parent = "RightUpperLeg";
	bones.ptrw()[53].reference_pose = Transform3D(-1, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0.375, 0);
	bones.ptrw()[53].handle_offset = Vector2(0.452, 0.683);
	bones.ptrw()[53].group = "Body";
	bones.ptrw()[53].required = true;

	bones.ptrw()[54].bone_name = "RightFoot";
	bones.ptrw()[54].bone_parent = "RightLowerLeg";
	bones.ptrw()[54].reference_pose = Transform3D(-1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0.375, 0);
	bones.ptrw()[54].handle_offset = Vector2(0.455, 0.9);
	bones.ptrw()[54].group = "Body";
	bones.ptrw()[54].required = true;

	bones.ptrw()[55].bone_name = "RightToes";
	bones.ptrw()[55].bone_parent = "RightFoot";
	bones.ptrw()[55].reference_pose = Transform3D(-1, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0.15, 0);
	bones.ptrw()[55].handle_offset = Vector2(0.455, 0.95);
	bones.ptrw()[55].group = "Body";
}

SkeletonProfileHumanoid::~SkeletonProfileHumanoid() {
}

//////////////////////////////////////
