#include <andy/lang/object.hpp>

#include <andy/lang/class.hpp>
#include <andy/lang/method.hpp>
#include <andy/lang/interpreter.hpp>

#include <uva/console.hpp>

andy::lang::object::object(std::shared_ptr<andy::lang::structure> c)
    : cls(c)
{
    if(cls) {
        uva::console::log_debug("{}#{} created", cls->name, (void*)this);
    }
}

andy::lang::object::~object()
{
    if(cls) {
        if(native_destructor) {
            native_destructor(this);
        }

        uva::console::log_debug("{}#{} destroyed", cls->name, (void*)this);
    }
}

void andy::lang::object::initialize(andy::lang::interpreter *interpreter, andy::lang::function_call new_call)
{
    for(auto& instance_variable : cls->instance_variables) {
        instance_variables[instance_variable.first] = andy::lang::object::instantiate(interpreter, instance_variable.second, nullptr);
    }

    instance_variables["this"] = shared_from_this();

    //if(cls->base) {
        //base_instance = andy::lang::object::instantiate(interpreter, cls->base, nullptr);
    //     base_instance = std::make_shared<andy::lang::object>(cls->base);
    //     base_instance->derived_instance = shared_from_this();
    //     base_instance->initialize(interpreter);
    // }

    auto new_it = cls->instance_methods.find("new");

    if(new_it != cls->instance_methods.end()) {
        auto new_it = cls->instance_methods.find("new");

        if(new_it != cls->instance_methods.end()) {
            new_call.name = "new";
            new_call.cls = cls;
            new_call.object = shared_from_this();
            new_call.method = &new_it->second;

            interpreter->call(new_call);
        }
    }
}

void andy::lang::object::log_native_destructor()
{
    uva::console::log_debug("{}#{} native destructor", cls->name, (void*)this);
}

bool andy::lang::object::is_present() const
{
    if(!cls) {
        throw std::runtime_error("object has no class");
    }
    
    auto it = cls->instance_methods.find("present?");

    if(it == cls->instance_methods.end()) {
        throw std::runtime_error("present? is not defined in class " + cls->name);
    } else {
        auto this_without_const = const_cast<object*>(this);

        auto obj = it->second.call( this_without_const->shared_from_this() );

        if(obj->cls->name == "True") {
            return true;
        } else {
            return false;
        }
    }
}

var andy::lang::object::to_var() const
{
    if(cls->object_to_var) {
        return cls->object_to_var(shared_from_this());
    }

    return std::format("{}#{}", cls->name, (void*)this);
}
