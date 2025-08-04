#pragma once
#include <andy/lang/class.hpp>
#include <andy/lang/method.hpp>
#include <andy/lang/object.hpp>



#define UNARY_OPERATOR_INLINE(op, T) \
    [](andy::lang::interpreter* interpreter, std::shared_ptr<andy::lang::object>& object, const andy::lang::parser::ast_node& source_code) { \
        T& value = object->as<T>(); \
        value op; \
        return object; \
    }

#define BINARY_ASSIGNMENT_OPERATOR_INLINE(op, T) \
    [](andy::lang::interpreter* interpreter, std::shared_ptr<andy::lang::object>& object, const andy::lang::parser::ast_node& source_code) { \
        const auto* params_node = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_params); \
        if (!params_node || params_node->childrens().size() != 1) { \
            throw std::runtime_error(std::string(object->cls->name) + "::operator " #op " requires exactly one parameter"); \
        } \
        const auto& param = params_node->childrens()[0]; \
        T& value = object->as<T>(); \
        if (param.type() != andy::lang::parser::ast_node_type::ast_node_valuedecl || param.token().type() != andy::lang::lexer::token_type::token_literal) { \
            throw std::runtime_error(std::string(object->cls->name) + "::operator " #op " requires a numeric literal"); \
        } \
        switch(param.token().kind()) { \
        case andy::lang::lexer::token_kind::token_integer: \
            value op param.token().integer_literal; \
            break; \
        case andy::lang::lexer::token_kind::token_float: \
            value op param.token().float_literal; \
            break; \
        case andy::lang::lexer::token_kind::token_double: \
            value op param.token().double_literal; \
            break; \
        default: \
            throw std::runtime_error(std::string(object->cls->name) + "::operator " #op " requires a numeric literal"); \
        } \
        return object; \
    }
#define BINARY_COMPARISON_OPERATOR_INLINE(op, T) \
    [](andy::lang::interpreter* interpreter, std::shared_ptr<andy::lang::object>& object, const andy::lang::parser::ast_node& source_code) { \
        const auto* params_node = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_params); \
        if (!params_node || params_node->childrens().size() != 1) { \
            throw std::runtime_error(std::string(object->cls->name) + "::operator " #op " requires exactly one parameter"); \
        } \
        const auto& param = params_node->childrens()[0]; \
        T& value = object->as<T>(); \
        bool comparison_result = false; \
        if (param.type() != andy::lang::parser::ast_node_type::ast_node_valuedecl || param.token().type() != andy::lang::lexer::token_type::token_literal) { \
            throw std::runtime_error(std::string(object->cls->name) + "::operator " #op " requires a numeric literal"); \
        } \
        switch(param.token().kind()) { \
        case andy::lang::lexer::token_kind::token_integer: \
            comparison_result = value op param.token().integer_literal; \
            break; \
        case andy::lang::lexer::token_kind::token_float: \
            comparison_result = value op param.token().float_literal; \
            break; \
        case andy::lang::lexer::token_kind::token_double: \
            comparison_result = value op param.token().double_literal; \
            break; \
        default: \
            throw std::runtime_error(std::string(object->cls->name) + "::operator " #op " requires a numeric literal"); \
        } \
        return comparison_result ? std::make_shared<andy::lang::object>(interpreter->TrueClass) : std::make_shared<andy::lang::object>(interpreter->FalseClass); \
    }
#define BINARY_ARITHMETIC_OPERATOR_INLINE(op, T) \
    [](andy::lang::interpreter* interpreter, std::shared_ptr<andy::lang::object>& object, const andy::lang::parser::ast_node& source_code) { \
        const auto* params_node = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_params); \
        if (!params_node || params_node->childrens().size() != 1) { \
            throw std::runtime_error(std::string(object->cls->name) + "::operator " #op " requires exactly one parameter"); \
        } \
        const auto& param = params_node->childrens()[0]; \
        T value = object->as<T>(); \
        if (param.type() != andy::lang::parser::ast_node_type::ast_node_valuedecl || param.token().type() != andy::lang::lexer::token_type::token_literal) { \
            throw std::runtime_error(std::string(object->cls->name) + "::operator " #op " requires a numeric literal"); \
        } \
        switch(param.token().kind()) { \
        case andy::lang::lexer::token_kind::token_integer: \
            value = value op param.token().integer_literal; \
            break; \
        case andy::lang::lexer::token_kind::token_float: \
            value = value op param.token().float_literal; \
            break; \
        case andy::lang::lexer::token_kind::token_double: \
            value = value op param.token().double_literal; \
            break; \
        default: \
            throw std::runtime_error(std::string(object->cls->name) + "::operator " #op " requires a numeric literal"); \
        } \
        return andy::lang::object::create(interpreter, object->cls, value); \
    }

#define UNARY_OPERATOR(op, T) \
    andy::lang::method(#op, andy::lang::method_storage_type::instance_method, {}, [interpreter](andy::lang::function_call& call) { \
        T& value = call.object->as<T>(); \
        value op; \
        return call.object; \
    })

#define BINARY_ASSIGNMENT_OPERATOR(op, T) \
    andy::lang::method(#op, andy::lang::method_storage_type::instance_method, { "other" }, [interpreter](andy::lang::function_call& call) { \
        const auto& params = call.positional_params; \
        T& value = call.object->as<T>(); \
        auto& param = params[0]; \
        if (params[0]->cls == interpreter->IntegerClass) { \
            value op params[0]->as<int>(); \
        } else if (params[0]->cls == interpreter->FloatClass) { \
            value op params[0]->as<float>(); \
        } else if (params[0]->cls == interpreter->DoubleClass) { \
            value op params[0]->as<double>(); \
        } else { \
            throw std::runtime_error("undefined operator " #op " (" + std::string(call.object->cls->name) + ", " + std::string(params[0]->cls->name) + ")"); \
        } \
        return call.object; \
    })

#define BINARY_COMPARISON_OPERATOR(op, T) \
    andy::lang::method(#op, andy::lang::method_storage_type::instance_method, { "other" }, [interpreter](andy::lang::function_call& call) { \
        const auto& params = call.positional_params; \
        std::shared_ptr<andy::lang::object> other = params[0]; \
        T& value = call.object->as<T>(); \
        bool comparison_result = false; \
        if (params[0]->cls == interpreter->IntegerClass) { \
            comparison_result = value op params[0]->as<int>(); \
        } else if (params[0]->cls == interpreter->FloatClass) { \
            comparison_result = value op params[0]->as<float>(); \
        } else if (params[0]->cls == interpreter->DoubleClass) { \
            comparison_result = value op params[0]->as<double>(); \
        } else { \
            throw std::runtime_error("undefined operator " #op " (" + std::string(call.object->cls->name) + ", " + std::string(params[0]->cls->name) + ")"); \
        } \
        return comparison_result ? std::make_shared<andy::lang::object>(interpreter->TrueClass) : std::make_shared<andy::lang::object>(interpreter->FalseClass); \
    })

#define BINARY_ARITHMETIC_OPERATOR(op, T) \
    andy::lang::method(#op, andy::lang::method_storage_type::instance_method, { "other" }, [interpreter](andy::lang::function_call& call) { \
        auto object = call.object; \
        auto other = call.positional_params[0]; \
        const auto& params = call.positional_params; \
        T value = call.object->as<T>(); \
        if (other->cls == interpreter->IntegerClass) { \
            value = value op other->as<int>(); \
        } else if (other->cls == interpreter->FloatClass) { \
            value = value op other->as<float>(); \
        } else if (other->cls == interpreter->DoubleClass) { \
            value = value op other->as<double>(); \
        } else { \
            throw std::runtime_error("undefined operator " #op " (" + std::string(call.object->cls->name) + ", " + std::string(params[0]->cls->name) + ")"); \
        } \
        return andy::lang::object::create(interpreter, object->cls, value); \
    })

namespace andy
{
    namespace lang  
    {
        template<typename T>
        void add_operators(std::shared_ptr<andy::lang::structure> cls, interpreter* interpreter)
        {
            std::map<std::string_view, andy::lang::inline_function> inline_functions = {
                { "++", UNARY_OPERATOR_INLINE(++, T) },
                { "--", UNARY_OPERATOR_INLINE(--, T) },
                { "+=", BINARY_ASSIGNMENT_OPERATOR_INLINE(+=, T) },
                { "-=", BINARY_ASSIGNMENT_OPERATOR_INLINE(-=, T) },
                { "*=", BINARY_ASSIGNMENT_OPERATOR_INLINE(*=, T) },
                { "/=", BINARY_ASSIGNMENT_OPERATOR_INLINE(/=, T) },
                { "==", BINARY_COMPARISON_OPERATOR_INLINE(==, T) },
                { "!=", BINARY_COMPARISON_OPERATOR_INLINE(!=, T) },
                { "<",  BINARY_COMPARISON_OPERATOR_INLINE(<, T)  },
                { ">",  BINARY_COMPARISON_OPERATOR_INLINE(>, T)  },
                { "<=", BINARY_COMPARISON_OPERATOR_INLINE(<=, T) },
                { ">=", BINARY_COMPARISON_OPERATOR_INLINE(>=, T) },
                { "+",  BINARY_ARITHMETIC_OPERATOR_INLINE(+, T)  },
                { "-",  BINARY_ARITHMETIC_OPERATOR_INLINE(-, T)  },
                { "*",  BINARY_ARITHMETIC_OPERATOR_INLINE(*, T)  },
                { "/",  BINARY_ARITHMETIC_OPERATOR_INLINE(/, T)  }
            };
            std::map<std::string_view, andy::lang::method> instance_methods = {
                { "++", UNARY_OPERATOR(++, T) },
                { "--", UNARY_OPERATOR(--, T) },
                { "+=", BINARY_ASSIGNMENT_OPERATOR(+=, T) },
                { "-=", BINARY_ASSIGNMENT_OPERATOR(-=, T) },
                { "*=", BINARY_ASSIGNMENT_OPERATOR(*=, T) },
                { "/=", BINARY_ASSIGNMENT_OPERATOR(/=, T) },
                { "==", BINARY_COMPARISON_OPERATOR(==, T) },
                { "!=", BINARY_COMPARISON_OPERATOR(!=, T) },
                { "<",  BINARY_COMPARISON_OPERATOR(<, T)  },
                { ">",  BINARY_COMPARISON_OPERATOR(>, T)  },
                { "<=", BINARY_COMPARISON_OPERATOR(<=, T) },
                { ">=", BINARY_COMPARISON_OPERATOR(>=, T) },
                { "+",  BINARY_ARITHMETIC_OPERATOR(+, T)  },
                { "-",  BINARY_ARITHMETIC_OPERATOR(-, T)  },
                { "*",  BINARY_ARITHMETIC_OPERATOR(*, T)  },
                { "/",  BINARY_ARITHMETIC_OPERATOR(/, T)  },
            };
            if constexpr (std::is_same_v<T, int>) {
                cls->instance_methods["%"] = andy::lang::method("%", andy::lang::method_storage_type::instance_method, { "other" }, [interpreter](andy::lang::function_call& call) {
                    auto object = call.object;
                    auto other = call.positional_params[0];
                    if(other->cls != interpreter->IntegerClass) {
                        throw std::runtime_error("undefined operator % (" + std::string(object->cls->name) + ", " + std::string(other->cls->name) + ")");
                    }
                    int value = object->as<int>();
                    value %= other->as<int>();
                    return andy::lang::object::create(interpreter, object->cls, value);
                });
                cls->inline_functions["%"] = [](andy::lang::interpreter* interpreter, std::shared_ptr<andy::lang::object>& object, const andy::lang::parser::ast_node& source_code) {
                    const auto* params_node = source_code.child_from_type(andy::lang::parser::ast_node_type::ast_node_fn_params);
                    if (!params_node || params_node->childrens().size() != 1) {
                        throw std::runtime_error(std::string(object->cls->name) + "::operator % requires exactly one parameter");
                    }
                    const auto& other = params_node->childrens()[0];
                    if(other.type() != andy::lang::parser::ast_node_type::ast_node_valuedecl ||
                       other.token().type() != andy::lang::lexer::token_type::token_literal ||
                       other.token().kind() != andy::lang::lexer::token_kind::token_integer) {
                        throw std::runtime_error(std::string(object->cls->name) + "::operator % requires an integer literal");
                    }
                    int value = object->as<int>();
                    value %= other.token().integer_literal;
                    return andy::lang::object::create(interpreter, object->cls, value);
                };
            }
            cls->inline_functions.insert(inline_functions.begin(), inline_functions.end());
            cls->instance_methods.insert(instance_methods.begin(), instance_methods.end());
        }
    };
};