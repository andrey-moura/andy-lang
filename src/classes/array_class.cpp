#include <andy/lang/lang.hpp>
#include <andy/lang/api.hpp>
#include <andy/lang/interpreter.hpp>

std::shared_ptr<andy::lang::structure> create_array_class(andy::lang::interpreter* interpreter)
{
    auto ArrayClass = std::make_shared<andy::lang::structure>("Array");

    ArrayClass->instance_methods = {
        {"to_string", andy::lang::method("to_string",andy::lang::method_storage_type::instance_method, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string result = "[";

            std::vector<std::shared_ptr<andy::lang::object>>& items = object->as<std::vector<std::shared_ptr<andy::lang::object>>>();

            for(auto& item : items) {
                if(result.size() > 1) {
                    result += ", ";
                }

                result += item->cls->instance_methods["to_string"].call(item)->as<std::string>();
            }

            result += "]";

            return andy::lang::object::instantiate(interpreter, interpreter->StringClass, std::move(result));
        })},

        {"join", andy::lang::method("join",andy::lang::method_storage_type::instance_method, {"separator"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& separator = params[0]->as<std::string>();
            std::string result;

            std::vector<std::shared_ptr<andy::lang::object>>& items = object->as<std::vector<std::shared_ptr<andy::lang::object>>>();

            for(auto& item : items) {
                if(result.size()) {
                    result += separator;
                }

                result += item->cls->instance_methods["to_string"].call(item)->as<std::string>();
            }

            return andy::lang::object::instantiate(interpreter, interpreter->StringClass, std::move(result));
        })},

        {"front", andy::lang::method("front",andy::lang::method_storage_type::instance_method, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::vector<std::shared_ptr<andy::lang::object>>& items = object->as<std::vector<std::shared_ptr<andy::lang::object>>>();

            if(items.empty()) {
                return std::make_shared<andy::lang::object>(interpreter->NullClass);
            }

            return items.front();
        })},

        {"size", andy::lang::method("size",andy::lang::method_storage_type::instance_method, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::vector<std::shared_ptr<andy::lang::object>>& items = object->as<std::vector<std::shared_ptr<andy::lang::object>>>();

            return andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, items.size());
        })},

        {"push", andy::lang::method("push",andy::lang::method_storage_type::instance_method, {"item"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::vector<std::shared_ptr<andy::lang::object>>& items = object->as<std::vector<std::shared_ptr<andy::lang::object>>>();

            items.push_back(params[0]);

            return nullptr;
        })},


        {"[]", andy::lang::method("[]",andy::lang::method_storage_type::instance_method, {"index"} , [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::vector<std::shared_ptr<andy::lang::object>>& items = object->as<std::vector<std::shared_ptr<andy::lang::object>>>();

            auto index = params[0]->as<int>();

            return items[index];
        })},

        {"==", andy::lang::method("==",andy::lang::method_storage_type::instance_method, {"other"} , [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::vector<std::shared_ptr<andy::lang::object>>& items = object->as<std::vector<std::shared_ptr<andy::lang::object>>>();
            if(params[0]->cls != interpreter->ArrayClass) {
                return std::make_shared<andy::lang::object>(interpreter->FalseClass);
            }
            auto& other_items = params[0]->as<std::vector<std::shared_ptr<andy::lang::object>>>();
            if(items.size() != other_items.size()) {
                return std::make_shared<andy::lang::object>(interpreter->FalseClass);
            }
            for(size_t i = 0; i < other_items.size(); ++i) {
                auto it = items[i]->cls->instance_methods.find("==");
                if(it == items[i]->cls->instance_methods.end()) {
                    throw std::runtime_error("class " + std::string(items[i]->cls->name) + " does not have a method '=='");
                }
                andy::lang::function_call call{
                    "==",
                    items[i]->cls,
                    items[i],
                    &it->second,
                    { other_items[i] }
                };
                auto result = interpreter->call(call);
                if(result->cls == interpreter->FalseClass) {
                    result;
                }
            }
            return std::make_shared<andy::lang::object>(interpreter->TrueClass);
        })},

    };
    
    return ArrayClass;
}