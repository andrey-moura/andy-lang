#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>
#include <andy/lang/api.hpp>

std::shared_ptr<andy::lang::structure> create_string_class(andy::lang::interpreter* interpreter)
{
    auto StringClass = std::make_shared<andy::lang::structure>("String");

    StringClass->instance_functions["*"] = std::make_shared<andy::lang::function>("*", andy::lang::function_storage_type::instance_function, std::vector<std::string>{"what"}, [interpreter](andy::lang::function_call& call) {
        const std::string& value = call.object->as<std::string>();
        std::string result;

        if(call.positional_params[0]->cls != interpreter->IntegerClass) {
            throw std::runtime_error("undefined operator * (" + std::string(call.object->cls->name) + ", " + std::string(call.positional_params[0]->cls->name) + ")");
        }
   
        int times = call.positional_params[0]->as<int>();

        for(int i = 0; i < times; i++) {
            result += value;
        }

        return andy::lang::object::instantiate(interpreter, call.object->cls, result);
    });

        StringClass->instance_functions["present?"] = std::make_shared<andy::lang::function>("present?", andy::lang::function_storage_type::instance_function, std::vector<std::string>{}, [interpreter](andy::lang::function_call& call) {
            const std::string& value = call.object->as<std::string>();

            if(value.empty()) {
                return std::make_shared<andy::lang::object>(interpreter->FalseClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->TrueClass);
        });

    StringClass->instance_functions["to_string"] = std::make_shared<andy::lang::function>("to_string", andy::lang::function_storage_type::instance_function, std::vector<std::string>{}, [interpreter](andy::lang::function_call& call) {
            const std::string& value = call.object->as<std::string>();
            return andy::lang::object::instantiate(interpreter, call.object->cls, value);
        });

    StringClass->instance_functions["find"] = std::make_shared<andy::lang::function>("find", andy::lang::function_storage_type::instance_function, std::vector<std::string>{"what"}, [interpreter](andy::lang::function_call& call) {
            const std::string& value = call.object->as<std::string>();
            size_t pos = value.find(call.positional_params[0]->as<std::string>());
            return andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, (int32_t)pos);
        });

    StringClass->instance_functions["substring"] = std::make_shared<andy::lang::function>("substring", andy::lang::function_storage_type::instance_function, std::vector<std::string>{"start", "size"}, [interpreter](andy::lang::function_call& call) {
            const std::string& value = call.object->as<std::string>();
            size_t start = call.positional_params[0]->as<int32_t>();
            size_t size = call.positional_params[1]->as<int32_t>();

            return andy::lang::object::instantiate(interpreter, call.object->cls, value.substr(start, size));
        });

    StringClass->instance_functions["to_lower_case!"] = std::make_shared<andy::lang::function>("to_lower_case!", andy::lang::function_storage_type::instance_function, std::vector<std::string>{}, [](andy::lang::function_call& call) {
            std::string& value = call.object->as<std::string>();

            for(char & c : value) {
                c = std::tolower(c);
            }

            return nullptr;
        });

    StringClass->instance_functions["to_lower_case"] = std::make_shared<andy::lang::function>("to_lower_case", andy::lang::function_storage_type::instance_function, std::vector<std::string>{}, [interpreter](andy::lang::function_call& call) {
            std::string value = call.object->as<std::string>();

            for(char & c : value) {
                c = std::tolower(c);
            }

            return andy::lang::object::instantiate(interpreter, call.object->cls, value);
        });

    StringClass->instance_functions["to_integer!"] = std::make_shared<andy::lang::function>("to_integer!", andy::lang::function_storage_type::instance_function, std::vector<std::string>{}, [interpreter](andy::lang::function_call& call) {
            std::string& value = call.object->as<std::string>();

            if(value.empty()) {
                call.object->cls = interpreter->NullClass;
                call.object->set_native(0);

                return call.object;
            }

            if(!isdigit(value[0])) {
                call.object->cls = interpreter->NullClass;
                call.object->set_native(0);

                return call.object;
            }

            size_t pos = 0;
            int result = std::stoi(value, &pos);

            if(pos != value.size()) {
                call.object->cls = interpreter->NullClass;
                call.object->set_native(0);

                return call.object;
            }

            call.object->cls = interpreter->IntegerClass;
            call.object->set_native(result);

            return call.object;
        });

    StringClass->instance_functions["to_integer"] = std::make_shared<andy::lang::function>("to_integer", andy::lang::function_storage_type::instance_function, std::vector<std::string>{}, [interpreter](andy::lang::function_call& call) {
            std::string value = call.object->as<std::string>();

            if(value.empty()) return std::make_shared<andy::lang::object>(interpreter->NullClass);

            if(!isdigit(value[0])) return std::make_shared<andy::lang::object>(interpreter->NullClass);

            size_t pos = 0;
            int result = std::stoi(value, &pos);

            if(pos != value.size()) return std::make_shared<andy::lang::object>(interpreter->NullClass);

            return andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, result);
        });

    StringClass->instance_functions["erase!"] = std::make_shared<andy::lang::function>("erase!", andy::lang::function_storage_type::instance_function, std::vector<std::string>{"start", "size"}, [](andy::lang::function_call& call) {
            std::string& value = call.object->as<std::string>();
            size_t start = call.positional_params[0]->as<int32_t>();
            size_t size = call.positional_params[1]->as<int32_t>();

            value.erase(start, size);

            return nullptr;
        });

    StringClass->instance_functions["starts_with?"] = std::make_shared<andy::lang::function>("starts_with?", andy::lang::function_storage_type::instance_function, std::vector<std::string>{"what"}, [interpreter](andy::lang::function_call& call) {
        std::string& value = call.object->as<std::string>();
        const std::string& what = call.positional_params[0]->as<std::string>();

        bool starts = value.starts_with(what);

        return andy::lang::api::to_object(interpreter, starts);
    });

    StringClass->instance_functions["ends_with?"] = std::make_shared<andy::lang::function>("ends_with?", andy::lang::function_storage_type::instance_function, std::vector<std::string>{"what"}, [interpreter](andy::lang::function_call& call) {
        std::string& value = call.object->as<std::string>();
        const std::string& what = call.positional_params[0]->as<std::string>();

        bool ends = value.ends_with(what);

        return andy::lang::api::to_object(interpreter, ends);
    });

    StringClass->instance_functions["=="] = std::make_shared<andy::lang::function>("==", andy::lang::function_storage_type::instance_function, std::vector<std::string>{"other"}, [interpreter](andy::lang::function_call& call) {
            const std::string& value = call.object->as<std::string>();
            const std::string& other = call.positional_params[0]->as<std::string>();

            if(value == other) {
                return std::make_shared<andy::lang::object>(interpreter->TrueClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        });

    StringClass->instance_functions["!="] = std::make_shared<andy::lang::function>("!=", andy::lang::function_storage_type::instance_function, std::vector<std::string>{"other"}, [interpreter](andy::lang::function_call& call) {
            const std::string& value = call.object->as<std::string>();
            const std::string& other = call.positional_params[0]->as<std::string>();

            if(value != other) {
                return std::make_shared<andy::lang::object>(interpreter->TrueClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        });

    StringClass->instance_functions["+"] = std::make_shared<andy::lang::function>("+", andy::lang::function_storage_type::instance_function, std::vector<std::string>{"other"}, [interpreter](andy::lang::function_call& call) {
            const std::string& value = call.object->as<std::string>();
            const std::string& other = call.positional_params[0]->as<std::string>();

            return andy::lang::object::instantiate(interpreter, call.object->cls, value + other);
        });

    StringClass->instance_functions["size"] = std::make_shared<andy::lang::function>("size", andy::lang::function_storage_type::instance_function, std::vector<std::string>{}, [interpreter](andy::lang::function_call& call) {
            const std::string& value = call.object->as<std::string>();
            return andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, (int32_t)value.size());
        });

    StringClass->instance_functions["empty?"] = std::make_shared<andy::lang::function>("empty?", andy::lang::function_storage_type::instance_function, std::vector<std::string>{}, [interpreter](andy::lang::function_call& call) {
            const std::string& value = call.object->as<std::string>();

            if(value.empty()) {
                return std::make_shared<andy::lang::object>(interpreter->TrueClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        });

    StringClass->instance_functions["include?"] = std::make_shared<andy::lang::function>("include?", andy::lang::function_storage_type::instance_function, std::vector<std::string>{"other"}, [interpreter](andy::lang::function_call& call) {
            const std::string& value = call.object->as<std::string>();
            const std::string& other = call.positional_params[0]->as<std::string>();

            if(value.find(other) != std::string::npos) {
                return std::make_shared<andy::lang::object>(interpreter->TrueClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        });

    StringClass->instance_functions["capitalize!"] = std::make_shared<andy::lang::function>("capitalize!", andy::lang::function_storage_type::instance_function, std::vector<std::string>{}, [](andy::lang::function_call& call) {
            std::string& value = call.object->as<std::string>();

            if(!value.empty()) {
                value[0] = std::toupper(value[0]);
            }

            return nullptr;
        });

    
    return StringClass;
}