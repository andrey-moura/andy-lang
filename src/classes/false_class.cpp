#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>

std::shared_ptr<andy::lang::structure> create_false_class(andy::lang::interpreter* interpreter)
{
    auto FalseClass = std::make_shared<andy::lang::structure>("False");

    FalseClass->instance_functions = {
        {"present?", andy::lang::function("present?", andy::lang::function_storage_type::instance_function, [FalseClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            return std::make_shared<andy::lang::object>(FalseClass);
        })},
        {"||", andy::lang::function("||" ,andy::lang::function_storage_type::instance_function, {"other"}, [FalseClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            andy::lang::function_call call = {
                "present?",
                FalseClass,
                object,
                &object->cls->instance_functions["present?"],
                {}
            };
            return params[0]->cls->instance_functions["present?"].native_function(call);
        })},
        {"!", andy::lang::function("!", andy::lang::function_storage_type::instance_function, {}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            return std::make_shared<andy::lang::object>(interpreter->TrueClass);
        })},
        {"==", andy::lang::function("==", andy::lang::function_storage_type::instance_function, {"other"}, [interpreter, FalseClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            auto other = params[0];
            return std::make_shared<andy::lang::object>(other->cls == FalseClass ? interpreter->TrueClass : interpreter->FalseClass);
        })},

    };
    
    return FalseClass;
}