#include <andy/lang/lang.hpp>

#include <andy/lang/interpreter.hpp>
#include <andy/lang/array_class.hpp>

std::shared_ptr<andy::lang::structure> andy::lang::array_class::create(andy::lang::interpreter* interpreter)
{
    auto ArrayClass = std::make_shared<andy::lang::structure>("Array");
    ArrayClass->object_to_var = [](std::shared_ptr<const andy::lang::object> obj) {
        std::vector<std::shared_ptr<andy::lang::object>> objects = obj->as<std::vector<std::shared_ptr<andy::lang::object>>>();

        var::array_type result;
        result.reserve(objects.size());

        for(auto& obj : objects) {
            result.push_back(obj->to_var());
        }

        return var(std::move(result));
    };

    ArrayClass->methods = {
        {"to_string", andy::lang::method("to_string", method_storage_type::instance_method, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string result = "[";

            std::vector<std::shared_ptr<andy::lang::object>>& items = object->as<std::vector<std::shared_ptr<andy::lang::object>>>();

            for(auto& item : items) {
                if(result.size() > 1) {
                    result += ", ";
                }

                result += item->cls->methods["to_string"].call(item)->as<std::string>();
            }

            result += "]";

            return andy::lang::object::instantiate(interpreter, interpreter->StringClass, std::move(result));
        })},

        {"join", andy::lang::method("join", method_storage_type::instance_method, {"separator"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& separator = params[0]->as<std::string>();
            std::string result;

            std::vector<std::shared_ptr<andy::lang::object>>& items = object->as<std::vector<std::shared_ptr<andy::lang::object>>>();

            for(auto& item : items) {
                if(result.size()) {
                    result += separator;
                }

                result += item->cls->methods["to_string"].call(item)->as<std::string>();
            }

            return andy::lang::object::instantiate(interpreter, interpreter->StringClass, std::move(result));
        })},

        {"front", andy::lang::method("front", method_storage_type::instance_method, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::vector<std::shared_ptr<andy::lang::object>>& items = object->as<std::vector<std::shared_ptr<andy::lang::object>>>();

            if(items.empty()) {
                return std::make_shared<andy::lang::object>(interpreter->NullClass);
            }

            return items.front();
        })},

        {"size", andy::lang::method("size", method_storage_type::instance_method, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::vector<std::shared_ptr<andy::lang::object>>& items = object->as<std::vector<std::shared_ptr<andy::lang::object>>>();

            return andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, items.size());
        })},

        {"pop_front!", andy::lang::method("pop_front!", method_storage_type::instance_method, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::vector<std::shared_ptr<andy::lang::object>>& items = object->as<std::vector<std::shared_ptr<andy::lang::object>>>();

            if(items.size()) {
                items.erase(items.begin());
            }

            return nullptr;
        })},

        {"[]", andy::lang::method("[]", method_storage_type::instance_method, {"index"} , [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::vector<std::shared_ptr<andy::lang::object>>& items = object->as<std::vector<std::shared_ptr<andy::lang::object>>>();

            auto index = params[0]->as<int>();

            return items[index];
        })},
    };
    
    return ArrayClass;
}