#include <andy/lang/parser.hpp>

#include <uva/file.hpp>
#include <uva/console.hpp>

#include <exception>
#include <iostream>
#include <regex>

#include <andy/lang/object.hpp>
#include <andy/lang/class.hpp>
#include <andy/lang/method.hpp>

using namespace andy;
using namespace lang;

andy::lang::parser::parser()
{
}

andy::lang::parser::ast_node parser::parse_node(andy::lang::lexer &lexer)
{
    // We are at the middle of the source code.
    // What we can have in the middle of the source code:
    // - A comment (we can ignore it)
    // - A keyword
    // - An identifier
    // - A literal
    // - A delimiter (only ';', which is ignored or '{' which is the start of a context)
    // Anything else is an syntax error.

    // We need to see the next token to know what to do.

    const andy::lang::lexer::token& token = lexer.see_next();

    switch (token.type())
    {
    case andy::lang::lexer::token_type::token_comment:
        // Ignore the comment and return the next node
        lexer.consume_token();
        return parse_node(lexer);
        break;
    case andy::lang::lexer::token_type::token_identifier:
    case andy::lang::lexer::token_type::token_literal:
        return parse_identifier_or_literal(lexer);
        break;
    case andy::lang::lexer::token_type::token_delimiter:
        return parse_delimiter(lexer);
    break;
    case andy::lang::lexer::token_type::token_keyword:
        return parse_keyword(lexer);
        break;
    case andy::lang::lexer::token_type::token_eof:
        return parse_eof(lexer);
        break;
    case andy::lang::lexer::token_type::token_preprocessor:
        return parse_preprocessor(lexer);
        break;
    default:
        break;
    }
    
    // If none of the above, it is an error
    throw std::runtime_error(token.error_message_at_current_position("Unexpected token"));
}

andy::lang::parser::ast_node andy::lang::parser::parse_all(andy::lang::lexer &lexer)
{
    ast_node root_node(ast_node_type::ast_node_unit);

    do {
        const andy::lang::lexer::token token = lexer.see_next();
        if(token.is_eof()) {
            break;
        }
        ast_node child = parse_node(lexer);
        root_node.add_child(std::move(child));
    } while(lexer.has_next_token());

    return root_node;
}

andy::lang::parser::ast_node andy::lang::parser::extract_fn_call(andy::lang::lexer &lexer)
{
    // Function call
    andy::lang::lexer::token token = lexer.next_token(); 

    ast_node method_node(ast_node_type::ast_node_fn_call);
    method_node.add_child(std::move(ast_node(std::move(token), ast_node_type::ast_node_declname)));

    token = lexer.next_token(); // (
    
    ast_node params = extract_fn_call_params(lexer);

    method_node.add_child(std::move(params));

    return method_node;
}

andy::lang::parser::ast_node andy::lang::parser::extract_pair(andy::lang::lexer &lexer)
{
    // If this function has been called, we are sure the next tokens are a pair. No need to check for errors.

    // Parse the key
    ast_node pair_node(ast_node_type::ast_node_pair);
    ast_node key_node = parse_identifier_or_literal(lexer);
    key_node.set_type(ast_node_type::ast_node_declname);
    pair_node.add_child(std::move(key_node));
    
    // Consume the ':' token
    lexer.next_token();

    // Extract the value
    ast_node value_node = parse_identifier_or_literal(lexer);
    value_node.set_type(ast_node_type::ast_node_valuedecl);
    pair_node.add_child(std::move(value_node));

    return pair_node;
}

andy::lang::parser::ast_node andy::lang::parser::extract_fn_call_params(andy::lang::lexer &lexer)
{
    ast_node params_node(ast_node_type::ast_node_fn_params);

    andy::lang::lexer::token token = lexer.see_next();

    while(token.content() != ")") {
        ast_node param_node = parse_identifier_or_literal(lexer);
        params_node.add_child(std::move(param_node));
after_extracted_param:
        token = lexer.see_next();

        if(token.type() == lexer::token_type::token_delimiter)
        {
            if(token.content() == ",") {
                token = lexer.next_token();
                continue;
            } else if(token.content() == ")") {
                break;
            } else if(token.content() == ":") {
                lexer.consume_token();

                ast_node named_param(ast_node_type::ast_node_valuedecl);
                ast_node key_node(std::move(params_node.childrens().back()));
                key_node.set_type(ast_node_type::ast_node_declname);
                named_param.add_child(std::move(key_node));
                params_node.childrens().pop_back();

                ast_node value_node = parse_identifier_or_literal(lexer);
                value_node.set_type(ast_node_type::ast_node_valuedecl);
                named_param.add_child(std::move(value_node));

                params_node.add_child(std::move(named_param));

                goto after_extracted_param;
            }
        }

        throw std::runtime_error(token.error_message_at_current_position("Expected ',' or ')'"));
    }

    // The ')' token was seen, so we need to consume it
    lexer.next_token();

    return params_node;
}

