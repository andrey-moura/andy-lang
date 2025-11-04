#include <andy/lang/lang.hpp>
#include <andy/lang/api.hpp>
#include <andy/lang/interpreter.hpp>

std::shared_ptr<andy::lang::structure> create_dictionary_class(andy::lang::interpreter* interpreter)
{
    auto DictionaryClass = std::make_shared<andy::lang::structure>("Dictionary");

        DictionaryClass->functions["present?"] = std::make_shared<andy::lang::function>("present?",andy::lang::function_storage_type::instance_function, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& value = object->as<std::string>();

            if(value.empty()) {
                return std::make_shared<andy::lang::object>(interpreter->FalseClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->TrueClass);
        });

    DictionaryClass->functions["[]"] = std::make_shared<andy::lang::function>("[]",andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"key"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::shared_ptr<andy::lang::object> key = params[0];

            auto& dictionary = object->as<andy::lang::dictionary>();

            auto operator_it = key->cls->instance_functions.find("==");

            for(auto& pair : dictionary) {
                andy::lang::function_call __call = {
                    "==",
                    key->cls,
                    key,
                    operator_it->second.get(),
                    { pair.first }
                };
                auto result = interpreter->call(__call);
                if(result->cls == interpreter->TrueClass) {
                    return pair.second;
                }
            }

            return std::make_shared<andy::lang::object>(interpreter->NullClass);
        });

    DictionaryClass->functions["to_string"] = std::make_shared<andy::lang::function>("to_string",andy::lang::function_storage_type::instance_function, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string result = "{";
            auto& dictionary = object->as<andy::lang::dictionary>();
            for(auto& pair : dictionary) {
                result += andy::lang::api::call<std::string>(interpreter, andy::lang::function_call{
                    "to_string",
                    pair.first->cls,
                    pair.first,
                    pair.first->cls->functions["to_string"].get(),
                    {},
                    {},
                    nullptr
                });
                result += ": ";
                result += andy::lang::api::call<std::string>(interpreter, andy::lang::function_call{
                    "to_string",
                    pair.second->cls,
                    pair.second,
                    pair.second->cls->functions["to_string"].get(),
                    {},
                    {},
                    nullptr
                });
                result += ", ";
            }
            result += "}";
            return andy::lang::object::instantiate(interpreter, interpreter->StringClass, std::move(result));
        });

    
    return DictionaryClass;
}