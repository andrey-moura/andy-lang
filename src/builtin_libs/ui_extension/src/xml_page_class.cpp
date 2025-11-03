#include <iostream>

#include "andy/lang/api.hpp"
#include "andy/ui/xml_page.hpp"

// It is actually andy/
#include "andy/widgets.hpp"
#include "andy/xml.hpp"
#include "andy/file.hpp"

class andylang_ui_xml_page : public andy::ui::xml_page
{
public:
    andylang_ui_xml_page(
        std::shared_ptr<andy::xml> xml,
        std::shared_ptr<andy::widgets::layout> layout,
        std::shared_ptr<andy::xml::schema> schema,
        andy::lang::interpreter* __interpreter,
        std::shared_ptr<andy::lang::object> __xml_page_instance
    )
        : andy::ui::xml_page(std::move(xml), std::move(layout), std::move(schema)), interpreter(__interpreter)
    {
        interpreter = __interpreter;
        // xml_page_instance = __xml_page_instance.get();
        void* renderer = nullptr;
    }
private:
    andy::lang::interpreter* interpreter = nullptr;
    // Do not use shared_ptr here, as it will cause a circular reference and xml_page_instance will never be released.
    std::shared_ptr<andy::lang::object> xml_page_instance;
    andy::widgets::layout root_layout;
};

std::shared_ptr<andy::lang::structure> create_xml_page_class(andy::lang::interpreter* interpreter)
{
    auto xml_page_class = std::make_shared<andy::lang::structure>("XMLPage");
    xml_page_class->instance_functions = {
        {"new", andy::lang::function("new", andy::lang::function_storage_type::instance_function, {}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
            auto xml_string = params[0]->as<std::string>();
            //auto xml_page = std::make_shared<andylang_ui_xml_page>(std::move(xml_string), interpreter, object->derived_instance);
            //object->set_native(std::move(xml_page));

            return nullptr;
        })},
    };
    return xml_page_class;
}