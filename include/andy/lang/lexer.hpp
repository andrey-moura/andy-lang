#pragma once

#include <vector>
#include <memory>
#include <map>
#include <utility>
#include <string>
#include <stdexcept>
#include <filesystem>

namespace andy
{
    namespace lang
    {
        class lexer
        {
        public:
            lexer() = default;
            lexer(std::string_view __file_name, std::string_view __source);
            ~lexer() = default;
        public:
            enum token_type {
                token_undefined,
                token_comment,
                token_keyword,
                token_identifier,
                token_literal,
                token_delimiter,
                token_operator,
                token_preprocessor,
                token_eof,
                token_type_max
            };
            enum token_kind {
                token_null,
                token_boolean,
                token_integer,
                token_float,
                token_double,
                token_string,
            };
            enum operator_type {
                operator_null,
                operator_plus,
                operator_minus,
                operator_multiply,
                operator_divide,
                operator_modulo,
                operator_power,
                operator_and,
                operator_or,
                operator_not,
                operator_equal,
                operator_not_equal,
                operator_less,
                operator_less_equal,
                operator_greater,
                operator_greater_equal,
                operator_increment,
                operator_decrement,
                operator_max
            };
            struct token_position {
                size_t line = 0;
                size_t column = 0;
                size_t offset = 0;
            };
            class token {
            protected:
                std::string_view m_content;
                token_type m_type;
                token_kind m_kind;
                operator_type m_operator;
            public:
                struct {
                    union {
                        int integer_value;
                        double double_value;
                        float float_value;
                        bool boolean_value;
                    };
                } m_literal;
            public:
                std::string_view m_file_name;
            public:
                token(token_position start, token_position end, std::string_view content, token_type type, token_kind kind, std::string_view file_name, operator_type op = operator_type::operator_max);
                token(token_position start, token_position end, std::string_view content, token_type type, token_kind kind = token_kind::token_null);
                token(token&& other);
                token(const token&) = default;
                token() = default;
                ~token() = default;
            public:
            public:
                bool is_eof() const { return m_type == token_type::token_eof; }
            public:
                std::string error_message_at_current_position(std::string_view what) const;
                std::string unexpected_eof_message() const;
                std::string_view human_start_position() const;

                void merge(const token& other);
            public:
                /// @brief Return the content of the token.
                const std::string_view & content() const { return m_content; }
                /// @brief Return the type of the token.
                token_type type() const { return m_type; }
                /// @brief Return the kind of the token.
                token_kind kind() const { return m_kind; }
                /// @brief Return the operator type of the token.
                operator_type op() const { return m_operator; }
            public:
                token_position start;
                token_position end;
            public:
                andy::lang::lexer::token& operator=(const andy::lang::lexer::token& other) = default;
            };
        protected:
            std::string_view m_file_name;
            std::string_view m_source;
            std::string_view m_current;
            std::string_view m_buffer;
            std::vector<andy::lang::lexer::token> m_tokens;

            token_position m_start;

            // iterating
            size_t iterator = 0;
        public:
            std::string_view path() const { return m_file_name; }
        protected:
            /// @brief Update a position (line, column, offset).
            /// @param position The position to update.
            void update_position(token_position& position, const char& token);
            /// @brief Update the start position (line, column, offset).
            /// @param token The token which should update the position.
            void update_start_position(const char& token);

            /// @brief Discard the first character from the m_current and update the start position.
            const char& discard();
            /// @brief Discard all whitespaces from the m_current.
            void discard_whitespaces();

            /// @brief Read the first character from the m_current, stores it in m_buffer and update the start position.
            void read();

            template<typename T>
            void discard_while(T&& condition) {
                while(m_current.size() && condition(m_current.front())) {
                    discard();
                }
            }

            template<typename T>
            void read_while(T&& condition) {
                while(m_current.size() && condition(m_current.front())) {
                    read();
                }
            }

            void push_token(token_position start, token_type type, token_kind kind = token_kind::token_null, operator_type op = operator_type::operator_max);
            void read_next_token();
            public:
                /// @brief Tokenize the source code. Equivalent to the constructor.
                /// @param __file_name The name of the file.
                /// @param __source The source code.
                void tokenize(std::string_view __file_name, std::string_view __source);
            public:
                void extract_and_push_string(token_position start);
        // iterating
        public:
            /// @brief Increment the iterator
            void consume_token();
            /// @brief Return the next token and increment the iterator.
            /// @return The next token.
            andy::lang::lexer::token& next_token();
            /// @brief Return the next token without incrementing the iterator.
            const andy::lang::lexer::token& see_next();
            /// @brief Decrement the iterator and return the next token.
            /// @return The previous token.
            const andy::lang::lexer::token& previous_token();
            /// @brief The current token.
            /// @return The current token.
            const andy::lang::lexer::token& current_token() const { return m_tokens[iterator-1]; }
            bool has_previous_token() const { return iterator > 0; }
            /// @brief Rollback the token iterator. The next call to next_token will return the same token.
            void rollback_token();
            /// @brief Check if there is a next token.
            bool has_next_token() const { return iterator < m_tokens.size(); }
            /// @brief Reset the iterator to 0.
            void reset() { iterator = 0; }
            /// @brief Erase a number of tokens starting from the current iterator.
            /// @param count The number of tokens to erase.
            void erase_tokens(size_t count);
            /// @brief Erase the EOF token.
            void erase_eof();
            /// @brief Insert new tokens at the current iterator and update it.
            /// @param tokens The tokens to insert.
            void insert(const std::vector<andy::lang::lexer::token>& tokens);
            /// @brief The tokens.
            /// @return The tokens.
            const std::vector<andy::lang::lexer::token>& tokens() const { return m_tokens; }
        protected:
        public:
            //extern std::vector<std::pair<std::string_view, andy::lang::lexer::cursor_type>> cursor_type_from_string_map;
        };
    };
}; // namespace andy