#include "andy/lang/object.hpp"
#include "andy/lang/interpreter.hpp"
#include "andy/lang/api.hpp"

std::shared_ptr<andy::lang::structure> create_class_class(andy::lang::interpreter* interpreter)
{
    auto cls = std::make_shared<andy::lang::structure>("Class");

    cls->instance_methods["new"] = andy::lang::method("new", andy::lang::method_storage_type::class_method, { "class_name" }, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        const std::string& class_name = params[0]->as<std::string>();

        auto cls = interpreter->find_class(class_name);

        if(!cls) {
            throw std::runtime_error("class " + class_name + " not found");
        }

        object->instance_variables["name"] = params[0];
        object->set_native<std::shared_ptr<andy::lang::structure>>(cls);
        return nullptr;
    });

    return cls;
}