#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>

#include "add_operators.hpp"

std::shared_ptr<andy::lang::structure> create_float_class(andy::lang::interpreter* interpreter)
{
    std::shared_ptr<andy::lang::structure> FloatClass = std::make_shared<andy::lang::structure>("Float");

        FloatClass->instance_functions["present?"] = std::make_shared<andy::lang::function>("present?", [](andy::lang::interpreter* interpreter) {
            float i = interpreter->current_context->self->as<float>();
            
            if(i == 0) {
                return std::make_shared<andy::lang::object>(interpreter->FalseClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->TrueClass);
        });

        FloatClass->instance_functions["to_string"] = std::make_shared<andy::lang::function>("to_string", [](andy::lang::interpreter* interpreter) {
            float value = interpreter->current_context->self->as<float>();

            return andy::lang::object::instantiate(interpreter, interpreter->StringClass, std::move(std::to_string(value)));
        });


    andy::lang::add_operators<float>(FloatClass, interpreter);

    return FloatClass;
}