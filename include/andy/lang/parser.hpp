#pragma once

#include <filesystem>
#include <map>
#include <functional>
#include <string>

#include <uva/core.hpp>

#include <andy/lang/lexer.hpp>

namespace andy
{
    namespace lang
    {
        class parser
        {
        public:
            parser();
            ~parser() = default;
        public:
            enum ast_node_type {
                ast_node_undefined,

                ast_node_unit,
                ast_node_expansion,

                ast_node_context,

                ast_node_classdecl,
                ast_node_classdecl_base,

                ast_node_fn_decl,
                ast_node_fn_return,
                ast_node_fn_call,
                ast_node_fn_params,
                ast_node_fn_object,

                ast_node_valuedecl,
                ast_node_pair,
                ast_node_interpolated_string,
                ast_node_arraydecl,
                ast_node_dictionarydecl,
                ast_node_vardecl,

                ast_node_decltype,
                ast_node_declname,
                ast_node_declstatic,

                ast_node_conditional,
                ast_node_while,
                ast_node_for,
                ast_node_for_start,
                ast_node_for_step,
                ast_node_for_end,
                ast_node_foreach,
                ast_node_break,
                ast_node_else,
                ast_node_condition,
                ast_node_yield
            };
            class ast_node
            {
            public:
                ast_node()
                    : m_type(ast_node_type::ast_node_undefined) {
                    
                }
                ast_node(ast_node_type __type)
                    : m_type(__type) {
                    
                }
                ast_node(andy::lang::lexer::token __token, ast_node_type __type)
                    : m_token(__token), m_type(__type) {
                    
                }
            protected:
                andy::lang::lexer::token m_token;
                ast_node_type m_type;
                std::vector<ast_node> m_children;
            public:
                bool is_undefined() const {
                    return m_type == ast_node_type::ast_node_undefined;
                }
            // Setters
            public:
                void add_child(ast_node child) {
                    m_children.push_back(std::move(child));
                }
                void set_type(ast_node_type __type)
                {
                    m_type = __type;
                }
                void set_token(andy::lang::lexer::token __token)
                {
                    m_token = std::move(__token);
                }
            // Getters
            public:
                andy::lang::lexer::token& token() {
                    return m_token;
                }

                const andy::lang::lexer::token& token() const {
                    return m_token;
                }

                std::vector<ast_node>& childrens() {
                    return m_children;
                }

                const std::vector<ast_node>& childrens() const {
                    return m_children;
                }

                ast_node_type type() const {
                    return m_type;
                }

                const ast_node* child_from_type(const andy::lang::parser::ast_node_type& __type) const {
                    for(auto& child : m_children) {
                        if(child.type() == __type) {
                            return &child;
                        }
                    }

                    return nullptr;
                }

                ast_node* child_from_type(const andy::lang::parser::ast_node_type& __type) {
                    for(auto& child : m_children) {
                        if(child.type() == __type) {
                            return &child;
                        }
                    }

                    return nullptr;
                }

                const andy::lang::lexer::token* child_token_from_type(const andy::lang::parser::ast_node_type& __type) const {
                    return &child_from_type(__type)->token();
                }

                const std::string_view child_content_from_type(const ast_node_type& __type) const {
                    return child_token_from_type(__type)->content();
                }

                const std::string_view decname() const {
                    return child_content_from_type(ast_node_type::ast_node_declname);
                }

                const std::string_view decl_type() const {
                    return child_content_from_type(ast_node_type::ast_node_decltype);
                }

                const std::string_view value() const {
                    return child_content_from_type(ast_node_type::ast_node_valuedecl);
                }

                const ast_node* condition() const {
                    return child_from_type(ast_node_type::ast_node_condition);
                }

                const ast_node* block() const {
                    return child_from_type(ast_node_type::ast_node_context);
                }

                const ast_node* context() const {
                    return child_from_type(ast_node_type::ast_node_context);
                }

                const ast_node* fn_object() const {
                    return child_from_type(ast_node_type::ast_node_fn_object);
                }
            };
        protected:
            std::filesystem::path current_path;
        public:
            std::filesystem::path absolute(const std::string& path) {
                return current_path / path;
            }
        public:
            andy::lang::parser::ast_node parse_node(andy::lang::lexer& lexer);
            andy::lang::parser::ast_node parse_all(andy::lang::lexer& lexer);

        // Commons extract functions used by parsers
        protected:
            /// @brief Extract a function call parameters. You must consume the ')' token.
            /// @param lexer The lexer.
            /// @return A function call parameters node.
            andy::lang::parser::ast_node extract_fn_call_params(andy::lang::lexer& lexer);
            /// @brief Extract a function call.
            /// @param lexer The lexer.
            /// @return A function call node.
            andy::lang::parser::ast_node extract_fn_call(andy::lang::lexer& lexer);
            /// @brief Extract a pair
            /// @param lexer The lexer.
            /// @return The pair node.
            andy::lang::parser::ast_node extract_pair(andy::lang::lexer& lexer);
            andy::lang::parser::ast_node parse_identifier_or_literal(andy::lang::lexer& lexer, bool chain = true);
        // Parsers functions
        protected:
            /// @brief Parse a keyword.
            /// @param lexer The lexer.
            /// @return A keyword node.
            andy::lang::parser::ast_node parse_keyword(andy::lang::lexer& lexer);
            /// @brief Parse a delimiter.
            /// @param lexer The lexer.
            /// @return A delimiter node.
            andy::lang::parser::ast_node parse_delimiter(andy::lang::lexer& lexer);
            /// @brief Parse a context.
            /// @param lexer The lexer.
            /// @return A context node.
            andy::lang::parser::ast_node parse_eof(andy::lang::lexer& lexer);
            /// @brief Parse a preprocessing directive.
            /// @param lexer The lexer.
            /// @return An exception
            andy::lang::parser::ast_node parse_preprocessor(andy::lang::lexer& lexer);
        protected:
            andy::lang::parser::ast_node parse_keyword_class(andy::lang::lexer& lexer);
            andy::lang::parser::ast_node parse_keyword_var(andy::lang::lexer& lexer);
            andy::lang::parser::ast_node parse_keyword_function(andy::lang::lexer& lexer);
            andy::lang::parser::ast_node parse_keyword_return(andy::lang::lexer& lexer);
            andy::lang::parser::ast_node parse_keyword_if(andy::lang::lexer& lexer);
            andy::lang::parser::ast_node parse_keyword_namespace(andy::lang::lexer& lexer);
            andy::lang::parser::ast_node parse_keyword_for(andy::lang::lexer& lexer);
            andy::lang::parser::ast_node parse_keyword_foreach(andy::lang::lexer& lexer);
            andy::lang::parser::ast_node parse_keyword_while(andy::lang::lexer& lexer);
            andy::lang::parser::ast_node parse_keyword_break(andy::lang::lexer& lexer);
            andy::lang::parser::ast_node parse_keyword_static(andy::lang::lexer& lexer);
            andy::lang::parser::ast_node parse_keyword_yield(andy::lang::lexer& lexer);
        };
    }
}; // namespace andy