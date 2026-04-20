#include "andy/lang/object.hpp"
#include "andy/lang/interpreter.hpp"
#include "andy/lang/api.hpp"

std::shared_ptr<andy::lang::structure> create_class_class(andy::lang::interpreter* interpreter)
{
    auto cls = std::make_shared<andy::lang::structure>("Class");

    cls->functions["new"] = std::make_shared<andy::lang::function>("new", std::initializer_list<std::string>{ "class_name" }, [](andy::lang::interpreter* interpreter) {
        auto object = interpreter->current_context->self;
        auto& params = interpreter->current_context->positional_params;
        if(params.size() == 1) {
            const std::string& class_name = params[0]->as<std::string>();

            auto cls = interpreter->find_class(class_name);

            if(!cls) {
                throw std::runtime_error("class " + class_name + " not found");
            }

            object->variables["name"] = params[0];
            object->set_native<std::shared_ptr<andy::lang::structure>>(cls);
        } else {
            // Called from interpreter
            object->variables["name"] = andy::lang::api::to_object(interpreter, object->as<std::shared_ptr<andy::lang::structure>>()->name);
        }
        return nullptr;
    });

    return cls;
}