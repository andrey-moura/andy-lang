#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>

std::shared_ptr<andy::lang::structure> create_string_class(andy::lang::interpreter* interpreter)
{
    auto StringClass = std::make_shared<andy::lang::structure>("String");

        StringClass->functions["present?"] = std::make_shared<andy::lang::function>("present?", andy::lang::function_storage_type::instance_function, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& value = object->as<std::string>();

            if(value.empty()) {
                return std::make_shared<andy::lang::object>(interpreter->FalseClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->TrueClass);
        });

    StringClass->functions["to_string"] = std::make_shared<andy::lang::function>("to_string", andy::lang::function_storage_type::instance_function, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& value = object->as<std::string>();
            return andy::lang::object::instantiate(interpreter, StringClass, value);
        });

    StringClass->functions["find"] = std::make_shared<andy::lang::function>("find", andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"what"}, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& value = object->as<std::string>();
            size_t pos = value.find(params[0]->as<std::string>());
            return andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, (int32_t)pos);
        });

    StringClass->functions["substring"] = std::make_shared<andy::lang::function>("substring", andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"start", "size"}, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& value = object->as<std::string>();
            size_t start = params[0]->as<int32_t>();
            size_t size = params[1]->as<int32_t>();

            return andy::lang::object::instantiate(interpreter, StringClass, value.substr(start, size));
        });

    StringClass->functions["to_lower_case!"] = std::make_shared<andy::lang::function>("to_lower_case!", andy::lang::function_storage_type::instance_function, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string& value = object->as<std::string>();

            for(char & c : value) {
                c = std::tolower(c);
            }

            return nullptr;
        });

    StringClass->functions["to_lower_case"] = std::make_shared<andy::lang::function>("to_lower_case!", andy::lang::function_storage_type::instance_function, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string value = object->as<std::string>();

            for(char & c : value) {
                c = std::tolower(c);
            }

            return andy::lang::object::instantiate(interpreter, StringClass, value);
        });

    StringClass->functions["to_integer!"] = std::make_shared<andy::lang::function>("to_integer!", andy::lang::function_storage_type::instance_function, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
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
        });

    StringClass->functions["to_integer"] = std::make_shared<andy::lang::function>("to_integer", andy::lang::function_storage_type::instance_function, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string value = object->as<std::string>();

            if(value.empty()) return std::make_shared<andy::lang::object>(interpreter->NullClass);

            if(!isdigit(value[0])) return std::make_shared<andy::lang::object>(interpreter->NullClass);

            size_t pos = 0;
            int result = std::stoi(value, &pos);

            if(pos != value.size()) return std::make_shared<andy::lang::object>(interpreter->NullClass);

            return andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, result);
        });

    StringClass->functions["erase!"] = std::make_shared<andy::lang::function>("erase!", andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"start", "size"}, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string& value = object->as<std::string>();
            size_t start = params[0]->as<int32_t>();
            size_t size = params[1]->as<int32_t>();

            value.erase(start, size);

            return nullptr;
        });

    StringClass->functions["starts_with?"] = std::make_shared<andy::lang::function>("starts_with?", andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"what"}, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string& value = object->as<std::string>();
            const std::string& what = params[0]->as<std::string>();

            bool starts = value.starts_with(what);

            if(starts) {
                return std::make_shared<andy::lang::object>(interpreter->TrueClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        });

    StringClass->functions["=="] = std::make_shared<andy::lang::function>("==", andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"other"}, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& value = object->as<std::string>();
            const std::string& other = params[0]->as<std::string>();

            if(value == other) {
                return std::make_shared<andy::lang::object>(interpreter->TrueClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        });

    StringClass->functions["!="] = std::make_shared<andy::lang::function>("!=", andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"other"}, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& value = object->as<std::string>();
            const std::string& other = params[0]->as<std::string>();

            if(value != other) {
                return std::make_shared<andy::lang::object>(interpreter->TrueClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        });

    StringClass->functions["+"] = std::make_shared<andy::lang::function>("+", andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"other"}, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& value = object->as<std::string>();
            const std::string& other = params[0]->as<std::string>();

            return andy::lang::object::instantiate(interpreter, StringClass, value + other);
        });

    StringClass->functions["size"] = std::make_shared<andy::lang::function>("size", andy::lang::function_storage_type::instance_function, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& value = object->as<std::string>();
            return andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, (int32_t)value.size());
        });

    StringClass->functions["empty?"] = std::make_shared<andy::lang::function>("empty?", andy::lang::function_storage_type::instance_function, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& value = object->as<std::string>();

            if(value.empty()) {
                return std::make_shared<andy::lang::object>(interpreter->TrueClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        });

    StringClass->functions["include?"] = std::make_shared<andy::lang::function>("include?", andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"other"}, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            const std::string& value = object->as<std::string>();
            const std::string& other = params[0]->as<std::string>();

            if(value.find(other) != std::string::npos) {
                return std::make_shared<andy::lang::object>(interpreter->TrueClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        });

    StringClass->functions["capitalize!"] = std::make_shared<andy::lang::function>("capitalize!", andy::lang::function_storage_type::instance_function, [interpreter, StringClass](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string& value = object->as<std::string>();

            if(!value.empty()) {
                value[0] = std::toupper(value[0]);
            }

            return nullptr;
        });

    
    return StringClass;
}