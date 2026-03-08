#include <iostream>

#include "andy/file.hpp"
#include "andy/console.hpp"

#include "andy/lang/error.hpp"
#include "andy/lang/interpreter.hpp"
#include "andy/lang/extension.hpp"
#include "andy/lang/lang.hpp"
#include "andy/lang/api.hpp"

andy::lang::function execute_method_definition(const andy::lang::parser::ast_node& class_child)
{
    std::string_view method_name = class_child.decname();

    auto* params_node = class_child.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_params);

    std::vector<andy::lang::fn_parameter> positional_params;
    std::vector<andy::lang::fn_parameter> named_params;

    if(params_node) {
        positional_params.reserve(class_child.childrens().size());
        named_params.reserve(class_child.childrens().size());

        for(auto& param : params_node->childrens()) {
            andy::lang::fn_parameter fn_param;
            if(param.type() == andy::lang::parser::ast_node_type::ast_node_pair) {
                fn_param.name = param.child_content_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
                fn_param.default_value_node = param.child_from_type(andy::lang::parser::ast_node_type::ast_node_valuedecl);
                fn_param.named = true;
                fn_param.has_default_value = fn_param.default_value_node != nullptr;
                named_params.push_back(std::move(fn_param));
            } else {
                fn_param.name = std::string(param.token().content);
                positional_params.push_back(std::move(fn_param));
            }
        }
    }

    auto static_node = class_child.child_from_type(andy::lang::parser::ast_node_type::ast_node_declstatic);

    auto method_type = static_node ? andy::lang::function_storage_type::class_function : andy::lang::function_storage_type::instance_function;

    andy::lang::function method;
    method.name = method_name;
    method.storage_type = method_type;
    method.positional_params = std::move(positional_params);
    method.named_params = std::move(named_params);
    method.block_ast = class_child;
    return method;
}

andy::lang::interpreter::interpreter()
{
    init();
}

void andy::lang::interpreter::load(std::shared_ptr<andy::lang::structure> cls)
{
    cls->functions["subclasses"] = std::make_shared<andy::lang::function>("subclasses", function_storage_type::instance_function, [cls,this](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        std::vector<std::shared_ptr<andy::lang::object>> subclasses;
        subclasses.reserve(cls->deriveds.size());

        for(auto& cls : cls->deriveds) {
            std::vector<std::shared_ptr<andy::lang::object>> params = { andy::lang::object::create(this, StringClass, cls->name) };
            auto c = andy::lang::object::instantiate(this, ClassClass, nullptr, params);
            subclasses.push_back(c);
        }

        return andy::lang::object::create(this, ArrayClass, std::move(subclasses));
    });

    cls->cls = cls;

    current_context->classes[cls->name] = cls;
}

std::shared_ptr<andy::lang::structure> andy::lang::interpreter::find_class(const std::string_view& name)
{
    if(current_context != global_context) {
        auto it = current_context->classes.find(name);

        if(it != current_context->classes.end()) {
            return it->second;
        }
    }

    auto it = global_context->classes.find(name);

    if(it != global_context->classes.end()) {
        return it->second;
    }

    return nullptr;
}

static bool is_named_param(const andy::lang::parser::ast_node& param)
{
    return param.type() == andy::lang::parser::ast_node_type::ast_node_valuedecl && param.childrens().size();
}

static void push_context_from_node_object_if_any(andy::lang::interpreter* interpreter, const andy::lang::parser::ast_node& node)
{
    const andy::lang::parser::ast_node* object_node = node.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_object);

    if(object_node) {
        object_node = object_node->childrens().data();
        auto object = interpreter->execute(*object_node);
        if(object == nullptr) {
            object = andy::lang::object::instantiate(interpreter, interpreter->NullClass);
        }
        if(object) {
            interpreter->push_context_with_object(object);
        }
    }
}

static void pop_context_from_node_object_if_any(andy::lang::interpreter* interpreter, const andy::lang::parser::ast_node& node)
{
    const andy::lang::parser::ast_node* object_node = node.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_object);

    if(object_node) {
        interpreter->pop_context();
    }
}

static void push_context_from_node(andy::lang::interpreter* interpreter, const andy::lang::parser::ast_node& node)
{
    const andy::lang::parser::ast_node* object_node = node.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_object);

    if(object_node) {
        object_node = object_node->childrens().data();
        auto object = interpreter->execute(*object_node);
        if(object) {
            interpreter->push_context_with_object(object);
        }
    } else {
        interpreter->push_context();
    }
}

static void pop_context_from_node(andy::lang::interpreter* interpreter, const andy::lang::parser::ast_node& node)
{
    interpreter->pop_context();
}

std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute_fn_decl(const andy::lang::parser::ast_node& source_code)
{
    auto method = execute_method_definition(source_code);
    current_context->functions[method.name] = std::make_shared<andy::lang::function>(std::move(method));
    return nullptr;
}

