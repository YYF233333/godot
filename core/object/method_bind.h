/**************************************************************************/
/*  method_bind.h                                                         */
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

#include "core/variant/binder_common.h"

VARIANT_BITFIELD_CAST(MethodFlags)

// some helpers

class MethodBind {
	int method_id;
	uint32_t hint_flags = METHOD_FLAGS_DEFAULT;
	StringName name;
	StringName instance_class;
	Vector<Variant> default_arguments;
	int default_argument_count = 0;
	int argument_count = 0;

	bool _static = false;
	bool _const = false;
	bool _returns = false;
	bool _returns_raw_obj_ptr = false;

protected:
	Variant::Type *argument_types = nullptr;
#ifdef DEBUG_ENABLED
	Vector<StringName> arg_names;
#endif // DEBUG_ENABLED
	void _set_const(bool p_const);
	void _set_static(bool p_static);
	void _set_returns(bool p_returns);
	virtual Variant::Type _gen_argument_type(int p_arg) const = 0;
	virtual PropertyInfo _gen_argument_type_info(int p_arg) const = 0;
	void _generate_argument_types(int p_count);

	void set_argument_count(int p_count) { argument_count = p_count; }

public:
	_FORCE_INLINE_ const Vector<Variant> &get_default_arguments() const { return default_arguments; }
	_FORCE_INLINE_ int get_default_argument_count() const { return default_argument_count; }

	_FORCE_INLINE_ Variant has_default_argument(int p_arg) const {
		int idx = p_arg - (argument_count - default_arguments.size());

		if (idx < 0 || idx >= default_arguments.size()) {
			return false;
		} else {
			return true;
		}
	}

	_FORCE_INLINE_ Variant get_default_argument(int p_arg) const {
		int idx = p_arg - (argument_count - default_arguments.size());

		if (idx < 0 || idx >= default_arguments.size()) {
			return Variant();
		} else {
			return default_arguments[idx];
		}
	}

	_FORCE_INLINE_ Variant::Type get_argument_type(int p_argument) const {
		ERR_FAIL_COND_V(p_argument < -1 || p_argument >= argument_count, Variant::NIL);
		return argument_types[p_argument + 1];
	}

	PropertyInfo get_argument_info(int p_argument) const;
	PropertyInfo get_return_info() const;

#ifdef DEBUG_ENABLED
	void set_argument_names(const Vector<StringName> &p_names); // Set by ClassDB, can't be inferred otherwise.
	Vector<StringName> get_argument_names() const;

	virtual GodotTypeInfo::Metadata get_argument_meta(int p_arg) const = 0;
#endif // DEBUG_ENABLED

	void set_hint_flags(uint32_t p_hint) { hint_flags = p_hint; }
	uint32_t get_hint_flags() const { return hint_flags | (is_const() ? METHOD_FLAG_CONST : 0) | (is_vararg() ? METHOD_FLAG_VARARG : 0) | (is_static() ? METHOD_FLAG_STATIC : 0); }
	_FORCE_INLINE_ StringName get_instance_class() const { return instance_class; }
	_FORCE_INLINE_ void set_instance_class(const StringName &p_class) { instance_class = p_class; }

	_FORCE_INLINE_ int get_argument_count() const { return argument_count; }

#ifdef TOOLS_ENABLED
	virtual bool is_valid() const { return true; }
#endif

	virtual Variant call(Object *p_object, const Variant **p_args, int p_arg_count, Callable::CallError &r_error) const = 0;
	virtual void validated_call(Object *p_object, const Variant **p_args, Variant *r_ret) const = 0;

	virtual void ptrcall(Object *p_object, const void **p_args, void *r_ret) const = 0;

	StringName get_name() const;
	void set_name(const StringName &p_name);
	_FORCE_INLINE_ int get_method_id() const { return method_id; }
	_FORCE_INLINE_ bool is_const() const { return _const; }
	_FORCE_INLINE_ bool is_static() const { return _static; }
	_FORCE_INLINE_ bool has_return() const { return _returns; }
	virtual bool is_vararg() const { return false; }

