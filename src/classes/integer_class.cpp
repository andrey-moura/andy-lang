#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>

#include "add_operators.hpp"

std::shared_ptr<andy::lang::structure> create_integer_class(andy::lang::interpreter* interpreter)
{
    std::shared_ptr<andy::lang::structure> IntegerClass = std::make_shared<andy::lang::structure>("Integer");

    IntegerClass->instance_functions["present?"] = std::make_shared<andy::lang::function>("present?", [](andy::lang::interpreter* interpreter) {
        int i = interpreter->current_context->self->as<int>();
        
        if(i == 0) {
            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        }

        return std::make_shared<andy::lang::object>(interpreter->TrueClass);
    });

    IntegerClass->instance_functions["to_string"] = std::make_shared<andy::lang::function>("to_string", [](andy::lang::interpreter* interpreter) {
        int value = interpreter->current_context->self->as<int>();

        return andy::lang::object::instantiate(interpreter, interpreter->StringClass, std::move(std::to_string(value)));
    });

    IntegerClass->instance_functions["positive?"] = std::make_shared<andy::lang::function>("positive?", [](andy::lang::interpreter* interpreter) {
        int value = interpreter->current_context->self->as<int>();

        if(value == 0) {
            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        }

        return std::make_shared<andy::lang::object>(interpreter->TrueClass);
    });

    andy::lang::add_operators<int>(IntegerClass, interpreter);

    return IntegerClass;
}