static std::shared_ptr<andy::lang::structure> do_execute_classdecl(andy::lang::interpreter* interpreter, const andy::lang::parser::ast_node& source_code)
{
    std::string_view class_name = source_code.decname();

    auto cls = std::make_shared<andy::lang::structure>(class_name);

    auto baseclass_node = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_classdecl_base);

    if (baseclass_node)
    {
        auto object = andy::lang::api::to_object(interpreter, cls);

        interpreter->push_context_with_object(object);

        auto declname_node = baseclass_node->child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
        auto base_class_object = interpreter->execute(*declname_node);

        interpreter->pop_context();
        
        if(!base_class_object) {
            throw std::runtime_error("base class " + std::string(baseclass_node->decname()) + " not found");
        }

        if(base_class_object->cls->name != "Class") {
            throw std::runtime_error("base class " + std::string(baseclass_node->decname()) + " is not a class");
        }

        std::shared_ptr<andy::lang::structure> base_class = base_class_object->as<std::shared_ptr<andy::lang::structure>>();

        cls->base = base_class;
        base_class->deriveds.push_back(cls);
    }

    for(const auto& class_child : source_code.context()->childrens()) {
        switch (class_child.type())
        {
        case andy::lang::parser::ast_node_type::ast_node_fn_decl: {
            auto method = execute_method_definition(class_child);
            if(method.storage_type == andy::lang::function_storage_type::class_function || source_code.decl_type() == "namespace") {
                cls->functions[method.name] = std::make_shared<andy::lang::function>(std::move(method));
            } else {
                cls->instance_functions[method.name] = std::make_shared<andy::lang::function>(std::move(method));
            }
        }
        break;
        case andy::lang::parser::ast_node_type::ast_node_vardecl: {
            std::string_view var_name = class_child.decname();
            cls->instance_variables[var_name] = class_child.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_call);
        }
        break;
        case andy::lang::parser::ast_node_type::ast_node_classdecl: {
            auto child_cls = do_execute_classdecl(interpreter, class_child);
            auto cls_object = andy::lang::object::create(interpreter, interpreter->ClassClass, child_cls);
            cls_object->cls->functions["new"]->call(cls_object);
            cls->variables[child_cls->name] = cls_object;
            interpreter->load(child_cls);
        }
        break;
        default:
            throw std::runtime_error(class_child.token().error_message_at_current_position("unexpected token in class declaration"));
            break;
        }
    }
    return cls;
}

std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute_classdecl(const andy::lang::parser::ast_node& source_code)
{
    auto cls = do_execute_classdecl(this, source_code);
    load(cls);
    return nullptr;
}

std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute_valuedecl(const andy::lang::parser::ast_node& source_code)
{
    switch(source_code.token().kind)
    {
        case lexer::token_kind::token_boolean: {
            if(source_code.token().boolean_literal) {
                return std::make_shared<andy::lang::object>(TrueClass);
            } else {
                return std::make_shared<andy::lang::object>(FalseClass);
            }
        }
        break;
        case lexer::token_kind::token_integer: {
            std::shared_ptr<andy::lang::object> obj = andy::lang::object::instantiate(this, IntegerClass, source_code.token().integer_literal);
            return obj;
        }
        case lexer::token_kind::token_float: {
            std::shared_ptr<andy::lang::object> obj = andy::lang::object::instantiate(this, FloatClass, source_code.token().float_literal);
            return obj;
        }
        break;
        case lexer::token_kind::token_double: {
            std::shared_ptr<andy::lang::object> obj = andy::lang::object::instantiate(this, DoubleClass, source_code.token().double_literal);
            return obj;
        }
        break;
        case lexer::token_kind::token_string: {
            std::shared_ptr<andy::lang::object> obj = andy::lang::object::instantiate(this, StringClass, source_code.token().string_literal);
            return obj;
        }
        break;
        case lexer::token_kind::token_null: {
            std::shared_ptr<andy::lang::object> obj = andy::lang::object::instantiate(this, NullClass);
            return obj;
        }
        break;
        default:
            throw std::runtime_error("interpreter: unknown node kind");
        break;
    }
}

