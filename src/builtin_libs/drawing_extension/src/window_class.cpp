#include <iostream>

#include "andy/lang/api.hpp"

#include "andy/xml.hpp"
#include "andy/drawing.hpp"

class andylang_drawing_window : public andy::drawing::window
{
public:
    andylang_drawing_window(std::string_view title, andy::lang::interpreter* __interpreter, std::shared_ptr<andy::lang::object> __window_instance)
        : andy::drawing::window(title), interpreter(__interpreter)
    {
        interpreter = __interpreter;
        window_instance = __window_instance.get();
    }
private:
    andy::lang::interpreter* interpreter = nullptr;
    // Do not use shared_ptr here, as it will cause a circular reference and window_instance will never be released.
    andy::lang::object* window_instance = nullptr;
public:
    std::shared_ptr<andy::drawing::page> current_page = nullptr;
public:
    virtual void draw() override
    {
        andy::drawing::software_renderer renderer(this);
        if(current_page) {
            current_page->draw(renderer);
        }
    }
    virtual void closed() override
    {
        auto bindings_it = window_instance->derived_instance->instance_variables.find("bindings");
        if(bindings_it == window_instance->derived_instance->instance_variables.end()) {
            return;
        }
        auto& bindings = bindings_it->second->as<andy::lang::dictionary>();
        for (auto& [key_obj, value_obj] : bindings) {
            if(key_obj->cls != interpreter->StringClass) {
                continue;
            }
            if(key_obj->as<std::string>() != "closed") {
                continue;
            }
            if(value_obj->cls != interpreter->FunctionClass) {
                throw std::runtime_error("binding for 'closed' event is not a function, got '" + std::string(value_obj->cls->name) + "'");
            }
            andy::lang::function_call call = {
                "closed",
                window_instance->derived_instance->cls,
                window_instance->derived_instance->shared_from_this(),
                value_obj->as<andy::lang::function*>(),
                {},
                {},
                nullptr
            };
        }
    }
};

std::shared_ptr<andy::lang::structure> create_window_class(andy::lang::interpreter* interpreter)
{
    auto window_class = std::make_shared<andy::lang::structure>("Window");
    window_class->instance_functions = {
        {"new", andy::lang::function("new", andy::lang::function_storage_type::instance_function, {"title"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string_view title = params[0]->as<std::string>();
            auto window = std::make_shared<andylang_drawing_window>(title, interpreter, object);
            object->set_native(std::move(window));
            object->instance_variables["bindings"] = andy::lang::api::to_object(interpreter, andy::lang::dictionary{});

            return nullptr;
        })},
        { "show", andy::lang::function("show", andy::lang::function_storage_type::instance_function, { "maximized: false" }, [](andy::lang::function_call& call) {
            std::shared_ptr<andy::lang::object> maximized = call.named_params["maximized"];
            auto window = call.object->as<std::shared_ptr<andylang_drawing_window>>();
            window->show(maximized->is_present());

            return nullptr;
        })},
        { "set_page", andy::lang::function("set_page", andy::lang::function_storage_type::instance_function, { "page" }, [interpreter](andy::lang::function_call& call) {
            std::shared_ptr<andy::lang::object> page_name = call.positional_params[0];
            if(page_name->cls != interpreter->StringClass) {
                throw std::runtime_error("function 'set_page' expects a string as parameter, got '" + std::string(page_name->cls->name) + "'");
            }

            std::string view_filename = page_name->as<std::string>();
            view_filename += "_page.xml";
#ifdef ANDY_DRAWING_EMSCRIPTEN
            std::string xml_content = load_file_emscripten("pages/" + view_filename);
            if(xml_content.empty()) {
                throw std::runtime_error("file 'pages/" + view_filename + "' not found");
            }
            andy::xml xml = andy::xml::decode(xml_content);
            andy::xml::schema schema = andy::xml::schema(andy::xml::decode("pages/schema.xsd"));
#else
            std::filesystem::path views_file = std::filesystem::absolute("pages");
            views_file /= view_filename;
            if(!std::filesystem::exists(views_file)) {
                throw std::runtime_error("file '" + views_file.string() + "' not found");
            }
            andy::xml xml = andy::xml::decode(views_file);
            std::filesystem::path schema_file_path = std::filesystem::absolute("pages/schema.xsd");
            andy::xml::schema schema = andy::xml::schema(andy::xml::decode(schema_file_path));
#endif
            call.object->as<std::shared_ptr<andylang_drawing_window>>()->current_page = std::make_shared<andy::drawing::page>(std::move(schema), std::move(xml));

            return nullptr;
        })},
    };
    return window_class;
}