andy::lang::parser::ast_node andy::lang::parser::parse_delimiter(andy::lang::lexer &lexer)
{
    const andy::lang::lexer::token& token = lexer.next_token();

    if(token.content() == ";") {
        // ; in the middle of the code is considered a whitespace
        return parse_node(lexer);
    } else if(token.content() == "{") {
        ast_node context_node(ast_node_type::ast_node_context);

        while(true) {
            const andy::lang::lexer::token& next_token = lexer.see_next();

            if(next_token.type() == andy::lang::lexer::token_delimiter) {
                if(next_token.content() == "}") {
                    lexer.consume_token();
                    break;
                } else if (next_token.content() == ";") {
                    // We have to consume ';' here because we are expecting ')'. If there is a ';' just before a
                    // '}', the call to parse_node will ignore ';' and throw an error at the '}' token.
                    lexer.consume_token();
                    continue;
                }
            } else if(next_token.type() == andy::lang::lexer::token_comment) {
                lexer.consume_token();
                continue;
            }
            
            ast_node context_child = parse_node(lexer);
            context_node.add_child(std::move(context_child));
        }

        return context_node;
    } else {
        throw std::runtime_error(token.error_message_at_current_position("Unexpected delimiter"));
    }

    return ast_node(std::move(token), ast_node_type::ast_node_undefined);
}

andy::lang::parser::ast_node andy::lang::parser::parse_eof(andy::lang::lexer &lexer)
{
    andy::lang::lexer::token token = lexer.next_token();
    return ast_node(std::move(token), ast_node_type::ast_node_undefined);
}

andy::lang::parser::ast_node andy::lang::parser::parse_preprocessor(andy::lang::lexer &lexer)
{
    andy::lang::lexer::token token = lexer.next_token();

    // If the directive has not been removed by the preprocessor, it is probably in an invalid location
    throw std::runtime_error(token.error_message_at_current_position("Unexpected '" + std::string(token.content()) + "' directive"));
}

