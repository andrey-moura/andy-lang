#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>
#include <andy/lang/api.hpp>
#include <andy/lang/error.hpp>

std::shared_ptr<andy::lang::structure> create_true_class(andy::lang::interpreter* interpreter)
{
    auto TrueClass = std::make_shared<andy::lang::structure>("True");

        TrueClass->instance_functions["to_string"] = std::make_shared<andy::lang::function>("to_string", [](andy::lang::interpreter* interpreter) {
                return andy::lang::api::to_object(interpreter, "true");
    });

        TrueClass->instance_functions["present?"] = std::make_shared<andy::lang::function>("present?", [TrueClass](andy::lang::interpreter* interpreter) {
                return std::make_shared<andy::lang::object>(TrueClass);
    });
    
        TrueClass->instance_functions["=="] = std::make_shared<andy::lang::function>("==", std::initializer_list<std::string>{"other"}, [](andy::lang::interpreter* interpreter) {
                return interpreter->current_context->positional_params[0]->cls == interpreter->TrueClass ? std::make_shared<andy::lang::object>(interpreter->TrueClass) : std::make_shared<andy::lang::object>(interpreter->FalseClass);
    });
    
        TrueClass->instance_functions["||"] = std::make_shared<andy::lang::function>("||", std::initializer_list<std::string>{"other"}, [TrueClass](andy::lang::interpreter* interpreter) {
                return std::make_shared<andy::lang::object>(TrueClass);
    });
    
        TrueClass->instance_functions["!"] = std::make_shared<andy::lang::function>("!", [](andy::lang::interpreter* interpreter) {
                return std::make_shared<andy::lang::object>(interpreter->FalseClass);
    });
    
        TrueClass->instance_functions["&&"] = std::make_shared<andy::lang::function>("&&", std::initializer_list<std::string>{"other"}, [TrueClass](andy::lang::interpreter* interpreter) {
                std::shared_ptr<andy::lang::object> other = interpreter->current_context->positional_params[0];
        // if(other->is_truthy()) {
        //     return std::make_shared<andy::lang::object>(TrueClass);
        // } else {
        //     return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        // }
        andy::lang::error::internal("Temporary disabled code reached at " + std::string(__FILE__) + ":" + std::to_string(__LINE__));

        return std::make_shared<andy::lang::object>(TrueClass);
    });
    
    return TrueClass;
}