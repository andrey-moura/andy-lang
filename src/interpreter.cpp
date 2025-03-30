#include <andy/lang/interpreter.hpp>

#include <iostream>

#include <uva/file.hpp>

#include <andy/lang/extension.hpp>
#include <andy/lang/lang.hpp>

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

    classes.push_back(cls);
}

std::shared_ptr<andy::lang::structure> andy::lang::interpreter::execute_classdecl(andy::lang::parser::ast_node source_code)
{
    std::string_view class_name = source_code.decname();

    auto cls = std::make_shared<andy::lang::structure>(std::string(class_name));

    auto baseclass_node = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_classdecl_base);

    if (baseclass_node)
    {
        std::shared_ptr<andy::lang::structure> base_class = nullptr;

        if(auto fn_object = baseclass_node->child_from_type(andy::lang::parser::ast_node_type::ast_node_declname)) {
            auto cls_object = try_object_from_declname(*fn_object);
            if(cls_object && cls_object->cls == ClassClass) {
                base_class = cls_object->as<std::shared_ptr<andy::lang::structure>>();
            } else {
                base_class = find_class(baseclass_node->decname());
            }
        }
        
        if(!base_class) {
            throw std::runtime_error("base class " + std::string(baseclass_node->decname()) + " not found");
        }

        cls->base = base_class;
        base_class->deriveds.push_back(cls);
    }

    for(auto& class_child : source_code.context()->childrens()) {
        switch (class_child.type())
        {
        case andy::lang::parser::ast_node_type::ast_node_fn_decl: {
            std::string_view method_name = class_child.decname();

            std::vector<std::string> params;
            params.reserve(class_child.childrens().size());

            for(auto& param : class_child.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_params)->childrens()) {
                params.push_back(std::string(param.token().content()));
            }

            auto static_node = class_child.child_from_type(andy::lang::parser::ast_node_type::ast_node_declstatic);

            auto method = andy::lang::method(std::move(std::string(method_name)), method_storage_type::instance_method, params, class_child);

            if(static_node || source_code.decl_type() == "namespace") {
                cls->class_methods[method_name] = std::move(method);
            } else {
                cls->instance_methods[method_name] = std::move(method);
            }
        }
        break;
        case andy::lang::parser::ast_node_type::ast_node_vardecl: {
            std::string_view var_name = class_child.decname();
            cls->instance_variables[var_name] = NullClass;
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

std::shared_ptr<andy::lang::object> andy::lang::interpreter::execute(const andy::lang::parser::ast_node& source_code, std::shared_ptr<andy::lang::object>& object)
{
    switch (source_code.type())
    {
        case andy::lang::parser::ast_node_type::ast_node_fn_decl: {
            
            std::string_view method_name = source_code.decname();

            std::vector<std::string> params;
            params.reserve(source_code.childrens().size());

            for(auto& param : source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_params)->childrens()) {
                params.push_back(std::string(param.token().content()));
            }

            current_context().functions[method_name] = andy::lang::method(std::string(method_name), method_storage_type::instance_method, params, source_code);
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
            andy::lang::method* method_to_call = nullptr;

            // And we have a shared_ptr in case the object is created, os it still alive in the current context
            std::shared_ptr<andy::lang::object> object_to_call = nullptr;

            std::shared_ptr<andy::lang::structure> class_to_call = nullptr;

            if(auto it = current_context().functions.find(source_code.decname()); it != current_context().functions.end()) {
                method_to_call = &it->second;
            }

            const andy::lang::parser::ast_node* object_node = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_object);

            std::string_view function_name = source_code.decname();
            bool is_super = function_name == "super";
            bool is_assignment = function_name == "=";

            if(!method_to_call && object_node) {
                // function call from a class/object/function return value

                object_node = object_node->childrens().data();

                if(object_node->type() == andy::lang::parser::ast_node_type::ast_node_declname) {
                    object_to_call = try_object_from_declname(*object_node, object ? object->cls : nullptr, object);

                    if(object_to_call) {
                        if(is_assignment) {
                            const auto& params_node = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_params);

                            std::shared_ptr<andy::lang::object> new_object = node_to_object(params_node->childrens().front());
                            *object_to_call = std::move(*new_object);

                            return object;
                        } else {
                            std::map<std::string_view, andy::lang::method>::iterator method_it;
                            
                            if(function_name == "new") {
                                if(object_to_call->cls == ClassClass) {
                                    auto real_class = object_to_call->as<std::shared_ptr<andy::lang::structure>>();
                                    method_it = real_class->instance_methods.find(function_name);

                                    if(method_it == real_class->instance_methods.end()) {
                                        // default constructor
                                        return andy::lang::object::instantiate(this, real_class, nullptr);
                                    } else {
                                        method_to_call = &method_it->second;
                                        class_to_call = real_class;
                                        object_to_call = andy::lang::object::instantiate(this, real_class, nullptr, { andy::lang::object::create(this, StringClass, object_to_call->cls->name) });
                                    }
                                } else {
                                    method_it = object_to_call->cls->instance_methods.find(function_name);
                                    if(method_it == object_to_call->cls->instance_methods.end()) {
                                        // default constructor
                                        return andy::lang::object::instantiate(this, object_to_call->cls, nullptr);
                                    } else {
                                        method_to_call = &method_it->second;
                                        class_to_call = object_to_call->cls;
                                    }
                                }
                            } else {
                                method_it = object_to_call->cls->instance_methods.find(function_name);

                                if(method_it == object_to_call->cls->instance_methods.end()) {
                                    auto throw_exception = [&]() {
                                        std::string message;
                                        message.reserve(100);
                                        message += "class ";
                                        message += object_to_call->cls->name;
                                        message += " does not have a function called ";
                                        message += std::string(function_name);
                                        throw std::runtime_error(message);
                                    };
                                    if(object_to_call->cls == ClassClass) {
                                        auto real_class = object_to_call->as<std::shared_ptr<andy::lang::structure>>();
                                        method_it = real_class->class_methods.find(function_name);

                                        if(method_it == real_class->class_methods.end()) {
                                            throw_exception();
                                        }

                                        method_to_call = &method_it->second;
                                        class_to_call = real_class;
                                    } else {
                                        if(object_to_call->cls->base) {
                                            method_it = object_to_call->cls->base->instance_methods.find(function_name);

                                            if(method_it != object_to_call->cls->base->instance_methods.end()) {
                                                method_to_call = &method_it->second;
                                                class_to_call = object_to_call->cls->base;
                                                object_to_call = object_to_call->base_instance;
                                            } else {
                                                throw_exception();
                                            }
                                        } else {
                                            throw_exception();
                                        }
                                    }
                                } else {
                                    method_to_call = &method_it->second;
                                    class_to_call = object_to_call->cls;
                                }
                            }
                        }
                    } else {
                        std::string_view class_or_object_name = object_node->token().content();
                        for(auto& cls : classes) {
                            if(cls->name == class_or_object_name) {
                                if(function_name == "new") {
                                    auto it = cls->instance_methods.find(function_name);
                                    if(it == cls->instance_methods.end()) {
                                        // default constructor
                                        return andy::lang::object::instantiate(this, cls, nullptr);
                                    } else {
                                        method_to_call = &it->second;
                                        class_to_call = cls;
                                    }
                                } else {
                                    auto it = cls->class_methods.find(function_name);

                                    if(it == cls->class_methods.end()) {
                                        throw std::runtime_error("class " + std::string(class_or_object_name) + " does not have a function called " + std::string(function_name));
                                    }

                                    method_to_call = &it->second;
                                    class_to_call = cls;
                                }

                                break;
                            }
                        }
                    }
                } else if (object_node->type() == andy::lang::parser::ast_node_type::ast_node_fn_call) {
                    object_to_call = execute(*object_node, object);

                    if(!object_to_call) {
                        throw std::runtime_error(object_node->token().error_message_at_current_position("undefined operator '.' for null"));
                    }

                    auto it = object_to_call->cls->instance_methods.find(std::string(function_name));

                    if(it == object_to_call->cls->instance_methods.end()) {
                        throw std::runtime_error("class " + object_to_call->cls->name + " does not have a function called " + std::string(function_name));
                    }

                    method_to_call = &it->second;
                    class_to_call = object_to_call->cls;
                } else if(object_node->type() == andy::lang::parser::ast_node_type::ast_node_valuedecl) {
                    for(auto& cls : classes) {
                        if(cls->name == object_node->token().content()) {
                            class_to_call = cls;
                            break;
                        }
                    }

                    if(class_to_call) {
                        auto it = class_to_call->instance_methods.find(std::string(function_name));

                        if(it == class_to_call->instance_methods.end()) {
                            throw std::runtime_error("class " + class_to_call->name + " does not have a function called " + std::string(function_name));
                        }

                        method_to_call = &it->second;
                    } else {
                        std::shared_ptr<andy::lang::structure> object_class = nullptr;

                        if(object) {
                            object_class = object->cls;
                        }

                        object_to_call = node_to_object(*object_node, object_class, object);

                        auto it = object_to_call->cls->instance_methods.find(std::string(function_name));

                        if(it == object_to_call->cls->instance_methods.end()) {
                            throw std::runtime_error("class " + object_to_call->cls->name + " does not have a function called " + std::string(function_name));
                        }

                        method_to_call = &it->second;
                        class_to_call = object_to_call->cls;
                    }
                }
            } else {
                if(is_super) {
                    if(!object) {
                        throw std::runtime_error("super can only be called from an instance object");
                    }

                    if(!object->cls->base) {
                        throw std::runtime_error("class " + object->cls->name + " does not have a base class");
                    }

                    auto it = object->cls->base->instance_methods.find("new");

                    if(it == object->cls->base->instance_methods.end()) {
                        throw std::runtime_error("base class " + object->cls->base->name + " does not have a constructor");
                    }

                    method_to_call = &it->second;
                    object_to_call = object;
                    class_to_call = object->cls->base;
                } else {
                    if(object) {
                        auto it = object->cls->instance_methods.find(function_name);

                        if(it == object->cls->instance_methods.end()) {
                            if(object->cls->base) {
                                it = object->cls->base->instance_methods.find(function_name);

                                if(it != object->cls->base->instance_methods.end()) {
                                    if(!object->base_instance) {
                                        throw std::runtime_error("object has no base instance");
                                    }

                                    method_to_call = &it->second;
                                    class_to_call = object->cls->base;
                                    object_to_call = object->base_instance;
                                } else {
                                    // ?????
                                    // Why was it throwing exceptions?
                                    //throw std::runtime_error("function '" + function_name + "' not found in class " + object->cls->name);
                                }
                            }
                        } else {
                            method_to_call = &it->second;
                            object_to_call = object;
                            class_to_call = object->cls;
                        }
                    } 
                    
                    if(!method_to_call) {
                        auto it = current_context().functions.find(function_name);

                        if(it == current_context().functions.end()) {
                            if(!is_global_context()) {
                                auto it = global_context.functions.find(function_name);

                                if(it != global_context.functions.end()) {
                                    method_to_call = &it->second;
                                    class_to_call = nullptr;
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
                        } else {
                            method_to_call = &it->second;
                        }
                    }
                }
            }

            std::vector<std::shared_ptr<andy::lang::object>> positional_params;
            std::map<std::string, std::shared_ptr<andy::lang::object>> named_params;

            const andy::lang::parser::ast_node* params_node = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_params);

            if(params_node) {
                for(auto& param : params_node->childrens()) {
                    const andy::lang::parser::ast_node* value_node = &param;
                    if(param.type() == andy::lang::parser::ast_node_type::ast_node_valuedecl && param.childrens().size()) {
                        // Named parameter
                        if(auto __value_node = param.child_from_type(andy::lang::parser::ast_node_type::ast_node_valuedecl)) {
                            value_node = __value_node;
                        }
                    }
                    std::shared_ptr<andy::lang::object> value = nullptr;
                    
                    value = node_to_object(*value_node);

                    const andy::lang::parser::ast_node* name = nullptr;
                    
                    if(param.type() == andy::lang::parser::ast_node_type::ast_node_valuedecl) {
                        name = param.child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
                    }

                    if(name) {
                        named_params[std::string(name->token().content())] = value;
                    } else {
                        positional_params.push_back(value);
                    }
                }
            }

            andy::lang::function_call __call = {
                function_name,
                class_to_call,
                object_to_call,
                *method_to_call,
                std::move(positional_params),
                std::move(named_params),
                source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_context)
            };

            std::shared_ptr<andy::lang::object> ret = call(__call);

            if(is_super) {
                object_to_call->base_instance = ret;
                return nullptr;
            }

            return ret;
        }
        break;
        case andy::lang::parser::ast_node_type::ast_node_vardecl: {
            std::string_view var_name = source_code.decname();
            std::shared_ptr<andy::lang::structure> cls = nullptr;
            if(object) {
                cls = object->cls;
            }
            std::shared_ptr<andy::lang::object> value = node_to_object(source_code.childrens()[1], cls, object);
            current_context().variables[var_name] = value;
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
                
                if(e) {
                    auto else_context = e->child_from_type(andy::lang::parser::ast_node_type::ast_node_context);

                    ret = execute(*else_context, object);
                }
            }

            return ret;
        }
        break;
        case andy::lang::parser::ast_node_type::ast_node_while: {
            std::shared_ptr<andy::lang::object> ret = nullptr;

            while(execute(*source_code.condition(), object)->is_present()) {
                ret = execute(*source_code.context(), object);

                if(current_context().has_returned) {
                    return current_context().return_value;
                }
            }

            return ret;
        }
        break;
        case andy::lang::parser::ast_node_type::ast_node_break: {
            current_context().has_returned = true;
            return nullptr;
        }
        case andy::lang::parser::ast_node_type::ast_node_context:
            return execute_all(source_code, object);
        break;
        case andy::lang::parser::ast_node_type::ast_node_condition: {
            return node_to_object(source_code.childrens().front());
        }
        break;
        case andy::lang::parser::ast_node_type::ast_node_fn_return: {
            if(source_code.childrens().size()) {
                return node_to_object(source_code.childrens().front());
            } else {
                return std::make_shared<andy::lang::object>(NullClass);
            }
        }
        break;
        case andy::lang::parser::ast_node_type::ast_node_foreach: {
            auto* valuedecl = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_valuedecl);

            std::shared_ptr<andy::lang::object> array_or_dictionary = node_to_object(*valuedecl);

            auto* vardecl = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_vardecl);

            if(array_or_dictionary->cls == ArrayClass) {
                std::vector<std::shared_ptr<andy::lang::object>>& array_values = array_or_dictionary->as<std::vector<std::shared_ptr<andy::lang::object>>>();
                for(auto& value : array_values) {
                    current_context().variables[vardecl->decname()] = value;
                    execute_all(*source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_context), object);
                }
            } else if(array_or_dictionary->cls == DictionaryClass) {
                andy::lang::dictionary& dictionary_values = array_or_dictionary->as<andy::lang::dictionary>();
                for(auto& [key, value] : dictionary_values) {
                    std::vector<std::shared_ptr<andy::lang::object>> params = { key, value };
                    std::shared_ptr<andy::lang::object> params_object = andy::lang::object::instantiate(this, ArrayClass, params);

                    current_context().variables[vardecl->decname()] = params_object;

                    execute_all(*source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_context), object);
                }
            } else {
                throw std::runtime_error("foreach should iterate over an array or a dictionary");
            }
        }
        break;
        case andy::lang::parser::ast_node_type::ast_node_for: {
            auto* vardecl = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_vardecl);
            auto* valuedecl = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_valuedecl);
            auto* condition_node = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_condition);
            auto* fn_call = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_call);

            std::string var_name(vardecl->decname());

            std::shared_ptr<andy::lang::object> start = execute(*vardecl, object);

            while(true) {
                std::shared_ptr<andy::lang::object> condition = execute(*condition_node, object);

                if(!condition->is_present()) {
                    break;
                }

                push_context(true);
                execute_all(*source_code.context(), object);
                pop_context();

                execute(*fn_call, object);
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
                method,
                {},
                {},
                current_context().given_block
            };
            return call(__call);
        }
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
    push_context();

    current_context().given_block = call.given_block;

    bool is_constructor = call.name == "new";

    if(is_constructor) {
        // Special case
        // The object is created before the method is called
        // If the object was instantiated in from native code, it will be passed as a parameter
        if(!call.object) {
            call.object = std::make_shared<andy::lang::object>(call.cls);
        }
    }

    std::shared_ptr<andy::lang::object> ret = nullptr;

    if(call.positional_params.size() != call.method.positional_params.size()) {
        throw std::runtime_error("function " + call.method.name + " expects " + std::to_string(call.method.positional_params.size()) + " parameters, but " + std::to_string(call.positional_params.size()) + " were given");
    }

    for(const auto& param : call.method.named_params) {
        auto it = call.named_params.find(param.name);

        if(it == call.named_params.end()) {
            if(param.has_default_value) {
                call.named_params[param.name] = var_to_object(param.default_value);
            } else {
                throw std::runtime_error("function " + call.method.name + " called without parameter " + param.name);
            }
        }
    }

    if(call.method.block_ast.childrens().size()) {
        for(size_t i = 0; i < call.method.positional_params.size(); i++) {
            current_context().variables[call.method.positional_params[i].name] = call.positional_params[i];
        }

        for(auto& [name, value] : call.named_params) {
            current_context().variables[name] = value;
        }
        
        if(call.method.block_ast.type() == andy::lang::parser::ast_node_type::ast_node_context) {
            ret = execute_all(call.method.block_ast, call.object);
        } else {
            ret = execute(*call.method.block_ast.block(), call.object);
        }
    } else if(call.method.function) {
        ret = call.method.function(call);
    }

    if(is_constructor) {
        if(ret) {
            throw std::runtime_error("constructor should not return a value");
        }

        ret = call.object;
    }

    for (auto& [name, value] : current_context().variables) {
        if(value->base_instance) {
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
    andy::lang::structure::create_structures(this);
}

const std::shared_ptr<andy::lang::object> andy::lang::interpreter::try_object_from_declname(const andy::lang::parser::ast_node& node, std::shared_ptr<andy::lang::structure> cls, std::shared_ptr<andy::lang::object> object)
{
    if(object) {
        auto it = object->instance_variables.find(node.token().content());

        if(it != object->instance_variables.end()) {
            return it->second;
        }

        if(object->cls == ClassClass) {
            auto cls = object->as<std::shared_ptr<andy::lang::structure>>();
            auto it = cls->class_variables.find(node.token().content());

            if(it != cls->class_variables.end()) {
                return it->second;
            }
        }
    }

    auto* fn_object = node.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_object);

    if(fn_object) {
        auto* fn_object_decname = fn_object->child_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
        
        if(fn_object_decname) {
            std::string_view class_name = fn_object_decname->token().content();
            std::string_view var_name = node.token().content();
    
            if(auto fn_object = try_object_from_declname(*fn_object_decname)) {
                return try_object_from_declname(node, fn_object->cls, fn_object);
            }

            for(auto& cls : classes) {
                if(cls->name == class_name) {
                    auto it = cls->class_variables.find(var_name);
    
                    if(it == cls->class_variables.end()) {
                        throw std::runtime_error("class " + std::string(class_name) + " does not have a variable called " + std::string(var_name));
                    } else {
                        return it->second;
                    }
                }
            }
    
            throw std::runtime_error("class " + std::string(class_name) + " not found");
        } else {
            auto fn_object_fn_call = fn_object->child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_call);

            if(fn_object_fn_call) {
                std::shared_ptr fn_object = execute(*fn_object_fn_call, object);
                if(fn_object) {
                    auto it = fn_object->instance_variables.find(node.token().content());
                    if(it != fn_object->instance_variables.end()) {
                        return it->second;
                    }
                    throw std::runtime_error("Class " + fn_object->cls->name + " does not have a variable called " + std::string(node.token().content()));
                } else {
                    throw std::runtime_error("Cannot read property '" + std::string(node.token().content()) + "' of void");
                }
            }
        }
    }

    auto it = current_context().variables.find(node.token().content());

    if(it != current_context().variables.end()) {
        return it->second;
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
                        throw std::runtime_error("object of class " + obj->cls->name + " does not have a function called 'to_string'");
                    }
                    andy::lang::function_call __call = {
                        "to_string",
                        obj->cls,
                        obj,
                        method->second,
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

std::shared_ptr<andy::lang::object> andy::lang::interpreter::var_to_object(var v)
{
    switch(v.type)
    {
    case var::var_type::string:
        return andy::lang::object::instantiate(this, StringClass, std::move(v.as<var::string>()));
        break;
    case var::var_type::integer:
        return andy::lang::object::instantiate(this, IntegerClass, v.as<var::integer>());
        break;
    default:
        throw std::runtime_error("interpreter: unknown var type");
    break;
    }

    return nullptr;
}

void andy::lang::interpreter::load_extension(andy::lang::extension* extension)
{
    extension->load(this);
    extensions.push_back(extension);
}

void andy::lang::interpreter::start_extensions()
{
    for(auto& extension : extensions) {
        extension->start(this);
    }
}
