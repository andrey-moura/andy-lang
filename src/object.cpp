#include <andy/lang/object.hpp>

#include <andy/lang/class.hpp>
#include <andy/lang/function.hpp>
#include <andy/lang/interpreter.hpp>

#include <andy/console.hpp>

andy::lang::object::object(std::shared_ptr<andy::lang::structure> c)
{
    cls = c;

    if(cls) {
        andy::console::log_debug("{}#{} created", cls->name, (void*)this);
    }
}

andy::lang::object::~object()
{
    if(cls) {
        if(native_destructor) {
            native_destructor(this);
        }

        andy::console::log_debug("{}#{} destroyed", cls->name, (void*)this);
    }
}

void andy::lang::object::initialize(andy::lang::interpreter* interpreter)
{
    self = shared_from_this().get();
    for(auto& instance_variable : cls->instance_variables) {
        if(instance_variable.second) {
            variables[instance_variable.first] = andy::lang::object::create(interpreter, interpreter->NullClass);
            interpreter->execute(*instance_variable.second);
        }
    }
    functions = cls->instance_functions;
    inline_functions = cls->instance_inline_functions;
}

void andy::lang::object::initialize(andy::lang::interpreter *interpreter, andy::lang::function_call new_call)
{
    initialize(interpreter);
}

void andy::lang::object::log_native_destructor()
{
    andy::console::log_debug("{}#{} native destructor", cls->name, (void*)this);
}