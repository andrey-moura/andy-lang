#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>

std::shared_ptr<andy::lang::structure> create_true_class(andy::lang::interpreter* interpreter)
{
    auto TrueClass = std::make_shared<andy::lang::structure>("True");

    TrueClass->functions["present?"] = std::make_shared<andy::lang::function>("present?", andy::lang::function_storage_type::instance_function, [TrueClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        return std::make_shared<andy::lang::object>(TrueClass);
    });
    
    TrueClass->functions["=="] = std::make_shared<andy::lang::function>("==", andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"other"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        return params[0]->cls == interpreter->TrueClass ? std::make_shared<andy::lang::object>(interpreter->TrueClass) : std::make_shared<andy::lang::object>(interpreter->FalseClass);
    });
    
    TrueClass->functions["||"] = std::make_shared<andy::lang::function>("||", andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"other"}, [TrueClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        return std::make_shared<andy::lang::object>(TrueClass);
    });
    
    TrueClass->functions["!"] = std::make_shared<andy::lang::function>("!", andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        return std::make_shared<andy::lang::object>(interpreter->FalseClass);
    });
    
    TrueClass->functions["&&"] = std::make_shared<andy::lang::function>("&&", andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"other"}, [TrueClass, interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        std::shared_ptr<andy::lang::object> other = params[0];
        if(other->is_present()) {
            return std::make_shared<andy::lang::object>(TrueClass);
        } else {
            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        }
    });
    
    return TrueClass;
}