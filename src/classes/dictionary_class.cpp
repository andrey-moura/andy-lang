#include <andy/lang/lang.hpp>

#include <andy/lang/interpreter.hpp>

std::shared_ptr<andy::lang::structure> create_dictionary_class(andy::lang::interpreter* interpreter)
{
    auto DictionaryClass = std::make_shared<andy::lang::structure>("Dictionary");
    DictionaryClass->object_to_var = [](std::shared_ptr<const andy::lang::object> obj) {
        andy::lang::dictionary data = obj->as<andy::lang::dictionary>();

        var::dictionary_type result;

        for(auto& pair : data) {
            result[pair.first->to_var()] = pair.second->to_var();
        }

        return var(std::move(result));
    };
    DictionaryClass->methods = {
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

            auto operator_it = key->cls->methods.find("==");

            for(auto& pair : dictionary) {
                auto result = interpreter->call(key->cls, key, operator_it->second, { pair.first });
                if(result->cls == interpreter->TrueClass) {
                    return pair.second;
                }
            }

            return std::make_shared<andy::lang::object>(interpreter->NullClass);
        })},
    };
    
    return DictionaryClass;
}