std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute_fn_call(const andy::lang::parser::ast_node& source_code)
{
    std::string_view function_name;

    switch(source_code.type())
    {
        // declname interpreted as a function call with no parameters
        case andy::lang::parser::ast_node_type::ast_node_declname:
            function_name = source_code.token().content;
        break;
        default:
            function_name = source_code.decname();
        break;
    }

    // Capture the immediate calling context (excluding global at index 0) before any pushes.
    // This becomes the lexical_parent for any DO...END block passed to this call, ensuring
    // that blocks close over the scope where they are *written* — whether that is a block
    // context (describe/context/it) or a function context (status_tester).
    std::shared_ptr<interpreter_context> call_site_lexical_ctx = nullptr;
    if(stack.size() >= 2) {
        call_site_lexical_ctx = stack.back();
    }

    std::vector<std::shared_ptr<andy::lang::object>> positional_params;
    std::map<std::string, std::shared_ptr<andy::lang::object>> named_params;

    const andy::lang::parser::ast_node* params_node = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_params);

    if(params_node) {
        for(auto& param : params_node->childrens()) {
            const andy::lang::parser::ast_node* value_node = &param;
            const andy::lang::parser::ast_node* name = nullptr;
            if(param.type() == andy::lang::parser::ast_node_type::ast_node_pair) {
                value_node = param.child_from_type(andy::lang::parser::ast_node_type::ast_node_valuedecl);
                name = param.child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
            }

            std::shared_ptr<andy::lang::object> value = nullptr;

            value = execute(*value_node);

            if(name) {
                std::string content = std::string(name->token().content);
                named_params[content] = value;
            } else {
                positional_params.push_back(value);
            }
        }
    }

    // If the function call has an object (obj.fn()), we need to push the context to search for the
    // function in the object and its class, and then pop it. If we don't have an object we need to
    // search in the current context and then in the global context.
    push_context_from_node_object_if_any(this, source_code);

    std::string_view object_name = "";
    bool is_super = function_name == "super";
    bool is_assignment = function_name == "=";
    bool is_new = function_name == "new";
    andy::lang::function* method_to_call = nullptr;

    if(is_assignment && !current_context->self) {
        throw std::runtime_error("assignment operator '=' can only be used with a variable");
    }

    std::shared_ptr<andy::lang::object> ret = nullptr;

    if(is_assignment) {
        if(positional_params.size() != 1) {
            throw std::runtime_error("assignment operator '=' requires exactly one parameter");
        }
        if(positional_params[0] == nullptr) {
            *current_context->self = std::move(*andy::lang::object::instantiate(this, NullClass).get());
        } else {
            // If the parameter is only used once (on the right side of the assignment only),
            // we can move it instead of copying it.
            auto use_count = positional_params[0].use_count();
            if(use_count == 1) {
                *current_context->self = std::move(*positional_params[0].get());
            } else {
                auto copy = positional_params[0]->native_copy();
                if(!copy) {
                    throw std::runtime_error("object of class " + std::string(positional_params[0]->cls->name) + " cannot be assigned because it does not support copying");
                }
                *current_context->self = std::move(*copy.get());
            }

        }
    } else {
        if(is_new) {
            if(!current_context->cls) {
                andy::lang::error::internal("new called without a class");
            }

            auto it = current_context->cls->functions.find("new");

            if(it == current_context->cls->functions.end()) {
                if(is_new) {
                    // Simple default constructor call
                    ret = andy::lang::object::instantiate(this, current_context->cls);
                    pop_context_from_node_object_if_any(this, source_code);
                    return ret;
                }
                throw std::runtime_error("function '" + std::string(function_name) + "' not found in class " + std::string(current_context->cls->name));
            } else {
                method_to_call = it->second.get();
            }
        } else {
            auto it = current_context->functions.find(function_name);

            if(it != current_context->functions.end()) {
                method_to_call = it->second.get();
            } else if(current_context->cls) {
                auto it = current_context->cls->functions.find(function_name);

                if(it != current_context->cls->functions.end()) {
                    method_to_call = it->second.get();
                }
            }

            // Search the lexical_parent chain for the function if not found yet.
            if(!method_to_call) {
                for(auto ctx = current_context->lexical_parent; ctx != nullptr; ctx = ctx->lexical_parent) {
                    auto it = ctx->functions.find(function_name);
                    if(it != ctx->functions.end()) {
                        method_to_call = it->second.get();
                        break;
                    }
                }
            }

            if(!method_to_call) {
                if(auto inline_it = current_context->inline_functions.find("new"); inline_it != current_context->inline_functions.end()) {
                    auto self_ptr = current_context->self->shared_from_this();
                    ret = (*inline_it->second)(this, self_ptr, source_code);
                    pop_context_from_node_object_if_any(this, source_code);
                    return ret;
                }
            }
        }

        if(!method_to_call) {
            auto it = global_context->functions.find(function_name);

            if(it != global_context->functions.end()) {
                method_to_call = it->second.get();
            }

            if(!method_to_call) {
                if(current_context->self && !is_new) {
                    throw std::runtime_error("function '" + std::string(function_name) + "' not found in object of class " + std::string(current_context->cls->name));
                }
            }

            if(!method_to_call) {
                // Last chance
                for(auto ctx = current_context; ctx != nullptr; ctx = ctx->lexical_parent) {
                    auto it = ctx->functions.find("missing");
                    if(it != ctx->functions.end()) {
                        method_to_call = it->second.get();
                        break;
                    }
                }

                if(!method_to_call && current_context->cls) {
                    auto it = current_context->cls->functions.find("missing");
                    if(it != current_context->cls->functions.end()) {
                        method_to_call = it->second.get();
                    }
                }

                if(!method_to_call) {
                    auto it = global_context->functions.find("missing");
                    if(it != global_context->functions.end()) {
                        method_to_call = it->second.get();
                    }
                }

                if(!method_to_call) {
                    throw std::runtime_error("function '" + std::string(function_name) + "' not found in current context");
                }

                auto previous_positional_params = std::move(positional_params);
                positional_params = std::vector<std::shared_ptr<andy::lang::object>>();

                auto function_name_obj = andy::lang::api::to_object(this, function_name);

                positional_params.push_back(function_name_obj);
                positional_params.push_back(andy::lang::api::to_object(this, std::move(previous_positional_params)));
                positional_params.push_back(andy::lang::api::to_object(this, std::move(named_params)));
            }
        }

        // If the function call has an object (obj.fn()), we have already pushed the context to search for
        // the function in the object and its class, so we don't need to push it again. If we don't have an
        // object we need to push the context to execute the function in it's own context.
        if(!source_code.fn_object()) {
            push_context();
        }

        current_context->caller_node = &source_code;

        // Store the call-site lexical context on the function's execution context so that
        // execute_yield can use it as the lexical_parent for the DO...END block.
        current_context->given_block_lexical_context = call_site_lexical_ctx;

        andy::lang::function_call __call = {
            function_name,
            current_context->cls,
            current_context->self ? current_context->self->shared_from_this() : nullptr,
            method_to_call,
            std::move(positional_params),
            std::move(named_params),
            source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_context)
        };

        if(is_new) {
            ret = andy::lang::object::instantiate(this, current_context->cls, __call);
        } else {
            ret = call(__call);
        }
    }

