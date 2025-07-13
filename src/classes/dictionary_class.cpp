#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>

std::shared_ptr<andy::lang::structure> create_dictionary_class(andy::lang::interpreter* interpreter)
{
    auto DictionaryClass = std::make_shared<andy::lang::structure>("Dictionary");

    DictionaryClass->instance_methods = {
        {"present?", andy::lang::method("present?",andy::lang::method_storage_type::instance_method, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& value = object->as<std::string>();

            if(value.empty()) {
                return std::make_shared<andy::lang::object>(interpreter->FalseClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->TrueClass);
        })},
        {"[]", andy::lang::method("[]",andy::lang::method_storage_type::instance_method, {"key"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::shared_ptr<andy::lang::object> key = params[0];

            auto& dictionary = object->as<andy::lang::dictionary>();

            auto operator_it = key->cls->instance_methods.find("==");

            for(auto& pair : dictionary) {
                andy::lang::function_call __call = {
                    "==",
                    key->cls,
                    key,
                    &operator_it->second,
                    { pair.first }
                };
                auto result = interpreter->call(__call);
                if(result->cls == interpreter->TrueClass) {
                    return pair.second;
                }
            }

            return std::make_shared<andy::lang::object>(interpreter->NullClass);
        })},
    };
    
    return DictionaryClass;
}