	_FORCE_INLINE_ bool is_return_type_raw_object_ptr() { return _returns_raw_obj_ptr; }
	_FORCE_INLINE_ void set_return_type_is_raw_object_ptr(bool p_returns_raw_obj) { _returns_raw_obj_ptr = p_returns_raw_obj; }

	void set_default_arguments(const Vector<Variant> &p_defargs);

	uint32_t get_hash() const;

	MethodBind();
	virtual ~MethodBind();
};

// MethodBindVarArg base CRTP
template <typename Derived, typename T, typename R, bool should_returns>
class MethodBindVarArgBase : public MethodBind {
protected:
	R (T::*method)(const Variant **, int, Callable::CallError &);
	MethodInfo method_info;

public:
	virtual PropertyInfo _gen_argument_type_info(int p_arg) const override {
		if (p_arg < 0) {
			return _gen_return_type_info();
		} else if (p_arg < method_info.arguments.size()) {
			return method_info.arguments[p_arg];
		} else {
			return PropertyInfo(Variant::NIL, "arg_" + itos(p_arg), PROPERTY_HINT_NONE, String(), PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_NIL_IS_VARIANT);
		}
	}

	virtual Variant::Type _gen_argument_type(int p_arg) const override {
		return _gen_argument_type_info(p_arg).type;
	}

#ifdef DEBUG_ENABLED
	virtual GodotTypeInfo::Metadata get_argument_meta(int) const override {
		return GodotTypeInfo::METADATA_NONE;
	}
#endif // DEBUG_ENABLED

	virtual void validated_call(Object *p_object, const Variant **p_args, Variant *r_ret) const override {
		ERR_FAIL_MSG("Validated call can't be used with vararg methods. This is a bug.");
	}

	virtual void ptrcall(Object *p_object, const void **p_args, void *r_ret) const override {
		ERR_FAIL_MSG("ptrcall can't be used with vararg methods. This is a bug.");
	}

	virtual bool is_const() const { return false; }

	virtual bool is_vararg() const override { return true; }

	MethodBindVarArgBase(
			R (T::*p_method)(const Variant **, int, Callable::CallError &),
			const MethodInfo &p_method_info,
			bool p_return_nil_is_variant) :
			method(p_method), method_info(p_method_info) {
		set_argument_count(method_info.arguments.size());
		Variant::Type *at = memnew_arr(Variant::Type, method_info.arguments.size() + 1);
		at[0] = _gen_return_type_info().type;
		if (method_info.arguments.size()) {
#ifdef DEBUG_ENABLED
			Vector<StringName> names;
			names.resize(method_info.arguments.size());
#endif // DEBUG_ENABLED
			for (int64_t i = 0; i < method_info.arguments.size(); ++i) {
				at[i + 1] = method_info.arguments[i].type;
#ifdef DEBUG_ENABLED
				names.write[i] = method_info.arguments[i].name;
#endif // DEBUG_ENABLED
			}

#ifdef DEBUG_ENABLED
			set_argument_names(names);
#endif // DEBUG_ENABLED
		}
		argument_types = at;
		if (p_return_nil_is_variant) {
			method_info.return_val.usage |= PROPERTY_USAGE_NIL_IS_VARIANT;
		}

		_set_returns(should_returns);
	}

private:
	PropertyInfo _gen_return_type_info() const {
		return Derived::_gen_return_type_info_impl();
	}
};

// variadic, no return
template <typename T>
class MethodBindVarArgT : public MethodBindVarArgBase<MethodBindVarArgT<T>, T, void, false> {
	friend class MethodBindVarArgBase<MethodBindVarArgT<T>, T, void, false>;

public:
	virtual Variant call(Object *p_object, const Variant **p_args, int p_arg_count, Callable::CallError &r_error) const override {
#ifdef TOOLS_ENABLED
		ERR_FAIL_COND_V_MSG(p_object && p_object->is_extension_placeholder() && p_object->get_class_name() == MethodBind::get_instance_class(), Variant(), vformat("Cannot call method bind '%s' on placeholder instance.", MethodBind::get_name()));
#endif
		(static_cast<T *>(p_object)->*MethodBindVarArgBase<MethodBindVarArgT<T>, T, void, false>::method)(p_args, p_arg_count, r_error);
		return {};
	}

