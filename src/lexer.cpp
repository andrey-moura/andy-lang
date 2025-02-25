#include <andy/lang/lexer.hpp>

#include <algorithm>

bool is_delimiter(const char& c) {
    static std::vector<bool> delimiters;
    if(delimiters.empty()) {
        delimiters.resize(256, false);

        delimiters['{'] = true;
        delimiters['}'] = true;
        delimiters['('] = true;
        delimiters[')'] = true;
        delimiters[';'] = true;
        delimiters[','] = true;
        delimiters[':'] = true;
    }

    return delimiters[(uint8_t)c];
}

bool is_operator(const char& c) {
    static std::vector<bool> operators;
    if(operators.empty()) {
        operators.resize(256, false);

        operators['+'] = true;
        operators['-'] = true;
        operators['*'] = true;
        operators['/'] = true;
        operators['%'] = true;
        operators['='] = true;
        operators['!'] = true;
        operators['?'] = true;
        operators['<'] = true;
        operators['>'] = true;
        operators['['] = true;
        operators[']'] = true;
        operators['|'] = true;
        operators['&'] = true;
        operators['.'] = true;
        operators['!'] = true;
    }

    return operators[(uint8_t)c];
}

bool is_keyword(const std::string& str) {
    static std::vector<std::string> keywords;

    if(keywords.empty()) {
        keywords = {
            "var",
            "function",
            "return",
            "class",
            "if",
            "for",
            "foreach",
            "new",
            "else",
            "while",
            "break"
        };

        std::sort(keywords.begin(), keywords.end());
    }

    return std::binary_search(keywords.begin(), keywords.end(), str);
}

bool is_preprocessor(std::string_view str) {
    if(str.starts_with('#')) {
        return true;
    }
    static std::vector<std::string> preprocessor;

    if(preprocessor.empty()) {
        preprocessor = {
            "#include",
            "#define",
            "#ifdef",
            "#endif",
        };

        std::sort(preprocessor.begin(), preprocessor.end());
    }

    return std::binary_search(preprocessor.begin(), preprocessor.end(), str);
}

andy::lang::lexer::operator_type operator_type_from_string(std::string_view str) {
    static std::map<std::string_view, andy::lang::lexer::operator_type> operators;

    if(operators.empty()) {
        operators = {
            { "+",  andy::lang::lexer::operator_type::operator_plus          },
            { "-",  andy::lang::lexer::operator_type::operator_minus         },
            { "*",  andy::lang::lexer::operator_type::operator_multiply      },
            { "/",  andy::lang::lexer::operator_type::operator_divide        },
            { "%",  andy::lang::lexer::operator_type::operator_modulo        },
            { "^",  andy::lang::lexer::operator_type::operator_power         },
            { "&&", andy::lang::lexer::operator_type::operator_and           },
            { "||", andy::lang::lexer::operator_type::operator_or            },
            { "!",  andy::lang::lexer::operator_type::operator_not           },
            { "==", andy::lang::lexer::operator_type::operator_equal         },
            { "!=", andy::lang::lexer::operator_type::operator_not_equal     },
            { "<",  andy::lang::lexer::operator_type::operator_less          },
            { "<=", andy::lang::lexer::operator_type::operator_less_equal    },
            { ">",  andy::lang::lexer::operator_type::operator_greater       },
            { ">=", andy::lang::lexer::operator_type::operator_greater_equal },
            { "++", andy::lang::lexer::operator_type::operator_increment     },
            { "--", andy::lang::lexer::operator_type::operator_decrement     },
        };
    }

    auto it = operators.find(str);

    if(it == operators.end()) {
        return andy::lang::lexer::operator_type::operator_null;
    }

    return it->second;
}

andy::lang::lexer::lexer(std::string __file_name, std::string_view __source)
{
    tokenize(std::move(__file_name), __source);
}

void andy::lang::lexer::update_start_position(const char &token)
{
    if(token == '\n') {
        m_start.line++;
        m_start.column = 0;
    } else {
        m_start.column++;
    }

    m_start.offset++;
}

const char &andy::lang::lexer::discard()
{
    const char& c = m_current.front();

    update_start_position(c);

    m_current.remove_prefix(1);

    return c;
}


void andy::lang::lexer::discard_whitespaces()
{
    discard_while([](const char& c) {
        return isspace(c);
    });
}

const char &andy::lang::lexer::read()
{
    // If it needs to read a character, and the buffer is empty, it is an error.
    if(m_current.empty()) {
        throw std::runtime_error("lexer: unexpected end of file");
    }
    
    const char& c = m_current.front();

    update_start_position(c);

    m_current.remove_prefix(1);

    m_buffer.push_back(c);

    return c;
}

