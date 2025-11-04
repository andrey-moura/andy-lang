#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include "andy/lang/api.hpp"
#include "andy/lang/extension.hpp"
#include "andy/file.hpp"
#include "andy/xml.hpp"

#include "app_class.hpp"

extern std::shared_ptr<andy::lang::structure> create_app_class(andy::lang::interpreter* interpreter);

class ui_extension : public andy::lang::extension
{
public:
    ui_extension()
        : andy::lang::extension("ui")
    {
    }
protected:
    std::shared_ptr<andy::lang::object> application_instance;
public:
    virtual void load(andy::lang::interpreter* interpreter) override
    {
        auto UIClass = std::make_shared<andy::lang::structure>("UI");
            UIClass->class_functions["main"] = std::make_shared<andy::lang::function>("main", andy::lang::function_storage_type::class_function, std::initializer_list<std::string>{"app_class"}, [this, interpreter](andy::lang::function_call& call) {
                auto app_class_class = call.positional_params[0];
                if (!app_class_class || app_class_class->cls != interpreter->ClassClass) {
                    throw std::runtime_error("UI.main expects an class, got " + (app_class_class ? std::string(app_class_class->cls->name) : "null"));
                }
                auto app_class = app_class_class->as<std::shared_ptr<andy::lang::structure>>();
                application_instance = andy::lang::object::instantiate(interpreter, app_class, nullptr,std::initializer_list<std::string>{});
                auto app_native = application_instance->as<std::shared_ptr<andylang_ui_app>>();
                app_native->run();
                return nullptr;
            });


        auto classes = {
            create_app_class(interpreter),
        };
    
        for (auto& cls : classes) {
            andy::lang::api::contained_class(interpreter, UIClass, cls);
        }

        interpreter->load(UIClass);
    }
};

std::shared_ptr<andy::lang::extension> create_ui_extension()
{
    return std::make_shared<ui_extension>();
}