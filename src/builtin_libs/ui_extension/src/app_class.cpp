#include "app_class.hpp"

#include <memory>

andylang_ui_app::andylang_ui_app(andy::lang::interpreter* __interpreter, std::shared_ptr<andy::lang::object> __app_instance)
    : andy::ui::app(), interpreter(__interpreter)
{
    app_instance = __app_instance.get();
}

void andylang_ui_app::on_init()
{
    auto run_it = app_instance->cls->instance_methods.find("init");

    if(run_it == app_instance->cls->instance_methods.end()) {
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

    AppClass->instance_methods = {
        { "new", andy::lang::method("new", andy::lang::method_storage_type::instance_method, {}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params){
            std::shared_ptr<andylang_ui_app> app = std::make_shared<andylang_ui_app>(interpreter, object->derived_instance);
            object->set_native(app);
            return nullptr;
        })},
    };

    return AppClass;
}