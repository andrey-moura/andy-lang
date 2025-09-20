#include <iostream>

#include "andy/file.hpp"

#include "andy/lang/interpreter.hpp"
#include "andy/lang/extension.hpp"
#include "andy/lang/lang.hpp"
#include "andy/lang/api.hpp"

andy::lang::method execute_method_definition(const andy::lang::parser::ast_node& class_child)
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
                fn_param.name = std::string(param.token().content());
                positional_params.push_back(std::move(fn_param));
            }
        }
    }

    auto static_node = class_child.child_from_type(andy::lang::parser::ast_node_type::ast_node_declstatic);

    auto method_type = static_node ? andy::lang::method_storage_type::class_method : andy::lang::method_storage_type::instance_method;

    andy::lang::method method;
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
    cls->class_methods["subclasses"] = andy::lang::method("subclasses", method_storage_type::instance_method, [cls,this](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {
        std::vector<std::shared_ptr<andy::lang::object>> subclasses;
        subclasses.reserve(cls->deriveds.size());

        for(auto& cls : cls->deriveds) {
            std::vector<std::shared_ptr<andy::lang::object>> params = { andy::lang::object::create(this, StringClass, cls->name) };
            auto c = andy::lang::object::instantiate(this, ClassClass, nullptr, params);
            subclasses.push_back(c);
        }

        return andy::lang::object::create(this, ArrayClass, std::move(subclasses));
    });

    for(auto& previous_cls : classes) {
        if(previous_cls->name == cls->name) {
            previous_cls = cls;
            return;
        }
    }

    classes.push_back(cls);
}

std::shared_ptr<andy::lang::structure> andy::lang::interpreter::execute_classdecl(const andy::lang::parser::ast_node& source_code)
{
    std::string_view class_name = source_code.decname();

    auto cls = std::make_shared<andy::lang::structure>(class_name);

    auto baseclass_node = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_classdecl_base);

    if (baseclass_node)
    {
        auto object = andy::lang::api::to_object(this, cls);

        auto declname_node = baseclass_node->child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
        auto base_class_object = execute(*declname_node, object);
        
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
            if(method.storage_type == andy::lang::method_storage_type::class_method || source_code.decl_type() == "namespace") {
                cls->class_methods[method.name] = std::move(method);
            } else {
                cls->instance_methods[method.name] = std::move(method);
            }
        }
        break;
        case andy::lang::parser::ast_node_type::ast_node_vardecl: {
            std::string_view var_name = class_child.decname();
            cls->instance_variables[var_name] = class_child.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_call);
        }
        break;
        case andy::lang::parser::ast_node_type::ast_node_classdecl: {
            auto child_cls = execute_classdecl(class_child);
            auto cls_object = andy::lang::object::create(this, ClassClass, child_cls);
            cls_object->cls->instance_methods["new"].call(cls_object);
            cls->class_variables[child_cls->name] = cls_object;
        }
        default:
            class_child.token().error_message_at_current_position("unexpected token in class declaration");
            break;
        }
    }

    return cls;
}

static bool is_named_param(const andy::lang::parser::ast_node& param)
{
    return param.type() == andy::lang::parser::ast_node_type::ast_node_valuedecl && param.childrens().size();
}

