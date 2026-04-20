#include "app_class.hpp"

#include <memory>

#include <andy/lang/api.hpp>

andylang_ui_app::andylang_ui_app(andy::lang::interpreter* __interpreter, std::shared_ptr<andy::lang::object> __app_instance)
    : andy::ui::app(), interpreter(__interpreter)
{
    app_instance = __app_instance.get();
}

void andylang_ui_app::on_init()
{
    andy::lang::api::call(interpreter, "init", app_instance->shared_from_this());
}

std::shared_ptr<andy::lang::structure> create_app_class(andy::lang::interpreter* interpreter)
{
    auto AppClass = std::make_shared<andy::lang::structure>("Application");

    AppClass->functions["new"] = std::make_shared<andy::lang::function>("new", [](andy::lang::interpreter* interpreter){
        auto object = interpreter->current_context->self;
        std::shared_ptr<andylang_ui_app> app = std::make_shared<andylang_ui_app>(interpreter, object->derived_instance);
        object->set_native(app);
        return nullptr;
    });

    AppClass->instance_functions["bind"] = std::make_shared<andy::lang::function>("bind", std::initializer_list<std::string>{"event", "on: null", "to: null"}, [](andy::lang::interpreter* interpreter) {
        std::shared_ptr<andy::lang::object> event_name = interpreter->current_context->positional_params[0];
        std::shared_ptr<andy::lang::object> handler_name = interpreter->current_context->named_params["to"];
        std::shared_ptr<andy::lang::object> on_param = interpreter->current_context->named_params["on"];
        auto object = interpreter->current_context->self;

        if(event_name->cls != interpreter->StringClass) {
            throw std::runtime_error("function 'bind' expects a string as first parameter, got '" + std::string(event_name->cls->name) + "'");
        }
        if(handler_name->cls != interpreter->StringClass)
        {
            throw std::runtime_error("function 'bind' expects a function as second parameter, got '" + std::string(handler_name->cls->name) + "'");
        }
        if(on_param->cls != interpreter->StringClass)
        {
            throw std::runtime_error("function 'bind' expects a string as 'on' parameter, got '" + std::string(on_param->cls->name) + "'");
        }

        std::string event_name_str = event_name->as<std::string>();
        std::string handler_name_str = handler_name->as<std::string>();
        std::string on_str = on_param->as<std::string>();

        auto variable_it = object->derived_instance->variables.find(on_str);

        if(variable_it == object->derived_instance->variables.end()) {
            throw std::runtime_error("variable '" + on_str + "' not found in object of type '" + std::string(object->derived_instance->cls->name) + "'");
        }

        auto variable = variable_it->second;

        auto variable_bindings = variable_it->second->variables.find("bindings");

        if(variable_bindings == variable_it->second->variables.end()) {
            throw std::runtime_error("variable 'bindings' not found in object of type '" + std::string(variable_it->second->cls->name) + "'");
        }

        auto& bindings_map = variable_bindings->second->as<andy::lang::dictionary>();

        auto handler_function = object->derived_instance->cls->instance_functions.find(handler_name_str);

        if(handler_function == object->derived_instance->cls->instance_functions.end()) {
            throw std::runtime_error("function '" + handler_name_str + "' is not defined in type " + std::string(variable_it->second->cls->name));
        }

        auto handler_function_object = andy::lang::api::to_object(interpreter, handler_function->second);

        bindings_map.push_back({event_name, handler_function_object});

        return nullptr;
    });


    return AppClass;
}