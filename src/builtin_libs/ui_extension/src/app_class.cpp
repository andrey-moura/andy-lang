#include <memory>

#include <andy/lang/api.hpp>
#include <andy/ui/app.hpp>

class andylang_ui_app : public andy::ui::app
{
public:
    andylang_ui_app(andy::lang::interpreter* __interpreter, std::shared_ptr<andy::lang::object> __application_instance)
        : andy::ui::app("andy", "andy"), interpreter(__interpreter)
    {
        application_instance = __application_instance;
    }
protected:
    andy::lang::interpreter* interpreter = nullptr;
    std::shared_ptr<andy::lang::object> application_instance;
public:
    virtual void on_init(int argc, char** argv) override
    {
        auto run_it = application_instance->cls->instance_methods.find("init");

        if(run_it == application_instance->cls->instance_methods.end()) {
            throw std::runtime_error("function 'init' is not defined in type " + application_instance->cls->name);
        }

        andy::lang::function_call run_it_call = {
            "init",
            application_instance->cls,
            application_instance,
            &run_it->second,
            {},
            {},
            nullptr
        };

        interpreter->call(run_it_call);
    }
};

std::shared_ptr<andy::lang::structure> create_app_class(andy::lang::interpreter* interpreter)
{
    auto AppClass = std::make_shared<andy::lang::structure>("Application");

    AppClass->instance_methods = {
        { "new", andy::lang::method("new", andy::lang::method_storage_type::instance_method, {}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params){
            std::shared_ptr<andy::ui::app> app = std::make_shared<andylang_ui_app>(interpreter, object->derived_instance);
            object->set_native(app);
            return nullptr;
        })},
    };

    return AppClass;
}