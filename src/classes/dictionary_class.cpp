#include <andy/lang/lang.hpp>
#include <andy/lang/api.hpp>
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
        {"to_string", andy::lang::method("to_string",andy::lang::method_storage_type::instance_method, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string result = "{";
            auto& dictionary = object->as<andy::lang::dictionary>();
            for(auto& pair : dictionary) {
                result += andy::lang::api::call<std::string>(interpreter, andy::lang::function_call{
                    "to_string",
                    pair.first->cls,
                    pair.first,
                    &pair.first->cls->instance_methods["to_string"],
                    {},
                    {},
                    nullptr
                });
                result += ": ";
                result += andy::lang::api::call<std::string>(interpreter, andy::lang::function_call{
                    "to_string",
                    pair.second->cls,
                    pair.second,
                    &pair.second->cls->instance_methods["to_string"],
                    {},
                    {},
                    nullptr
                });
                result += ", ";
            }
            result += "}";
            return andy::lang::object::instantiate(interpreter, interpreter->StringClass, std::move(result));
        })},
    };
    
    return DictionaryClass;
}