	MethodBindVarArgT(
			void (T::*p_method)(const Variant **, int, Callable::CallError &),
			const MethodInfo &p_method_info,
			bool p_return_nil_is_variant) :
			MethodBindVarArgBase<MethodBindVarArgT<T>, T, void, false>(p_method, p_method_info, p_return_nil_is_variant) {
	}

private:
	static PropertyInfo _gen_return_type_info_impl() {
		return {};
	}
};

template <typename T>
MethodBind *create_vararg_method_bind(void (T::*p_method)(const Variant **, int, Callable::CallError &), const MethodInfo &p_info, bool p_return_nil_is_variant) {
	MethodBind *a = memnew((MethodBindVarArgT<T>)(p_method, p_info, p_return_nil_is_variant));
	a->set_instance_class(T::get_class_static());
	return a;
}

// variadic, return
template <typename T, typename R>
class MethodBindVarArgTR : public MethodBindVarArgBase<MethodBindVarArgTR<T, R>, T, R, true> {
	friend class MethodBindVarArgBase<MethodBindVarArgTR<T, R>, T, R, true>;

public:
	GODOT_GCC_WARNING_PUSH_AND_IGNORE("-Wmaybe-uninitialized") // Workaround GH-66343 raised only with UBSAN, seems to be a false positive.

	virtual Variant call(Object *p_object, const Variant **p_args, int p_arg_count, Callable::CallError &r_error) const override {
#ifdef TOOLS_ENABLED
		ERR_FAIL_COND_V_MSG(p_object && p_object->is_extension_placeholder() && p_object->get_class_name() == MethodBind::get_instance_class(), Variant(), vformat("Cannot call method bind '%s' on placeholder instance.", MethodBind::get_name()));
#endif
		return (static_cast<T *>(p_object)->*MethodBindVarArgBase<MethodBindVarArgTR<T, R>, T, R, true>::method)(p_args, p_arg_count, r_error);
	}

	GODOT_GCC_WARNING_POP

	MethodBindVarArgTR(
			R (T::*p_method)(const Variant **, int, Callable::CallError &),
			const MethodInfo &p_info,
			bool p_return_nil_is_variant) :
			MethodBindVarArgBase<MethodBindVarArgTR<T, R>, T, R, true>(p_method, p_info, p_return_nil_is_variant) {
	}

private:
	static PropertyInfo _gen_return_type_info_impl() {
		return GetTypeInfo<R>::get_class_info();
	}
};

template <typename T, typename R>
MethodBind *create_vararg_method_bind(R (T::*p_method)(const Variant **, int, Callable::CallError &), const MethodInfo &p_info, bool p_return_nil_is_variant) {
	MethodBind *a = memnew((MethodBindVarArgTR<T, R>)(p_method, p_info, p_return_nil_is_variant));
	a->set_instance_class(T::get_class_static());
	return a;
}

/**** VARIADIC TEMPLATES ****/

#ifndef TYPED_METHOD_BIND
class __UnexistingClass;
#define CAST reinterpret_cast
#else
#define CAST static_cast
#endif

template <typename R, typename C, typename... Args>
struct _MethodInfoBase {
#ifdef TYPED_METHOD_BIND
	using ClassT = C;
#else
	using ClassT = __UnexistingClass;
#endif
	inline static constexpr bool has_ret_val = !std::is_same_v<R, void>;
	inline static constexpr int arg_count = sizeof...(Args);
	inline static constexpr Variant::Type (*_call_get_argument_type)(int) = call_get_argument_type<Args...>;
	inline static constexpr void (*_call_get_argument_type_info)(int, PropertyInfo &) = call_get_argument_type_info<Args...>;
#ifdef DEBUG_ENABLED
	inline static constexpr GodotTypeInfo::Metadata (*_call_get_argument_metadata)(int) = call_get_argument_metadata<Args...>;
#endif // DEBUG_ENABLED
};

template <typename>
struct _MethodInfo;

