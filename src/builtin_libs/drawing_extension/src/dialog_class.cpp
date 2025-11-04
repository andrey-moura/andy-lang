#include <iostream>

#include "andy/lang/api.hpp"

#include "andy/xml.hpp"
#include "andy/string.hpp"
#include "andy/drawing/dialog.hpp"
#include "andy/drawing/page.hpp"

class andylang_drawing_dialog : public andy::drawing::dialog
{
public:
    andylang_drawing_dialog(std::string_view title, andy::lang::interpreter* __interpreter, std::shared_ptr<andy::lang::object> __dialog_instance)
        : andy::drawing::dialog(title), interpreter(__interpreter)
    {
        interpreter = __interpreter;
        dialog_instance = __dialog_instance.get();
    }
private:
    andy::lang::interpreter* interpreter = nullptr;
    // Do not use shared_ptr here, as it will cause a circular reference and dialog_instance will never be released.
    andy::lang::object* dialog_instance = nullptr;
};

std::shared_ptr<andy::lang::structure> create_dialog_class(andy::lang::interpreter* interpreter)
{
    auto dialog_class = std::make_shared<andy::lang::structure>("Dialog");
        dialog_class->instance_functions["new"] = std::make_shared<andy::lang::function>("new", andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"title"}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            std::string_view title = params[0]->as<std::string>();
            auto dialog = std::make_shared<andylang_drawing_dialog>(title, interpreter, object->derived_instance);
            object->set_native(std::move(dialog));

            std::filesystem::path views_file = std::filesystem::absolute("views");
            std::string snake_case_class_name = andy::string::to_snake_case(std::string(object->derived_instance->cls->name));
            views_file /= snake_case_class_name;
            views_file.replace_extension(".xml");
            if(std::filesystem::exists(views_file)) {
                andy::xml xml = andy::xml::decode(views_file);
                std::filesystem::path schema_file_path = std::filesystem::absolute("views/schema.xsd");
                andy::xml::schema schema = andy::xml::schema(andy::xml::decode(schema_file_path));
                auto page = std::make_shared<andy::drawing::page>(std::move(schema), std::move(xml));
                auto page_class_call = andy::lang::function_call("Drawing.Page", nullptr);
                auto page_class_object = interpreter->call(page_class_call);
                auto page_instance = andy::lang::object::create(interpreter, page_class_object->as<std::shared_ptr<andy::lang::structure>>(), std::move(page));
            }
            return nullptr;
        });

    dialog_class->instance_functions["show"] = std::make_shared<andy::lang::function>("show", andy::lang::function_storage_type::instance_function, std::initializer_list<std::string>{"maximized: false"}, [](andy::lang::function_call& call) {
            std::shared_ptr<andy::lang::object> maximized = call.named_params["maximized"];
            auto dialog = call.object->as<std::shared_ptr<andylang_drawing_dialog>>();
            dialog->show(maximized->is_present());

            return nullptr;
        });

    return dialog_class;
}