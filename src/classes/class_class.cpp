#include <andy/lang/object.hpp>
#include <andy/lang/interpreter.hpp>

std::shared_ptr<andy::lang::structure> create_class_class(andy::lang::interpreter* interpreter)
{
    auto cls = std::make_shared<andy::lang::structure>("Class");

    cls->instance_methods["new"] = andy::lang::method("new", andy::lang::method_storage_type::class_method, { "class_name" }, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        std::shared_ptr<andy::lang::structure> cls;
        if(params.size()) {
            // Called from code
            const std::string& class_name = params[0]->as<std::string>();

            cls = interpreter->find_class(class_name);

            if(!cls) {
                throw std::runtime_error("class " + class_name + " not found");
            }

            object->instance_variables["name"] = params[0];
            object->set_native<std::shared_ptr<andy::lang::structure>>(cls);
        } else {
            // Called from interpreter. It already has native
            cls = object->as<std::shared_ptr<andy::lang::structure>>();

            object->instance_variables["name"] = andy::lang::object::create(interpreter, interpreter->ClassClass, cls->name);
        }

        return nullptr;
    });

    return cls;
}