template <typename C, typename... Args>
struct _MethodInfo<void (C::*)(Args...)> : _MethodInfoBase<void, C, Args...> {
	using ClassT = typename _MethodInfoBase<void, C, Args...>::ClassT;
	using ActualClassT = C;
	using ReturnT = void;
	using MethodT = void (ClassT::*)(Args...);
	inline static constexpr bool is_const = false;
	inline static constexpr void (*_call_with_ptr_args)(ClassT *, MethodT, const void **) = call_with_ptr_args<ClassT, Args...>;
};

template <typename C, typename... Args>
struct _MethodInfo<void (C::*)(Args...) const> : _MethodInfoBase<void, C, Args...> {
	using ClassT = typename _MethodInfoBase<void, C, Args...>::ClassT;
	using ActualClassT = C;
	using ReturnT = void;
	using MethodT = void (ClassT::*)(Args...) const;
	inline static constexpr bool is_const = true;
	inline static constexpr void (*_call_with_ptr_argsc)(ClassT *, MethodT, const void **) = call_with_ptr_argsc<ClassT, Args...>;
};

template <typename R, typename C, typename... Args>
struct _MethodInfo<R (C::*)(Args...)> : _MethodInfoBase<R, C, Args...> {
	using ClassT = typename _MethodInfoBase<R, C, Args...>::ClassT;
	using ActualClassT = C;
	using ReturnT = R;
	using MethodT = R (ClassT::*)(Args...);
	inline static constexpr bool is_const = false;
	inline static constexpr void (*_call_with_ptr_args_ret)(ClassT *, MethodT, const void **, void *) = call_with_ptr_args_ret<ClassT, R, Args...>;
};

template <typename R, typename C, typename... Args>
struct _MethodInfo<R (C::*)(Args...) const> : _MethodInfoBase<R, C, Args...> {
	using ClassT = typename _MethodInfoBase<R, C, Args...>::ClassT;
	using ActualClassT = C;
	using ReturnT = R;
	using MethodT = R (ClassT::*)(Args...) const;
	inline static constexpr bool is_const = true;
	inline static constexpr void (*_call_with_ptr_args_retc)(ClassT *, MethodT, const void **, void *) = call_with_ptr_args_retc<ClassT, R, Args...>;
};

template <typename M>
class MethodBindT : public MethodBind {
	using MethodInfo = _MethodInfo<M>;
	using ClassT = typename MethodInfo::ClassT;
	using ReturnT = typename MethodInfo::ReturnT;
	M method;

protected:
	virtual Variant::Type _gen_argument_type(int p_arg) const override {
		if (p_arg >= 0 && p_arg < MethodInfo::arg_count) {
			return MethodInfo::_call_get_argument_type(p_arg);
		} else {
			if constexpr (MethodInfo::has_ret_val) {
				return GetTypeInfo<ReturnT>::VARIANT_TYPE;
			} else {
				return Variant::NIL;
			}
		}
	}

	virtual PropertyInfo _gen_argument_type_info(int p_arg) const override {
		if constexpr (MethodInfo::has_ret_val) {
			if (p_arg < 0 || p_arg >= MethodInfo::arg_count) {
				return GetTypeInfo<ReturnT>::get_class_info();
			}
		}
		PropertyInfo pi;
		MethodInfo::_call_get_argument_type_info(p_arg, pi);
		return pi;
	}

public:
#ifdef DEBUG_ENABLED
	virtual GodotTypeInfo::Metadata get_argument_meta(int p_arg) const override {
		if constexpr (MethodInfo::has_ret_val) {
			if (p_arg < 0) {
				return GetTypeInfo<ReturnT>::METADATA;
			}
		}
		return MethodInfo::_call_get_argument_metadata(p_arg);
	}

#endif // DEBUG_ENABLED
	virtual Variant call(Object *p_object, const Variant **p_args, int p_arg_count, Callable::CallError &r_error) const override {
#ifdef TOOLS_ENABLED
		ERR_FAIL_COND_V_MSG(p_object && p_object->is_extension_placeholder() && p_object->get_class_name() == get_instance_class(), Variant(), vformat("Cannot call method bind '%s' on placeholder instance.", MethodBind::get_name()));
#endif
		if constexpr (!MethodInfo::is_const && !MethodInfo::has_ret_val) {
			call_with_variant_args_dv(CAST<ClassT *>(p_object), method, p_args, p_arg_count, r_error, get_default_arguments());
		} else if constexpr (MethodInfo::is_const && !MethodInfo::has_ret_val) {
			call_with_variant_argsc_dv(CAST<ClassT *>(p_object), method, p_args, p_arg_count, r_error, get_default_arguments());
		} else if constexpr (!MethodInfo::is_const && MethodInfo::has_ret_val) {
			Variant ret;
			call_with_variant_args_ret_dv(CAST<ClassT *>(p_object), method, p_args, p_arg_count, ret, r_error, get_default_arguments());
			return ret;
		} else if constexpr (MethodInfo::is_const && MethodInfo::has_ret_val) {
			Variant ret;
			call_with_variant_args_retc_dv(CAST<ClassT *>(p_object), method, p_args, p_arg_count, ret, r_error, get_default_arguments());
			return ret;
		}

		return Variant();
	}