andy::lang::parser::ast_node andy::lang::parser::parse_identifier_or_literal(andy::lang::lexer &lexer, bool chain)
{
    const andy::lang::lexer::token& token = lexer.see_next();

    andy::lang::lexer::token identifier_or_literal;

    // We can have something like:
    // 12345
    // "test"
    // my_var
    // Class::var
    // Class::fn()
    // !value

    switch(token.type()) {
        case andy::lang::lexer::token_type::token_identifier:
        case andy::lang::lexer::token_type::token_literal:
            identifier_or_literal = std::move(lexer.next_token());
            break;
        case andy::lang::lexer::token_type::token_operator:
            // The lexer sees array as operator and we need to handle it here
            if(token.content() == "[") {
                ast_node array_node(ast_node_type::ast_node_arraydecl);
                lexer.consume_token(); // Consume the '[' token
                while(true) {
                    ast_node value_node = parse_identifier_or_literal(lexer);

                    if(value_node.type() != ast_node_type::ast_node_valuedecl && value_node.type() != ast_node_type::ast_node_dictionarydecl && value_node.type() != ast_node_type::ast_node_arraydecl) {
                        throw std::runtime_error(token.error_message_at_current_position("Expected value in array"));
                    }

                    array_node.add_child(std::move(value_node));

                    const andy::lang::lexer::token& comma_or_closing = lexer.next_token();

                    if(comma_or_closing.type() == andy::lang::lexer::token_type::token_delimiter && comma_or_closing.content() == ",") {
                        if(lexer.see_next().content() == "]") {
                            lexer.consume_token();
                            break;
                        }
                    } else {
                        if(comma_or_closing.type() == andy::lang::lexer::token_type::token_operator && comma_or_closing.content() == "]") {
                            break;
                        } else {
                            throw std::runtime_error(token.error_message_at_current_position("Expected ',' or ']'"));
                        }
                    }
                }

                return array_node;
            } else if (token.content() == "!") {
                identifier_or_literal = std::move(lexer.next_token());

                ast_node unary_op(ast_node_type::ast_node_fn_call);
                unary_op.add_child(std::move(ast_node(std::move(identifier_or_literal), ast_node_type::ast_node_declname)));

                ast_node object_node(ast_node_type::ast_node_fn_object);
                object_node.add_child(parse_identifier_or_literal(lexer));

                unary_op.add_child(std::move(object_node));

                return unary_op;
            } else {
                throw std::runtime_error(token.error_message_at_current_position("Unexpected operator"));
            }
        break;
        case andy::lang::lexer::token_type::token_delimiter: {
            // Can be a map {}

            if(token.content() == "{") {
                ast_node map_node(ast_node_type::ast_node_dictionarydecl);

                // The token was seen, so we need to consume it
                lexer.consume_token();

                while(true) {
                    ast_node key_node = parse_identifier_or_literal(lexer);

                    if(key_node.type() != ast_node_type::ast_node_valuedecl) {
                        throw std::runtime_error(token.error_message_at_current_position("Expected key in map"));
                    }

                    const andy::lang::lexer::token& colon_token = lexer.next_token();

                    if(colon_token.content() != ":") {
                        throw std::runtime_error(colon_token.error_message_at_current_position("Expected ':' after key in map"));
                    }

                    ast_node value_node = parse_identifier_or_literal(lexer);

                    if(value_node.type() != ast_node_type::ast_node_valuedecl && value_node.type() != ast_node_type::ast_node_dictionarydecl && value_node.type() != ast_node_type::ast_node_arraydecl) {
                        throw std::runtime_error(token.error_message_at_current_position("Expected value in map"));
                    }

                    ast_node pair_node = ast_node(ast_node_type::ast_node_valuedecl);

                    ast_node key_value_node = ast_node(ast_node_type::ast_node_declname);
                    key_value_node.add_child(std::move(key_node));

                    ast_node value_value_node = ast_node(ast_node_type::ast_node_valuedecl);
                    value_value_node.add_child(std::move(value_node));

                    pair_node.add_child(std::move(key_value_node));
                    pair_node.add_child(std::move(value_value_node));

                    map_node.add_child(std::move(pair_node));

                    const andy::lang::lexer::token& comma_token = lexer.next_token();

                    if(comma_token.content() == ",") {
                        if(lexer.see_next().content() == "}") {
                            lexer.next_token();
                            break;
                        }
                    } else {
                        if(comma_token.content() == "}") {
                            break;
                        } else if(comma_token.is_eof()) {
                            throw std::runtime_error(comma_token.error_message_at_current_position("Expected '}'"));
                        } else {
                            continue;
                        }
                    }
                }

                return map_node;
            } else {
                throw std::runtime_error(token.error_message_at_current_position("Unexpected delimiter"));
            }
        }
        break;
        case andy::lang::lexer::token_type::token_keyword:
            if(token.content() == "new") {
                lexer.consume_token(); // Consume the 'new' token

                ast_node fn_call = parse_identifier_or_literal(lexer);

                if(fn_call.type() != ast_node_type::ast_node_fn_call) {
                    throw std::runtime_error(token.error_message_at_current_position("Expected function call after 'new'"));
                }

                // Ex: Test() where Test will be the declaration name

                fn_call.set_type(ast_node_type::ast_node_declname);

                // The function call has a declaration name as a child, we need to move to the parent and
                // remove it from the children list

                fn_call.set_token(fn_call.childrens().front().token());
                fn_call.childrens().erase(fn_call.childrens().begin());

                // Now we need to add the 'new' keyword as a function call

                ast_node new_node(ast_node_type::ast_node_fn_call);
                new_node.add_child(std::move(ast_node(token, ast_node_type::ast_node_declname)));

                // Need to extract the params from the function call and add it to the new node
                auto params_it = std::find_if(fn_call.childrens().begin(), fn_call.childrens().end(), [](const ast_node& node) {
                    return node.type() == ast_node_type::ast_node_fn_params;
                });

                if(params_it != fn_call.childrens().end()) {
                    new_node.add_child(std::move(*params_it));
                    fn_call.childrens().erase(params_it);
                }
                
                ast_node obj_node(ast_node_type::ast_node_fn_object);
                obj_node.add_child(std::move(fn_call));

                new_node.add_child(std::move(obj_node));

                return new_node;
            } else {
                throw std::runtime_error(token.error_message_at_current_position("Unexpected keyword"));
            }
            break;
        default:
            throw std::runtime_error(token.error_message_at_current_position("Expected identifier or literal"));
            break;
    }

    // After a literal or identifier we can have:
    // '(' (function call)
    ast_node identifier_or_literal_node;

    if(const auto& next_token = lexer.see_next();
        (next_token.type() == andy::lang::lexer::token_type::token_delimiter && next_token.content() == "(")
        || (next_token.type() == andy::lang::lexer::token_type::token_operator && (next_token.content() == "!" || next_token.content() == "?"))) {
        
        // Function call

        if(identifier_or_literal.type() == andy::lang::lexer::token_type::token_literal) {
            throw std::runtime_error(identifier_or_literal.error_message_at_current_position("Illegal invocation of literal as function"));
        }

        if(next_token.type() == andy::lang::lexer::token_type::token_operator) {
            identifier_or_literal.merge(next_token);
            lexer.consume_token(); // Consume the '!' or '?' token
        }

        ast_node fn_node(ast_node_type::ast_node_fn_call);
        fn_node.add_child(std::move(ast_node(std::move(identifier_or_literal), ast_node_type::ast_node_declname)));

        if(lexer.see_next().content() == "(") {
            lexer.consume_token(); // Consume the '(' token
            ast_node params_node = extract_fn_call_params(lexer);

            fn_node.add_child(std::move(params_node));
        }

        const auto& possible_block = lexer.see_next();

        if(possible_block.type() == andy::lang::lexer::token_type::token_delimiter && possible_block.content() == "{") {
            ast_node block_node = parse_delimiter(lexer);
            fn_node.add_child(std::move(block_node));
        }

        identifier_or_literal_node = fn_node;
    } else {
        ast_node_type node_type;
    
        if(identifier_or_literal.type() == andy::lang::lexer::token_type::token_literal) {
            node_type = ast_node_type::ast_node_valuedecl;
        } else {
            node_type = ast_node_type::ast_node_declname;
        }

        identifier_or_literal_node = andy::lang::parser::ast_node(std::move(identifier_or_literal), ast_node_type::ast_node_declname);
    }

    if(identifier_or_literal_node.token().type() == andy::lang::lexer::token_type::token_literal &&
        identifier_or_literal_node.token().kind() == andy::lang::lexer::token_kind::token_interpolated_string)
    {
        identifier_or_literal_node.token().m_kind = andy::lang::lexer::token_kind::token_string;
        ast_node interpolated_node(ast_node_type::ast_node_interpolated_string);
        interpolated_node.add_child(std::move(identifier_or_literal_node));

        for(auto next_token = lexer.see_next(); next_token.type() != andy::lang::lexer::token_type::token_eof; next_token = lexer.see_next())
        {
            if(next_token.type() == andy::lang::lexer::token_type::token_delimiter)
            {
                lexer.consume_token();
                break;
            }

            ast_node child_node = parse_identifier_or_literal(lexer);

            interpolated_node.add_child(std::move(child_node));
        }

        identifier_or_literal_node = interpolated_node;
    }

    if(!chain) {
        return identifier_or_literal_node;
    }

    // And after this we can have:
    // Operator ('.', '+', etc)

    // And it can be chained like:
    // my_var.fn1().fn2().fn3()

    std::vector<andy::lang::parser::ast_node> chained_nodes;

    while(true) {
        const andy::lang::lexer::token& next_token = lexer.see_next();

        if(next_token.type() == andy::lang::lexer::token_type::token_operator) {
            if(next_token.content() == "]") {
                // Already handled in the array declaration
                break;
            }
            if(next_token.content() == ".") {
                lexer.consume_token(); // Consume the '.' token

                ast_node next_node = parse_identifier_or_literal(lexer, false);
                chained_nodes.push_back(std::move(next_node));
            } else {
                ast_node operator_node(ast_node_type::ast_node_fn_call);

                andy::lang::lexer::token& operator_token = lexer.next_token();

                std::string matching;

                if(operator_token.content() == "[") {
                    matching = "]";
                }

                // ++ and -- are unary operators
                if(operator_token.content() != "++" && operator_token.content() != "--") {
                    ast_node right_node = parse_identifier_or_literal(lexer);

                    ast_node params_node(ast_node_type::ast_node_fn_params);
                    params_node.add_child(std::move(right_node));

                    if(matching.size()) {
                        andy::lang::lexer::token& matching_token = lexer.next_token();

                        if(matching_token.content() != matching) {
                            throw std::runtime_error(matching_token.error_message_at_current_position("No matching '" + std::string(matching) + "' found for '" + std::string(operator_node.token().content()) + "'"));
                        }

                        operator_token.merge(matching_token);
                    }

                    operator_node.add_child(std::move(params_node));
                }

                operator_node.add_child(ast_node(std::move(operator_token), ast_node_type::ast_node_declname));

                chained_nodes.push_back(std::move(operator_node));
            }
        } else {
            break;
        }
    }

    if(chained_nodes.size() == 0) {
        return identifier_or_literal_node;
    }

    chained_nodes.insert(chained_nodes.begin(), std::move(identifier_or_literal_node));

    for(size_t i = 0; i < chained_nodes.size(); i++) {
        ast_node& node      = chained_nodes[i];

        if(i < chained_nodes.size() - 1) {
            ast_node& next_node = chained_nodes[i + 1];

            ast_node object_node(ast_node_type::ast_node_fn_object);
            object_node.add_child(std::move(node));

            next_node.add_child(std::move(object_node));
        }
    }

    return chained_nodes[chained_nodes.size() - 1];
}

