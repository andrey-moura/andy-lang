#include <iostream>

#include "andy/lang/api.hpp"

#include <andy/ui/frame.hpp>

#include <uva/widgets.hpp>
#include <uva/xml.hpp>

std::shared_ptr<andy::lang::structure> create_frame_class(andy::lang::interpreter* interpreter)
{
    auto frame_class = std::make_shared<andy::lang::structure>("Frame");
    frame_class->instance_methods = {
        {"new", andy::lang::method("new", andy::lang::method_storage_type::instance_method, {"title"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string_view title = params[0]->as<std::string>();
            auto frame = std::make_shared<andy::ui::frame>(title);
            object->set_native(std::move(frame));

            return nullptr;
        })},
        { "show", andy::lang::method("show", andy::lang::method_storage_type::instance_method, {andy::lang::fn_parameter("maximized", true, false)}, [](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params, std::map<std::string, std::shared_ptr<andy::lang::object>> named_params){
            std::shared_ptr<andy::lang::object> maximized = named_params["maximized"];
            auto frame = object->as<std::shared_ptr<andy::ui::frame>>();
            frame->show(maximized->is_present());

            return nullptr;
        })},
    };
    return frame_class;
}