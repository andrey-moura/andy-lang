#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>

std::shared_ptr<andy::lang::structure> create_false_class(andy::lang::interpreter* interpreter)
{
    auto FalseClass = std::make_shared<andy::lang::structure>("False");

    FalseClass->instance_methods = {
        {"present?", andy::lang::method("present?", andy::lang::method_storage_type::instance_method, [FalseClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            return std::make_shared<andy::lang::object>(FalseClass);
        })},
        {"||", andy::lang::method("||" ,andy::lang::method_storage_type::instance_method, {"other"}, [FalseClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            andy::lang::function_call call = {
                "present?",
                FalseClass,
                object,
                object->cls->instance_methods["present?"],
                {}
            };
            return params[0]->cls->instance_methods["present?"].function(call);
        })},
        {"!", andy::lang::method("!", andy::lang::method_storage_type::instance_method, {}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            return std::make_shared<andy::lang::object>(interpreter->TrueClass);
        })},
        {"==", andy::lang::method("==", andy::lang::method_storage_type::instance_method, {"other"}, [interpreter, FalseClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            auto other = params[0];
            return std::make_shared<andy::lang::object>(other->cls == FalseClass ? interpreter->TrueClass : interpreter->FalseClass);
        })},

    };
    
    return FalseClass;
}