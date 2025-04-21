#include <filesystem>
#include <fstream>

#include <andy/lang/api.hpp>
#include <andy/lang/extension.hpp>

#include <andy/tests.hpp>

context_or_describe of;
static bool first = true;

template<typename T>
inline static void add_describe_like(andy::lang::interpreter* interpreter, std::string_view name) {
    interpreter->StdClass->class_methods[name] = andy::lang::method(std::string(name), andy::lang::method_storage_type::class_method, { "what" }, [interpreter](andy::lang::function_call& call) {
        const std::string& what = call.positional_params[0]->as<std::string>();
        T d = T(what, [interpreter,call]() {
            andy::lang::method yield_method;
            yield_method.name = "yield";
            yield_method.block_ast = *call.given_block;
            andy::lang::function_call yield_call = {
                "yield",
                nullptr,
                nullptr,
                &yield_method,
                {},
                {},
                nullptr
            };
            interpreter->call(yield_call);
        });
        if constexpr(!std::is_same_v<T, andy::tests::it>) {
            if(first) {
                of = std::move(d);
                first = false;
            }
        }
        return nullptr;
    });
}

class tests_extension : public andy::lang::extension
{
public:
    tests_extension()
        : andy::lang::extension("tests")
    {
    }

    void load(andy::lang::interpreter* interpreter) override
    {
        auto expect_class = std::make_shared<andy::lang::structure>("Expect");
        expect_class->instance_methods = {
            { "to_eq", andy::lang::method("to_eq", andy::lang::method_storage_type::instance_method, { "what" }, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
                auto actul_object = object->as<std::shared_ptr<andy::lang::object>>();
                auto eq_method = actul_object->cls->instance_methods.find("==");

                if(eq_method == actul_object->cls->instance_methods.end()) {
                    throw std::runtime_error("object of class " + actul_object->cls->name + " does not have a function called '=='");
                }

                andy::lang::function_call eq_call = {
                    "==",
                    actul_object->cls,
                    actul_object,
                    &eq_method->second,
                    { params[0] },
                    {},
                    nullptr
                };

                auto result = interpreter->call(eq_call);

                if(result->cls != interpreter->TrueClass) {
                    std::string actual   = andy::lang::api::call<std::string>(interpreter, actul_object,"to_string");
                    std::string expected = andy::lang::api::call<std::string>(interpreter, params[0], "to_string");

                    throw std::runtime_error("Expected " + expected + ", got " + actual);
                }

                return nullptr;
            }) },
        };

        andy::lang::api::contained_class(interpreter, interpreter->StdClass, expect_class);

        add_describe_like<andy::tests::context>(interpreter, "context");
        add_describe_like<andy::tests::describe>(interpreter, "describe");
        add_describe_like<andy::tests::it>(interpreter, "it");

        // Convert the following to Andy:
        // expect(interpreter).to<eq>(1);

        // Which becomes:
        // expect(interpreter).to_eq(1);

        interpreter->StdClass->class_methods["expect"] = andy::lang::method("expect", andy::lang::method_storage_type::class_method, { "object" }, [=](andy::lang::function_call& call) {
            return andy::lang::object::create(interpreter, expect_class, call.positional_params[0]);
        });
    }

    void start(andy::lang::interpreter* interpreter) override
    {
        andy::tests::run();
    }
};

std::shared_ptr<andy::lang::extension> create_tests_extension()
{
    return std::make_shared<tests_extension>();
}