// Todo: Use the current_context.return_value and current_context.has_returned to handle returns

    pop_context();

    return ret;
}

std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute_arraydecl(const andy::lang::parser::ast_node& source_code)
{
    std::vector<std::shared_ptr<andy::lang::object>> array;
    array.reserve(source_code.childrens().size());

    for(auto& child : source_code.childrens()) {
        array.push_back(node_to_object(child));
    }

    return andy::lang::object::instantiate(this, ArrayClass, std::move(array));
}

std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute_interpolated_string(const andy::lang::parser::ast_node& source_code) 
{
    std::string str;
    for(size_t i = 0; i < source_code.childrens().size(); i++) {
        auto& node_child = source_code.childrens()[i];
        if(node_child.token().type == andy::lang::lexer::token_type::token_literal)
        {
            switch (node_child.token().kind)
            {
            case lexer::token_kind::token_string:
                str += node_child.token().string_literal;
                break;
            case lexer::token_kind::token_integer:
                str += std::to_string(node_child.token().integer_literal);
                break;
            case lexer::token_kind::token_float:
                str += std::to_string(node_child.token().float_literal);
                break;
            case lexer::token_kind::token_double:
                str += std::to_string(node_child.token().double_literal);
                break;
            case lexer::token_kind::token_boolean:
                str += node_child.token().boolean_literal ? "true" : "false";
                break;
            case lexer::token_kind::token_null:
                str += "null";
                break;
            default:
                node_child.token().error_message_at_current_position("interpreter: unknown token kind");
                break;
            }
        } else {
            std::shared_ptr<andy::lang::object> obj = execute(node_child);
            if(obj->cls != StringClass) {
                auto method = obj->cls->instance_functions.find("to_string");
                if(method == obj->cls->instance_functions.end()) {
                    throw std::runtime_error("object of class " + std::string(obj->cls->name) + " does not have a function called 'to_string'");
                }
                andy::lang::function_call __call = {
                    "to_string",
                    obj->cls,
                    obj,
                    method->second.get(),
                    {},
                    {},
                    nullptr
                };
                obj = call(__call);
            }
            str += obj->as<std::string>();
        }
    }
    return andy::lang::object::create(this, StringClass, std::move(str));
}

std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute_vardecl(const andy::lang::parser::ast_node& source_code)
{
    std::string_view var_name = source_code.decname();
    std::shared_ptr<andy::lang::object> value = std::make_shared<andy::lang::object>(NullClass);
    value->self = value.get();
    current_context->variables[var_name] = value;

    if(auto fn_call = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_call)) {
        value = execute(*fn_call);
    }

    return value;
}

std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute_conditional(const andy::lang::parser::ast_node& source_code)
{
        std::shared_ptr<andy::lang::object> ret = execute(*source_code.condition());

        if(ret && ret->is_present()) {
            auto context = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_context);

            ret = execute(*context);
        } else {
            auto e = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_else);
            if(!e) {
                e = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_conditional);
            }
            if(e) {
                ret = execute(*e);
            }
        }

        return ret;
}

std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute_while(const andy::lang::parser::ast_node& source_code)
{
    bool match_condition = source_code.decl_type() == "until";

    while(execute(*source_code.condition())->is_present() != match_condition) {
        push_block_context();
        execute(*source_code.context());

        if(current_context->has_returned) {
            return current_context->return_value;
        }
        pop_context();
    }

    return nullptr;
}

std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute_break(const andy::lang::parser::ast_node& source_code)
{
        current_context->has_returned = true;
        return nullptr;
}

std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute_context(const andy::lang::parser::ast_node& source_code)
{
    if(source_code.childrens().size() == 0) {
        return nullptr;
    }

    if(source_code.childrens().size() > 1) {
        if(source_code.childrens().front().type() == andy::lang::parser::ast_node_type::ast_node_fn_object) {
            auto* fn_object = source_code.childrens().data();
            std::shared_ptr<andy::lang::object> context_object = node_to_object(
                fn_object->childrens().front(),
                current_context->self ? current_context->self->cls : nullptr,
                current_context->self ? current_context->self->shared_from_this() : nullptr
            );
            push_context_with_object(context_object);
            return execute_all(source_code.childrens().begin() + 1, source_code.childrens().end());
            pop_context();
        }
    }

    return execute_all(source_code);
}

std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute_condition(const andy::lang::parser::ast_node& source_code)
{
    return node_to_object(source_code.childrens().front());
}

