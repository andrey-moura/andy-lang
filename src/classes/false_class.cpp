#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>
#include <andy/lang/error.hpp>
#include <andy/lang/api.hpp>

std::shared_ptr<andy::lang::structure> create_false_class(andy::lang::interpreter* interpreter)
{
    auto FalseClass = std::make_shared<andy::lang::structure>("False");

        FalseClass->instance_functions["present?"] = std::make_shared<andy::lang::function>("present?", [FalseClass](andy::lang::interpreter* interpreter) {
                return std::make_shared<andy::lang::object>(FalseClass);
    });

        FalseClass->instance_functions["to_string"] = std::make_shared<andy::lang::function>("to_string", [](andy::lang::interpreter* interpreter) {
                return andy::lang::api::to_object(interpreter, "false");
    });

    FalseClass->instance_functions["||"] = std::make_shared<andy::lang::function>("||", std::initializer_list<std::string>{"other"}, [FalseClass](andy::lang::interpreter* interpreter) {
        auto other = interpreter->current_context->positional_params[0];
        //return other->cls->instance_functions["present?"]->call(other);
        andy::lang::error::internal("Temporary disabled code reached at " + std::string(__FILE__) + ":" + std::to_string(__LINE__));

        return std::make_shared<andy::lang::object>(FalseClass);
    });
    
        FalseClass->instance_functions["!"] = std::make_shared<andy::lang::function>("!", [](andy::lang::interpreter* interpreter) {
                return std::make_shared<andy::lang::object>(interpreter->TrueClass);
    });
    
        FalseClass->instance_functions["=="] = std::make_shared<andy::lang::function>("==", std::initializer_list<std::string>{"other"}, [FalseClass](andy::lang::interpreter* interpreter) {
                auto other = interpreter->current_context->positional_params[0];
        return std::make_shared<andy::lang::object>(other->cls == FalseClass ? interpreter->TrueClass : interpreter->FalseClass);
    });
    
    return FalseClass;
}