andy::lang::parser::ast_node andy::lang::parser::parse_keyword(andy::lang::lexer &lexer)
{
    const andy::lang::lexer::token& token = lexer.see_next();

    const std::map<std::string_view, andy::lang::parser::ast_node(andy::lang::parser::*)(andy::lang::lexer&)> keyword_parsers = {
        { "class",     &andy::lang::parser::parse_keyword_class     },
        { "var",       &andy::lang::parser::parse_keyword_var       },
        { "function",  &andy::lang::parser::parse_keyword_function  },
        { "return",    &andy::lang::parser::parse_keyword_return    },
        { "if",        &andy::lang::parser::parse_keyword_if        },
        { "namespace", &andy::lang::parser::parse_keyword_namespace },
        { "for",       &andy::lang::parser::parse_keyword_for       },
        { "foreach",   &andy::lang::parser::parse_keyword_foreach   },
        { "while",     &andy::lang::parser::parse_keyword_while     },
        { "break",     &andy::lang::parser::parse_keyword_break     },
        { "static",    &andy::lang::parser::parse_keyword_static    },
        { "yield",     &andy::lang::parser::parse_keyword_yield     }
    };

    auto keyword_parser = keyword_parsers.find(token.content());

    if(keyword_parser == keyword_parsers.end()) {
        throw std::runtime_error(token.error_message_at_current_position("Unexpected keyword"));
    }

    return (this->*keyword_parser->second)(lexer);
}

