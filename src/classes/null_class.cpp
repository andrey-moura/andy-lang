#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>

std::shared_ptr<andy::lang::structure> create_null_class(andy::lang::interpreter* interpreter)
{
    auto NullClass = std::make_shared<andy::lang::structure>("Null");

    NullClass->instance_methods = {
        {"present?", andy::lang::method("present?", andy::lang::method_storage_type::instance_method, [interpreter, NullClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            return std::make_shared<andy::lang::object>( interpreter->FalseClass );
        })},
        {"to_string", andy::lang::method("to_string", andy::lang::method_storage_type::instance_method, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string str = "null";
            return andy::lang::object::instantiate( interpreter, interpreter->StringClass, std::move(str) );
        })},
        {"==", andy::lang::method("==", andy::lang::method_storage_type::instance_method, {"other"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            if(params[0]->cls == interpreter->NullClass) {
                return std::make_shared<andy::lang::object>( interpreter->TrueClass );
            } else {
                return std::make_shared<andy::lang::object>( interpreter->FalseClass );
            }
        })},
    };
    
    return NullClass;
}