std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute_fn_return(const andy::lang::parser::ast_node& source_code)
{
        if(source_code.childrens().size()) {
            return node_to_object(
                source_code.childrens().front(),
                current_context->self ? current_context->self->cls : nullptr,
                current_context->self ? current_context->self->shared_from_this() : nullptr
            );
        } else {
            return std::make_shared<andy::lang::object>(NullClass);
        }
        return nullptr;
}
std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute_foreach(const andy::lang::parser::ast_node& source_code)
{
    auto* valuedecl = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_valuedecl);

    std::shared_ptr<andy::lang::object> array_or_dictionary = node_to_object(valuedecl->childrens().front());

    auto* vardecl = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_vardecl);

    if(array_or_dictionary->cls == ArrayClass) {
        std::vector<std::shared_ptr<andy::lang::object>>& array_values = array_or_dictionary->as<std::vector<std::shared_ptr<andy::lang::object>>>();
        for(auto& value : array_values) {
            push_block_context();

            current_context->variables[vardecl->decname()] = value;
            execute_all(*source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_context));

            pop_context();
        }
    } else if(array_or_dictionary->cls == DictionaryClass) {
        andy::lang::dictionary& dictionary_values = array_or_dictionary->as<andy::lang::dictionary>();
        for(auto& [key, value] : dictionary_values) {
            push_block_context();

            std::vector<std::shared_ptr<andy::lang::object>> params = { key, value };
            std::shared_ptr<andy::lang::object> params_object = andy::lang::object::instantiate(this, ArrayClass, params);

            current_context->variables[vardecl->decname()] = params_object;

            execute_all(*source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_context));

            pop_context();
        }
    } else {
        throw std::runtime_error("foreach should iterate over an array or a dictionary");
    }
    return nullptr;
}
std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute_for(const andy::lang::parser::ast_node& source_code)
{
    auto* valuedecl = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_valuedecl);
    if(!valuedecl) {
        valuedecl = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
    }

    std::shared_ptr<andy::lang::object> max_object = execute(*valuedecl);

    if(!max_object || max_object->cls != IntegerClass) {
        throw std::runtime_error("Cannot iterate over a non-integer value");
    }

    int max = max_object->as<int>();
    int current = 0;

    while(current < max) {
        push_block_context();
        execute_all(*source_code.context());
        pop_context();
        current++;
    }

    return nullptr;
}

std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute_yield(const andy::lang::parser::ast_node& source_code)
{
    andy::lang::function method;
    method.name = "yield";
    method.block_ast = *current_context->given_block;
    andy::lang::function_call __call = {
        "yield",
        nullptr,
        nullptr,
        &method,
        {},
        {},
        current_context->given_block
    };

    // Create a block context whose lexical_parent is the context where the DO...END block
    // was written (captured at call time), not where yield is being executed.
    auto ctx = std::make_shared<interpreter_context>();
    ctx->is_block_context = true;
    ctx->lexical_parent = current_context->given_block_lexical_context;
    stack.push_back(ctx);
    update_current_context();

    auto ret = call(__call);
    pop_context();
    return ret;
}

std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute_declname(const andy::lang::parser::ast_node& source_code)
{
    std::string_view name = source_code.token().content;

    push_context_from_node_object_if_any(this, source_code);

    auto try_find_in_context = [&](const std::shared_ptr<interpreter_context>& ctx) -> std::shared_ptr<andy::lang::object> {
        auto variable_it = ctx->variables.find(name);
        if(variable_it != ctx->variables.end()) {
            return variable_it->second;
        }
        // If not found as a variable or function, it could be a class (in the case of a declname used as an expression), so we check for that before moving to the next context in the chain.
        auto class_it = ctx->classes.find(name);
        if(class_it != ctx->classes.end()) {
            auto cls_object = andy::lang::object::create(this, ClassClass, class_it->second);
            return cls_object;
        }
        return nullptr;
    };

    std::shared_ptr<andy::lang::object> ret = nullptr;

    // Walk the lexical_parent chain (starting from the current context) to find the variable.
    for(auto ctx = current_context; ctx != nullptr; ctx = ctx->lexical_parent) {
        ret = try_find_in_context(ctx);
        if(ret != nullptr) {
            break;
        }
    }

    // Always check the global context as a fallback.
    if(ret == nullptr && current_context != global_context) {
        ret = try_find_in_context(global_context);
    }

    // Fallback to a function call

    pop_context_from_node_object_if_any(this, source_code);

    if(ret != nullptr) {
        return ret;
    }

    return execute_fn_call(source_code);

    throw std::runtime_error("'" + std::string(name) + "' is undefined");
}

std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute_else(const andy::lang::parser::ast_node& source_code)
{
    auto context = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_context);
    return execute_all(*context);
}

struct andy_lang_runtime_exception {
    andy_lang_runtime_exception(std::shared_ptr<andy::lang::object> exception_object)
        : exception_object(std::move(exception_object))
    {
        
    }
    std::shared_ptr<andy::lang::object> exception_object;
};

std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute_try(const andy::lang::parser::ast_node& source_code)
{
    push_block_context();

    std::map<std::string_view, const andy::lang::parser::ast_node*> catchers;

    for(const auto& child : source_code.childrens()) {
        if(child.type() != andy::lang::parser::ast_node_type::ast_node_catch) {
            continue;
        }
        std::string_view exception_type = child.decl_type();
        catchers[exception_type] = &child;
    }

    try {
        current_context->catching_exception = true;

        auto context = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_context);
        execute(*context);
    } catch(const andy_lang_runtime_exception& e) {
        // Go back to the push_context on the beginning of this function
        while(current_context && !current_context->catching_exception) {
            pop_context();
        }
        auto catcher = catchers.find(e.exception_object->cls->name);
        if(catcher == catchers.end()) {
            // If we don't have a catcher for the exception type, we have to throw it again to be caught by an outer try...catch or to terminate the program if it's uncaught.
            throw;
        }
        auto catch_context = catcher->second->child_from_type(andy::lang::parser::ast_node_type::ast_node_context);
        push_block_context();
        current_context->variables[catcher->second->decname()] = e.exception_object;
        execute(*catch_context);
        pop_context();
    }

    pop_context();

    return nullptr;
}

