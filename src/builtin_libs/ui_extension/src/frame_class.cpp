#include <iostream>

#include "andy/lang/api.hpp"
#include "andy/ui/frame.hpp"

// It is actually andy/
#include "uva/widgets.hpp"
#include "uva/xml.hpp"
#include "uva/file.hpp"

#ifdef ANDY_USE_SDL3
#   include <SDL3/SDL.h>
#endif

extern std::filesystem::path view_folder;
extern uva::xml::schema schema;

static auto find_xml_layout_for_class(void* data, std::string_view class_name)
{
    std::string view_name;
    view_name.reserve(class_name.size() + 10);

    for(const char& c : class_name) {
        if(isupper(c)) {
            if(view_name.size()) {
                view_name.push_back('_');
            }
            view_name.push_back(tolower(c));
        } else {
            view_name.push_back(c);
        }
    }

    std::string_view sufix = "_frame";

    if(view_name.ends_with(sufix)) {
        view_name.erase(view_name.size() - sufix.size());
    } else {
        throw std::runtime_error("Class name " + std::string(class_name) + " does not end with " + std::string(sufix));
    }

    std::filesystem::path view_path = view_folder / (view_name + std::string(sufix));
    view_path.replace_extension(".xml");

    std::string content = uva::file::read_all_text<char>(view_path);
    uva::xml xml = uva::xml::decode(std::move(content));

    uva::widgets::layout root_layout;
    root_layout.parse(data, schema, xml);

    return root_layout;
}

class andylang_ui_frame : public andy::ui::frame
{
public:
    andylang_ui_frame(std::string_view title, andy::lang::interpreter* __interpreter, std::shared_ptr<andy::lang::object> __frame_instance)
        : andy::ui::frame(title), interpreter(__interpreter)
    {
        interpreter = __interpreter;
        frame_instance = __frame_instance.get();
        void* renderer = nullptr;
#ifdef ANDY_USE_SDL3
        SDL_Window* window = (SDL_Window*)m_data;
        renderer = SDL_CreateRenderer(window, nullptr);

        if(!renderer) {
            throw std::runtime_error("Failed to create renderer.");
        }
#endif
        root_layout = find_xml_layout_for_class(renderer, __frame_instance->cls->name);
    }
protected:
    virtual void on_draw() override
    {
#ifdef ANDY_USE_SDL3
        SDL_Window* window = (SDL_Window*)m_data;
        void* renderer = SDL_GetRenderer(window);
#endif
        uva::size size = this->size();
        root_layout.calculate_layout(0, 0, size.w, size.h);
        root_layout.render(renderer);
        SDL_RenderPresent((SDL_Renderer*)renderer);
    }
private:
    andy::lang::interpreter* interpreter = nullptr;
    // Do not use shared_ptr here, as it will cause a circular reference and frame_instance will never be released.
    andy::lang::object* frame_instance;
    uva::widgets::layout root_layout;
};

std::shared_ptr<andy::lang::structure> create_frame_class(andy::lang::interpreter* interpreter)
{
    auto frame_class = std::make_shared<andy::lang::structure>("Frame");
    frame_class->instance_methods = {
        {"new", andy::lang::method("new", andy::lang::method_storage_type::instance_method, {"title"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string_view title = params[0]->as<std::string>();
            auto frame = std::make_shared<andylang_ui_frame>(title, interpreter, object->derived_instance);
            object->set_native(std::move(frame));

            return nullptr;
        })},
        { "show", andy::lang::method("show", andy::lang::method_storage_type::instance_method, {andy::lang::fn_parameter("maximized", true, false)}, [](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params, std::map<std::string, std::shared_ptr<andy::lang::object>> named_params){
            std::shared_ptr<andy::lang::object> maximized = named_params["maximized"];
            auto frame = object->as<std::shared_ptr<andylang_ui_frame>>();
            frame->show(maximized->is_present());

            return nullptr;
        })},
    };
    return frame_class;
}