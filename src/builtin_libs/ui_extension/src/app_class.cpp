#include "app_class.hpp"

#include <memory>

andylang_ui_app::andylang_ui_app(andy::lang::interpreter* __interpreter, std::shared_ptr<andy::lang::object> __app_instance)
    : andy::ui::app(), interpreter(__interpreter)
{
    app_instance = __app_instance.get();
}

void andylang_ui_app::on_init()
{
    auto run_it = app_instance->cls->instance_functions.find("init");

    if(run_it == app_instance->cls->instance_functions.end()) {
        throw std::runtime_error("function 'init' is not defined in type " + std::string(app_instance->cls->name));
    }

    andy::lang::function_call run_it_call = {
        "init",
        app_instance->cls,
        app_instance->shared_from_this(),
        &run_it->second,
        {},
        {},
        nullptr
    };

    interpreter->call(run_it_call);
}

std::shared_ptr<andy::lang::structure> create_app_class(andy::lang::interpreter* interpreter)
{
    auto AppClass = std::make_shared<andy::lang::structure>("Application");

        AppClass->instance_functions["new"] = std::make_shared<andy::lang::function>("new", andy::lang::function_storage_type::instance_function,std::initializer_list<std::string>{}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params){
            std::shared_ptr<andylang_ui_app> app = std::make_shared<andylang_ui_app>(interpreter, object->derived_instance);
            object->set_native(app);
            return nullptr;
        });

    AppClass->instance_functions["bind"] = std::make_shared<andy::lang::function>("bind", andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"event", "on: null", "to: null"}, [interpreter](andy::lang::function_call& call) {
            std::shared_ptr<andy::lang::object> event_name = call.positional_params[0];
            std::shared_ptr<andy::lang::object> handler_name = call.named_params["to"];
            std::shared_ptr<andy::lang::object> on_param = call.named_params["on"];

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

            auto variable_it = call.object->derived_instance->instance_variables.find(on_str);

            if(variable_it == call.object->derived_instance->instance_variables.end()) {
                throw std::runtime_error("variable '" + on_str + "' not found in object of type '" + std::string(call.object->derived_instance->cls->name) + "'");
            }

            auto variable = variable_it->second;

            auto variable_bindings = variable_it->second->instance_variables.find("bindings");

            if(variable_bindings == variable_it->second->instance_variables.end()) {
                throw std::runtime_error("variable 'bindings' not found in object of type '" + std::string(variable_it->second->cls->name) + "'");
            }

            auto& bindings_map = variable_bindings->second->as<andy::lang::dictionary>();

            auto handler_function = call.object->derived_instance->cls->instance_functions.find(handler_name_str);

            if(handler_function == call.object->derived_instance->cls->instance_functions.end()) {
                throw std::runtime_error("function '" + handler_name_str + "' is not defined in type " + std::string(variable_it->second->cls->name));
            }

            auto handler_function_object = andy::lang::api::to_object(interpreter, handler_function->second);

            bindings_map.push_back({event_name, handler_function_object});

            return nullptr;
        });


    return AppClass;
}