std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute_throw(const andy::lang::parser::ast_node& source_code)
{
    auto exception_object = execute(source_code.childrens().front());

    throw andy_lang_runtime_exception(exception_object);

    return exception_object;
}

std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute(const andy::lang::parser::ast_node& source_code)
{
    static auto executors = std::map<andy::lang::parser::ast_node_type, std::shared_ptr<andy::lang::object>(andy::lang::interpreter::*)(const andy::lang::parser::ast_node&)>{
        { andy::lang::parser::ast_node_type::ast_node_classdecl,           &andy::lang::interpreter::execute_classdecl           },
        { andy::lang::parser::ast_node_type::ast_node_context,             &andy::lang::interpreter::execute_context             },
        { andy::lang::parser::ast_node_type::ast_node_fn_return,           &andy::lang::interpreter::execute_fn_return           },
        { andy::lang::parser::ast_node_type::ast_node_fn_decl,             &andy::lang::interpreter::execute_fn_decl             },
        { andy::lang::parser::ast_node_type::ast_node_valuedecl,           &andy::lang::interpreter::execute_valuedecl           },
        { andy::lang::parser::ast_node_type::ast_node_fn_call,             &andy::lang::interpreter::execute_fn_call             },
        { andy::lang::parser::ast_node_type::ast_node_interpolated_string, &andy::lang::interpreter::execute_interpolated_string },
        { andy::lang::parser::ast_node_type::ast_node_arraydecl,           &andy::lang::interpreter::execute_arraydecl           },
        { andy::lang::parser::ast_node_type::ast_node_vardecl,             &andy::lang::interpreter::execute_vardecl             },
        { andy::lang::parser::ast_node_type::ast_node_declname,            &andy::lang::interpreter::execute_declname            },
        { andy::lang::parser::ast_node_type::ast_node_conditional,         &andy::lang::interpreter::execute_conditional         },
        { andy::lang::parser::ast_node_type::ast_node_while,               &andy::lang::interpreter::execute_while               },
        { andy::lang::parser::ast_node_type::ast_node_for,                 &andy::lang::interpreter::execute_for                 },
        { andy::lang::parser::ast_node_type::ast_node_foreach,             &andy::lang::interpreter::execute_foreach             },
        { andy::lang::parser::ast_node_type::ast_node_break,               &andy::lang::interpreter::execute_break               },
        { andy::lang::parser::ast_node_type::ast_node_condition,           &andy::lang::interpreter::execute_condition           },
        { andy::lang::parser::ast_node_type::ast_node_else,                &andy::lang::interpreter::execute_else                },
        { andy::lang::parser::ast_node_type::ast_node_yield,               &andy::lang::interpreter::execute_yield               },
        { andy::lang::parser::ast_node_type::ast_node_try,                 &andy::lang::interpreter::execute_try                 },
        { andy::lang::parser::ast_node_type::ast_node_throw,               &andy::lang::interpreter::execute_throw               }
    };

    auto it = executors.find(source_code.type());

    if(it == executors.end()) {
        andy::lang::error::internal("No executor found for node type " + std::to_string(static_cast<int>(source_code.type())));
    }

    auto ret = (this->*it->second)(source_code);

    return ret;
}

std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute_all(
    std::vector<andy::lang::parser::ast_node>::const_iterator begin,
    std::vector<andy::lang::parser::ast_node>::const_iterator end
)
{
    std::shared_ptr<andy::lang::object> result = nullptr;

    for(auto it = begin; it != end; it++) {
        const andy::lang::parser::ast_node& node = *it;

        if(node.type() == andy::lang::parser::ast_node_type::ast_node_undefined && node.token().type == andy::lang::lexer::token_type::token_eof) {
            break;
        }

        result = execute(node);

        if(it->type() == andy::lang::parser::ast_node_type::ast_node_fn_return) {
            current_context->has_returned = true;
            current_context->return_value = result;
            return result;
        } else if(current_context->has_returned) {
            return current_context->return_value;
        }
    }

    return result;
}

std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute_all(const andy::lang::parser::ast_node& source_code)
{
    return execute_all(source_code.childrens().begin(), source_code.childrens().end());
}

