#include <andy/lang/lang.hpp>
#include <andy/lang/interpreter.hpp>
#include <andy/lang/api.hpp>

std::shared_ptr<andy::lang::structure> create_string_class(andy::lang::interpreter* interpreter)
{
    auto StringClass = std::make_shared<andy::lang::structure>("String");

    StringClass->instance_functions["*"] = std::make_shared<andy::lang::function>("*", std::vector<std::string>{"what"}, [](andy::lang::interpreter* interpreter) {
        const std::string& value = interpreter->current_context->self->as<std::string>();
        std::string result;

        if(interpreter->current_context->positional_params[0]->cls != interpreter->IntegerClass) {
            throw std::runtime_error("undefined operator * (" + std::string(interpreter->current_context->self->cls->name) + ", " + std::string(interpreter->current_context->positional_params[0]->cls->name) + ")");
        }

        int times = interpreter->current_context->positional_params[0]->as<int>();

        for(int i = 0; i < times; i++) {
            result += value;
        }

        interpreter->current_context->return_value = andy::lang::object::instantiate(interpreter, interpreter->current_context->self->cls, result);
    });

    StringClass->instance_functions["present?"] = std::make_shared<andy::lang::function>("present?", std::vector<std::string>{}, [](andy::lang::interpreter* interpreter) {
        const std::string& value = interpreter->current_context->self->as<std::string>();

        if(value.empty()) {
            interpreter->current_context->return_value = std::make_shared<andy::lang::object>(interpreter->FalseClass);
        } else {
            interpreter->current_context->return_value = std::make_shared<andy::lang::object>(interpreter->TrueClass);
        }
    });

    StringClass->instance_functions["to_string"] = std::make_shared<andy::lang::function>("to_string", std::vector<std::string>{}, [](andy::lang::interpreter* interpreter) {
        const std::string& value = interpreter->current_context->self->as<std::string>();
        interpreter->current_context->return_value = andy::lang::object::instantiate(interpreter, interpreter->current_context->self->cls, value);
    });

    StringClass->instance_functions["find"] = std::make_shared<andy::lang::function>("find", std::vector<std::string>{"what"}, [](andy::lang::interpreter* interpreter) {
        const std::string& value = interpreter->current_context->self->as<std::string>();
        size_t pos = value.find(interpreter->current_context->positional_params[0]->as<std::string>());
        interpreter->current_context->return_value = andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, (int32_t)pos);
    });

    StringClass->instance_functions["substring"] = std::make_shared<andy::lang::function>("substring", std::vector<std::string>{"start", "size"}, [](andy::lang::interpreter* interpreter) {
        const std::string& value = interpreter->current_context->self->as<std::string>();
        size_t start = interpreter->current_context->positional_params[0]->as<int32_t>();
        size_t size = interpreter->current_context->positional_params[1]->as<int32_t>();
        interpreter->current_context->return_value = andy::lang::object::instantiate(interpreter, interpreter->current_context->self->cls, value.substr(start, size));
    });

    StringClass->instance_functions["to_lower_case!"] = std::make_shared<andy::lang::function>("to_lower_case!", std::vector<std::string>{}, [](andy::lang::interpreter* interpreter) {
        std::string& value = interpreter->current_context->self->as<std::string>();

        for(char& c : value) {
            c = std::tolower(c);
        }
    });

    StringClass->instance_functions["to_lower_case"] = std::make_shared<andy::lang::function>("to_lower_case", std::vector<std::string>{}, [](andy::lang::interpreter* interpreter) {
        std::string value = interpreter->current_context->self->as<std::string>();

        for(char& c : value) {
            c = std::tolower(c);
        }

        interpreter->current_context->return_value = andy::lang::object::instantiate(interpreter, interpreter->current_context->self->cls, value);
    });

    StringClass->instance_functions["to_integer!"] = std::make_shared<andy::lang::function>("to_integer!", std::vector<std::string>{}, [](andy::lang::interpreter* interpreter) {
        auto self = interpreter->current_context->self;
        std::string& value = self->as<std::string>();

        if(value.empty()) {
            self->cls = interpreter->NullClass;
            self->set_native(0);
            interpreter->current_context->return_value = self->shared_from_this();
            return;
        }

        if(!isdigit(value[0])) {
            self->cls = interpreter->NullClass;
            self->set_native(0);
            interpreter->current_context->return_value = self->shared_from_this();
            return;
        }

        size_t pos = 0;
        int result = std::stoi(value, &pos);

        if(pos != value.size()) {
            self->cls = interpreter->NullClass;
            self->set_native(0);
            interpreter->current_context->return_value = self->shared_from_this();
            return;
        }

        self->cls = interpreter->IntegerClass;
        self->set_native(result);
        interpreter->current_context->return_value = self->shared_from_this();
    });

    StringClass->instance_functions["to_integer"] = std::make_shared<andy::lang::function>("to_integer", std::vector<std::string>{}, [](andy::lang::interpreter* interpreter) {
        std::string value = interpreter->current_context->self->as<std::string>();

        if(value.empty()) {
            interpreter->current_context->return_value = std::make_shared<andy::lang::object>(interpreter->NullClass);
            return;
        }

        if(!isdigit(value[0])) {
            interpreter->current_context->return_value = std::make_shared<andy::lang::object>(interpreter->NullClass);
            return;
        }

        size_t pos = 0;
        int result = std::stoi(value, &pos);

        if(pos != value.size()) {
            interpreter->current_context->return_value = std::make_shared<andy::lang::object>(interpreter->NullClass);
            return;
        }

        interpreter->current_context->return_value = andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, result);
    });

    StringClass->instance_functions["erase!"] = std::make_shared<andy::lang::function>("erase!", std::vector<std::string>{"start", "size"}, [](andy::lang::interpreter* interpreter) {
        std::string& value = interpreter->current_context->self->as<std::string>();
        size_t start = interpreter->current_context->positional_params[0]->as<int32_t>();
        size_t size = interpreter->current_context->positional_params[1]->as<int32_t>();
        value.erase(start, size);
    });

    StringClass->instance_functions["starts_with?"] = std::make_shared<andy::lang::function>("starts_with?", std::vector<std::string>{"what"}, [](andy::lang::interpreter* interpreter) {
        std::string& value = interpreter->current_context->self->as<std::string>();
        const std::string& what = interpreter->current_context->positional_params[0]->as<std::string>();
        bool starts = value.starts_with(what);
        interpreter->current_context->return_value = andy::lang::api::to_object(interpreter, starts);
    });

    StringClass->instance_functions["ends_with?"] = std::make_shared<andy::lang::function>("ends_with?", std::vector<std::string>{"what"}, [](andy::lang::interpreter* interpreter) {
        std::string& value = interpreter->current_context->self->as<std::string>();
        const std::string& what = interpreter->current_context->positional_params[0]->as<std::string>();
        bool ends = value.ends_with(what);
        interpreter->current_context->return_value = andy::lang::api::to_object(interpreter, ends);
    });

    StringClass->instance_functions["=="] = std::make_shared<andy::lang::function>("==", std::vector<std::string>{"other"}, [](andy::lang::interpreter* interpreter) {
        const std::string& value = interpreter->current_context->self->as<std::string>();
        const std::string& other = interpreter->current_context->positional_params[0]->as<std::string>();

        if(value == other) {
            interpreter->current_context->return_value = std::make_shared<andy::lang::object>(interpreter->TrueClass);
        } else {
            interpreter->current_context->return_value = std::make_shared<andy::lang::object>(interpreter->FalseClass);
        }
    });

    StringClass->instance_functions["!="] = std::make_shared<andy::lang::function>("!=", std::vector<std::string>{"other"}, [](andy::lang::interpreter* interpreter) {
        const std::string& value = interpreter->current_context->self->as<std::string>();
        const std::string& other = interpreter->current_context->positional_params[0]->as<std::string>();

        if(value != other) {
            interpreter->current_context->return_value = std::make_shared<andy::lang::object>(interpreter->TrueClass);
        } else {
            interpreter->current_context->return_value = std::make_shared<andy::lang::object>(interpreter->FalseClass);
        }
    });

    StringClass->instance_functions["+"] = std::make_shared<andy::lang::function>("+", std::vector<std::string>{"other"}, [](andy::lang::interpreter* interpreter) {
        const std::string& value = interpreter->current_context->self->as<std::string>();
        const std::string& other = interpreter->current_context->positional_params[0]->as<std::string>();
        interpreter->current_context->return_value = andy::lang::object::instantiate(interpreter, interpreter->current_context->self->cls, value + other);
    });

    StringClass->instance_functions["size"] = std::make_shared<andy::lang::function>("size", std::vector<std::string>{}, [](andy::lang::interpreter* interpreter) {
        const std::string& value = interpreter->current_context->self->as<std::string>();
        interpreter->current_context->return_value = andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, (int32_t)value.size());
    });

    StringClass->instance_functions["empty?"] = std::make_shared<andy::lang::function>("empty?", std::vector<std::string>{}, [](andy::lang::interpreter* interpreter) {
        const std::string& value = interpreter->current_context->self->as<std::string>();

        if(value.empty()) {
            interpreter->current_context->return_value = std::make_shared<andy::lang::object>(interpreter->TrueClass);
        } else {
            interpreter->current_context->return_value = std::make_shared<andy::lang::object>(interpreter->FalseClass);
        }
    });

    StringClass->instance_functions["include?"] = std::make_shared<andy::lang::function>("include?", std::vector<std::string>{"other"}, [](andy::lang::interpreter* interpreter) {
        const std::string& value = interpreter->current_context->self->as<std::string>();
        const std::string& other = interpreter->current_context->positional_params[0]->as<std::string>();

        if(value.find(other) != std::string::npos) {
            interpreter->current_context->return_value = std::make_shared<andy::lang::object>(interpreter->TrueClass);
        } else {
            interpreter->current_context->return_value = std::make_shared<andy::lang::object>(interpreter->FalseClass);
        }
    });

    StringClass->instance_functions["capitalize!"] = std::make_shared<andy::lang::function>("capitalize!", std::vector<std::string>{}, [](andy::lang::interpreter* interpreter) {
        std::string& value = interpreter->current_context->self->as<std::string>();

        if(!value.empty()) {
            value[0] = std::toupper(value[0]);
        }
    });

    return StringClass;
}