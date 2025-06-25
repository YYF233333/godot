/**************************************************************************/
/*  method_ptrcall.cpp                                                    */
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

#include "method_ptrcall.h"

#include "core/math/face3.h"

Vector<Face3> PtrToArg<Vector<Face3>>::convert(const void *p_ptr) {
	const Vector<Vector3> *dvs = reinterpret_cast<const Vector<Vector3> *>(p_ptr);
	Vector<Face3> ret;
	int len = dvs->size() / 3;
	ret.resize(len);
	{
		const Vector3 *r = dvs->ptr();
		Face3 *w = ret.ptrw();
		for (int i = 0; i < len; i++) {
			w[i].vertex[0] = r[i * 3 + 0];
			w[i].vertex[1] = r[i * 3 + 1];
			w[i].vertex[2] = r[i * 3 + 2];
		}
	}
	return ret;
}

void PtrToArg<Vector<Face3>>::encode(const Vector<Face3> &p_vec, void *p_ptr) {
	Vector<Vector3> *arr = reinterpret_cast<Vector<Vector3> *>(p_ptr);
	int len = p_vec.size();
	arr->resize(len * 3);
	{
		const Face3 *r = p_vec.ptr();
		Vector3 *w = arr->ptrw();
		for (int i = 0; i < len; i++) {
			w[i * 3 + 0] = r[i].vertex[0];
			w[i * 3 + 1] = r[i].vertex[1];
			w[i * 3 + 2] = r[i].vertex[2];
		}
	}
}
