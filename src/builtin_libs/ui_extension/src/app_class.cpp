#include <memory>

#include <andy/lang/api.hpp>

class uvalang_ui_app : public andy::ui::app
{
public:
    uvalang_ui_app(andy::lang::interpreter* __interpreter, std::shared_ptr<andy::lang::object> __application_instance)
        : andy::ui::app("uva", "uva"), interpreter(__interpreter)
    {
        application_instance = __application_instance;
    }
protected:
    andy::lang::interpreter* interpreter = nullptr;
    std::shared_ptr<andy::lang::object> application_instance;
public:
    virtual void on_init(int argc, char** argv) override
    {
        auto run_it = application_instance->cls->instance_methods.find("run");

        if(run_it == application_instance->cls->instance_methods.end()) {
            throw std::runtime_error("function 'run' not defined in class Application");
        }

        andy::lang::function_call run_it_call = {
            "run",
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
            uvalang_ui_app* app = new uvalang_ui_app(interpreter, object->derived_instance);
            object->set_native_ptr(app);

            app->run(0, nullptr);

            return nullptr;
        })},
    };

    return AppClass;
}