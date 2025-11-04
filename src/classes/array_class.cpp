#include <andy/lang/lang.hpp>
#include <andy/lang/api.hpp>
#include <andy/lang/interpreter.hpp>

std::shared_ptr<andy::lang::structure> create_array_class(andy::lang::interpreter* interpreter)
{
    auto ArrayClass = std::make_shared<andy::lang::structure>("Array");

        ArrayClass->functions["to_string"] = std::make_shared<andy::lang::function>("to_string",andy::lang::function_storage_type::instance_function, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string result = "[";

            std::vector<std::shared_ptr<andy::lang::object>>& items = object->as<std::vector<std::shared_ptr<andy::lang::object>>>();

            for(auto& item : items) {
                if(result.size() > 1) {
                    result += ", ";
                }

                result += item->cls->functions["to_string"]->call(item)->as<std::string>();
            }

            result += "]";

            return andy::lang::object::instantiate(interpreter, interpreter->StringClass, std::move(result));
        });

    ArrayClass->functions["join"] = std::make_shared<andy::lang::function>("join",andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"separator"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& separator = params[0]->as<std::string>();
            std::string result;

            std::vector<std::shared_ptr<andy::lang::object>>& items = object->as<std::vector<std::shared_ptr<andy::lang::object>>>();

            for(auto& item : items) {
                if(result.size()) {
                    result += separator;
                }

                result += item->cls->functions["to_string"]->call(item)->as<std::string>();
            }

            return andy::lang::object::instantiate(interpreter, interpreter->StringClass, std::move(result));
        });

    ArrayClass->functions["front"] = std::make_shared<andy::lang::function>("front",andy::lang::function_storage_type::instance_function, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::vector<std::shared_ptr<andy::lang::object>>& items = object->as<std::vector<std::shared_ptr<andy::lang::object>>>();

            if(items.empty()) {
                return std::make_shared<andy::lang::object>(interpreter->NullClass);
            }

            return items.front();
        });

    ArrayClass->functions["size"] = std::make_shared<andy::lang::function>("size",andy::lang::function_storage_type::instance_function, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::vector<std::shared_ptr<andy::lang::object>>& items = object->as<std::vector<std::shared_ptr<andy::lang::object>>>();

            return andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, items.size());
        });

    ArrayClass->functions["push"] = std::make_shared<andy::lang::function>("push",andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"item"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::vector<std::shared_ptr<andy::lang::object>>& items = object->as<std::vector<std::shared_ptr<andy::lang::object>>>();

            items.push_back(params[0]);

            return nullptr;
        });

    ArrayClass->functions["[]"] = std::make_shared<andy::lang::function>("[]",andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"index"} , [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::vector<std::shared_ptr<andy::lang::object>>& items = object->as<std::vector<std::shared_ptr<andy::lang::object>>>();

            auto index = params[0]->as<int>();

            return items[index];
        });

    ArrayClass->functions["=="] = std::make_shared<andy::lang::function>("==",andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"other"} , [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::vector<std::shared_ptr<andy::lang::object>>& items = object->as<std::vector<std::shared_ptr<andy::lang::object>>>();
            if(params[0]->cls != interpreter->ArrayClass) {
                return std::make_shared<andy::lang::object>(interpreter->FalseClass);
            }
            auto& other_items = params[0]->as<std::vector<std::shared_ptr<andy::lang::object>>>();
            if(items.size() != other_items.size()) {
                return std::make_shared<andy::lang::object>(interpreter->FalseClass);
            }
            for(size_t i = 0; i < other_items.size(); ++i) {
                auto it = items[i]->cls->instance_functions.find("==");
                if(it == items[i]->cls->instance_functions.end()) {
                    throw std::runtime_error("class " + std::string(items[i]->cls->name) + " does not have a method '=='");
                }
                andy::lang::function_call call{
                    "==",
                    items[i]->cls,
                    items[i],
                    it->second.get(),
                    { other_items[i] }
                };
                auto result = interpreter->call(call);
                if(result->cls == interpreter->FalseClass) {
                    result;
                }
            }
            return std::make_shared<andy::lang::object>(interpreter->TrueClass);
        });

    
    return ArrayClass;
}