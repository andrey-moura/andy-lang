#include <iostream>

#include "andy/lang/api.hpp"

#include "andy/ui/window.hpp"
#include "andy/widgets.hpp"

class andylang_ui_window : public andy::ui::window
{
public:
    andylang_ui_window(std::string_view title, andy::lang::interpreter* __interpreter, std::shared_ptr<andy::lang::object> __window_instance)
        : andy::ui::window(title), interpreter(__interpreter)
    {
        interpreter = __interpreter;
        window_instance = __window_instance.get();
    }
private:
    andy::lang::interpreter* interpreter = nullptr;
    // Do not use shared_ptr here, as it will cause a circular reference and window_instance will never be released.
    andy::lang::object* window_instance = nullptr;
};

std::shared_ptr<andy::lang::structure> create_window_class(andy::lang::interpreter* interpreter)
{
    auto window_class = std::make_shared<andy::lang::structure>("Window");
    window_class->instance_methods = {
        {"new", andy::lang::method("new", andy::lang::method_storage_type::instance_method, {"title"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string_view title = params[0]->as<std::string>();
            auto window = std::make_shared<andylang_ui_window>(title, interpreter, object->derived_instance);
            object->set_native(std::move(window));

            return nullptr;
        })},
        { "show", andy::lang::method("show", andy::lang::method_storage_type::instance_method, { "maximized: false" }, [](andy::lang::function_call& call) {
            std::shared_ptr<andy::lang::object> maximized = call.named_params["maximized"];
            auto window = call.object->as<std::shared_ptr<andylang_ui_window>>();
            window->show(maximized->is_present());

            return nullptr;
        })},
        // { "set_page", andy::lang::method("set_page", andy::lang::method_storage_type::instance_method, {"page"}, [](andy::lang::function_call& call) {
        //     auto page = call.positional_params[0];
        //     auto window = call.object->as<std::shared_ptr<andylang_ui_window>>();
        //     window->set_page(page->as<std::shared_ptr<andy::ui::page>>());

        //     return nullptr;
        // })},
    };
    return window_class;
}