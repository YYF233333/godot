/**************************************************************************/
/*  test_gdscript2_signal.cpp                                             */
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

#include "modules/gdscript2/runtime/gdscript2_signal.h"

namespace TestGDScript2Signal {

TEST_CASE("[Modules][GDScript2] Signal - Registry creation") {
	Ref<GDScript2SignalRegistry> registry;
	registry.instantiate();

	CHECK(registry.is_valid());
	CHECK(registry->get_signal_list().is_empty());
}

TEST_CASE("[Modules][GDScript2] Signal - Define signal") {
	Ref<GDScript2SignalRegistry> registry;
	registry.instantiate();

	registry->define_signal("test_signal");

	CHECK(registry->has_signal("test_signal"));
	CHECK(registry->get_signal_list().size() == 1);
	CHECK(registry->get_signal_list()[0] == "test_signal");
}

TEST_CASE("[Modules][GDScript2] Signal - Define signal with parameters") {
	Ref<GDScript2SignalRegistry> registry;
	registry.instantiate();

	Vector<GDScript2SignalParam> params;
	params.push_back(GDScript2SignalParam("value", Variant::INT));
	params.push_back(GDScript2SignalParam("name", Variant::STRING));

	registry->define_signal_with_params("parameterized_signal", params);

	CHECK(registry->has_signal("parameterized_signal"));

	GDScript2SignalDefinition def = registry->get_signal_definition("parameterized_signal");
	CHECK(def.name == "parameterized_signal");
	CHECK(def.get_parameter_count() == 2);
	CHECK(def.parameters[0].name == "value");
	CHECK(def.parameters[0].type == Variant::INT);
	CHECK(def.parameters[1].name == "name");
	CHECK(def.parameters[1].type == Variant::STRING);
}

TEST_CASE("[Modules][GDScript2] Signal - Connect and disconnect") {
	Ref<GDScript2SignalRegistry> registry;
	registry.instantiate();

	Object *test_obj = memnew(Object);
	Callable test_callable = Callable(test_obj, "test_method");

	registry->define_signal("test_signal");

	// Connect
	Error err = registry->connect_signal("test_signal", test_callable);
	CHECK(err == OK);
	CHECK(registry->is_connected("test_signal", test_callable));
	CHECK(registry->get_connection_count("test_signal") == 1);

	// Disconnect
	registry->disconnect_signal("test_signal", test_callable);
	CHECK(!registry->is_connected("test_signal", test_callable));
	CHECK(registry->get_connection_count("test_signal") == 0);

	memdelete(test_obj);
}

TEST_CASE("[Modules][GDScript2] Signal - Multiple connections") {
	Ref<GDScript2SignalRegistry> registry;
	registry.instantiate();

	Object *obj1 = memnew(Object);
	Object *obj2 = memnew(Object);
	Object *obj3 = memnew(Object);

	Callable callable1 = Callable(obj1, "method1");
	Callable callable2 = Callable(obj2, "method2");
	Callable callable3 = Callable(obj3, "method3");

	registry->define_signal("multi_signal");

	registry->connect_signal("multi_signal", callable1);
	registry->connect_signal("multi_signal", callable2);
	registry->connect_signal("multi_signal", callable3);

	CHECK(registry->get_connection_count("multi_signal") == 3);
	CHECK(registry->is_connected("multi_signal", callable1));
	CHECK(registry->is_connected("multi_signal", callable2));
	CHECK(registry->is_connected("multi_signal", callable3));

	// Disconnect one
	registry->disconnect_signal("multi_signal", callable2);
	CHECK(registry->get_connection_count("multi_signal") == 2);
	CHECK(!registry->is_connected("multi_signal", callable2));

	// Disconnect all
	registry->disconnect_all("multi_signal");
	CHECK(registry->get_connection_count("multi_signal") == 0);

	memdelete(obj1);
	memdelete(obj2);
	memdelete(obj3);
}

TEST_CASE("[Modules][GDScript2] Signal - Emit signal") {
	Ref<GDScript2SignalRegistry> registry;
	registry.instantiate();

	Object *test_obj = memnew(Object);
	test_obj->set_meta("call_count", 0);

	// Create a lambda that increments call count
	Callable test_callable = callable_mp_lambda(test_obj, [test_obj](const Variant **p_args, int p_argcount) {
		int count = test_obj->get_meta("call_count");
		test_obj->set_meta("call_count", count + 1);
	});

	registry->define_signal("emit_test");
	registry->connect_signal("emit_test", test_callable);

	// Emit signal
	registry->emit_signal("emit_test");

	// Check that callback was called
	int call_count = test_obj->get_meta("call_count");
	CHECK(call_count == 1);

	// Emit again
	registry->emit_signal("emit_test");
	call_count = test_obj->get_meta("call_count");
	CHECK(call_count == 2);

	memdelete(test_obj);
}

TEST_CASE("[Modules][GDScript2] Signal - Emit with arguments") {
	Ref<GDScript2SignalRegistry> registry;
	registry.instantiate();

	Object *test_obj = memnew(Object);
	test_obj->set_meta("received_value", 0);

	// Create a lambda that stores the received value
	Callable test_callable = callable_mp_lambda(test_obj, [test_obj](const Variant **p_args, int p_argcount) {
		if (p_argcount > 0) {
			test_obj->set_meta("received_value", *p_args[0]);
		}
	});

	registry->define_signal("value_signal");
	registry->connect_signal("value_signal", test_callable);

	// Emit with argument
	Vector<Variant> args;
	args.push_back(42);
	registry->emit_signal("value_signal", args);

	int received = test_obj->get_meta("received_value");
	CHECK(received == 42);

	memdelete(test_obj);
}

TEST_CASE("[Modules][GDScript2] Signal - SignalEmitter basic") {
	Ref<GDScript2SignalEmitter> emitter;
	emitter.instantiate();

	emitter->_define_signal("test_signal");

	CHECK(emitter->has_signal("test_signal"));
	CHECK(emitter->get_signal_registry().is_valid());
}

TEST_CASE("[Modules][GDScript2] Signal - SignalEmitter with parameters") {
	Ref<GDScript2SignalEmitter> emitter;
	emitter.instantiate();

	Vector<String> param_names;
	param_names.push_back("value");
	param_names.push_back("name");

	emitter->_define_signal_with_params("parameterized", param_names);

	CHECK(emitter->has_signal("parameterized"));

	Ref<GDScript2SignalRegistry> registry = emitter->get_signal_registry();
	GDScript2SignalDefinition def = registry->get_signal_definition("parameterized");
	CHECK(def.get_parameter_count() == 2);
}

TEST_CASE("[Modules][GDScript2] Signal - SignalEmitter connect and emit") {
	Ref<GDScript2SignalEmitter> emitter;
	emitter.instantiate();

	Object *test_obj = memnew(Object);
	test_obj->set_meta("called", false);

	Callable test_callable = callable_mp_lambda(test_obj, [test_obj](const Variant **p_args, int p_argcount) {
		test_obj->set_meta("called", true);
	});

	emitter->_define_signal("action");
	emitter->connect_to_signal("action", test_callable);

	CHECK(emitter->is_connected("action", test_callable));

	// Emit
	emitter->emit("action");

	bool called = test_obj->get_meta("called");
	CHECK(called);

	memdelete(test_obj);
}

TEST_CASE("[Modules][GDScript2] Signal - Utils make_signal") {
	Object *test_obj = memnew(Object);
	test_obj->add_user_signal(MethodInfo("test_signal"));

	Signal sig = GDScript2SignalUtils::make_signal(test_obj, "test_signal");

	CHECK(sig.get_object() == test_obj);
	CHECK(sig.get_name() == "test_signal");

	memdelete(test_obj);
}

TEST_CASE("[Modules][GDScript2] Signal - Utils is_signal") {
	Object *test_obj = memnew(Object);
	test_obj->add_user_signal(MethodInfo("test_signal"));

	Signal sig = GDScript2SignalUtils::make_signal(test_obj, "test_signal");
	Variant sig_var = sig;

	CHECK(GDScript2SignalUtils::is_signal(sig_var));
	CHECK(!GDScript2SignalUtils::is_signal(Variant(42)));
	CHECK(!GDScript2SignalUtils::is_signal(Variant("string")));

	memdelete(test_obj);
}

TEST_CASE("[Modules][GDScript2] Signal - Utils safe_connect") {
	Object *test_obj = memnew(Object);
	Object *target_obj = memnew(Object);

	test_obj->add_user_signal(MethodInfo("test_signal"));
	Signal sig = GDScript2SignalUtils::make_signal(test_obj, "test_signal");

	Callable callable = Callable(target_obj, "test_method");

	Error err = GDScript2SignalUtils::safe_connect(sig, callable);
	CHECK(err == OK);
	CHECK(test_obj->is_connected("test_signal", callable));

	memdelete(test_obj);
	memdelete(target_obj);
}

TEST_CASE("[Modules][GDScript2] Signal - Utils safe_disconnect") {
	Object *test_obj = memnew(Object);
	Object *target_obj = memnew(Object);

	test_obj->add_user_signal(MethodInfo("test_signal"));
	Signal sig = GDScript2SignalUtils::make_signal(test_obj, "test_signal");

	Callable callable = Callable(target_obj, "test_method");

	test_obj->connect("test_signal", callable);
	CHECK(test_obj->is_connected("test_signal", callable));

	Error err = GDScript2SignalUtils::safe_disconnect(sig, callable);
	CHECK(err == OK);
	CHECK(!test_obj->is_connected("test_signal", callable));

	memdelete(test_obj);
	memdelete(target_obj);
}

TEST_CASE("[Modules][GDScript2] Signal - Clear all connections") {
	Ref<GDScript2SignalRegistry> registry;
	registry.instantiate();

	Object *obj1 = memnew(Object);
	Object *obj2 = memnew(Object);

	registry->define_signal("sig1");
	registry->define_signal("sig2");

	registry->connect_signal("sig1", Callable(obj1, "method1"));
	registry->connect_signal("sig2", Callable(obj2, "method2"));

	CHECK(registry->get_connection_count("sig1") == 1);
	CHECK(registry->get_connection_count("sig2") == 1);

	registry->clear_all_connections();

	CHECK(registry->get_connection_count("sig1") == 0);
	CHECK(registry->get_connection_count("sig2") == 0);

	memdelete(obj1);
	memdelete(obj2);
}

TEST_CASE("[Modules][GDScript2] Signal - Get connections list") {
	Ref<GDScript2SignalRegistry> registry;
	registry.instantiate();

	Object *obj1 = memnew(Object);
	Object *obj2 = memnew(Object);

	Callable callable1 = Callable(obj1, "method1");
	Callable callable2 = Callable(obj2, "method2");

	registry->define_signal("test_signal");
	registry->connect_signal("test_signal", callable1);
	registry->connect_signal("test_signal", callable2);

	Vector<Callable> connections = registry->get_connections("test_signal");
	CHECK(connections.size() == 2);
	CHECK(connections.has(callable1));
	CHECK(connections.has(callable2));

	memdelete(obj1);
	memdelete(obj2);
}

} // namespace TestGDScript2Signal