andy::lang::parser::ast_node andy::lang::parser::parse_keyword_class(andy::lang::lexer &lexer) {
    ast_node class_node(ast_node_type::ast_node_classdecl);

    class_node.add_child(std::move(ast_node(std::move(lexer.next_token()), ast_node_type::ast_node_decltype)));

    const andy::lang::lexer::token& identifier_token = lexer.see_next();

    if(identifier_token.type() != lexer::token_type::token_identifier) {
        throw std::runtime_error(identifier_token.error_message_at_current_position("Expected class name after 'class'"));
    }

    class_node.add_child(std::move(ast_node(std::move(lexer.next_token()), ast_node_type::ast_node_declname)));

    const andy::lang::lexer::token& extends_or_context_token = lexer.see_next();

    if(extends_or_context_token.content() == "extends" || extends_or_context_token.content() == ":" || extends_or_context_token.content() == "<") {
        lexer.next_token(); // Consume the extends token

        const andy::lang::lexer::token& baseclass_token = lexer.see_next();

        if(baseclass_token.type() != lexer::token_type::token_identifier) {
            throw std::runtime_error(baseclass_token.error_message_at_current_position("Expected identifier as base class name"));
        }

        ast_node base_class_name_node = parse_identifier_or_literal(lexer);

        ast_node base_class_node(ast_node_type::ast_node_classdecl_base);
        base_class_node.add_child(std::move(base_class_name_node));

        class_node.add_child(base_class_node);

        const andy::lang::lexer::token& context_token = lexer.see_next();

        if(context_token.content() != "{") {
            throw std::runtime_error(context_token.error_message_at_current_position("Expected '{' after base class name"));
        }
    } else if(extends_or_context_token.content() != "{") {
        throw std::runtime_error(extends_or_context_token.error_message_at_current_position("Expected '{' after class name"));
    }

    ast_node class_child = parse_node(lexer);

    if(class_child.type() != ast_node_type::ast_node_context) {
        throw std::runtime_error(class_child.token().error_message_at_current_position("Expected context after class declaration"));
    }

    class_node.add_child(std::move(class_child));

    return class_node;
}

andy::lang::parser::ast_node andy::lang::parser::parse_keyword_var(andy::lang::lexer &lexer){
    ast_node var_node(std::move(lexer.next_token()), ast_node_type::ast_node_vardecl);

    andy::lang::lexer::token identifier_token = std::move(lexer.next_token());

    if(identifier_token.type() != lexer::token_type::token_identifier) {
        throw std::runtime_error(identifier_token.error_message_at_current_position("Expected variable name after 'var'"));
    }

    var_node.add_child(std::move(ast_node(std::move(identifier_token), ast_node_type::ast_node_declname)));

    const andy::lang::lexer::token& equal_token = lexer.next_token();

    if(equal_token.type() != lexer::token_type::token_operator || equal_token.content() != "=") {
        throw std::runtime_error(equal_token.error_message_at_current_position("Expected '=' after variable name"));
    }

    ast_node value_node = parse_identifier_or_literal(lexer);

    var_node.add_child(std::move(value_node));

    return var_node;
}

andy::lang::parser::ast_node andy::lang::parser::parse_keyword_function(andy::lang::lexer &lexer) {
    ast_node method_node(ast_node_type::ast_node_fn_decl);
    method_node.add_child(ast_node(std::move(lexer.next_token()), ast_node_type::ast_node_decltype));

    const andy::lang::lexer::token& identifier_token = lexer.see_next();

    switch(identifier_token.type())
    {
        case lexer::token_type::token_identifier:
            // Simply use it as the function name
            break;
        case lexer::token_type::token_keyword:
            if(identifier_token.content() == "new") {
                // This is a constructor
                // Use it as the function name
            } else {
                throw std::runtime_error(identifier_token.error_message_at_current_position("Illegal use of keyword '" + std::string(identifier_token.content()) + "' as function name"));
            }
            break;
        default:
            throw std::runtime_error(identifier_token.error_message_at_current_position("Expected method name after 'function'"));
            break;
    }

    method_node.add_child(std::move(ast_node(std::move(lexer.next_token()), ast_node_type::ast_node_declname)));

    const andy::lang::lexer::token& parenthesis_token = lexer.see_next();

    if(parenthesis_token.content() != "(") {
        throw std::runtime_error(parenthesis_token.error_message_at_current_position("Expected '(' after method name"));
    }

    lexer.consume_token(); // Consume the '(' token

    ast_node params_node(ast_node_type::ast_node_fn_params);

    while(true) {
        const andy::lang::lexer::token& identifier_or_parenthesis = lexer.see_next();

        if(identifier_or_parenthesis.type() == lexer::token_type::token_identifier) {
            auto next_token = lexer.see_next(1);
            if (next_token.type() == lexer::token_type::token_delimiter && next_token.content() == ":") {
                params_node.add_child(extract_pair(lexer));
                continue;
            } else {
                params_node.add_child(ast_node(std::move(lexer.next_token()), ast_node_type::ast_node_declname));
            }

            auto possible_comma = lexer.see_next();

            if(possible_comma.type() == lexer::token_type::token_delimiter && possible_comma.content() == ",") {
                lexer.consume_token(); // Consume the ',' token
                continue;
            }
        } else if(identifier_or_parenthesis.type() == lexer::token_type::token_delimiter && identifier_or_parenthesis.content() == ")") {
            lexer.consume_token(); // Consume the ')' token
            break;
        }
        else {
            throw std::runtime_error(identifier_or_parenthesis.error_message_at_current_position("Expected parameter name"));
        }
    }

    method_node.add_child(std::move(params_node));

    ast_node fn_context = parse_node(lexer);

    if(fn_context.type() != ast_node_type::ast_node_context) {
        throw std::runtime_error(fn_context.token().error_message_at_current_position("Expected context after function declaration"));
    }

    method_node.add_child(std::move(fn_context));

    return method_node;
}

