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
            return params[0]->cls->instance_methods["present?"].function(params[0], {}, {});
        })},
        {"!", andy::lang::method("!", andy::lang::method_storage_type::instance_method, {}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            return std::make_shared<andy::lang::object>(interpreter->TrueClass);
        })},
    };
    
    return FalseClass;
}