std::shared_ptr<andy::lang::object> andy::lang::interpreter::call(function_call& call)
{
    std::shared_ptr<andy::lang::object> ret = nullptr;

    if(call.positional_params.size() != call.method->positional_params.size()) {
        throw std::runtime_error("function " + std::string(call.method->name) + " expects " + std::to_string(call.method->positional_params.size()) + " parameters, but " + std::to_string(call.positional_params.size()) + " were given");
    }

    for(const auto& param : call.method->named_params) {
        auto it = call.named_params.find(param.name);

        if(it == call.named_params.end()) {
            if(param.has_default_value) {
                if(param.default_value_node) {
                    call.named_params[param.name] = node_to_object(*param.default_value_node, call.cls, call.object);
                }
            } else {
                throw std::runtime_error("function '" + std::string(call.method->name) + "' called without named parameter " + param.name);
            }
        }
    }

    current_context->given_block = call.given_block;

    bool is_constructor = call.method->name == "new";

    if(is_constructor) {
        // Special case
        // The object is created before the method is called
        // If the object was instantiated in from native code, it will be passed as a parameter
        if(!call.object) {
            return andy::lang::object::instantiate(this, call.cls, call);
        } else {
            call.object->initialize(this);
        }
    }

    if(call.method->block_ast.childrens().size()) {
        for(size_t i = 0; i < call.method->positional_params.size(); i++) {
            current_context->variables[call.method->positional_params[i].name] = call.positional_params[i];
        }

        for(auto& [name, value] : call.named_params) {
            current_context->variables[name] = value;
        }
        
        if(call.method->block_ast.type() == andy::lang::parser::ast_node_type::ast_node_context) {
            ret = execute_all(call.method->block_ast);
        } else {
            ret = execute(*call.method->block_ast.block());
        }
    } else if(call.method->native_function) {
        ret = call.method->native_function(call);
    }

    if(is_constructor) {
        if(ret) {
            throw std::runtime_error("constructor should not return a value");
        }

        ret = call.object;
    }

    for (auto& [name, value] : current_context->variables) {
        if(value && value->base_instance) {
            if(value.use_count() == 2) {
                // used by base_instance and current_context->variables
                value->base_instance = nullptr;
                // Now the use count is 1, it will be destroyed when the current_context is destroyed
            }
        }
    }

    return ret;
}

void andy::lang::interpreter::init()
{
    // The global context
    push_context();
    andy::lang::structure::create_structures(this);
}

const std::shared_ptr<andy::lang::object> andy::lang::interpreter::try_object_from_declname(
    const andy::lang::parser::ast_node& node,
    std::shared_ptr<andy::lang::structure> cls,
    std::shared_ptr<andy::lang::object> object
)
{
    auto* fn_object = node.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_object);
    const andy::lang::parser::ast_node* fn_object_decname = nullptr;
    std::string_view var_name = node.token().content;

    if(fn_object) {
        fn_object_decname = fn_object->child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
        
        if(fn_object_decname) {    
            object = try_object_from_declname(*fn_object_decname, cls, object);
        } else if(auto fn_value = fn_object->child_from_type(andy::lang::parser::ast_node_type::ast_node_valuedecl)) {
            object = node_to_object(*fn_value, cls, object);
        }
        else {
            auto fn_object_fn_call = fn_object->child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_call);

            if(fn_object_fn_call) {
                std::shared_ptr fn_object = execute(*fn_object_fn_call);
                if(fn_object) {
                    auto it = fn_object->variables.find(node.token().content);
                    if(it != fn_object->variables.end()) {
                        return it->second;
                    }
                    throw std::runtime_error("Class " + std::string(fn_object->cls->name) + " does not have a variable called " + std::string(node.token().content));
                } else {
                    throw std::runtime_error("Cannot read property '" + std::string(node.token().content) + "' of null");
                }
            } else {
                throw std::runtime_error("Cannot determine the object for '" + std::string(node.token().content) + "'");
            }
        }
    }

    if(object) {
        auto it = object->variables.find(var_name);

        if(it != object->variables.end()) {
            return it->second;
        }

        if(object->cls == ClassClass) {
            auto cls = object->as<std::shared_ptr<andy::lang::structure>>();
            auto it = cls->variables.find(var_name);

            if(it != cls->variables.end()) {
                return it->second;
            }
        }
        andy::lang::function* method = nullptr;
        auto method_it = object->cls->instance_functions.find(var_name);
        if(method_it != object->cls->instance_functions.end()) {
            method = method_it->second.get();
        } else if(object->cls->base) {
            method_it = object->cls->base->instance_functions.find(var_name);
            if(method_it != object->cls->base->instance_functions.end()) {
                method = method_it->second.get();
                object = object->base_instance;
            }
        }

        if(method) {
            auto __call = andy::lang::function_call{
                var_name,
                object->cls,
                object,
                method_it->second.get(),
                {},
                {},
                fn_object
            };
            return call(__call);
        }

        if(fn_object) {
            throw std::runtime_error("type " + std::string(object->cls->name) + " does not have a variable or function called '" + std::string(node.token().content) + "'");
        }
    }

    if(fn_object_decname) {
        std::string_view class_name = fn_object_decname->token().content;
            
        auto cls = find_class(class_name);

        if(!cls) {
            throw std::runtime_error("class or variable '" + std::string(class_name) + "' not found");
        }

        auto it = cls->variables.find(var_name);

        if(it == cls->variables.end()) {
            // Andy supports calling functions which does not have parameters without parentheses
            if(auto it = cls->functions.find(var_name); it != cls->functions.end()) {
                auto __call = andy::lang::function_call{
                    var_name,
                    cls,
                    nullptr,
                    it->second.get(),
                    {},
                    {},
                    fn_object
                };
                return call(__call);
            }
            throw std::runtime_error("class " + std::string(class_name) + " does not have a variable or function called '" + std::string(var_name) + "'");
        } else {
            return it->second;
        }
    }

    // Walk the lexical_parent chain to find the variable.
    for(auto ctx = current_context; ctx != nullptr; ctx = ctx->lexical_parent) {
        auto it = ctx->variables.find(node.token().content);
        if(it != ctx->variables.end()) {
            return it->second;
        }

        auto fn_it = ctx->functions.find(node.token().content);
        if(fn_it != ctx->functions.end()) {
            auto method = fn_it->second;
            andy::lang::function_call __call = {
                method->name,
                nullptr,
                nullptr,
                method.get(),
                {},
                {},
                node.child_from_type(andy::lang::parser::ast_node_type::ast_node_context)
            };
            return call(__call);
        }
    }

    // Always check the global context as a fallback.
    if(current_context != global_context) {
        auto it = global_context->variables.find(node.token().content);
        if(it != global_context->variables.end()) {
            return it->second;
        }
    }

    cls = find_class(node.token().content);

    if(cls) {
        return andy::lang::api::to_object(this, cls);
    }

    return nullptr;
}

