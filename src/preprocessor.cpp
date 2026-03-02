#include <filesystem>

#include <andy/lang/preprocessor.hpp>

#include <andy.hpp>

#include <andy/file.hpp>

// TODO: move to andy::file
std::vector<std::string> list_files_with_wildcard(const std::filesystem::path& base_path, std::string_view pattern) {
    std::vector<std::string> files;
    std::filesystem::path path = base_path;

    while(pattern.size() && !pattern.starts_with("*")) {
        size_t pos = pattern.find_first_of("/\\");
        if(pos == std::string::npos) {
            break;
        }
        std::string_view dir = pattern.substr(0, pos);
        path /= dir;
        pattern.remove_prefix(pos);
        while(pattern.size() && (pattern.front() == '/' || pattern.front() == '\\')) {
            pattern.remove_prefix(1);
        }
    }

    if(pattern.size()) {
        pattern.remove_prefix(1);
        if(pattern.size() && pattern.front() == '/') {
            pattern.remove_prefix(1);
            // Directory wildcard
            if(pattern.starts_with('*')) {
                pattern.remove_prefix(1);
                std::string_view match_end = pattern;
                for(auto& d : std::filesystem::directory_iterator(path)) {
                    if(d.is_directory()) {
                        for(auto& f : std::filesystem::directory_iterator(d.path())) {
                            if(f.is_regular_file()) {
                                std::string file_name = f.path().filename().string();
                                if(file_name.ends_with(pattern)) {
                                    files.push_back(f.path().string());
                                }
                            }
                        }
                    }
                }
            }
        } else {
            std::string_view match_end = pattern;
            if(std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
                for(auto& f : std::filesystem::directory_iterator(path)) {
                    if(f.is_regular_file()) {
                        std::string file_name = f.path().filename().string();
                        if(file_name.ends_with(pattern)) {
                            files.push_back(f.path().string());
                        }
                    }
                }
            }
        }
    }

    return files;
}

std::map<std::string, void(andy::lang::preprocessor::*)(const std::filesystem::path&, andy::lang::lexer&), std::less<>> preprocessor_directives = {
    { "#include", &andy::lang::preprocessor::process_include },
    { "#compile", &andy::lang::preprocessor::process_compile }
};

andy::lang::preprocessor::preprocessor()
{
}

andy::lang::preprocessor::~preprocessor()
{
}

void andy::lang::preprocessor::process(const std::filesystem::path &__file_name, andy::lang::lexer &__lexer)
{
    // Now we have a rule defined: The preprocessors must be at the beginning of the file.
    // The preprocessor will stop executing when it finds a token that is not a preprocessor (and is not a comment).

    andy::lang::lexer::token token = __lexer.next_token();

    while(!token.is_eof()) {
        switch(token.type) {
            case andy::lang::lexer::token_type::token_comment:
                // Do nothing
            break;
            case andy::lang::lexer::token_type::token_preprocessor: {
                if(auto it = preprocessor_directives.find(token.content); it != preprocessor_directives.end()) {
                    (this->*it->second)(__file_name, __lexer);
                } else {
                    throw std::runtime_error(token.error_message_at_current_position("unknown preprocessor directive"));
                }
            }
            break;
        }

        token = __lexer.next_token();
    }

    __lexer.reset();
}

