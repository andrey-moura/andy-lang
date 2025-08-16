#include <iostream>
#include <filesystem>
#include <fstream>
#include <cstdint>

#include <andy/console.hpp>

#ifdef __UVA_DEBUG__
    #define try if(true)
    #define catch(e) if(false)

    std::exception e;
#endif

#ifdef __linux__
    // 2MiB of virtual memory
    char virtual_memory_heap_buffer[2*1024*1024];
#elif defined(__WINDOWS__)
    // 256KiB of virtual memory
    char virtual_memory_heap_buffer[256*1024];
#else
    // This will fit in most of other systems
    // 1KiB of virtual memory
    char virtual_memory_heap_buffer[1*1024];
#endif

class virtual_memory {
    // TODO: Allocate the memory in the heap when we go out of the stack.
    // This will not happen too often, so we can use the stack for now.

private:
    char* stack_ptr = virtual_memory_heap_buffer;
    size_t stack_size = sizeof(virtual_memory_heap_buffer);
    size_t available_stack_size = sizeof(virtual_memory_heap_buffer);
public:
    template<typename T>
    void push_back(const T& c) {
        const size_t size_of_t = sizeof(T);
        if(available_stack_size < size_of_t) {
            throw std::runtime_error("out of virtual memory");
        }
        available_stack_size -= size_of_t;
        (T*)stack_ptr = c;
        stack_ptr += size_of_t;
    };
};

static bool is_alpha(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static bool is_alnum_or_underscore(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || (c == '_');
}

int main(int argc, char** argv) {
    try {
        std::filesystem::path andy_executable_path = argv[0];
        //vm_instance = std::make_shared<andy::lang::vm>();

        std::filesystem::path file_path;
        if(file_path.empty()) {
            if(argc > 1) {
                file_path = std::filesystem::absolute(argv[1]);
            } else {
                file_path = std::filesystem::absolute("application.andy");
            }

            if(!std::filesystem::exists(file_path)) {
                throw std::runtime_error("input file does not exist");
            }

            if(!std::filesystem::is_regular_file(file_path)) {
                throw std::runtime_error("input file is not a regular file");
            }
        }

        std::ifstream file(file_path, std::ios::binary);
        if(!file.is_open()) {
            throw std::runtime_error("could not open file " + file_path.string());
        }

        // Step 1
        // Lexical analysis

        std::string buffer;
        file.seekg(0, std::ios::end);
        size_t file_size = file.tellg();
        file.seekg(0, std::ios::beg);
        buffer.resize(file_size);
        file.read(buffer.data(), file_size);
        if(!file) {
            throw std::runtime_error("could not read file " + file_path.string());
        }

        std::string_view buffer_view(buffer.data(), file_size);

        size_t line_number = 1;
        size_t column_number = 0;

        auto discard_white_spaces = [&buffer_view, &line_number, &column_number]() {
            if(buffer_view.empty()) {
                return;
            }
            while(buffer_view.size() && isspace(buffer_view[0])) {
                char c = buffer_view[0];
                buffer_view.remove_prefix(1);
                ++column_number;
                if(c == '\n') {
                    ++line_number;
                    column_number = 0;
                }
            }
        };

        // std::string_view read_literal = [&buffer_view, &line_number, &column_number]() {
            
        // };

        enum token_type {
            token_identifier,
            token_literal
        };

        std::vector<std::string_view> token_types = {
            "token_identifier",
            "token_literal"
        };

        struct token {
            std::string_view file_path;
            size_t line_number;
            size_t column_number;
            std::string_view content;
            token_type type;
            void throw_error_at_current_position(std::string_view what) const {
                std::string error_msg;
                error_msg += what;
                error_msg += " at ";
                error_msg += file_path;
                error_msg.push_back(':');
                error_msg += std::to_string(line_number);
                error_msg += ':';
                error_msg += std::to_string(column_number);
                throw std::runtime_error(error_msg);
            };
            void throw_unexpected_end_of_file() const {
                throw_error_at_current_position("unexpected end of file");
            };
        };

        enum operation : uint8_t {
            op_call,
        };

        struct opcode {
            struct {
                operation op;
                
            };
        };

        size_t token_index = 0;

        std::string file_path_str = file_path.string();
        
        auto extract_alnum_or_underscore = [&token_index, &buffer_view, &line_number, &column_number]() {
            token t;
            size_t len = 0;
            const char* start = buffer_view.data();
            while(!buffer_view.empty() && is_alnum_or_underscore(buffer_view[0])) {
                buffer_view.remove_prefix(1);
                ++column_number;
                ++len;
            }
            std::string_view current_token(start, len);
            std::cout << "Token: " << token_index << ": '" << current_token << "'" << std::endl;
            ++token_index;
            return t;
        };

        while(!buffer_view.empty()) {
            discard_white_spaces();
            if(buffer_view.empty()) {
                break;
            }
            token t {
                file_path_str,
                line_number,
                column_number
            };
            size_t len = 0;
            const char* start = buffer_view.data();
            // We are in a 'root' token
            while(!buffer_view.empty()) {
                char c = buffer_view[0];
                buffer_view.remove_prefix(1);
                ++column_number;
                if(c == '\n') {
                    ++line_number;
                    column_number = 0;
                }
                if(!is_alpha(c)) {
                    token_index++;
                    t.type = token_identifier;

                    // Finish reading the token
                    // Now we have a 'root' identifier token
                    // A root identifier token is always a function call
                    // Next tokens:
                    // A line break (meaning no parameters)
                    // Delimiter (meaning no parameters)
                    // Literal (starting a parameter list)
                    // Identifier (starting a parameter list)

                    size_t previous_line = line_number;

                    discard_white_spaces();

                    if(buffer_view.empty()) {
                        break;
                    }

                    if(previous_line == line_number) {
                        // Definitely a function call
                        // The next token can be a delimiter, a literal or an identifier

                        char c = buffer_view[0];

                        if(c == ':') {
                            // Definitely a symbol
                            buffer_view.remove_prefix(1);
                            if(buffer_view.empty()) {
                                t.throw_unexpected_end_of_file();
                            }
                            if(is_alpha(buffer_view[0])) {
                                t.type = token_literal;
                                ++len;
                                extract_alnum_or_underscore();
                                break;
                            } else {
                                t.throw_unexpected_end_of_file();
                            }
        
                        }
                    }
                    break;
                }
                len++;
            }
        }
    } catch (const std::exception& e) {
        andy::console::log_error(e.what());
        return 1;
    }

    return 0;
}