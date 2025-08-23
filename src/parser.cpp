#include <andy/lang/parser.hpp>

#include <andy/file.hpp>
#include <andy/console.hpp>

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

andy::lang::parser::ast_node extract_context(andy::lang::lexer &lexer, andy::lang::parser& parser, bool break_on_else = false)
{
    andy::lang::parser::ast_node output(andy::lang::parser::ast_node_type::ast_node_context);
    while(true) {
        const andy::lang::lexer::token& next_token = lexer.see_next();

        if(next_token.type() == andy::lang::lexer::token_delimiter) {
            if(next_token.content() == "end") {
                // The token is consumed by the caller
                break;
            } else if (next_token.content() == ";") {
                // We have to consume ';' here because we are expecting '}'. If there is a ';' just before a
                // '}', the call to parse_node will ignore ';' and throw an error at the '}' token.
                lexer.consume_token();
                continue;
            }
        } else if(next_token.type() == andy::lang::lexer::token_comment) {
            lexer.consume_token();
            continue;
        } else if(next_token.type() == andy::lang::lexer::token_type::token_eof) {
            throw std::runtime_error(next_token.error_message_at_current_position("Unexpected end of file, expecting 'end'"));
        } else if (break_on_else && next_token.type() == andy::lang::lexer::token_type::token_keyword && next_token.content() == "else") {
            // If we are breaking on else, we stop here.
            break;
        }
        
        andy::lang::parser::ast_node context_child = parser.parse_node(lexer);
        output.add_child(std::move(context_child));
    }
    return output;
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

andy::lang::parser::ast_node andy::lang::parser::extract_pair(andy::lang::lexer &lexer)
{
    // If this function has been called, we are sure the next tokens are a pair. No need to check for errors.

    // Parse the key
    ast_node pair_node(ast_node_type::ast_node_pair);

    ast_node key_node = ast_node(lexer.next_token(), ast_node_type::ast_node_declname);
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

    while(true) {
        ast_node param_node = parse_identifier_or_literal(lexer);
        params_node.add_child(std::move(param_node));

        auto& token = lexer.see_next();

        if(token.type() == lexer::token_type::token_delimiter)
        {
            if(token.content() == ":") {
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
            }

            if(lexer.see_next().content() == ",") {
                lexer.consume_token();
            } else {
                break;
            }
        } else {
        break;
        }
    }

    return params_node;
}

andy::lang::parser::ast_node andy::lang::parser::parse_delimiter(andy::lang::lexer &lexer)
{
    const andy::lang::lexer::token& token = lexer.next_token();

    if(token.content() == ";") {
        // ; in the middle of the code is considered a whitespace
        return parse_node(lexer);
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

static bool is_identifier_or_literal(const andy::lang::lexer::token& token)
{
    return token.type() == andy::lang::lexer::token_type::token_identifier ||
           token.type() == andy::lang::lexer::token_type::token_literal;
}

static bool is_on_same_line(const andy::lang::lexer::token& token, const andy::lang::lexer::token& other)
{
    return token.start.line == other.start.line;
}

static bool is_function_call(const andy::lang::lexer::token& token, const andy::lang::lexer& lexer)
{
    if(token.type() != andy::lang::lexer::token_type::token_identifier) {
        return false;
    }
    
    auto& next_token = lexer.see_next();

    return next_token.type() == andy::lang::lexer::token_type::token_delimiter
            && next_token.content() == "(";
}

static bool is_no_parentheses_function_call(const andy::lang::lexer::token& token, const andy::lang::lexer& lexer)
{
    if(token.type() != andy::lang::lexer::token_type::token_identifier) {
        return false;
    }

    auto& next_token = lexer.see_next();

    return is_identifier_or_literal(next_token) && is_on_same_line(token, next_token);
}

static bool is_any_function_call(const andy::lang::lexer::token& token, const andy::lang::lexer& lexer)
{
    return is_function_call(token, lexer) || is_no_parentheses_function_call(token, lexer);
}

static void extract_fn_yield_block_if_exists(andy::lang::parser::ast_node& node, andy::lang::parser& parser, const andy::lang::lexer::token& token, andy::lang::lexer& lexer)
{
    if(node.type() != andy::lang::parser::ast_node_type::ast_node_fn_call &&
        node.type() != andy::lang::parser::ast_node_type::ast_node_declname) {
        // Cant extract yield block from this node
        return;
    }

    auto& next_token = lexer.see_next();

    if(next_token.type() == andy::lang::lexer::token_type::token_identifier && next_token.content() == "do") {
        lexer.consume_token(); // Consume the 'do' token

        andy::lang::parser::ast_node yield_context = extract_context(lexer, parser);
        yield_context.set_end_token(lexer.next_token());
        node.add_child(std::move(yield_context));
    }
}

static andy::lang::parser::ast_node chain_if_exists(andy::lang::parser::ast_node& node, andy::lang::parser& parser, andy::lang::lexer& lexer)
{
    std::vector<andy::lang::parser::ast_node> chained_nodes;

    while(true) {
        const andy::lang::lexer::token& next_token = lexer.see_next();

        if(next_token.type() == andy::lang::lexer::token_type::token_operator) {
            if(next_token.content() == "]") {
                // Already handled in the array declaration
                break;
            }
            if(next_token.content() == "." || next_token.content() == "::") {
                lexer.consume_token(); // Consume the '.' token

                andy::lang::parser::ast_node next_node = parser.parse_identifier_or_literal(lexer, false);
                chained_nodes.push_back(std::move(next_node));
            } else {
                andy::lang::parser::ast_node operator_node(andy::lang::parser::ast_node_type::ast_node_fn_call);

                andy::lang::lexer::token& operator_token = lexer.next_token();

                std::string matching;

                if(operator_token.content() == "[") {
                    matching = "]";
                }

                // ++ and -- are unary operators
                if(operator_token.content() != "++" && operator_token.content() != "--") {
                    andy::lang::parser::ast_node right_node = parser.parse_identifier_or_literal(lexer);

                    andy::lang::parser::ast_node params_node(andy::lang::parser::ast_node_type::ast_node_fn_params);
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

                operator_node.add_child(andy::lang::parser::ast_node(std::move(operator_token), andy::lang::parser::ast_node_type::ast_node_declname));

                chained_nodes.push_back(std::move(operator_node));
            }
        } else {
            break;
        }
    }

    if(chained_nodes.size() == 0) {
        extract_fn_yield_block_if_exists(node, parser, node.token(), lexer);
        return node;
    }

    chained_nodes.insert(chained_nodes.begin(), std::move(node));

    for(size_t i = 0; i < chained_nodes.size(); i++) {
        andy::lang::parser::ast_node& node = chained_nodes[i];

        if(i < chained_nodes.size() - 1) {
            andy::lang::parser::ast_node& next_node = chained_nodes[i + 1];

            if(node.type() == andy::lang::parser::ast_node_type::ast_node_declname && next_node.type() == andy::lang::parser::ast_node_type::ast_node_declname) {
                if(next_node.token().content() == "new") {
                    andy::lang::parser::ast_node fn_call_node(andy::lang::parser::ast_node_type::ast_node_fn_call);
                    fn_call_node.add_child(std::move(next_node));

                    next_node = std::move(fn_call_node);
                }
            }

            andy::lang::parser::ast_node object_node(andy::lang::parser::ast_node_type::ast_node_fn_object);
            object_node.add_child(std::move(node));

            next_node.add_child(std::move(object_node));
        }
    }

    auto& last_node = chained_nodes[chained_nodes.size() - 1];

    extract_fn_yield_block_if_exists(last_node, parser, last_node.token(), lexer);

    return last_node;
}

andy::lang::parser::ast_node andy::lang::parser::parse_identifier_or_literal(andy::lang::lexer &lexer, bool chain)
{
    const andy::lang::lexer::token& token = lexer.see_next();

    andy::lang::lexer::token identifier_or_literal;

    switch(token.type()) {
        case andy::lang::lexer::token_type::token_identifier: {
            auto possible_colon = lexer.see_next(1);
            if(possible_colon.type() == andy::lang::lexer::token_type::token_delimiter && possible_colon.content() == ":") {
                return extract_pair(lexer);
            }
            identifier_or_literal = std::move(lexer.next_token());
            break;
        }
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
            } else if(token.content() == "[]") {
                // empty array declaration
                ast_node array_node(ast_node_type::ast_node_arraydecl);
                lexer.consume_token(); // Consume the '[]' token
                return array_node;
            }
            else {
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
            } else if(token.content() == "(") {
                // Expressions like (1 + 2)
                lexer.consume_token(); // Consume the '(' token
                ast_node expression_node = parse_identifier_or_literal(lexer);
                lexer.consume_token(); // Consume the ')' token
                return chain_if_exists(expression_node, *this, lexer);
            }
            else {
                throw std::runtime_error(token.error_message_at_current_position("Unexpected delimiter"));
            }
        }
        break;
        default:
            throw std::runtime_error(token.error_message_at_current_position("Expected identifier or literal"));
            break;
    }

    ast_node identifier_or_literal_node;
    const auto& next_token = lexer.see_next();

    if(is_any_function_call(identifier_or_literal, lexer)) {
        ast_node fn_node(ast_node_type::ast_node_fn_call);
        fn_node.add_child(std::move(ast_node(std::move(identifier_or_literal), ast_node_type::ast_node_declname)));

        if(next_token.type() == andy::lang::lexer::token_type::token_delimiter && next_token.content() == "(") {
            lexer.consume_token(); // Consume the '(' token

            if(auto next_token = lexer.see_next(); next_token.type() == andy::lang::lexer::token_type::token_delimiter && next_token.content() == ")") {
                // No parameters, just a closing parenthesis
                // Consumes the ')' token
                lexer.consume_token();
            } else {
                ast_node params_node = extract_fn_call_params(lexer);
                fn_node.add_child(std::move(params_node));            

                const auto& possible_closing = lexer.see_next();

                if(possible_closing.type() == andy::lang::lexer::token_type::token_delimiter && possible_closing.content() == ")") {
                    // Consume the ')' token
                    lexer.consume_token();
                } else {
                    throw std::runtime_error(possible_closing.error_message_at_current_position("Expected ')'"));
                }
            }
        } else {
            // fn call with literal or identifier
            ast_node params_node = extract_fn_call_params(lexer);
            fn_node.add_child(std::move(params_node));
        }
        identifier_or_literal_node = std::move(fn_node);
    }
    else {
        ast_node_type node_type;
    
        if(identifier_or_literal.type() == andy::lang::lexer::token_type::token_literal) {
            node_type = ast_node_type::ast_node_valuedecl;
        } else {
            node_type = ast_node_type::ast_node_declname;
        }

        identifier_or_literal_node = andy::lang::parser::ast_node(std::move(identifier_or_literal), node_type);
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
        extract_fn_yield_block_if_exists(identifier_or_literal_node, *this, identifier_or_literal_node.token(), lexer);
        return identifier_or_literal_node;
    }

    return chain_if_exists(identifier_or_literal_node, *this, lexer);
}

andy::lang::parser::ast_node andy::lang::parser::parse_keyword(andy::lang::lexer &lexer)
{
    const andy::lang::lexer::token& token = lexer.see_next();

    static std::map<std::string_view, andy::lang::parser::ast_node(andy::lang::parser::*)(andy::lang::lexer&)> keyword_parsers = {
        { "type",      &andy::lang::parser::parse_keyword_class     },
        { "var",       &andy::lang::parser::parse_keyword_var       },
        { "fn" ,       &andy::lang::parser::parse_keyword_function  },
        { "return",    &andy::lang::parser::parse_keyword_return    },
        { "if",        &andy::lang::parser::parse_keyword_if        },
        { "loop",      &andy::lang::parser::parse_keyword_loop      },
        { "namespace", &andy::lang::parser::parse_keyword_namespace },
        { "break",     &andy::lang::parser::parse_keyword_break     },
        { "static",    &andy::lang::parser::parse_keyword_static    },
        { "yield",     &andy::lang::parser::parse_keyword_yield     },
        { "within",    &andy::lang::parser::parse_keyword_within    }
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

    if(extends_or_context_token.type() == andy::lang::lexer::token_identifier && extends_or_context_token.content() == "extends") {
        ast_node base_class_node(ast_node_type::ast_node_classdecl_base);
        base_class_node.add_child(std::move(ast_node(extends_or_context_token, ast_node_type::ast_node_decltype)));
        lexer.consume_token(); // Consume the 'extends' or ':' token

        const andy::lang::lexer::token& baseclass_token = lexer.see_next();

        if(baseclass_token.type() != lexer::token_type::token_identifier) {
            throw std::runtime_error(baseclass_token.error_message_at_current_position("Expected identifier as base class name"));
        }

        ast_node base_class_name_node = parse_identifier_or_literal(lexer);

        base_class_node.add_child(std::move(base_class_name_node));

        class_node.add_child(base_class_node);
    }

    ast_node class_context = extract_context(lexer, *this);
    class_node.add_child(std::move(class_context));
    class_node.set_end_token(lexer.next_token());

    return class_node;
}

andy::lang::parser::ast_node andy::lang::parser::parse_keyword_var(andy::lang::lexer &lexer){
    ast_node var_node(ast_node_type::ast_node_vardecl);
    var_node.add_child(std::move(ast_node(std::move(lexer.next_token()), ast_node_type::ast_node_decltype)));

    andy::lang::lexer::token identifier_token = lexer.see_next();

    if(identifier_token.type() != lexer::token_type::token_identifier) {
        throw std::runtime_error(identifier_token.error_message_at_current_position("Expected variable name after 'var'"));
    }

    var_node.add_child(std::move(ast_node(std::move(identifier_token), ast_node_type::ast_node_declname)));

    const andy::lang::lexer::token& equal_token = lexer.see_next(1);

    if(equal_token.type() == lexer::token_type::token_operator && equal_token.content() == "=") {
        // There is an '=' token, extract as a function call
        var_node.add_child(parse_node(lexer));
    } else {
        // No '=' token, consume the name token.
        lexer.consume_token();
    }

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

    if(parenthesis_token.content() == "(") {
        lexer.consume_token(); // Consume the '(' token

        ast_node params_node(ast_node_type::ast_node_fn_params);

        while(true) {
            const andy::lang::lexer::token& identifier_or_parenthesis = lexer.see_next();

            if(identifier_or_parenthesis.type() == lexer::token_type::token_identifier) {
                auto next_token = lexer.see_next(1);
                if (next_token.type() == lexer::token_type::token_delimiter && next_token.content() == ":") {
                    params_node.add_child(extract_pair(lexer));
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
    }

    ast_node fn_context = extract_context(lexer, *this);
    method_node.add_child(std::move(fn_context));
    method_node.set_end_token(lexer.next_token());

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

    ast_node condition_node(ast_node_type::ast_node_condition);
    condition_node.add_child(std::move(parse_identifier_or_literal(lexer)));

    if_node.add_child(std::move(condition_node));

    ast_node if_context = extract_context(lexer, *this, true);
    if_node.add_child(std::move(if_context));
    
    andy::lang::lexer::token token = lexer.next_token();

    if_node.set_end_token(token); // Consume the closing 'end' or 'else'

    // Check if there is an else
    if(token.type() == andy::lang::lexer::token_type::token_keyword && token.content() == "else") {
        // Check if it is an else if
        const andy::lang::lexer::token& next_token = lexer.see_next();

        if(next_token.type() == andy::lang::lexer::token_type::token_keyword && next_token.content() == "if") {
            ast_node else_node = parse_keyword_if(lexer);
            if_node.add_child(std::move(else_node));
        } else {
            ast_node else_node(ast_node_type::ast_node_else);
            ast_node else_context = extract_context(lexer, *this);
            else_node.add_child(std::move(else_context));
            else_node.set_end_token(lexer.next_token());
            if_node.add_child(std::move(else_node));
        }
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

    ast_node namespace_context = extract_context(lexer, *this);
    namespace_node.add_child(std::move(namespace_context));
    namespace_node.set_end_token(lexer.next_token()); // Consume the closing 'end'

    return namespace_node;
}

andy::lang::parser::ast_node andy::lang::parser::parse_keyword_loop(andy::lang::lexer& lexer)
{
    lexer.consume_token(); // Consume the 'loop' keyword

    static std::map<std::string_view, andy::lang::parser::ast_node(andy::lang::parser::*)(andy::lang::lexer&)> loop_parsers = {
        { "while",   &andy::lang::parser::parse_keyword_while   },
        { "for",     &andy::lang::parser::parse_keyword_foreach },
        { "times",   &andy::lang::parser::parse_keyword_for },
        { "until",   &andy::lang::parser::parse_keyword_while   }
    };

    const andy::lang::lexer::token& next_token = lexer.see_next();

    if(next_token.type() == andy::lang::lexer::token_type::token_identifier) {
        auto loop_parser = loop_parsers.find(next_token.content());

        if(loop_parser != loop_parsers.end()) {
            // Call the appropriate loop parser
            return (this->*loop_parser->second)(lexer);
        }
    }

    throw std::runtime_error(next_token.error_message_at_current_position("Expected 'while', 'for', 'foreach' or 'times' after 'loop'"));
}

andy::lang::parser::ast_node andy::lang::parser::parse_keyword_for(andy::lang::lexer &lexer)
{
    ast_node for_node(ast_node_type::ast_node_for);

    const andy::lang::lexer::token& times_token = lexer.next_token();
    for_node.add_child(ast_node(std::move(times_token), ast_node_type::ast_node_decltype));

    ast_node value_node = parse_identifier_or_literal(lexer);
    for_node.add_child(std::move(value_node));

    ast_node context_node = extract_context(lexer, *this);
    for_node.add_child(std::move(context_node));
    for_node.set_end_token(lexer.next_token()); // Consume the closing 'end'
    return for_node;
}

andy::lang::parser::ast_node andy::lang::parser::parse_keyword_foreach(andy::lang::lexer &lexer) {
    ast_node foreach_node(ast_node_type::ast_node_foreach);
    foreach_node.add_child(ast_node(std::move(lexer.next_token()), ast_node_type::ast_node_decltype));

    ast_node var_node(ast_node_type::ast_node_vardecl);
    const andy::lang::lexer::token& identifier_token = lexer.next_token();

    if(identifier_token.type() != lexer::token_type::token_identifier) {
        throw std::runtime_error(identifier_token.error_message_at_current_position("Expected variable name after 'var'"));
    }

    var_node.add_child(ast_node(std::move(identifier_token), ast_node_type::ast_node_declname));

    foreach_node.add_child(std::move(var_node));

    const andy::lang::lexer::token& in_token = lexer.next_token();

    if(in_token.type() != andy::lang::lexer::token_type::token_identifier || in_token.content() != "in") {
        throw std::runtime_error(in_token.error_message_at_current_position("Expected 'in' after variable name"));
    }

    auto array_node = parse_identifier_or_literal(lexer);

    if(array_node.type() != ast_node_type::ast_node_arraydecl && array_node.type() != ast_node_type::ast_node_declname) {
        throw std::runtime_error(array_node.token().error_message_at_current_position("Expected array or dictionary after 'in'"));
    }

    ast_node value_node(ast_node_type::ast_node_valuedecl);
    value_node.add_child(std::move(array_node));
    foreach_node.add_child(std::move(value_node));

    ast_node context_node = extract_context(lexer, *this);
    foreach_node.add_child(std::move(context_node));
    foreach_node.set_end_token(lexer.next_token()); // Consume the closing 'end'

    return foreach_node;
}

andy::lang::parser::ast_node andy::lang::parser::parse_keyword_while(andy::lang::lexer &lexer)
{
    ast_node while_node(ast_node_type::ast_node_while);
    while_node.add_child(ast_node(std::move(lexer.next_token()), ast_node_type::ast_node_decltype));

    ast_node condition_node(ast_node_type::ast_node_condition);
    condition_node.add_child(std::move(parse_identifier_or_literal(lexer)));

    while_node.add_child(std::move(condition_node));

    ast_node while_context = extract_context(lexer, *this);
    while_node.add_child(std::move(while_context));
    while_node.set_end_token(lexer.next_token()); // Consume the closing 'end'

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
        if(next_token.content() == "fn") {
            ast_node node = parse_keyword_function(lexer);
            node.add_child(ast_node(std::move(static_token), ast_node_type::ast_node_declstatic));

            return node;
        } else if(next_token.content() == "var") {
            ast_node node = parse_keyword_var(lexer);
            node.add_child(ast_node(std::move(static_token), ast_node_type::ast_node_declstatic));

            return node;
        }
    }
    
    throw std::runtime_error(next_token.error_message_at_current_position("Expected keyword 'fun' or 'var' after 'static'"));
}

andy::lang::parser::ast_node andy::lang::parser::parse_keyword_within(andy::lang::lexer &lexer)
{
    lexer.consume_token(); // Consume the 'within' token
    const andy::lang::lexer::token& token = lexer.next_token();
    ast_node object_node(ast_node_type::ast_node_fn_object);
    object_node.add_child(ast_node(std::move(token), ast_node_type::ast_node_declname));

    ast_node within_node = extract_context(lexer, *this);
    within_node.add_child(std::move(object_node));
    within_node.set_end_token(lexer.next_token()); // Consume the closing 'end'
    return within_node;
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