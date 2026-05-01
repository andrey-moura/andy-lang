#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>
#include <andy/lang/api.hpp>

std::shared_ptr<andy::lang::structure> create_string_class(andy::lang::interpreter* interpreter)
{
    auto StringClass = std::make_shared<andy::lang::structure>("String");

    StringClass->instance_functions["*"] = std::make_shared<andy::lang::function>("*", std::initializer_list<std::string>{"what"}, [StringClass](andy::lang::interpreter* interpreter) {
        const std::string& value = interpreter->current_context->self->as<std::string>();
        const auto& params = interpreter->current_context->positional_params;
        std::string result;

        if(params[0]->cls != interpreter->IntegerClass) {
            throw std::runtime_error("undefined operator * (" + std::string(interpreter->current_context->self->cls->name) + ", " + std::string(params[0]->cls->name) + ")");
        }
   
        int times = params[0]->as<int>();

        for(int i = 0; i < times; i++) {
            result += value;
        }

        return andy::lang::object::instantiate(interpreter, StringClass, result);
    });

        StringClass->instance_functions["present?"] = std::make_shared<andy::lang::function>("present?", [](andy::lang::interpreter* interpreter) {
            const std::string& value = interpreter->current_context->self->as<std::string>();

            if(value.empty()) {
                return std::make_shared<andy::lang::object>(interpreter->FalseClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->TrueClass);
        });

        StringClass->instance_functions["to_string"] = std::make_shared<andy::lang::function>("to_string", [StringClass](andy::lang::interpreter* interpreter) {
            const std::string& value = interpreter->current_context->self->as<std::string>();
            return andy::lang::object::instantiate(interpreter, StringClass, value);
        });

        StringClass->instance_functions["find"] = std::make_shared<andy::lang::function>("find", std::initializer_list<std::string>{"what"}, [](andy::lang::interpreter* interpreter) {
            const std::string& value = interpreter->current_context->self->as<std::string>();
            size_t pos = value.find(interpreter->current_context->positional_params[0]->as<std::string>());
            return andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, (int32_t)pos);
        });

        StringClass->instance_functions["substring"] = std::make_shared<andy::lang::function>("substring", std::initializer_list<std::string>{"start", "size"}, [StringClass](andy::lang::interpreter* interpreter) {
            const std::string& value = interpreter->current_context->self->as<std::string>();
            const auto& params = interpreter->current_context->positional_params;
            size_t start = params[0]->as<int32_t>();
            size_t size = params[1]->as<int32_t>();

            return andy::lang::object::instantiate(interpreter, StringClass, value.substr(start, size));
        });

        StringClass->instance_functions["lower!"] = std::make_shared<andy::lang::function>("lower!", [](andy::lang::interpreter* interpreter) {
            std::string& value = interpreter->current_context->self->as<std::string>();

            for(char & c : value) {
                c = std::tolower(c);
            }

            return nullptr;
        });

        StringClass->instance_functions["lower"] = std::make_shared<andy::lang::function>("lower", [StringClass](andy::lang::interpreter* interpreter) {
            std::string value = interpreter->current_context->self->as<std::string>();

            for(char & c : value) {
                c = std::tolower(c);
            }

            return andy::lang::api::to_object(interpreter, value);
        });

        StringClass->instance_functions["to_integer!"] = std::make_shared<andy::lang::function>("to_integer!", [](andy::lang::interpreter* interpreter) {
            auto object = interpreter->current_context->self;
            std::string& value = object->as<std::string>();

            if(value.empty()) {
                object->cls = interpreter->NullClass;
                object->set_native(0);

                return object->shared_from_this();
            }

            if(!isdigit(value[0])) {
                object->cls = interpreter->NullClass;
                object->set_native(0);

                return object->shared_from_this();
            }

            size_t pos = 0;
            int result = std::stoi(value, &pos);

            if(pos != value.size()) {
                object->cls = interpreter->NullClass;
                object->set_native(0);

                return object->shared_from_this();
            }

            object->cls = interpreter->IntegerClass;
            object->set_native(result);

                return object->shared_from_this();
        });

            StringClass->instance_functions["to_integer"] = std::make_shared<andy::lang::function>("to_integer", [](andy::lang::interpreter* interpreter) {
                std::string value = interpreter->current_context->self->as<std::string>();

            if(value.empty()) return std::make_shared<andy::lang::object>(interpreter->NullClass);

            if(!isdigit(value[0])) return std::make_shared<andy::lang::object>(interpreter->NullClass);

            size_t pos = 0;
            int result = std::stoi(value, &pos);

            if(pos != value.size()) return std::make_shared<andy::lang::object>(interpreter->NullClass);

            return andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, result);
        });

        StringClass->instance_functions["erase!"] = std::make_shared<andy::lang::function>("erase!", std::initializer_list<std::string>{"start", "size"}, [](andy::lang::interpreter* interpreter) {
            std::string& value = interpreter->current_context->self->as<std::string>();
            const auto& params = interpreter->current_context->positional_params;
            size_t start = params[0]->as<int32_t>();
            size_t size = params[1]->as<int32_t>();

            value.erase(start, size);

            return nullptr;
        });

    StringClass->instance_functions["starts_with?"] = std::make_shared<andy::lang::function>("starts_with?", std::initializer_list<std::string>{"what"}, [](andy::lang::interpreter* interpreter) {
        std::string& value = interpreter->current_context->self->as<std::string>();
        const std::string& what = interpreter->current_context->positional_params[0]->as<std::string>();

        bool starts = value.starts_with(what);

        return andy::lang::api::to_object(interpreter, starts);
    });

    StringClass->instance_functions["ends_with?"] = std::make_shared<andy::lang::function>("ends_with?", std::initializer_list<std::string>{"what"}, [](andy::lang::interpreter* interpreter) {
        std::string& value = interpreter->current_context->self->as<std::string>();
        const std::string& what = interpreter->current_context->positional_params[0]->as<std::string>();

        bool ends = value.ends_with(what);

        return andy::lang::api::to_object(interpreter, ends);
    });

        StringClass->instance_functions["=="] = std::make_shared<andy::lang::function>("==", std::initializer_list<std::string>{"other"}, [](andy::lang::interpreter* interpreter) {
            const std::string& value = interpreter->current_context->self->as<std::string>();
            const std::string& other = interpreter->current_context->positional_params[0]->as<std::string>();

            if(value == other) {
                return std::make_shared<andy::lang::object>(interpreter->TrueClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        });

        StringClass->instance_functions["!="] = std::make_shared<andy::lang::function>("!=", std::initializer_list<std::string>{"other"}, [](andy::lang::interpreter* interpreter) {
            const std::string& value = interpreter->current_context->self->as<std::string>();
            const std::string& other = interpreter->current_context->positional_params[0]->as<std::string>();

            if(value != other) {
                return std::make_shared<andy::lang::object>(interpreter->TrueClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        });

        StringClass->instance_functions["+"] = std::make_shared<andy::lang::function>("+", std::initializer_list<std::string>{"other"}, [StringClass](andy::lang::interpreter* interpreter) {
            const std::string& value = interpreter->current_context->self->as<std::string>();
            const std::string& other = interpreter->current_context->positional_params[0]->as<std::string>();

            return andy::lang::object::instantiate(interpreter, StringClass, value + other);
        });

        StringClass->instance_functions["size"] = std::make_shared<andy::lang::function>("size", [](andy::lang::interpreter* interpreter) {
            const std::string& value = interpreter->current_context->self->as<std::string>();
            return andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, (int32_t)value.size());
        });

        StringClass->instance_functions["empty?"] = std::make_shared<andy::lang::function>("empty?", [](andy::lang::interpreter* interpreter) {
            const std::string& value = interpreter->current_context->self->as<std::string>();

            if(value.empty()) {
                return std::make_shared<andy::lang::object>(interpreter->TrueClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        });

        StringClass->instance_functions["include?"] = std::make_shared<andy::lang::function>("include?", std::initializer_list<std::string>{"other"}, [](andy::lang::interpreter* interpreter) {
            const std::string& value = interpreter->current_context->self->as<std::string>();
            const std::string& other = interpreter->current_context->positional_params[0]->as<std::string>();

            if(value.find(other) != std::string::npos) {
                return std::make_shared<andy::lang::object>(interpreter->TrueClass);
            }

            return std::make_shared<andy::lang::object>(interpreter->FalseClass);
        });

        StringClass->instance_functions["capitalize!"] = std::make_shared<andy::lang::function>("capitalize!", [](andy::lang::interpreter* interpreter) {
            std::string& value = interpreter->current_context->self->as<std::string>();

            if(!value.empty()) {
                value[0] = std::toupper(value[0]);
            }

            return nullptr;
        });

    StringClass->instance_functions["size"] = std::make_shared<andy::lang::function>("size", [](andy::lang::interpreter* interpreter) {
        const std::string& value = interpreter->current_context->self->as<std::string>();
        return andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, (int)value.size());
    });
    
    
    return StringClass;
}