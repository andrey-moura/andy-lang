#include <andy/lang/lexer.hpp>

#include <algorithm>

// Permitted delimiters: (){};:,
const static bool* is_delimiter_lookup = (bool*) (uint64_t[]){ 0, 0, 0, 0, 0, 0x100000101, 0, 0x1010000, 0, 0, 0, 0, 0, 0, 0, 0x10001000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const static uint64_t is_operator_lookup[] = { 0, 0, 0, 0, 0x1010000000100, 0x101010001010000, 0, 0x101010100000000, 0, 0, 0, 0x10001000000, 0, 0, 0, 0x100000000, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
// Keep ordered as it is used in binary search
const static std::vector<std::string_view> keywords_lookup = {
    "break",
    "else",
    "fn",
    "for",
    "foreach",
    "if",
    "module",
    "new",
    "return",
    "static",
    "type",
    "var",
    "while",
    "within",
    "yield"
};
const static std::map<std::string_view, andy::lang::lexer::operator_type> string_to_operator_lookup = {
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

static size_t is_delimiter(std::string_view str)
{
    if(str.starts_with("end")) {
        return 3;
    }
    return (size_t)((bool*)is_delimiter_lookup)[(uint8_t)str.front()];
}

static bool is_operator(const char& c) {
    return ((bool*)is_operator_lookup)[(uint8_t)c];
}

static bool is_keyword(std::string_view str) {
    return std::binary_search(keywords_lookup.begin(), keywords_lookup.end(), str);
}

static bool is_preprocessor(std::string_view str) {
    if(str.starts_with('#')) {
        return true;
    }

    return false;
}

static andy::lang::lexer::operator_type to_operator(std::string_view str) {

    auto it = string_to_operator_lookup.find(str);

    if(it == string_to_operator_lookup.end()) {
        return andy::lang::lexer::operator_type::operator_null;
    }

    return it->second;
}

andy::lang::lexer::lexer(std::string_view __file_name, std::string_view __source)
{
    tokenize(__file_name, __source);
}

void andy::lang::lexer::include(std::string __file_name, std::string __source)
{
    m_includes[std::move(__file_name)] = std::move(__source);
}

std::string_view andy::lang::lexer::source(const andy::lang::lexer::token& token) const
{
    if(token.m_file_name == m_file_name) {
        return m_source;
    }

    auto it = m_includes.find(token.m_file_name);

    if(it != m_includes.end()) {
        return it->second;
    }

    throw std::runtime_error("lexer: cannot find source for token");
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

void andy::lang::lexer::read(size_t c)
{
    for(size_t i = 0; i < c; i++) {
        // If it needs to read a character, and the buffer is empty, it is an error.
        if(m_current.empty()) {
            throw std::runtime_error("lexer: unexpected end of file");
        }
        
        const char& c = m_current.front();

        if(m_buffer.empty()) {
            m_buffer = std::string_view(m_current.data(), 1);
        } else {
            m_buffer = std::string_view(m_buffer.data(), m_buffer.size() + 1);
        }

        update_start_position(c);

        m_current.remove_prefix(1);
    }
}

void andy::lang::lexer::push_token(token_position start, token_type type, token_kind kind, operator_type op)
{
    token t(start, m_start, m_buffer, type, kind, m_file_name, m_source, op);

    m_buffer = "";

    if(t.type() == token_type::token_literal) {
        switch(t.kind())
        {
        case token_kind::token_integer:
            t.integer_literal = atoi(t.content().data());
            break;
        case token_kind::token_float:
            t.float_literal = atof(t.content().data());
            break;
        case token_kind::token_double:
            t.double_literal = atof(t.content().data());
            break;
        case token_kind::token_boolean:
            t.boolean_literal = t.content() == "true";
            break;
        case token_kind::token_string:
        case token_kind::token_interpolated_string:
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
    m_buffer = "";

    // First, make sure we are at the beginning of the source code, not line breaks or spaces.
    discard_whitespaces();

    token_position start = m_start;

    if(m_current.empty()) {
        push_token(start, token_type::token_eof);
        return;
    }

    const char& c = m_current.front();

    // The comment starts with /, which is the division operator. So we need to check if it is a comment first.
    if(c == '/' && m_current.size() > 2 && m_current[1] == '/') {
        discard();
        discard();

        read_while([](const char& c) {
            return c != '\n';
        });

        push_token(start, token_type::token_comment);
        return;
    }

    size_t delimiter_size = is_delimiter(m_current);
    if(delimiter_size) {
        if(c == ':' && m_current.size() >= 1) {
            if(isalpha(m_current[1])) {
                discard();
                while(m_current.size() && (isalnum(m_current.front()) || m_current.front() == '_')) { 
                    read();
                }
                push_token(start, token_type::token_literal, token_kind::token_string);
                return;
            }
        }
        read(delimiter_size);
        push_token(start, token_type::token_delimiter);
        return;
    }

    if(isdigit(c) || (c == '-' && isdigit(m_current[1]))) {
        size_t index = m_current.data() - m_source.data();
        if(index > 0) {
            // Not beginning of the source code.
            
            // If the last immediate previous character is a digit, then we have a expression like
            // 1-1 which is not a negative number, but a subtraction.

            if(isdigit(m_source[index - 1])) {
                read();
                push_token(start, token_type::token_operator, token_kind::token_null, operator_type::operator_minus);
                
                // Let the next digit be read as a number
            }

        }
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

        push_token(start, token_type::token_literal, kind);
        return;
    }

    if(is_operator(c)) {
        read();

        // If the next character is also an operator, it is a double operator.
        if(m_current.size()) {
            // No operator can be double with a dot
            if(m_current.front() != '.') {
                if(is_operator(m_current.front())) {
                    read();
                }
            }
        }

        operator_type op = to_operator(m_buffer);

        push_token(start, token_type::token_operator, token_kind::token_null, op);
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

            push_token(start, token_type::token_literal, token_kind::token_string);
            return;
        break;
    }

    if(is_preprocessor(m_current)) {
        read_while([](const char& c) {
            return !isspace(c);
        });

        push_token(start, token_type::token_preprocessor);
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

    if(m_current.size() > 1 && (m_current.front() == '?' || m_current.front() == '!')) {
        // It is identifier ended with ? or !
        read();
        push_token(start, token_type::token_identifier);
        return;
    }

    // Todo: map
    if(m_buffer == "null") {
        push_token(start, token_type::token_literal, token_kind::token_null);
        return;
    } else if(m_buffer == "false") {
        push_token(start, token_type::token_literal, token_kind::token_boolean);
        return;
    } else if(m_buffer == "true") {
        push_token(start, token_type::token_literal, token_kind::token_boolean);
        return;
    }

    if(is_keyword(m_buffer)) {
        push_token(start, token_type::token_keyword);
        return;
    } else {
        push_token(start, token_type::token_identifier);
        return;
    }

    throw std::runtime_error("lexer: unknown token");
}

void andy::lang::lexer::tokenize(std::string_view __file_name, std::string_view __source)
{
    m_file_name  = __file_name;
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

const andy::lang::lexer::token &andy::lang::lexer::see_next(int offset) const
{
    if(iterator + offset >= m_tokens.size()) {
        throw std::runtime_error("unexpected end of file");
    }

    return m_tokens[iterator + offset];
}

const andy::lang::lexer::token& andy::lang::lexer::previous_token()
{
    if(!has_previous_token()) {
        throw std::runtime_error("unexpected begin of file");
    }

    --iterator;

    return m_tokens[iterator - 1];
}

const andy::lang::lexer::token& andy::lang::lexer::see_previous(int offset) const
{
    if(iterator - offset < 1) {
        throw std::runtime_error("unexpected begin of file");
    }

    return m_tokens[iterator - offset - 1];
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

andy::lang::lexer::token::token(token_position start, token_position end, std::string_view content, token_type type, token_kind kind, std::string_view file_name, std::string_view source, operator_type op)
    : start(start), end(end), m_content(content), m_type(type), m_kind(kind), m_file_name(file_name), m_operator(op)
{
}

andy::lang::lexer::token::token(token_position start, token_position end, std::string_view content, token_type type, token_kind kind)
    : start(start), end(end), m_content(content), m_type(type), m_kind(kind)
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
    if(string_literal.empty()) {
        string_literal = m_content;
    }
    string_literal += other.m_content;

    end = other.end;
}

std::string_view andy::lang::lexer::token::content() const
{
    if(string_literal.size()) {
        return string_literal;
    }

    return m_content;
}

std::string_view andy::lang::lexer::token::human_type() const
{
    static std::vector<std::string_view> types = {
        "undefined",
        "comment",
        "keyword",
        "identifier",
        "literal",
        "delimiter",
        "operator",
        "preprocessor",
        "eof",
    };

    return types[(int)m_type];
}

void andy::lang::lexer::extract_and_push_string(token_position start)
{
    std::string output;
    while(m_current.size()) {
        char ch = m_current.front();

        switch(ch)
        {
            case '\\':
                read(); // Remove the backslash
                ch = m_current.front();     // Save the escaped character
                read(); // Remove the escaped character
                output.push_back(unescape(ch));
            break;
            case '\"':
                discard();
                push_token(start, token_type::token_literal, token_kind::token_string);
                m_tokens.back().string_literal = std::move(output);
                return;
            break;
            case '$':
                if(m_current.size() > 1 && m_current[1] == '{') {
                    discard(); // Remove the dollar sign
                    discard(); // Remove the opening curly brace open

                    // Push the string before the variable or expression
                    push_token(start, token_type::token_literal, token_kind::token_interpolated_string);
                    m_tokens.back().string_literal = std::move(output);

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

                    // Read the continuation of the string after the variable or expression
                    extract_and_push_string(m_start);

                    // So the parser knows where the string ends
                    push_token(start, token_type::token_delimiter);
                    return;
                }

                read();
            break;
            default:
                output.push_back(ch);
                read();
                break;
        }
    }

    throw std::runtime_error("lexer: unexpected end of file");
}