andy::lang::parser::ast_node andy::lang::parser::parse_keyword_return(andy::lang::lexer &lexer) {
    ast_node return_node(std::move(lexer.next_token()), ast_node_type::ast_node_fn_return);

    return_node.add_child(std::move(parse_identifier_or_literal(lexer)));

    return return_node;
}

andy::lang::parser::ast_node andy::lang::parser::parse_keyword_if(andy::lang::lexer &lexer){
    ast_node if_node(ast_node_type::ast_node_conditional);
    if_node.add_child(ast_node(std::move(lexer.next_token()), ast_node_type::ast_node_decltype));

    const andy::lang::lexer::token& parenthesis_token = lexer.next_token();

    if(parenthesis_token.content() != "(") {
        throw std::runtime_error(parenthesis_token.error_message_at_current_position("Expected '(' after 'if'"));
    }

    ast_node condition_node(ast_node_type::ast_node_condition);
    condition_node.add_child(std::move(parse_identifier_or_literal(lexer)));

    if_node.add_child(std::move(condition_node));

    const andy::lang::lexer::token& close_parenthesis_token = lexer.next_token();

    if(close_parenthesis_token.content() != ")") {
        throw std::runtime_error(close_parenthesis_token.error_message_at_current_position("Expected ')' after 'if' condition"));
    }

    ast_node if_context = parse_node(lexer);

    if(if_context.type() != ast_node_type::ast_node_context) {
        throw std::runtime_error(if_context.token().error_message_at_current_position("Expected context after 'if'"));
    }

    if_node.add_child(std::move(if_context));

    // Check if there is an else

    const andy::lang::lexer::token& token = lexer.see_next();

    if(token.type() == andy::lang::lexer::token_type::token_keyword && token.content() == "else") {
        lexer.next_token(); // Consume the else token

        ast_node else_node(ast_node_type::ast_node_else);

        ast_node else_context = parse_node(lexer);

        if(else_context.type() != ast_node_type::ast_node_context) {
            throw std::runtime_error(token.error_message_at_current_position("Expected context after 'else'"));
        }

        else_node.add_child(std::move(else_context));

        if_node.add_child(std::move(else_node));
    }

    return if_node;
}

andy::lang::parser::ast_node andy::lang::parser::parse_keyword_namespace(andy::lang::lexer &lexer) {
    ast_node namespace_node(ast_node_type::ast_node_classdecl);
    namespace_node.add_child(ast_node(std::move(lexer.next_token()), ast_node_type::ast_node_decltype));

    const andy::lang::lexer::token& identifier_token = lexer.see_next();

    if(identifier_token.type() != lexer::token_type::token_identifier) {
        throw std::runtime_error(identifier_token.error_message_at_current_position("Expected namespace name after 'namespace'"));
    }

    namespace_node.add_child(std::move(ast_node(std::move(lexer.next_token()), ast_node_type::ast_node_declname)));

    ast_node namespace_context = parse_node(lexer);

    if(namespace_context.type() != ast_node_type::ast_node_context) {
        throw std::runtime_error(namespace_context.token().error_message_at_current_position("Expected context after namespace declaration"));
    }

    namespace_node.add_child(std::move(namespace_context));

    return namespace_node;
}

