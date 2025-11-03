#include <andy/lang/function.hpp>
#include <andy/lang/object.hpp>
#include <andy/lang/class.hpp>

andy::lang::fn_parameter::fn_parameter(std::string_view __name)
{
    name.reserve(__name.size());
    andy::lang::lexer* lexer = new andy::lang::lexer();
    lexer->tokenize("", __name);
    if(lexer->tokens().size() == 1) {
        name = __name;
        return;
    }

    andy::lang::parser* parser = new andy::lang::parser();
    auto* node = new andy::lang::parser::ast_node(std::move(parser->parse_all(*lexer)));
    node = node->childrens().data();
    switch(node->type()) {
        case andy::lang::parser::ast_node_type::ast_node_declname:
            name = node->token().content();
            break;
        case andy::lang::parser::ast_node_type::ast_node_pair:
            name = node->child_content_from_type(andy::lang::parser::ast_node_type::ast_node_declname);
            named = true;
            default_value_node = node->child_from_type(andy::lang::parser::ast_node_type::ast_node_valuedecl);
            has_default_value = default_value_node != nullptr;
            break;
        default:
            throw std::runtime_error("Invalid parameter type: " + std::to_string(static_cast<int>(node->type())));
    }
}

std::shared_ptr<andy::lang::object> andy::lang::function::call(std::shared_ptr<andy::lang::object> o)
{
    andy::lang::function_call call = {
        name,
        nullptr,
        o,
        this,
        {},
        {},
        nullptr
    };
    return native_function(call);
}

void andy::lang::function::init_params(std::vector<std::string> __params)
{
    for(auto& param : __params) {
        fn_parameter fn_param(std::move(param));
        if(fn_param.named) {
            named_params.push_back(std::move(fn_param));
        } else {
            positional_params.push_back(std::move(fn_param));
        }
    }
}
