#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>

std::shared_ptr<andy::lang::structure> create_false_class(andy::lang::interpreter* interpreter)
{
    auto FalseClass = std::make_shared<andy::lang::structure>("False");

    FalseClass->functions["present?"] = std::make_shared<andy::lang::function>("present?", andy::lang::function_storage_type::instance_function, [FalseClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        return std::make_shared<andy::lang::object>(FalseClass);
    });
    
    FalseClass->functions["||"] = std::make_shared<andy::lang::function>("||" ,andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"other"}, [FalseClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        andy::lang::function_call call = {
            "present?",
            FalseClass,
            object,
            object->cls->functions["present?"].get(),
            {}
        };
        return params[0]->cls->functions["present?"]->native_function(call);
    });
    
    FalseClass->functions["!"] = std::make_shared<andy::lang::function>("!", andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        return std::make_shared<andy::lang::object>(interpreter->TrueClass);
    });
    
    FalseClass->functions["=="] = std::make_shared<andy::lang::function>("==", andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"other"}, [interpreter, FalseClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        auto other = params[0];
        return std::make_shared<andy::lang::object>(other->cls == FalseClass ? interpreter->TrueClass : interpreter->FalseClass);
    });
    
    return FalseClass;
}