andy::lang::parser::ast_node andy::lang::parser::parse_keyword_for(andy::lang::lexer &lexer)
{
    ast_node for_node(ast_node_type::ast_node_for);
    for_node.add_child(ast_node(std::move(lexer.next_token()), ast_node_type::ast_node_decltype));

    const andy::lang::lexer::token& parenthesis_token = lexer.next_token();

    if(parenthesis_token.type() != lexer::token_type::token_delimiter || parenthesis_token.content() != "(") {
        throw std::runtime_error(parenthesis_token.error_message_at_current_position("Expected '(' after 'for'"));
    }

    const andy::lang::lexer::token& var_token = lexer.see_next();

    if(var_token.type() != lexer::token_type::token_keyword || var_token.content() != "var") {
        throw std::runtime_error(var_token.error_message_at_current_position("Expected 'var' after '('"));
    }

    ast_node var_node = parse_keyword_var(lexer);

    const andy::lang::lexer::token& semicolon_token = lexer.next_token();

    if(parenthesis_token.type() != lexer::token_type::token_delimiter || semicolon_token.content() != ";") {
        throw std::runtime_error(semicolon_token.error_message_at_current_position("Expected ';' after 'for' variable declaration"));
    }

    ast_node condition_node(ast_node_type::ast_node_condition);
    ast_node condition_child = parse_identifier_or_literal(lexer);

    if(condition_child.type() != ast_node_type::ast_node_fn_call) {
        throw std::runtime_error(condition_child.token().error_message_at_current_position("Expected condition after 'for' variable declaration"));
    }

    condition_node.add_child(std::move(condition_child));

    const andy::lang::lexer::token& semicolon2_token = lexer.next_token();

    if(parenthesis_token.type() != lexer::token_type::token_delimiter || semicolon2_token.content() != ";") {
        throw std::runtime_error(semicolon2_token.error_message_at_current_position("Expected ';' after 'for' condition"));
    }

    ast_node increment_node = parse_node(lexer);

    if(increment_node.type() != ast_node_type::ast_node_fn_call) {
        throw std::runtime_error(increment_node.token().error_message_at_current_position("Expected increment after 'for' condition"));
    }

    const andy::lang::lexer::token& close_parenthesis_token = lexer.next_token();

    if(close_parenthesis_token.content() != ")") {
        throw std::runtime_error(close_parenthesis_token.error_message_at_current_position("Expected ')' after 'for' increment"));
    }

    ast_node for_context = parse_node(lexer);

    if(for_context.type() != ast_node_type::ast_node_context) {
        throw std::runtime_error(for_context.token().error_message_at_current_position("Expected context after 'for'"));
    }

    // Optimization:
    // If the var_node is an integer declaration, the codition is a comparison
    // with a literal and the increment is a simple increment, the we can
    // extract these literals and do a for loop. This will exclude 2 functions
    // calls (which is expensive) per iteration.

    // auto value = var_node.child_from_type(ast_node_valuedecl);

    // if(value && value->token().type() == lexer::token_type::token_literal && value->token().kind() == lexer::token_kind::token_integer)
    // {
    //     auto condition_fn = condition_node.child_from_type(ast_node_fn_call);
    //     if(condition_fn)
    //     {
    //         auto condition_fn_declname = condition_fn->child_from_type(ast_node_declname);

    //         if(condition_fn_declname->token().op() != lexer::operator_type::operator_null)
    //         {
    //             auto condition_fn_params = condition_fn->child_from_type(ast_node_fn_params);

    //             if(condition_fn_params && condition_fn_params->childrens().size() == 1)
    //             {
    //                 auto condition_fn_param = condition_fn_params->childrens().front();

    //                 if(condition_fn_param.type() == ast_node_type::ast_node_valuedecl && condition_fn_param.token().type() == lexer::token_type::token_literal && condition_fn_param.token().kind() == lexer::token_kind::token_integer)
    //                 {
    //                     auto increment_fn = increment_node.child_from_type(ast_node_fn_call);

    //                     if(increment_fn)
    //                     {
    //                         auto increment_fn_declname = increment_fn->child_from_type(ast_node_declname);

    //                         if(increment_fn_declname->token().op() != lexer::operator_type::operator_null)
    //                         {
    //                             auto increment_fn_params = increment_fn->child_from_type(ast_node_fn_params);

    //                             // Unary de/increment
    //                             if(increment_fn_params && increment_fn_params->childrens().size() == 0)
    //                             {
    //                             }
    //                         }
    //                     }
    //                 }
    //             }      
    //         }
    //     }
    // }

    for_node.add_child(std::move(var_node));
    for_node.add_child(std::move(condition_node));
    for_node.add_child(std::move(increment_node));
    for_node.add_child(std::move(for_context));

    return for_node;
}

