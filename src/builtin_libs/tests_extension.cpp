#include <filesystem>
#include <fstream>

#include <andy/lang/api.hpp>
#include <andy/lang/extension.hpp>

#include <andy/tests.hpp>

context_or_describe of;
static bool first = true;

template<typename T>
inline static void add_describe_like(andy::lang::interpreter* interpreter, std::string_view name) {
    interpreter->StdClass->class_methods[name] = andy::lang::method(name, andy::lang::method_storage_type::class_method, { "what" }, [interpreter](andy::lang::function_call& call) {
        std::string& what = call.positional_params[0]->as<std::string>();
        T d = T(what, [interpreter,call]() {
            andy::lang::method yield_method;
            yield_method.name = "yield";
            if(call.given_block == nullptr) {
                throw std::runtime_error("yield called without a block");
            }
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
        if constexpr(!std::is_same_v<T, andy::tests::it> && !std::is_same_v<T, andy::tests::pending>) {
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
            { "to", andy::lang::method("to", andy::lang::method_storage_type::instance_method, { "matcher_result" }, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
                // TODO refactor
                auto matcher = params[0];
                if(matcher->cls->name != "Matcher") {
                    throw std::runtime_error("Argument to 'to' must be a Matcher, got " + std::string(matcher->cls->name));
                }
                auto matcher_params = matcher->instance_variables["params"];
                if(matcher->instance_variables["name"]->as<std::string>() == "eq") {
                    auto actual_object = object->as<std::shared_ptr<andy::lang::object>>();
                    auto matcher_params_object = matcher_params->as<std::vector<std::shared_ptr<andy::lang::object>>>();
                    auto expected_object = matcher_params_object[0];

                    andy::lang::function_call eq_call = {
                        "==",
                        actual_object->cls,
                        actual_object,
                        nullptr,
                        { expected_object },
                        { },
                    };

                    bool result = andy::lang::api::call<bool>(interpreter, eq_call);

                    if(!result) {
                        std::string actual   = andy::lang::api::call<std::string>(interpreter, andy::lang::function_call("to_string", actual_object));
                        std::string expected = andy::lang::api::call<std::string>(interpreter, andy::lang::function_call("to_string", expected_object));

                        throw std::runtime_error("Expected " + expected + ", got " + actual);
                    }
                }

                return nullptr;
            }) },
        };

        andy::lang::api::contained_class(interpreter, interpreter->StdClass, expect_class);

        add_describe_like<andy::tests::context>(interpreter, "context");
        add_describe_like<andy::tests::describe>(interpreter, "describe");
        add_describe_like<andy::tests::it>(interpreter, "it");
        add_describe_like<andy::tests::pending>(interpreter, "pending");

        auto matcher_result_class = std::make_shared<andy::lang::structure>("Matcher");
        interpreter->load(matcher_result_class);
        interpreter->StdClass->class_methods["expect"] = andy::lang::method("expect", andy::lang::method_storage_type::class_method, { "object" }, [=](andy::lang::function_call& call) {
            return andy::lang::object::create(interpreter, expect_class, call.positional_params[0]);
        });
        interpreter->StdClass->class_methods["eq"] = andy::lang::method("eq", andy::lang::method_storage_type::class_method, { "what" }, [interpreter,matcher_result_class](andy::lang::function_call& call) {
            auto matcher = std::make_shared<andy::lang::object>(matcher_result_class);
            std::vector<std::shared_ptr<andy::lang::object>> matcher_params;
            matcher_params.push_back(call.positional_params[0]);
            auto matcher_params_object = andy::lang::object::create(interpreter, interpreter->ArrayClass, std::move(matcher_params));
            matcher->instance_variables["name"] = andy::lang::api::to_object(interpreter, "eq");
            matcher->instance_variables["params"] = matcher_params_object;
            return matcher;
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