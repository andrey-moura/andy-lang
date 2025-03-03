#include <andy/lang/class_class.hpp>

#include <andy/lang/object.hpp>
#include <andy/lang/interpreter.hpp>

namespace andy
{
    namespace lang
    {
        std::shared_ptr<structure> class_class::create(andy::lang::interpreter* interpreter)
        {
            auto cls = std::make_shared<structure>("Class");

            cls->methods["new"] = andy::lang::method("new", method_storage_type::class_method, { "class_name" }, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {

                if(params.size() == 0 || params[0]->cls != interpreter->StringClass) {
                    throw std::runtime_error("Class.new: missing class name");
                }

                const std::string& class_name = params[0]->as<std::string>();

                auto cls = interpreter->find_class(class_name);

                if(!cls) {
                    throw std::runtime_error("class " + class_name + " not found");
                }

                object->instance_variables["name"] = andy::lang::object::instantiate(interpreter, interpreter->StringClass, class_name);
                object->set_native<std::shared_ptr<andy::lang::structure>>(cls);

                return nullptr;
            });

            return cls;
        }
    };
};