	virtual void validated_call(Object *p_object, const Variant **p_args, Variant *r_ret) const override {
#ifdef TOOLS_ENABLED
		ERR_FAIL_COND_MSG(p_object && p_object->is_extension_placeholder() && p_object->get_class_name() == get_instance_class(), vformat("Cannot call method bind '%s' on placeholder instance.", MethodBind::get_name()));
#endif
		if constexpr (!MethodInfo::is_const && !MethodInfo::has_ret_val) {
			call_with_validated_object_instance_args(CAST<ClassT *>(p_object), method, p_args);
		} else if constexpr (MethodInfo::is_const && !MethodInfo::has_ret_val) {
			call_with_validated_object_instance_argsc(CAST<ClassT *>(p_object), method, p_args);
		} else if constexpr (!MethodInfo::is_const && MethodInfo::has_ret_val) {
			call_with_validated_object_instance_args_ret(CAST<ClassT *>(p_object), method, p_args, r_ret);
		} else if constexpr (MethodInfo::is_const && MethodInfo::has_ret_val) {
			call_with_validated_object_instance_args_retc(CAST<ClassT *>(p_object), method, p_args, r_ret);
		}
	}

	virtual void ptrcall(Object *p_object, const void **p_args, void *r_ret) const override {
#ifdef TOOLS_ENABLED
		ERR_FAIL_COND_MSG(p_object && p_object->is_extension_placeholder() && p_object->get_class_name() == get_instance_class(), vformat("Cannot call method bind '%s' on placeholder instance.", MethodBind::get_name()));
#endif
		if constexpr (!MethodInfo::is_const && !MethodInfo::has_ret_val) {
			MethodInfo::_call_with_ptr_args(CAST<ClassT *>(p_object), method, p_args);
		} else if constexpr (MethodInfo::is_const && !MethodInfo::has_ret_val) {
			MethodInfo::_call_with_ptr_argsc(CAST<ClassT *>(p_object), method, p_args);
		} else if constexpr (!MethodInfo::is_const && MethodInfo::has_ret_val) {
			MethodInfo::_call_with_ptr_args_ret(CAST<ClassT *>(p_object), method, p_args, r_ret);
		} else if constexpr (MethodInfo::is_const && MethodInfo::has_ret_val) {
			MethodInfo::_call_with_ptr_args_retc(CAST<ClassT *>(p_object), method, p_args, r_ret);
		}
	}

	MethodBindT(M p_method) {
		method = p_method;
		if constexpr (MethodInfo::has_ret_val) {
			_set_returns(true);
		}
		if constexpr (MethodInfo::is_const) {
			_set_const(true);
		}
		_generate_argument_types(MethodInfo::arg_count);
		set_argument_count(MethodInfo::arg_count);
	}
};

template <typename M>
MethodBind *create_method_bind(M p_method) {
#ifdef TYPED_METHOD_BIND
	MethodBind *a = memnew((MethodBindT<M>)(p_method));
#else
	MethodBind *a = memnew((MethodBindT<M>)(reinterpret_cast<typename _MethodInfo<M>::MethodT>(p_method)));
#endif
	a->set_instance_class(_MethodInfo<M>::ActualClassT::get_class_static());
	return a;
}

