#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include "andy/lang/api.hpp"
#include "andy/lang/extension.hpp"
#include "andy/file.hpp"
#include "andy/xml.hpp"

extern std::shared_ptr<andy::lang::structure> create_app_class(andy::lang::interpreter* interpreter);
extern std::shared_ptr<andy::lang::structure> create_window_class(andy::lang::interpreter* interpreter);
extern std::shared_ptr<andy::lang::structure> create_xml_page_class(andy::lang::interpreter* interpreter);

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

        auto classes = {
            create_app_class(interpreter),
            create_window_class(interpreter),
        };
    
        for (auto& cls : classes) {
            andy::lang::api::contained_class(interpreter, UIClass, cls);
        }

        interpreter->load(UIClass);
    }

    virtual void start(andy::lang::interpreter* interpreter) override
    {
        auto UI = interpreter->find_class("UI");

        if(!UI) {
            throw std::runtime_error("UI namespace not found");
        }

        auto AppClass = UI->class_variables["Application"]->as<std::shared_ptr<andy::lang::structure>>();

        if(AppClass->deriveds.empty()) {
            throw std::runtime_error("UI.Application has no derived classes");
        }

        application_instance = andy::lang::object::instantiate(interpreter, AppClass->deriveds.front(), nullptr);

        auto app = application_instance->base_instance->as<std::shared_ptr<andy::ui::app>>();
        if(!app) {
            throw std::runtime_error("UI.Application is not a valid application");
        }

        app->run(0, nullptr);
    }
};

std::shared_ptr<andy::lang::extension> create_ui_extension()
{
    return std::make_shared<ui_extension>();
}