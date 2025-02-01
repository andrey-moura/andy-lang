#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>

#include "add_operators.hpp"

std::shared_ptr<andy::lang::structure> andy::lang::integer_class::create(interpreter* interpreter)
{
    std::shared_ptr<andy::lang::structure> IntegerClass = std::make_shared<andy::lang::structure>("IntegerClass");
    IntegerClass->object_to_var = [](std::shared_ptr<const andy::lang::object> obj) {
        return var(obj->as<int>());
    };

    IntegerClass->methods = {
        {"present?", andy::lang::method("present?", method_storage_type::instance_method, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            int i = object->as<int>();
            
            if(i == 0) {
                return std::make_shared<andy::lang::object>(interpreter->FalseClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->TrueClass);
        })},
        {"to_string", andy::lang::method("to_string", method_storage_type::instance_method, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            int value = object->as<int>();

            return andy::lang::object::instantiate(interpreter, interpreter->StringClass, std::move(std::to_string(value)));
        })}
    };

    andy::lang::add_operators<int>(IntegerClass, interpreter);

    return IntegerClass;
}