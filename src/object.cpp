#include <andy/lang/object.hpp>

#include <andy/lang/class.hpp>
#include <andy/lang/method.hpp>
#include <andy/lang/interpreter.hpp>

#include <andy/console.hpp>

andy::lang::object::object(std::shared_ptr<andy::lang::structure> c)
    : cls(c)
{
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
    auto self = shared_from_this();
    for(auto& instance_variable : cls->instance_variables) {
        if(instance_variable.second) {
            instance_variables[instance_variable.first] = std::make_shared<andy::lang::object>(interpreter->NullClass);
            interpreter->execute(*instance_variable.second, self);
        }
    }
}

void andy::lang::object::initialize(andy::lang::interpreter *interpreter, andy::lang::function_call new_call)
{
    initialize(interpreter);

    auto new_it = cls->instance_methods.find("new");

    if(new_it == cls->instance_methods.end()) {
        // default constructor
        if(new_call.positional_params.size() || new_call.named_params.size()) {
            throw std::runtime_error("Default constructor does not accept parameters in class " + std::string(cls->name));
        }
        if(cls->base) {
            // call the base class constructor
            auto base_new_it = cls->base->instance_methods.find("new");
            if(base_new_it != cls->base->instance_methods.end()) {
                base_instance = std::make_shared<object>(cls->base);
                base_instance->derived_instance = shared_from_this();
                base_instance->initialize(interpreter, new_call);
            }
        }
    } else {
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
    andy::console::log_debug("{}#{} native destructor", cls->name, (void*)this);
}

bool andy::lang::object::is_present() const
{
    if(!cls) {
        throw std::runtime_error("object has no class");
    }
    
    auto it = cls->instance_methods.find("present?");

    if(it == cls->instance_methods.end()) {
        throw std::runtime_error("present? is not defined in class " + std::string(cls->name));
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