#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include "andy/lang/api.hpp"
#include "andy/lang/extension.hpp"
#include "andy/ui/app.hpp"
#include "andy/file.hpp"
#include "andy/xml.hpp"

extern std::shared_ptr<andy::lang::structure> create_app_class(andy::lang::interpreter* interpreter);
extern std::shared_ptr<andy::lang::structure> create_frame_class(andy::lang::interpreter* interpreter);

std::filesystem::path view_folder;
uva::xml::schema schema;

#if ANDY_USE_SDL3
#   include <SDL3/SDL.h>
#endif

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

        auto AppClass = create_app_class(interpreter);
        auto FrameClass = create_frame_class(interpreter);
        // auto DialogClass = create_dialog_class(interpreter);

        auto app_obj    = andy::lang::object::create(interpreter, interpreter->ClassClass, AppClass);
        auto frame_obj  = andy::lang::object::create(interpreter, interpreter->ClassClass, FrameClass);
        // auto dialog_obj = andy::lang::object::create(interpreter, interpreter->ClassClass, DialogClass);

        app_obj->cls->instance_methods["new"].call(app_obj);
        app_obj->cls->instance_methods["new"].call(frame_obj);
        // app_obj->cls->instance_methods["new"].call(dialog_obj);

        UIClass->class_variables = {
            { "Application", app_obj    },
            { "Frame",       frame_obj  },
            // { "Dialog",      dialog_obj },
        };

        interpreter->load(UIClass);
    }

    virtual void start(andy::lang::interpreter* interpreter) override
    {
        view_folder = std::filesystem::absolute("views");
        std::string schema_content = uva::file::read_all_text<char>(view_folder / "schema.xsd");
        schema = uva::xml::decode(std::move(schema_content));

#if ANDY_USE_SDL3
        // Get the OS theme name (ie. "dark" or "light")
        SDL_SystemTheme theme = SDL_GetSystemTheme();
        theme = theme == SDL_SYSTEM_THEME_UNKNOWN ? SDL_SYSTEM_THEME_DARK : theme;
        switch(theme) {
            case SDL_SYSTEM_THEME_DARK:
                for(auto& element : schema.elements) {
                    for(auto& attribute : element.attributes) {
                        if(attribute.name == "background-color") {
                            attribute.default_value = "#121212";
                        }
                    }
                }
                break;
        }
#endif
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