std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute(const andy::lang::parser::ast_node& source_code, std::shared_ptr<andy::lang::object>& object)
{
    switch (source_code.type())
    {
        case andy::lang::parser::ast_node_type::ast_node_fn_decl: {
            auto method = execute_method_definition(source_code);
            current_context().functions[method.name] = std::make_shared<andy::lang::method>(std::move(method));
        }
        break;
        case andy::lang::parser::ast_node_type::ast_node_classdecl: {
            auto cls = execute_classdecl(source_code);
            load(cls);
        }
        break;
        case andy::lang::parser::ast_node_type::ast_node_valuedecl: {
            return node_to_object(source_code);
        }
        break;
        case andy::lang::parser::ast_node_fn_call: {
            std::string_view function_name = source_code.decname();
            bool is_super = function_name == "super";
            bool is_assignment = function_name == "=";
            bool is_new = function_name == "new";
            andy::lang::method* method_to_call = nullptr;
            std::shared_ptr<andy::lang::object> object_to_call = nullptr;
            std::shared_ptr<andy::lang::structure> class_to_call = nullptr;

            const andy::lang::parser::ast_node* object_node = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_object);

            const andy::lang::parser::ast_node* params_node = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_params);

            if(is_super) {
                if(!object || object_node) {
                    throw std::runtime_error("super can only be called from an instance object");
                }

                if(!object->cls->base) {
                    throw std::runtime_error("class " + std::string(object->cls->name) + " does not have a base class");
                }

                auto it = object->cls->base->instance_methods.find("new");

                if(it == object->cls->base->instance_methods.end()) {
                    throw std::runtime_error("base class " + std::string(object->cls->base->name) + " does not have a constructor");
                }

                method_to_call = &it->second;
                object_to_call = std::make_shared<andy::lang::object>(object->cls->base);
                object_to_call->derived_instance = object;
                object->base_instance = object_to_call;
                object_to_call->derived_instance = object;
                class_to_call = object->cls->base;
            } else {
                if(object_node) {
                    object_node = object_node->childrens().data();
                    object_to_call = execute(*object_node, object);
                    if(!object_to_call && object_node->type() == andy::lang::parser::ast_node_type::ast_node_declname) {
                        // Possibly a class name, so we try to instantiate it.
                       class_to_call = find_class(object_node->token().content());
                    }
                } else {
                    object_to_call = object;
                }

                if(object_to_call) {
                    if(object_to_call->cls == ClassClass) {
                        class_to_call = object_to_call->as<std::shared_ptr<andy::lang::structure>>();
                        object_to_call = nullptr;
                    } else {
                        class_to_call = object_to_call->cls;
                    }

                    bool are_all_parameters_literals = true;
                    if(params_node) {
                        for(auto& param : params_node->childrens()) {
                            if(is_named_param(param)) {
                                are_all_parameters_literals = false;
                                break;
                            }
                            if(param.token().type() != andy::lang::lexer::token_type::token_literal) {
                                are_all_parameters_literals = false;
                                break;
                            }
                        }
                    }
                    if(are_all_parameters_literals) {
                        // We can inline!
                        auto it = class_to_call->inline_functions.find(function_name);
                        if(it != class_to_call->inline_functions.end()) {
                            auto& inline_fn = it->second;
                            return inline_fn(this, object_to_call, source_code);
                        }
                    }
                }

                if(is_new && object_to_call) {
                    throw std::runtime_error("new can only be called on a class");
                } else if(is_assignment) {
                    if(!object_to_call) {
                        throw std::runtime_error("assignment operator '=' can only be used with a variable");
                    }
                }
            }
            if(!is_assignment) {
                if(object_to_call) {
                    auto it = object_to_call->cls->instance_methods.find(function_name);

                    if(it == object_to_call->cls->instance_methods.end()) {
                        if(object_to_call->cls->base) {
                            it = object_to_call->cls->base->instance_methods.find(function_name);

                            if(it != object_to_call->cls->base->instance_methods.end()) {
                                if(!object_to_call->base_instance) {
                                    throw std::runtime_error("object has no base instance");
                                }

                                method_to_call = &it->second;
                                class_to_call = object_to_call->cls->base;
                                object_to_call = object_to_call->base_instance;
                            }
                        }
                    } else {
                        method_to_call = &it->second;
                        object_to_call = object_to_call;
                        class_to_call = object_to_call->cls;
                    }

                    if(!method_to_call) {
                        // Calling directly on an object and the object does not have the method.
                        if(object_node) {
                            throw std::runtime_error("function '" + std::string(function_name) + "' not found in class " + std::string(object_to_call->cls->name));
                        }
                    }
                } else if(class_to_call && function_name != "new") {
                    auto it = class_to_call->class_methods.find(function_name);

                    if(it != class_to_call->class_methods.end()) {
                        method_to_call = &it->second;
                    } else {
                        // If the class does not have the method, we search in the base class.
                        if(class_to_call->base) {
                            it = class_to_call->base->class_methods.find(function_name);

                            if(it != class_to_call->base->class_methods.end()) {
                                method_to_call = &it->second;
                                class_to_call = class_to_call->base;
                            }
                        }
                    }
                    if(!method_to_call) {
                        throw std::runtime_error("function '" + std::string(function_name) + "' not found in class " + std::string(class_to_call->name));
                    }
                }

                if(function_name != "new") {
                    if(!method_to_call) {
                        // Yet not found, so we search in the current context.
                        // If the current context is the global, we skip the
                        // check as it will be made after.
                        bool checked_global = false;
                        if(!is_global_context()) {
                            for(int i = stack.size() - 1; i >= 0; --i) {
                                checked_global = i == 0;

                                auto& context = stack[i];

                                auto it = context.functions.find(function_name);

                                if(it != context.functions.end()) {
                                    method_to_call = it->second.get();
                                    break;
                                }

                                // If the current context is not inherited, we can stop searching
                                if(!context.inherited && i == stack.size() - 1) {
                                    break;
                                }
                            }
                        }
                        if(!method_to_call && !checked_global) {
                            auto& context = stack.front();

                            auto it = context.functions.find(function_name);

                            if(it != context.functions.end()) {
                                method_to_call = it->second.get();
                            }
                        }
                    }

                    if(!method_to_call) {
                        auto it = StdClass->class_methods.find(function_name);

                        if(it == StdClass->class_methods.end()) {
                            throw std::runtime_error("function '" + std::string(function_name) + "' not found");
                        } else {
                            method_to_call = &it->second;
                            class_to_call = StdClass;
                        }
                    }
                }
            }

            std::vector<std::shared_ptr<andy::lang::object>> positional_params;
            std::map<std::string, std::shared_ptr<andy::lang::object>> named_params;

            if(params_node) {
                for(auto& param : params_node->childrens()) {
                    const andy::lang::parser::ast_node* value_node = &param;
                    const andy::lang::parser::ast_node* name = nullptr;
                    if(param.type() == andy::lang::parser::ast_node_type::ast_node_pair) {
                        value_node = param.child_from_type(andy::lang::parser::ast_node_type::ast_node_valuedecl);
                        name = param.child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
                    }

                    std::shared_ptr<andy::lang::object> value = nullptr;
                    
                    value = node_to_object(*value_node, object ? object->cls : nullptr, object);
                    
                    if(name) {
                        std::string content = std::string(name->token().content());
                        named_params[content] = value;
                    } else {
                        positional_params.push_back(value);
                    }
                }
            }

            if(is_assignment) {
                if(positional_params.size() != 1) {
                    throw std::runtime_error("assignment operator '=' requires exactly one parameter");
                }

                auto other = positional_params.front();
                auto use_count = other.use_count();
                if(other->base_instance) {
                    use_count--;
                }
                // One for positional_params + one for other (variabled declared above)
                if(use_count > 2) {
                    other->native_copy_to(object_to_call.get());
                    if(other->base_instance) {
                        object_to_call->base_instance = std::make_shared<andy::lang::object>(NullClass);
                        other->base_instance->native_copy_to(object_to_call->base_instance.get());
                    }
                } else {
                    other->native_move_to<void>(object_to_call.get());
                    object_to_call->base_instance = other->base_instance;
                    other->base_instance = nullptr;
                }
                
                object_to_call->cls = other->cls;
                object_to_call->instance_variables = other->instance_variables;

                return object_to_call;
            }

            andy::lang::function_call __call = {
                function_name,
                class_to_call,
                object_to_call,
                method_to_call,
                std::move(positional_params),
                std::move(named_params),
                source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_context)
            };

            if(is_new) {
                object_to_call = andy::lang::object::instantiate(this, class_to_call, __call);
                return object_to_call;
            }

            std::shared_ptr<andy::lang::object> ret = call(__call);
            return ret;
        }
        break;
        case andy::lang::parser::ast_node_type::ast_node_vardecl: {
            std::string_view var_name = source_code.decname();
            std::shared_ptr<andy::lang::structure> cls = nullptr;
            if(object) {
                cls = object->cls;
            }
            std::shared_ptr<andy::lang::object> value = std::make_shared<andy::lang::object>(NullClass);
            current_context().variables[var_name] = value;
            if(auto fn_call = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_call)) {
                value = execute(*fn_call, object);
            }
            return value;
        }
        break;
        case andy::lang::parser::ast_node_type::ast_node_conditional: {
            std::shared_ptr<andy::lang::object> ret = execute(*source_code.condition(), object);

            if(ret && ret->is_present()) {
                auto context = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_context);

                ret = execute(*context, object);
            } else {
                auto e = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_else);
                if(!e) {
                    e = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_conditional);
                }
                if(e) {
                    ret = execute(*e, object);
                }
            }

            return ret;
        }
        break;
        case andy::lang::parser::ast_node_type::ast_node_while: {
            std::shared_ptr<andy::lang::object> ret = nullptr;

            bool match_condition = source_code.decl_type() == "until";

            while(execute(*source_code.condition(), object)->is_present() != match_condition) {
                push_context(true);
                ret = execute(*source_code.context(), object);

                if(current_context().has_returned) {
                    return current_context().return_value;
                }
                pop_context();
            }

            return ret;
        }
        break;
        case andy::lang::parser::ast_node_type::ast_node_break: {
            current_context().has_returned = true;
            return nullptr;
        }
        break;
        case andy::lang::parser::ast_node_type::ast_node_context: {
            if(source_code.childrens().size() == 0) {
                return nullptr;
            }

            if(source_code.childrens().size() > 1) {
                if(source_code.childrens().front().type() == andy::lang::parser::ast_node_type::ast_node_fn_object) {
                    auto* fn_object = source_code.childrens().data();
                    std::shared_ptr<andy::lang::object> context_object = node_to_object(fn_object->childrens().front(), object ? object->cls : nullptr, object);
                    push_context(true);
                    return execute_all(source_code.childrens().begin() + 1, source_code.childrens().end(), context_object);
                    pop_context();
                }
            }
            return execute_all(source_code, object);
        }
        break;
        case andy::lang::parser::ast_node_type::ast_node_condition: {
            return node_to_object(source_code.childrens().front());
        }
        break;
        case andy::lang::parser::ast_node_type::ast_node_fn_return: {
            if(source_code.childrens().size()) {
                return node_to_object(source_code.childrens().front(), object ? object->cls : nullptr, object);
            } else {
                return std::make_shared<andy::lang::object>(NullClass);
            }
        }
        break;
        case andy::lang::parser::ast_node_type::ast_node_foreach: {
            auto* valuedecl = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_valuedecl);

            std::shared_ptr<andy::lang::object> array_or_dictionary = node_to_object(valuedecl->childrens().front());

            auto* vardecl = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_vardecl);

            if(array_or_dictionary->cls == ArrayClass) {
                std::vector<std::shared_ptr<andy::lang::object>>& array_values = array_or_dictionary->as<std::vector<std::shared_ptr<andy::lang::object>>>();
                for(auto& value : array_values) {
                    push_context(true);

                    current_context().variables[vardecl->decname()] = value;
                    execute_all(*source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_context), object);

                    pop_context();
                }
            } else if(array_or_dictionary->cls == DictionaryClass) {
                andy::lang::dictionary& dictionary_values = array_or_dictionary->as<andy::lang::dictionary>();
                for(auto& [key, value] : dictionary_values) {
                    push_context(true);

                    std::vector<std::shared_ptr<andy::lang::object>> params = { key, value };
                    std::shared_ptr<andy::lang::object> params_object = andy::lang::object::instantiate(this, ArrayClass, params);

                    current_context().variables[vardecl->decname()] = params_object;

                    execute_all(*source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_context), object);

                    pop_context();
                }
            } else {
                throw std::runtime_error("foreach should iterate over an array or a dictionary");
            }
        }
        break;
        case andy::lang::parser::ast_node_type::ast_node_for: {
            auto* valuedecl = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_valuedecl);
            if(!valuedecl) {
                valuedecl = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
            }

            std::shared_ptr<andy::lang::object> max_object = execute(*valuedecl, object);

            if(!max_object || max_object->cls != IntegerClass) {
                throw std::runtime_error("Cannot iterate over a non-integer value");
            }

            int max = max_object->as<int>();
            int current = 0;

            while(current < max) {
                push_context(true);
                execute_all(*source_code.context(), object);
                pop_context();
                current++;
            }
        }
        break;
        case andy::lang::parser::ast_node_type::ast_node_yield: {
            andy::lang::method method;
            method.name = "yield";
            method.block_ast = *current_context().given_block;
            andy::lang::function_call __call = {
                "yield",
                nullptr,
                nullptr,
                &method,
                {},
                {},
                current_context().given_block
            };
            return call(__call);
        }
        break;
        case andy::lang::parser::ast_node_type::ast_node_declname: {
            return try_object_from_declname(source_code, object ? object->cls : nullptr, object);
        }
        break;
        case andy::lang::parser::ast_node_type::ast_node_else: {
            auto context = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_context);
            return execute_all(*context, object);
        }
        break;
    default:
        throw std::runtime_error(source_code.token().error_message_at_current_position("interpreter: Unexpected token"));
        break;
    }

    return nullptr;
}

