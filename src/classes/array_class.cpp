#include <andy/lang/lang.hpp>
#include <andy/lang/api.hpp>
#include <andy/lang/interpreter.hpp>
#include <andy/lang/error.hpp>

std::shared_ptr<andy::lang::structure> create_array_class(andy::lang::interpreter* interpreter)
{
    auto ArrayClass = std::make_shared<andy::lang::structure>("Array");

        ArrayClass->instance_functions["to_string"] = std::make_shared<andy::lang::function>("to_string", [](andy::lang::interpreter* interpreter) {
            std::string result = "[";

            std::vector<std::shared_ptr<andy::lang::object>>& items = interpreter->current_context->self->as<std::vector<std::shared_ptr<andy::lang::object>>>();

            for(auto& item : items) {
                if(result.size() > 1) {
                    result += ", ";
                }

                result += andy::lang::api::call(interpreter, "to_string", item)->as<std::string>();
            }

            result += "]";

            return andy::lang::object::instantiate(interpreter, interpreter->StringClass, std::move(result));
        });

        ArrayClass->instance_functions["join"] = std::make_shared<andy::lang::function>("join", std::vector<std::string>{"separator"}, [](andy::lang::interpreter* interpreter) {
            const std::string& separator = interpreter->current_context->positional_params[0]->as<std::string>();
            std::string result;

            std::vector<std::shared_ptr<andy::lang::object>>& items = interpreter->current_context->self->as<std::vector<std::shared_ptr<andy::lang::object>>>();

            for(auto& item : items) {
                if(result.size()) {
                    result += separator;
                }

                result += andy::lang::api::call(interpreter, "to_string", item)->as<std::string>();
            }

            return andy::lang::object::instantiate(interpreter, interpreter->StringClass, std::move(result));
        });

        ArrayClass->instance_functions["front"] = std::make_shared<andy::lang::function>("front", [](andy::lang::interpreter* interpreter) {
            std::vector<std::shared_ptr<andy::lang::object>>& items = interpreter->current_context->self->as<std::vector<std::shared_ptr<andy::lang::object>>>();

            if(items.empty()) {
                return std::make_shared<andy::lang::object>(interpreter->NullClass);
            }

            return items.front();
        });

        ArrayClass->instance_functions["size"] = std::make_shared<andy::lang::function>("size", [](andy::lang::interpreter* interpreter) {
            std::vector<std::shared_ptr<andy::lang::object>>& items = interpreter->current_context->self->as<std::vector<std::shared_ptr<andy::lang::object>>>();

            return andy::lang::object::instantiate(interpreter, interpreter->IntegerClass, items.size());
        });

    ArrayClass->instance_functions["push"] = std::make_shared<andy::lang::function>("push", std::initializer_list<std::string>{"item"}, [](andy::lang::interpreter* interpreter) {
        std::vector<std::shared_ptr<andy::lang::object>>& items = interpreter->current_context->self->as<std::vector<std::shared_ptr<andy::lang::object>>>();

        items.push_back(interpreter->current_context->positional_params[0]->native_copy());

        return nullptr;
    });

    ArrayClass->instance_functions["pop"] = std::make_shared<andy::lang::function>("pop", [](andy::lang::interpreter* interpreter) {
        std::vector<std::shared_ptr<andy::lang::object>>& items = interpreter->current_context->self->as<std::vector<std::shared_ptr<andy::lang::object>>>();

        if(items.empty()) {
            return std::make_shared<andy::lang::object>(interpreter->NullClass);
        }

        auto item = items.back();
        items.pop_back();

        return item;
    });

    ArrayClass->instance_functions["[]"] = std::make_shared<andy::lang::function>("[]", std::initializer_list<std::string>{"index"}, [](andy::lang::interpreter* interpreter) {
        std::vector<std::shared_ptr<andy::lang::object>>& items = interpreter->current_context->self->as<std::vector<std::shared_ptr<andy::lang::object>>>();

        auto index = interpreter->current_context->positional_params[0]->as<int>();

        return items[index];
    });

    ArrayClass->instance_functions["=="] = std::make_shared<andy::lang::function>("==", std::initializer_list<std::string>{"other"}, [](andy::lang::interpreter* interpreter) {
            std::vector<std::shared_ptr<andy::lang::object>>& items = interpreter->current_context->self->as<std::vector<std::shared_ptr<andy::lang::object>>>();
            if(interpreter->current_context->positional_params[0]->cls != interpreter->ArrayClass) {
                return std::make_shared<andy::lang::object>(interpreter->FalseClass);
            }
            auto& other_items = interpreter->current_context->positional_params[0]->as<std::vector<std::shared_ptr<andy::lang::object>>>();
            if(items.size() != other_items.size()) {
                return std::make_shared<andy::lang::object>(interpreter->FalseClass);
            }
            for(size_t i = 0; i < other_items.size(); ++i) {
                auto it = items[i]->cls->instance_functions.find("==");
                if(it == items[i]->cls->instance_functions.end()) {
                    throw std::runtime_error("class " + std::string(items[i]->cls->name) + " does not have a method '=='");
                }
                andy::lang::function_call call{
                    "==",
                    items[i]->cls,
                    items[i],
                    it->second.get(),
                    { other_items[i] }
                };
                auto result = andy::lang::api::call(interpreter, "==", items[i], { other_items[i] });
                if(result->cls == interpreter->FalseClass) {
                    return result;
                }
            }
            return std::make_shared<andy::lang::object>(interpreter->TrueClass);
        });

    
    return ArrayClass;
}