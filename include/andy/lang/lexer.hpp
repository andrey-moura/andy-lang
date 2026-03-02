#pragma once

#include <vector>
#include <map>
#include <string>
#include <string_view>
#include <memory>

namespace andy
{
    namespace lang
    {
        class lexer
        {
        public:
            lexer() = default;
            lexer(std::string __file_name, std::string __source);
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
                token_interpolated_string
            };
            // Cannot use token_delimiter as the name of the enum because it will conflict with the token_delimiter type in the token class.
            enum token_delimiter_type {
                delimiter_comma,
                delimiter_end
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
            public:
                token() 
                    : type(token_type::token_undefined), kind(token_kind::token_null)
                    {

                    }
                ~token() = default;
            public:
                bool is_eof() const { return type == token_type::token_eof; }
            public:
                std::string error_message_at_current_position(std::string_view what) const;
                std::string unexpected_eof_message() const;
                std::string human_start_position() const;

                void merge(const token& other);
            public:
                /// @brief Return the human type of the token.
                std::string_view human_type() const;
            public:
                std::shared_ptr<std::string> file_name;

                token_position start;
                token_position end;
                struct {
                    union {
                        int integer_literal;
                        double double_literal;
                        float float_literal;
                        bool boolean_literal;
                    };
                };
                std::string string_literal;
                std::string   content;
                token_type    type;
                operator_type op_type;
                token_delimiter_type delimiter;
                token_kind kind;
                size_t index;
            public:
                andy::lang::lexer::token& operator=(const andy::lang::lexer::token& other) = default;
            };
        protected:
            std::string m_source;
            std::shared_ptr<std::string> m_file_name;

            std::vector<std::string> m_includes;            
            std::vector<andy::lang::lexer::token> m_tokens;
            
            // iterating
            std::string_view m_current;
            std::string_view m_buffer;
            token_position m_start;
            token_position m_end;

            size_t iterator = 0;
        public:
            const std::string& path() const { return *m_file_name; }
            void include(std::string __file_name, std::string __source);
            /// @brief Check if the file is included.
            /// @param file_name The name of the file.
            /// @return True if the file is included, false otherwise.
            bool includes(const std::string& file_name);

            /// @brief Discard the first character from the m_current and update the start position.
            const char& discard();
        protected:
            /// @brief Update the start position (line, column, offset).
            /// @param token The token which should update the position.
            void update_start_position(const char& c);
            /// @brief Update the end position (line, column, offset).
            /// @param token The token which should update the position.
            void update_end_position(const char& c);
            /// @brief Discard all whitespaces from the m_current.
            void discard_whitespaces();

            /// @brief Read the specifed number of characters from the m_current, stores it in m_buffer and update the start position.
            /// @param c The number of characters to read.
            void read(size_t c = 1);

            template<typename T>
            void discard_while(T&& condition) {
                while(m_current.size() && condition(m_current.front())) {
                    discard();
                }
            }

            template<typename T>
            void read_while(T&& condition) {
                while(condition(m_current.front())) {
                    read();
                    if(m_current.empty()) {
                       break;
                    }
                }
            }

            void push_token(token_type type, token_kind kind = token_kind::token_null, operator_type op = operator_type::operator_max);
            void push_delimiter(token_delimiter_type delimiter);
            void read_next_token();
            public:
                /// @brief Tokenize the source code. Equivalent to the constructor.
                /// @param __file_name The name of the file.
                /// @param __source The source code.
                void tokenize();
            public:
                void extract_and_push_string(bool is_interpolated = false);
        // iterating
        public:
            /// @brief Increment the iterator
            void consume_token();
            /// @brief Return the next token and increment the iterator.
            /// @return The next token.
            andy::lang::lexer::token& next_token();
            /// @brief Return the next token without incrementing the iterator.
            const andy::lang::lexer::token& see_next(int offset = 0) const;
            /// @brief Return the previous token without incrementing the iterator.
            /// @param offset The offset from the current iterator.
            /// @return The previous token.
            const andy::lang::lexer::token& see_previous(int offset = 0) const;
            /// @brief Decrement the iterator and return the next token.
            /// @return The previous token.
            const andy::lang::lexer::token& previous_token();
            /// @brief The current token.
            /// @return The current token.
            const andy::lang::lexer::token& current_token() const { return m_tokens[iterator-1]; }
            bool has_previous_token(int offset = 0) const { return iterator > offset; }
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
            /// @brief The tokens.
            /// @return The tokens.
            const std::vector<andy::lang::lexer::token>& tokens() const { return m_tokens; }
        protected:
        public:
            //extern std::vector<std::pair<std::string_view, andy::lang::lexer::cursor_type>> cursor_type_from_string_map;
        };
    };
}; // namespace andy