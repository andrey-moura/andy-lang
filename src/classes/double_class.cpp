#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>

#include "add_operators.hpp"

std::shared_ptr<andy::lang::structure> create_double_class(andy::lang::interpreter* interpreter)
{
    std::shared_ptr<andy::lang::structure> DoubleClass = std::make_shared<andy::lang::structure>("Double");

        DoubleClass->instance_functions["present?"] = std::make_shared<andy::lang::function>("present?", andy::lang::function_storage_type::instance_function, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            double i = object->as<double>();
            
            if(i == 0) {
                return std::make_shared<andy::lang::object>(interpreter->FalseClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->TrueClass);
        });

    DoubleClass->instance_functions["to_string"] = std::make_shared<andy::lang::function>("to_string", andy::lang::function_storage_type::instance_function, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            double value = object->as<double>();

            return andy::lang::object::instantiate(interpreter, interpreter->StringClass, std::move(std::to_string(value)));
        });


    andy::lang::add_operators<double>(DoubleClass, interpreter);

    return DoubleClass;
}