void andy::lang::lexer::push_token(token_position start, token_type type, std::string content, token_kind kind, operator_type op)
{
    token t(start, m_start, std::move(content), type, kind, m_file_name, op);

    if(t.type() == token_type::token_literal) {
        switch(t.kind())
        {
        case token_kind::token_integer:
            t.m_literal.integer_value = std::stoi(t.content());
            break;
        case token_kind::token_float:
            t.m_literal.float_value = std::stof(t.content());
            break;
        case token_kind::token_double:
            t.m_literal.double_value = std::stod(t.content());
            break;
        case token_kind::token_boolean:
            t.m_literal.boolean_value = t.content() == "true";
            break;
        case token_kind::token_string:
        case token_kind::token_null:
            break;
        default:
            throw std::runtime_error("lexer: unknown token kind");
            break;
        }
    }

    m_tokens.emplace_back(std::move(t));
}

char unescape(const char& c)
{
    switch(c) {
        case '\\':
        case '"':
        case '\'':
            return c;
        case 'n':
            return '\n';
        case 't':
            return '\t';
        case 'r':
            return '\r';
        case 'b':
            return '\b';
        case 'a':
            return '\b';
        case 'f':
            return '\f';
        case 'v':
            return '\v';
        default:
            break;
    }

    throw std::runtime_error("lexer: cannot unescape '" + std::string(1, c) + "'");
}

void andy::lang::lexer::read_next_token()
{
    m_buffer.clear();

    // First, make sure we are at the beginning of the source code, not line breaks or spaces.
    discard_whitespaces();

    token_position start = m_start;

    if(m_current.empty()) {
        push_token(start, token_type::token_eof);
        return;
    }

    const char& c = m_current.front();

    // Ordered by less expensive checks first

    if(is_delimiter(c)) {
        if(c == ':' && m_current.size() >= 1) {
            if(isalpha(m_current[1])) {
                discard();
                while(m_current.size() && (isalnum(m_current.front()) || m_current.front() == '_')) { 
                    read();
                }
                push_token(start, token_type::token_literal, std::move(m_buffer), token_kind::token_string);
                return;
            }
        }
        read();
        push_token(start, token_type::token_delimiter, std::move(m_buffer));
        return;
    }

    // The comment starts with /, which is the division operator. So we need to check if it is a comment first.
    if(c == '/' && m_current.size() > 2 && m_current[1] == '/') {
        discard();
        discard();

        read_while([](const char& c) {
            return c != '\n';
        });

        push_token(start, token_type::token_comment, std::move(m_buffer));
        return;
    }

    if(isdigit(c) || (c == '-' && isdigit(m_current[1]))) {
        token_kind kind = token_kind::token_integer;
        // if a token starts with a digit or a minus sign followed by a digit, it is a number
        read_while([this](const char& c) {
            return isdigit(c) || (m_buffer.empty() && c == '-');
        });

        if(m_current.front() == '.') {
            read();
            read_while([](const char& c) {
                return isdigit(c);
            });

            kind = token_kind::token_float;
            
            if(m_current.front() == 'f') {
                discard();
                kind = token_kind::token_double;
            }
        }

        push_token(start, token_type::token_literal, std::move(m_buffer), kind);
        return;
    }

    if(is_operator(c)) {
        read();

        // If the next character is also an operator, it is a double operator.
        if(is_operator(m_current.front())) {
            read();
        }

        operator_type op = operator_type_from_string(m_buffer);

        push_token(start, token_type::token_operator, std::move(m_buffer), token_kind::token_null, op);
        return;
    }

    switch(c)
    {
        case '\"':
            discard();
            extract_and_push_string(start);
            return;
        break;
        case '\'':
            discard();

            while(m_current.front() != '\'') {
                if(m_current.empty()) {
                    throw std::runtime_error("lexer: unexpected end of file");
                }

                read();
            }

            discard();

            push_token(start, token_type::token_literal, std::move(m_buffer), token_kind::token_string);
            return;
        break;
    }

    if(is_preprocessor(m_current)) {
        read_while([](const char& c) {
            return !isspace(c);
        });

        push_token(start, token_type::token_preprocessor, std::move(m_buffer));
        return;
    }

    // It must be a identifier or a keyword
    read_while([](const char& c) {
        return isalnum(c) || c == '_';
    });

    if(m_buffer.empty()) {
        read();
        push_token(start, token_type::token_undefined);
        return;
    }

    // Todo: map
    if(m_buffer == "null") {
        push_token(start, token_type::token_literal, std::move(m_buffer), token_kind::token_null);
        return;
    } else if(m_buffer == "false") {
        push_token(start, token_type::token_literal, std::move(m_buffer), token_kind::token_boolean);
        return;
    } else if(m_buffer == "true") {
        push_token(start, token_type::token_literal, std::move(m_buffer), token_kind::token_boolean);
        return;
    }

    if(is_keyword(m_buffer)) {
        push_token(start, token_type::token_keyword, std::move(m_buffer));
        return;
    } else {
        push_token(start, token_type::token_identifier, std::move(m_buffer));
        return;
    }

    throw std::runtime_error("lexer: unknown token");
}

void andy::lang::lexer::tokenize(std::string __file_name, std::string_view __source)
{
    m_file_name = std::move(__file_name);
    m_current    = __source;
    m_source     = __source;

    do {
        read_next_token();
    } while(!m_tokens.back().is_eof());
}

