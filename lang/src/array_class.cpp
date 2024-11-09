#include <lang/lang.hpp>

#include <interpreter/interpreter.hpp>
#include "array_class.hpp"

std::shared_ptr<uva::lang::structure> uva::lang::array_class::create(uva::lang::interpreter* interpreter)
{
    auto ArrayClass = std::make_shared<uva::lang::structure>("Array");

    ArrayClass->methods = {
        {"to_s", uva::lang::method("to_s", method_storage_type::instance_method, {}, [interpreter](std::shared_ptr<uva::lang::object> object, std::vector<std::shared_ptr<uva::lang::object>> params) {
            std::string result = "[";

            std::vector<std::shared_ptr<uva::lang::object>>& items = object->as<std::vector<std::shared_ptr<uva::lang::object>>>();

            for(auto& item : items) {
                if(result.size() > 1) {
                    result += ", ";
                }

                result += item->cls->methods["to_s"].call(item)->as<std::string>();
            }

            result += "]";

            return uva::lang::object::instantiate(interpreter, interpreter->StringClass, std::move(result));
        })},
        {"join", uva::lang::method("join", method_storage_type::instance_method, {"separator"}, [interpreter](std::shared_ptr<uva::lang::object> object, std::vector<std::shared_ptr<uva::lang::object>> params) {
            const std::string& separator = params[0]->as<std::string>();
            std::string result;

            std::vector<std::shared_ptr<uva::lang::object>>& items = object->as<std::vector<std::shared_ptr<uva::lang::object>>>();

            for(auto& item : items) {
                if(result.size()) {
                    result += separator;
                }

                result += item->cls->methods["to_s"].call(item)->as<std::string>();
            }

            return uva::lang::object::instantiate(interpreter, interpreter->StringClass, std::move(result));
        })}
    };
    
    return ArrayClass;
}