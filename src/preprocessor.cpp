#include <filesystem>

#include <andy/lang/preprocessor.hpp>

#include <uva.hpp>

#include <uva/file.hpp>

// TODO: move to uva::file

std::string wildcard_to_regex(const std::string& wildcard) {
    std::string regex_pattern = "^";
    for (char ch : wildcard) {
        switch (ch) {
            case '*':
                regex_pattern += ".*"; // '*' corresponde a qualquer sequência de caracteres
                break;
            case '?':
                regex_pattern += ".";  // '?' corresponde a um único caractere
                break;
            case '.':
                regex_pattern += "\\."; // Escape do ponto, pois em regex, '.' é um caractere especial
                break;
            default:
                regex_pattern += ch;    // Adiciona o caractere literal
                break;
        }
    }
    regex_pattern += "$"; // Final da expressão regular
    return regex_pattern;
}

// Função para listar arquivos com base em um wildcard
std::vector<std::string> list_files_with_wildcard(const std::filesystem::path& base_path, std::string pattern) {
    std::vector<std::string> files;
    pattern = "*/" + pattern; // Adiciona um coringa para buscar em subdiretórios
    std::regex regex_pattern(wildcard_to_regex(pattern));  // Converte o padrão para regex

    for (const auto& entry : std::filesystem::recursive_directory_iterator(base_path)) {
        if (std::filesystem::is_regular_file(entry.path())) {
            std::string filename = entry.path().string();
#ifdef __UVA_WIN__
            std::replace(filename.begin(), filename.end(), '\\', '/');
#endif
            // Verifica se o arquivo corresponde ao padrão
            if (std::regex_match(filename, regex_pattern)) {
                files.push_back(filename);
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
        switch(token.type()) {
            case andy::lang::lexer::token_type::token_comment:
                // Do nothing
            break;
            case andy::lang::lexer::token_type::token_preprocessor: {
                if(auto it = preprocessor_directives.find(token.content()); it != preprocessor_directives.end()) {
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

    if(file_name_token.type() != lexer::token_type::token_literal || file_name_token.kind() != lexer::token_kind::token_string) {
        throw std::runtime_error(file_name_token.error_message_at_current_position("Expected string literal after include directive"));
    }

    std::string_view file_path_string = file_name_token.content();

    std::filesystem::path file_path = __lexer.path();

    auto files = list_files_with_wildcard(file_path.parent_path(), std::string(file_path_string));

    __lexer.erase_tokens(2); // Remove the directive and the file name token

    // After this the iterator is at the position of the next token
    
    for(std::string& file : files) {
        std::string file_content = uva::file::read_all_text<char>(file);
        andy::lang::lexer l(file, file_content);

        process(file, l);

        l.erase_eof();

        __lexer.insert(l.tokens());
        __lexer.include(std::move(file), std::move(file_content));
    }
}

void andy::lang::preprocessor::process_compile(const std::filesystem::path &__file_name, andy::lang::lexer &__lexer)
{
    // Moves becase it will be removed
    andy::lang::lexer::token directive       = std::move(__lexer.current_token());
    andy::lang::lexer::token file_name_token = std::move(__lexer.see_next());

    __lexer.erase_tokens(2); // Remove the directive and the file name token

    if(file_name_token.type() != lexer::token_type::token_literal || file_name_token.kind() != lexer::token_kind::token_string) {
        throw std::runtime_error(file_name_token.error_message_at_current_position("Expected string literal after compile directive"));
    }

    std::string_view file_path_string = file_name_token.content();

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