std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute_all(std::vector<andy::lang::parser::ast_node>::const_iterator begin, std::vector<andy::lang::parser::ast_node>::const_iterator end, std::shared_ptr<andy::lang::object>& object)
{
    std::shared_ptr<andy::lang::object> result = nullptr;

    for(auto it = begin; it != end; it++) {
        const andy::lang::parser::ast_node& node = *it;

        if(node.type() == andy::lang::parser::ast_node_type::ast_node_undefined && node.token().type() == andy::lang::lexer::token_type::token_eof) {
            break;
        }

        result = execute(node, object);

        if(it->type() == andy::lang::parser::ast_node_type::ast_node_fn_return) {
            current_context().has_returned = true;
            current_context().return_value = result;
            return result;
        } else if(current_context().has_returned) {
            return current_context().return_value;
        }
    }

    return nullptr;
}

std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute_all(const andy::lang::parser::ast_node& source_code, std::shared_ptr<andy::lang::object>& object)
{
    return execute_all(source_code.childrens().begin(), source_code.childrens().end(), object);
}

std::shared_ptr<andy::lang::object> andy::lang::interpreter::call(function_call& call)
{
    push_context(call.name == "yield");

    current_context().given_block = call.given_block;

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
            current_context().variables[call.method->positional_params[i].name] = call.positional_params[i];
        }

        for(auto& [name, value] : call.named_params) {
            current_context().variables[name] = value;
        }
        
        if(call.method->block_ast.type() == andy::lang::parser::ast_node_type::ast_node_context) {
            ret = execute_all(call.method->block_ast, call.object);
        } else {
            ret = execute(*call.method->block_ast.block(), call.object);
        }
    } else if(call.method->function) {
        ret = call.method->function(call);
    }

    if(is_constructor) {
        if(ret) {
            throw std::runtime_error("constructor should not return a value");
        }

        ret = call.object;
    }

    for (auto& [name, value] : current_context().variables) {
        if(value && value->base_instance) {
            if(value.use_count() == 2) {
                // used by base_instance and current_context().variables
                value->base_instance = nullptr;
                // Now the use count is 1, it will be destroyed when the current_context is destroyed
            }
        }
    }

    pop_context();

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
    std::string_view var_name = node.token().content();

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
                std::shared_ptr fn_object = execute(*fn_object_fn_call, object);
                if(fn_object) {
                    auto it = fn_object->instance_variables.find(node.token().content());
                    if(it != fn_object->instance_variables.end()) {
                        return it->second;
                    }
                    throw std::runtime_error("Class " + std::string(fn_object->cls->name) + " does not have a variable called " + std::string(node.token().content()));
                } else {
                    throw std::runtime_error("Cannot read property '" + std::string(node.token().content()) + "' of null");
                }
            } else {
                throw std::runtime_error("Cannot determine the object for '" + std::string(node.token().content()) + "'");
            }
        }
    }

    if(object) {
        auto it = object->instance_variables.find(var_name);

        if(it != object->instance_variables.end()) {
            return it->second;
        }

        if(object->cls == ClassClass) {
            auto cls = object->as<std::shared_ptr<andy::lang::structure>>();
            auto it = cls->class_variables.find(var_name);

            if(it != cls->class_variables.end()) {
                return it->second;
            }
        }
        andy::lang::method* method = nullptr;
        auto method_it = object->cls->instance_methods.find(var_name);
        if(method_it != object->cls->instance_methods.end()) {
            method = &method_it->second;
        } else if(object->cls->base) {
            method_it = object->cls->base->instance_methods.find(var_name);
            if(method_it != object->cls->base->instance_methods.end()) {
                method = &method_it->second;
                object = object->base_instance;
            }
        }

        if(method) {
            auto __call = andy::lang::function_call{
                var_name,
                object->cls,
                object,
                &method_it->second,
                {},
                {},
                fn_object
            };
            return call(__call);
        }

        if(fn_object) {
            throw std::runtime_error("type " + std::string(object->cls->name) + " does not have a variable or function called '" + std::string(node.token().content()) + "'");
        }
    }

    if(fn_object_decname) {
        std::string_view class_name = fn_object_decname->token().content();
            
        for(auto& cls : classes) {
            if(cls->name == class_name) {
                auto it = cls->class_variables.find(var_name);

                if(it == cls->class_variables.end()) {
                    // Andy supports calling functions which does not have parameters without parentheses
                    if(auto it = cls->class_methods.find(var_name); it != cls->class_methods.end()) {
                        auto __call = andy::lang::function_call{
                            var_name,
                            cls,
                            nullptr,
                            &it->second,
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
        }

        throw std::runtime_error("class or variable '" + std::string(class_name) + "' not found");
    }

    auto it = current_context().variables.find(node.token().content());

    for(int i = stack.size() - 1; i >= 0; --i) {
        auto& context = stack[i];

        auto it = context.variables.find(node.token().content());

        if(it != context.variables.end()) {
            return it->second;
        }

        auto fn_it = current_context().functions.find(node.token().content());

        if(fn_it != current_context().functions.end()) {
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

        // If the current context is not inherited, we can stop searching
        if(!context.inherited && i == stack.size() - 1) {
            break;
        }
    }

    return nullptr;
}

const std::shared_ptr<andy::lang::object> andy::lang::interpreter::node_to_object(const andy::lang::parser::ast_node& node, std::shared_ptr<andy::lang::structure> cls, std::shared_ptr<andy::lang::object> object)
{
    if(node.token().type() == andy::lang::lexer::token_type::token_literal) {
        switch(node.token().kind())
        {
            case lexer::token_kind::token_boolean: {
                if(node.token().boolean_literal) {
                    return std::make_shared<andy::lang::object>(TrueClass);
                } else {
                    return std::make_shared<andy::lang::object>(FalseClass);
                }
            }
            break;
            case lexer::token_kind::token_integer: {
                std::shared_ptr<andy::lang::object> obj = andy::lang::object::instantiate(this, IntegerClass, node.token().integer_literal);
                return obj;
            }
            case lexer::token_kind::token_float: {
                std::shared_ptr<andy::lang::object> obj = andy::lang::object::instantiate(this, FloatClass, node.token().float_literal);
                return obj;
            }
            break;
            case lexer::token_kind::token_double: {
                std::shared_ptr<andy::lang::object> obj = andy::lang::object::instantiate(this, DoubleClass, node.token().double_literal);
                return obj;
            }
            break;
            case lexer::token_kind::token_string: {
                std::shared_ptr<andy::lang::object> obj = andy::lang::object::instantiate(this, StringClass, std::move(std::string(node.token().content())));
                return obj;
            }
            break;
            case lexer::token_kind::token_null:
                return std::make_shared<andy::lang::object>(NullClass);
            break;
            default:    
                throw std::runtime_error("interpreter: unknown node kind");
            break;
        }
    } else if(node.type() == andy::lang::parser::ast_node_type::ast_node_fn_call) {
        return execute(node, object);
    } else if(node.type() == andy::lang::parser::ast_node_type::ast_node_declname || node.type() == andy::lang::parser::ast_node_type::ast_node_valuedecl) {
        std::shared_ptr<andy::lang::object> obj = try_object_from_declname(node, cls, object);

        if(obj) {
            return obj;
        }

        throw std::runtime_error("'" + std::string(node.token().content()) + "' is undefined");
    } else if(node.type() == andy::lang::parser::ast_node_type::ast_node_arraydecl) {
        std::vector<std::shared_ptr<andy::lang::object>> array;

        for(auto& child : node.childrens()) {
            array.push_back(node_to_object(child));
        }

        return andy::lang::object::instantiate(this, ArrayClass, std::move(array));
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
        std::string str;
        for(size_t i = 0; i < node.childrens().size(); i++) {
            auto& node_child = node.childrens()[i];
            if(node_child.token().type() == andy::lang::lexer::token_type::token_literal)
            {
                switch (node_child.token().kind())
                {
                case lexer::token_kind::token_string:
                    str += node_child.token().content();
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
                std::shared_ptr<andy::lang::object> obj = node_to_object(node.childrens()[i]);
                if(obj->cls != StringClass) {
                    auto method = obj->cls->instance_methods.find("to_string");
                    if(method == obj->cls->instance_methods.end()) {
                        throw std::runtime_error("object of class " + std::string(obj->cls->name) + " does not have a function called 'to_string'");
                    }
                    andy::lang::function_call __call = {
                        "to_string",
                        obj->cls,
                        obj,
                        &method->second,
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

    throw std::runtime_error("interpreter: unknown node type");

    return nullptr;
}

void andy::lang::interpreter::load_extension(andy::lang::extension* extension)
{
    extension->load(this);
    extensions.push_back(extension);
}