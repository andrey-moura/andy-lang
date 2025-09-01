#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>

std::shared_ptr<andy::lang::structure> create_string_class(andy::lang::interpreter* interpreter)
{
    auto StringClass = std::make_shared<andy::lang::structure>("String");

    StringClass->instance_methods = {
        {"present?", andy::lang::method("present?", andy::lang::method_storage_type::instance_method, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& value = object->as<std::string>();

            if(value.empty()) {
                return std::make_shared<andy::lang::object>(interpreter->FalseClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->TrueClass);
        })},

        {"to_string", andy::lang::method("to_string", andy::lang::method_storage_type::instance_method, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& value = object->as<std::string>();
            return andy::lang::object::instantiate(interpreter, StringClass, value);
        })},

        {"find", andy::lang::method("find", andy::lang::method_storage_type::instance_method, {"what"}, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& value = object->as<std::string>();
            size_t pos = value.find(params[0]->as<std::string>());
            return andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, (int32_t)pos);
        })},

        {"substring", andy::lang::method("substring", andy::lang::method_storage_type::instance_method, {"start", "size"}, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& value = object->as<std::string>();
            size_t start = params[0]->as<int32_t>();
            size_t size = params[1]->as<int32_t>();

            return andy::lang::object::instantiate(interpreter, StringClass, value.substr(start, size));
        })},

        {"to_lower_case!", andy::lang::method("to_lower_case!", andy::lang::method_storage_type::instance_method, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string& value = object->as<std::string>();

            for(char & c : value) {
                c = std::tolower(c);
            }

            return nullptr;
        })},

        {"to_lower_case", andy::lang::method("to_lower_case!", andy::lang::method_storage_type::instance_method, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string value = object->as<std::string>();

            for(char & c : value) {
                c = std::tolower(c);
            }

            return andy::lang::object::instantiate(interpreter, StringClass, value);
        })},

        {"to_integer!", andy::lang::method("to_integer!", andy::lang::method_storage_type::instance_method, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string& value = object->as<std::string>();

            if(value.empty()) {
                object->cls = interpreter->NullClass;
                object->set_native(0);

                return object;
            }

            if(!isdigit(value[0])) {
                object->cls = interpreter->NullClass;
                object->set_native(0);

                return object;
            }

            size_t pos = 0;
            int result = std::stoi(value, &pos);

            if(pos != value.size()) {
                object->cls = interpreter->NullClass;
                object->set_native(0);

                return object;
            }

            object->cls = interpreter->IntegerClass;
            object->set_native(result);

            return object;
        })},

        {"to_integer", andy::lang::method("to_integer", andy::lang::method_storage_type::instance_method, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string value = object->as<std::string>();

            if(value.empty()) return std::make_shared<andy::lang::object>(interpreter->NullClass);

            if(!isdigit(value[0])) return std::make_shared<andy::lang::object>(interpreter->NullClass);

            size_t pos = 0;
            int result = std::stoi(value, &pos);

            if(pos != value.size()) return std::make_shared<andy::lang::object>(interpreter->NullClass);

            return andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, result);
        })},

        {"erase!", andy::lang::method("erase!", andy::lang::method_storage_type::instance_method, {"start", "size"}, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string& value = object->as<std::string>();
            size_t start = params[0]->as<int32_t>();
            size_t size = params[1]->as<int32_t>();

            value.erase(start, size);

            return nullptr;
        })},

        {"starts_with?", andy::lang::method("starts_with?", andy::lang::method_storage_type::instance_method, {"what"}, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string& value = object->as<std::string>();
            const std::string& what = params[0]->as<std::string>();

            bool starts = value.starts_with(what);

            if(starts) {
                return std::make_shared<andy::lang::object>(interpreter->TrueClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        })},

        {"==", andy::lang::method("==", andy::lang::method_storage_type::instance_method, {"other"}, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& value = object->as<std::string>();
            const std::string& other = params[0]->as<std::string>();

            if(value == other) {
                return std::make_shared<andy::lang::object>(interpreter->TrueClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        })},

        {"!=", andy::lang::method("!=", andy::lang::method_storage_type::instance_method, {"other"}, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& value = object->as<std::string>();
            const std::string& other = params[0]->as<std::string>();

            if(value != other) {
                return std::make_shared<andy::lang::object>(interpreter->TrueClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        })},
        
        {"+", andy::lang::method("+", andy::lang::method_storage_type::instance_method, {"other"}, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& value = object->as<std::string>();
            const std::string& other = params[0]->as<std::string>();

            return andy::lang::object::instantiate(interpreter, StringClass, value + other);
        })},

        {"size", andy::lang::method("size", andy::lang::method_storage_type::instance_method, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& value = object->as<std::string>();
            return andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, (int32_t)value.size());
        })},

        {"empty?", andy::lang::method("empty?", andy::lang::method_storage_type::instance_method, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& value = object->as<std::string>();

            if(value.empty()) {
                return std::make_shared<andy::lang::object>(interpreter->TrueClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        })},

        {"include?", andy::lang::method("include?", andy::lang::method_storage_type::instance_method, { "other" }, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& value = object->as<std::string>();
            const std::string& other = params[0]->as<std::string>();

            if(value.find(other) != std::string::npos) {
                return std::make_shared<andy::lang::object>(interpreter->TrueClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        })},

        {"capitalize!", andy::lang::method("capitalize!", andy::lang::method_storage_type::instance_method, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string& value = object->as<std::string>();

            if(!value.empty()) {
                value[0] = std::toupper(value[0]);
            }

            return nullptr;
        })},
    };
    
    return StringClass;
}