void andy::lang::preprocessor::process_include(const std::filesystem::path &__file_name, andy::lang::lexer &__lexer)
{
    // Moves becase it will be removed
    andy::lang::lexer::token directive       = std::move(__lexer.current_token());
    andy::lang::lexer::token file_name_token = std::move(__lexer.see_next());

    if(file_name_token.type != lexer::token_type::token_literal || file_name_token.kind != lexer::token_kind::token_string) {
        throw std::runtime_error(file_name_token.error_message_at_current_position("Expected string literal after include directive"));
    }

    std::string file_path_string(file_name_token.content);

    std::filesystem::path file_path = __lexer.path();

    std::vector<std::string> files;
    // auto files = list_files_with_wildcard(file_path.parent_path(), file_path_string);
    std::vector<std::filesystem::path> include_paths;
#ifdef __ANDY_DEBUG__
    std::filesystem::path system_include_path = ANDYLANG_PROJECT_DIR;
#elif defined(__linux__)
    std::filesystem::path system_include_path = "/usr/local/include/andy";
#elif defined(_WIN32)
    std::filesystem::path system_include_path = "C:/Program Files (x86)/andy-lang/include/andy";
#else
    throw std::runtime_error("unsupported platform");

    std::filesystem::path system_include_path;
#endif

    include_paths.push_back(file_path.parent_path());
    include_paths.push_back(system_include_path);

    if(!file_path_string.ends_with(".andy")) {
        file_path_string += ".andy";
    }

    std::sort(include_paths.begin(), include_paths.end());
    include_paths.erase(std::unique(include_paths.begin(), include_paths.end()), include_paths.end());

    bool has_wildcard = file_path_string.find('*') != std::string::npos;

    for(const std::filesystem::path& include_path : include_paths) {
        if(!std::filesystem::exists(include_path)) {
            continue;
        }
        std::filesystem::path full_path = include_path / file_path_string;
        if(has_wildcard) {
            if(std::filesystem::exists(full_path) && std::filesystem::is_regular_file(full_path)) {
                files.push_back(full_path.string());
            }
        } else {
            auto wildcard_files = list_files_with_wildcard(include_path, file_path_string);
            files.insert(files.end(), wildcard_files.begin(), wildcard_files.end());
        }
    }

    if(!has_wildcard && files.empty()) {
        std::string error_message = "File '" + file_path_string + "' not found in include paths:\n";
        for(const auto& include_path : include_paths) {
            error_message += "  " + include_path.string() + "\n";
        }
        throw std::runtime_error(file_name_token.error_message_at_current_position(error_message));
    }

    std::sort(files.begin(), files.end());
    files.erase(std::unique(files.begin(), files.end()), files.end());

    __lexer.erase_tokens(2); // Remove the directive and the file name token

    // After this the iterator is at the position of the next token

    for(std::string& file : files) {
        if(__lexer.includes(file)) {
            continue;
        }
        std::string file_content = andy::file::read_all_text<char>(file);
        __lexer.include(file, std::move(file_content));
    }
}

void andy::lang::preprocessor::process_compile(const std::filesystem::path &__file_name, andy::lang::lexer &__lexer)
{
    // Moves becase it will be removed
    andy::lang::lexer::token directive       = std::move(__lexer.current_token());
    andy::lang::lexer::token file_name_token = std::move(__lexer.see_next());

    __lexer.erase_tokens(2); // Remove the directive and the file name token

    if(file_name_token.type != lexer::token_type::token_literal || file_name_token.kind != lexer::token_kind::token_string) {
        throw std::runtime_error(file_name_token.error_message_at_current_position("Expected string literal after compile directive"));
    }

    std::string_view file_path_string = file_name_token.content;

    std::filesystem::path file_path = __file_name.parent_path();

    file_path /= file_path_string;

    if(!std::filesystem::exists(file_path)) {
        throw std::runtime_error(file_name_token.error_message_at_current_position("Compile: folder '" + file_path.string() + "' not found"));
    }

    if(!std::filesystem::is_directory(file_path)) {
        throw std::runtime_error(file_name_token.error_message_at_current_position("Compile: file is not a directory"));
    }

    file_path /= "CMakeLists.txt";

    if(!std::filesystem::exists(file_path)) {
        throw std::runtime_error(file_name_token.error_message_at_current_position("Compile: Directory does not contain a CMakelists.txt file"));
    }

    std::filesystem::path current_path = std::filesystem::current_path();

    std::filesystem::current_path(file_path.parent_path());

    std::filesystem::path temp_file = std::filesystem::temp_directory_path() / "andy_temp_compile.txt";

    if(system(("cmake -B build . > " + temp_file.string()).c_str())) {
        throw std::runtime_error(file_name_token.error_message_at_current_position("Compile: CMake failed."));
    }

    if(system(("cmake --build build --config Debug > " + temp_file.string()).c_str())) {
        throw std::runtime_error(file_name_token.error_message_at_current_position("Compile: Build failed."));
    }

    std::filesystem::current_path(current_path);
}