andy::lang::parser::ast_node andy::lang::parser::parse_keyword_foreach(andy::lang::lexer &lexer) {
    ast_node foreach_node(ast_node_type::ast_node_foreach);
    foreach_node.add_child(ast_node(std::move(lexer.next_token()), ast_node_type::ast_node_decltype));

    const andy::lang::lexer::token& parenthesis_token = lexer.next_token();

    if(parenthesis_token.type() != lexer::token_type::token_delimiter || parenthesis_token.content() != "(") {
        throw std::runtime_error(parenthesis_token.error_message_at_current_position("Expected '(' after 'foreach'"));
    }

    const andy::lang::lexer::token& var_token = lexer.next_token();

    if(var_token.content() != "var") {
        throw std::runtime_error(var_token.error_message_at_current_position("Expected 'var' after '('"));
    }

    ast_node var_node(ast_node_type::ast_node_vardecl);
    var_node.add_child(ast_node(std::move(var_token), ast_node_type::ast_node_decltype));

    const andy::lang::lexer::token& identifier_token = lexer.next_token();

    if(identifier_token.type() != lexer::token_type::token_identifier) {
        throw std::runtime_error(identifier_token.error_message_at_current_position("Expected variable name after 'var'"));
    }

    var_node.add_child(ast_node(std::move(identifier_token), ast_node_type::ast_node_declname));

    foreach_node.add_child(std::move(var_node));

    const andy::lang::lexer::token& in_token = lexer.next_token();

    if(in_token.content() != "in" && in_token.content() != ":" && in_token.content() != "of") {
        throw std::runtime_error(in_token.error_message_at_current_position("Expected 'in', ':' or 'of after variable name"));
    }

    const andy::lang::lexer::token& identifier2_token = lexer.next_token();

    if(identifier2_token.type() != lexer::token_type::token_identifier) {
        throw std::runtime_error(identifier2_token.error_message_at_current_position("Expected identifier after 'in'"));
    }

    foreach_node.add_child(ast_node(std::move(identifier2_token), ast_node_type::ast_node_valuedecl));

    const andy::lang::lexer::token& close_parenthesis_token = lexer.next_token();

    if(close_parenthesis_token.content() != ")") {
        throw std::runtime_error(close_parenthesis_token.error_message_at_current_position("Expected ')' after 'foreach' declaration"));
    }

    ast_node context_node = parse_node(lexer);

    if(context_node.type() != ast_node_type::ast_node_context) {
        throw std::runtime_error(context_node.token().error_message_at_current_position("Expected context after 'foreach'"));
    }

    foreach_node.add_child(std::move(context_node));

    return foreach_node;
}

andy::lang::parser::ast_node andy::lang::parser::parse_keyword_while(andy::lang::lexer &lexer)
{
    ast_node while_node(ast_node_type::ast_node_while);
    while_node.add_child(ast_node(std::move(lexer.next_token()), ast_node_type::ast_node_decltype));

    const andy::lang::lexer::token& parenthesis_token = lexer.next_token();

    if(parenthesis_token.content() != "(") {
        throw std::runtime_error(parenthesis_token.error_message_at_current_position("Expected '(' after 'while'"));
    }

    ast_node condition_node(ast_node_type::ast_node_condition);
    condition_node.add_child(std::move(parse_identifier_or_literal(lexer)));

    while_node.add_child(std::move(condition_node));

    const andy::lang::lexer::token& close_parenthesis_token = lexer.next_token();

    if(close_parenthesis_token.content() != ")") {
        throw std::runtime_error(close_parenthesis_token.error_message_at_current_position("Expected ')' after 'while' condition"));
    }

    ast_node while_context = parse_node(lexer);

    if(while_context.type() != ast_node_type::ast_node_context) {
        throw std::runtime_error(while_context.token().error_message_at_current_position("Expected context after 'while'"));
    }

    while_node.add_child(std::move(while_context));

    return while_node;
}

andy::lang::parser::ast_node andy::lang::parser::parse_keyword_break(andy::lang::lexer &lexer)
{
    ast_node break_node(ast_node_type::ast_node_break);
    break_node.add_child(ast_node(std::move(lexer.next_token()), ast_node_type::ast_node_decltype));

    return break_node;
}

andy::lang::parser::ast_node andy::lang::parser::parse_keyword_static(andy::lang::lexer &lexer)
{
    const andy::lang::lexer::token& static_token = lexer.next_token();

    const andy::lang::lexer::token& next_token = lexer.see_next();

    if(next_token.type() == andy::lang::lexer::token_type::token_keyword) {
        if(next_token.content() == "function") {
            ast_node node = parse_keyword_function(lexer);
            node.add_child(ast_node(std::move(static_token), ast_node_type::ast_node_declstatic));

            return node;
        } else if(next_token.content() == "var") {
            ast_node node = parse_keyword_var(lexer);
            node.add_child(ast_node(std::move(static_token), ast_node_type::ast_node_declstatic));

            return node;
        }
    }
    
    throw std::runtime_error(next_token.error_message_at_current_position("Expected keyword 'function' or 'var' after 'static'"));
}

andy::lang::parser::ast_node andy::lang::parser::parse_keyword_yield(andy::lang::lexer &lexer)
{
    andy::lang::parser::ast_node node(std::move(lexer.next_token()), ast_node_type::ast_node_yield);

    auto next_token = lexer.see_next();

    if(next_token.type() == andy::lang::lexer::token_type::token_delimiter && next_token.content() == "(") {
        lexer.consume_token(); // Consume the '(' token
        node.add_child(extract_fn_call_params(lexer));
    }

    return node;
}