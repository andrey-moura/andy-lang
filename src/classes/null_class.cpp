#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>

std::shared_ptr<andy::lang::structure> create_null_class(andy::lang::interpreter* interpreter)
{
    auto NullClass = std::make_shared<andy::lang::structure>("Null");

    NullClass->instance_functions["present?"] = std::make_shared<andy::lang::function>("present?", [NullClass](andy::lang::interpreter* interpreter) {
        return std::make_shared<andy::lang::object>( interpreter->FalseClass );
    });
    
    NullClass->instance_functions["to_string"] = std::make_shared<andy::lang::function>("to_string", [](andy::lang::interpreter* interpreter) {
        std::string str = "null";
        return andy::lang::object::instantiate( interpreter, interpreter->StringClass, std::move(str) );
    });
    
    NullClass->instance_functions["=="] = std::make_shared<andy::lang::function>("==", std::initializer_list<std::string>{"other"}, [](andy::lang::interpreter* interpreter) {
        if(interpreter->current_context->positional_params[0]->cls == interpreter->NullClass) {
            return std::make_shared<andy::lang::object>( interpreter->TrueClass );
        } else {
            return std::make_shared<andy::lang::object>( interpreter->FalseClass );
        }
    });
    
    return NullClass;
}