const std::shared_ptr<andy::lang::object> andy::lang::interpreter::node_to_object(const andy::lang::parser::ast_node& node, std::shared_ptr<andy::lang::structure> cls, std::shared_ptr<andy::lang::object> object)
{
    if(node.token().type == andy::lang::lexer::token_type::token_literal) {
        return execute(node);
    } else if(node.type() == andy::lang::parser::ast_node_type::ast_node_fn_call) {
        return execute(node);
    } else if(node.type() == andy::lang::parser::ast_node_type::ast_node_declname || node.type() == andy::lang::parser::ast_node_type::ast_node_valuedecl) {
        return execute(node);
    } else if(node.type() == andy::lang::parser::ast_node_type::ast_node_arraydecl) {
        // Logic moved to execute_arraydecl to support array literals in more places
        return execute(node);
    } else if(node.type() == andy::lang::parser::ast_node_type::ast_node_dictionarydecl) {
        andy::lang::dictionary map;

        for(auto& child : node.childrens()) {
            const andy::lang::parser::ast_node* name_node = child.child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
            const andy::lang::parser::ast_node* value_node = child.child_from_type(andy::lang::parser::ast_node_type::ast_node_valuedecl);

            std::shared_ptr<andy::lang::object> key   = node_to_object(name_node->childrens().front());
            std::shared_ptr<andy::lang::object> value = node_to_object(value_node->childrens().front());

            map.push_back({ key, value });
        }

        return andy::lang::object::instantiate(this, DictionaryClass, std::move(map));
    } else if(node.type() == andy::lang::parser::ast_node_type::ast_node_interpolated_string) {
        return execute(node);
    }

    throw std::runtime_error("interpreter: unknown node type");

    return nullptr;
}

void andy::lang::interpreter::load_extension(andy::lang::extension* extension)
{
    extension->load(this);
    extensions.push_back(extension);
}

void andy::lang::interpreter::push_context()
{
    andy::console::log_debug("Pushing context");

    stack.push_back(std::make_shared<interpreter_context>());
    update_current_context();
}

void andy::lang::interpreter::push_block_context()
{
    andy::console::log_debug("Pushing block context");

    auto ctx = std::make_shared<interpreter_context>();
    ctx->is_block_context = true;
    ctx->given_block = current_context->given_block;
    ctx->given_block_lexical_context = current_context->given_block_lexical_context;

    // Always inherit the immediate parent context as lexical_parent so that a loop or
    // other block inside a function can see the function's own variables.
    // Walking past a function boundary is prevented naturally because push_context()
    // never sets lexical_parent on function contexts (their lexical_parent stays null).
    if(stack.size() >= 2) {
        ctx->lexical_parent = stack.back();
    }

    stack.push_back(ctx);
    update_current_context();
}

void andy::lang::interpreter::push_context(std::shared_ptr<andy::lang::object> object)
{
    andy::console::log_debug("Pushing context with object");

    if(object) {
        push_context_with_object(object);
    } else {
        push_context();
    }

    update_current_context();
}

void andy::lang::interpreter::push_context_with_object(std::shared_ptr<andy::lang::object> object)
{
    std::string_view class_name = object->cls ? object->cls->name : "null";

    if(object->cls == ClassClass) {
        auto cls = object->as<std::shared_ptr<andy::lang::structure>>();
        // Temporary workaround
        if(!cls->cls) {
            cls->cls = cls;
        }
        stack.push_back(std::static_pointer_cast<interpreter_context>(cls));
    } else {
        stack.push_back(std::static_pointer_cast<interpreter_context>(object));
    }

    andy::console::log_debug("Pushing context with {}#{}", class_name, (void*)object.get());

    update_current_context();
}

void andy::lang::interpreter::pop_context()
{
    if(stack.size() == 1) {
        throw std::runtime_error("interpreter: unexpected end of context stack");
    }

    andy::console::log_debug("Popping context");

    stack.pop_back();
    update_current_context();
}

void andy::lang::interpreter::update_current_context()
{
    if(stack.empty()) {
        current_context = nullptr;
        global_context = nullptr;
        return;
    }

    global_context = stack.front();
    previous_context = current_context;
    current_context = stack.back();
}