#undef CAST

/* STATIC BINDS */

// no return

template <typename... P>
class MethodBindTS : public MethodBind {
	void (*function)(P...);

protected:
	virtual Variant::Type _gen_argument_type(int p_arg) const override {
		if (p_arg >= 0 && p_arg < (int)sizeof...(P)) {
			return call_get_argument_type<P...>(p_arg);
		} else {
			return Variant::NIL;
		}
	}

	virtual PropertyInfo _gen_argument_type_info(int p_arg) const override {
		PropertyInfo pi;
		call_get_argument_type_info<P...>(p_arg, pi);
		return pi;
	}

public:
#ifdef DEBUG_ENABLED
	virtual GodotTypeInfo::Metadata get_argument_meta(int p_arg) const override {
		return call_get_argument_metadata<P...>(p_arg);
	}

#endif // DEBUG_ENABLED
	virtual Variant call(Object *p_object, const Variant **p_args, int p_arg_count, Callable::CallError &r_error) const override {
		(void)p_object; // unused
		call_with_variant_args_static_dv(function, p_args, p_arg_count, r_error, get_default_arguments());
		return Variant();
	}

	virtual void validated_call(Object *p_object, const Variant **p_args, Variant *r_ret) const override {
		call_with_validated_variant_args_static_method(function, p_args);
	}

	virtual void ptrcall(Object *p_object, const void **p_args, void *r_ret) const override {
		(void)p_object;
		(void)r_ret;
		call_with_ptr_args_static_method(function, p_args);
	}

	MethodBindTS(void (*p_function)(P...)) {
		function = p_function;
		_generate_argument_types(sizeof...(P));
		set_argument_count(sizeof...(P));
		_set_static(true);
	}
};

template <typename... P>
MethodBind *create_static_method_bind(void (*p_method)(P...)) {
	MethodBind *a = memnew((MethodBindTS<P...>)(p_method));
	return a;
}

// return

template <typename R, typename... P>
class MethodBindTRS : public MethodBind {
	R (*function)(P...);

protected:
	virtual Variant::Type _gen_argument_type(int p_arg) const override {
		if (p_arg >= 0 && p_arg < (int)sizeof...(P)) {
			return call_get_argument_type<P...>(p_arg);
		} else {
			return GetTypeInfo<R>::VARIANT_TYPE;
		}
	}

	virtual PropertyInfo _gen_argument_type_info(int p_arg) const override {
		if (p_arg >= 0 && p_arg < (int)sizeof...(P)) {
			PropertyInfo pi;
			call_get_argument_type_info<P...>(p_arg, pi);
			return pi;
		} else {
			return GetTypeInfo<R>::get_class_info();
		}
	}

public:
#ifdef DEBUG_ENABLED
	virtual GodotTypeInfo::Metadata get_argument_meta(int p_arg) const override {
		if (p_arg >= 0) {
			return call_get_argument_metadata<P...>(p_arg);
		} else {
			return GetTypeInfo<R>::METADATA;
		}
	}

#endif // DEBUG_ENABLED
	virtual Variant call(Object *p_object, const Variant **p_args, int p_arg_count, Callable::CallError &r_error) const override {
		Variant ret;
		call_with_variant_args_static_ret_dv(function, p_args, p_arg_count, ret, r_error, get_default_arguments());
		return ret;
	}

	virtual void validated_call(Object *p_object, const Variant **p_args, Variant *r_ret) const override {
		call_with_validated_variant_args_static_method_ret(function, p_args, r_ret);
	}

	virtual void ptrcall(Object *p_object, const void **p_args, void *r_ret) const override {
		(void)p_object;
		call_with_ptr_args_static_method_ret(function, p_args, r_ret);
	}

	MethodBindTRS(R (*p_function)(P...)) {
		function = p_function;
		_generate_argument_types(sizeof...(P));
		set_argument_count(sizeof...(P));
		_set_static(true);
		_set_returns(true);
	}
};

template <typename R, typename... P>
MethodBind *create_static_method_bind(R (*p_method)(P...)) {
	MethodBind *a = memnew((MethodBindTRS<R, P...>)(p_method));
	return a;
}