void andy::lang::lexer::consume_token()
{
    if(!has_next_token()) {
        // If the parser is trying to get a token that does not exist, it is an error.
        throw std::runtime_error("unexpected end of file");
    }

    iterator++;
}

andy::lang::lexer::token &andy::lang::lexer::next_token()
{
    andy::lang::lexer::token& token = m_tokens[iterator];
    consume_token();
    return token;
}

const andy::lang::lexer::token &andy::lang::lexer::see_next()
{
    if(!has_next_token()) {
        throw std::runtime_error("unexpected end of file");
    }

    return m_tokens[iterator];
}

const andy::lang::lexer::token& andy::lang::lexer::previous_token()
{
    if(!has_previous_token()) {
        throw std::runtime_error("unexpected begin of file");
    }

    --iterator;

    return m_tokens[iterator - 1];
}

void andy::lang::lexer::rollback_token()
{
    if(iterator < 1) {
        throw std::runtime_error("unexpected begin of file");
    }

    iterator--;
}

void andy::lang::lexer::erase_tokens(size_t count)
{
    if(iterator + count > m_tokens.size()) {
        throw std::runtime_error("unexpected end of file");
    }

    m_tokens.erase(m_tokens.begin() + iterator - 1, m_tokens.begin() + iterator + count - 1);

    iterator--;
}

void andy::lang::lexer::erase_eof()
{
    if(m_tokens.back().is_eof()) {
        m_tokens.pop_back();
    }
}

void andy::lang::lexer::insert(const std::vector<andy::lang::lexer::token> &tokens)
{
    m_tokens.insert(m_tokens.begin() + iterator, tokens.begin(), tokens.end());
    iterator += tokens.size();
}

andy::lang::lexer::token::token(token_position start, token_position end, std::string content, token_type type, token_kind kind, std::string file_name, operator_type op)
    : start(start), end(end), m_content(std::move(content)), m_type(type), m_kind(kind), m_file_name(std::move(file_name)), m_operator(op)
{
}

andy::lang::lexer::token::token(token_position start, token_position end, std::string content, token_type type, token_kind kind)
    : start(start), end(end), m_content(std::move(content)), m_type(type), m_kind(kind)
{

}

andy::lang::lexer::token::token(token &&other)
    : start(other.start), end(other.end),
      m_content(std::move(other.m_content)),
      m_type(other.m_type), m_kind(other.m_kind),
      m_file_name(std::move(other.m_file_name)),
      m_operator(other.m_operator),
      m_literal(other.m_literal)
{
}

std::string andy::lang::lexer::token::error_message_at_current_position(std::string_view what) const
{
    std::string output(what);
    output += " at ";
    output += human_start_position();
    
    return output;
}

std::string andy::lang::lexer::token::unexpected_eof_message() const
{
    return error_message_at_current_position("unexpected end of file");
}

std::string_view andy::lang::lexer::token::human_start_position() const
{
    static std::string result;
    result.clear();

    result += m_file_name;
    result.push_back(':');
    result += std::to_string(start.line+1);
    result.push_back(':');
    result += std::to_string(start.column+1);

    return result;
}

void andy::lang::lexer::token::merge(const token &other)
{
    m_content += other.m_content;
    end = other.end;
}

void andy::lang::lexer::extract_and_push_string(token_position start)
{
    while(m_current.size()) {
        char ch = m_current.front();

        switch(ch)
        {
            case '\\':
                m_current.remove_prefix(1); // Remove the backslash
                ch = m_current.front();     // Save the escaped character
                m_current.remove_prefix(1); // Remove the escaped character

                m_start.offset += 2;        // The backslash and the escaped character
                m_start.column += 2;        // The backslash and the escaped character

                m_buffer.push_back(unescape(ch));
            break;
            case '\"':
                discard();
                push_token(start, token_type::token_literal, std::move(m_buffer), token_kind::token_string);
                return;
            break;
            case '$':
                if(m_current.size() > 1 && m_current[1] == '{') {
                    discard(); // Remove the dollar sign
                    discard(); // Remove the opening curly brace open

                    // Push the string before the variable or expression
                    push_token(start, token_type::token_literal, std::move(m_buffer), token_kind::token_string);

                    // We call the operator + to concatenate the string with the variable or expression 
                    push_token(start, token_type::token_operator, "+", token_kind::token_string);

                    // Read the variable or expression
                    while(m_current.size() && m_current.front() != '}') {
                        read_next_token();
                    }

                    if(m_current.size()) {
                        discard(); // Remove the closing curly brace
                    }
                    
                    // Check if the string is finished
                    if(m_current.size() && m_current.front() == '\"') {
                        discard(); // Remove the closing quote
                        return;
                    }

                    // We call the operator + to concatenate the string with the variable or expression 
                    push_token(m_start, token_type::token_operator, "+", token_kind::token_string);

                    // Read the continuation of the string after the variable or expression
                    extract_and_push_string(m_start);
                    return;
                }

                read();
            break;
            default:
                read();
                break;
        }
    }

    throw std::runtime_error("lexer: unexpected end of file");
}