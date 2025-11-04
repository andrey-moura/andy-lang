#include <iostream>
#include <string>
#include <string_view>
#include <filesystem>
#include <fstream>

#include <andy/console.hpp>
#include <andy/file.hpp>

#include <andy/lang/interpreter.hpp>
#include <andy/lang/lexer.hpp>
#include <andy/lang/class.hpp>

#include <clang-c/Index.h>  // This is libclang.

bool debug = false;
bool extension = false;
std::string_view folder_arg;

std::map<std::string, std::string> results;
std::vector<std::string> all_classes;
std::vector<andy::lang::structure> classes;

std::string to_string(CXString str)
{
    std::string s = clang_getCString(str);
    clang_disposeString(str);
    return s;
}

std::ostream& operator<<(std::ostream& stream, const CXString& str)
{
    stream << clang_getCString(str);
    clang_disposeString(str);
    return stream;
}

void display_usage(std::string_view argv0)
{
    std::cout << "Usage: " << argv0 << " <input-file>" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << std::endl;
    andy::console::print_warning("--debug, -d");
    std::cout << "         Enable debug mode" << std::endl;
    andy::console::print_warning("--incremental, -i");
    std::cout << "   Only generate code if the original file changed" << std::endl;
    andy::console::print_warning("--file, -f <folder>");
    std::cout << " Output the generated code to a file in folder" << std::endl;
    andy::console::print_warning("--help");
    std::cout << "              Display this information" << std::endl;
}

int create_extension() {
    std::ofstream output_file;
    std::filesystem::path output_file_path = std::filesystem::absolute(folder_arg);
    output_file_path /= "extension.cpp";

    output_file.open(output_file_path);

    output_file << "#include <andy/lang/lang.hpp>" << std::endl;
    output_file << "#include <andy/lang/interpreter.hpp>" << std::endl;
    output_file << "#include <andy/lang/extension.hpp>" << std::endl;
    output_file << std::endl;
    for(const std::string_view& cls : all_classes) {
        std::string snake_case_name;
        for(const char& c : cls) {
            if(std::isupper(c)) {
                if(snake_case_name.size()) {
                    snake_case_name.push_back('_');
                }
                snake_case_name.push_back(std::tolower(c));
            } else {
                snake_case_name.push_back(c);
            }
        }
        output_file << "extern void create_" << snake_case_name << "_class(andy::lang::interpreter* interpreter);" << std::endl;
    }
    output_file << std::endl;
    output_file << "class AndyLangExtension : public andy::lang::extension {" << std::endl;
    output_file << "public:" << std::endl;
    output_file << "    AndyLangExtension() : andy::lang::extension(\"andy-lang-extension\") {}" << std::endl;
    output_file << "public:" << std::endl;
    output_file << "    void load(andy::lang::interpreter* interpreter) override {" << std::endl;

    for(const std::string_view& cls : all_classes) {
        std::string snake_case_name;
        for(const char& c : cls) {
            if(std::isupper(c)) {
                if(snake_case_name.size()) {
                    snake_case_name.push_back('_');
                }
                snake_case_name.push_back(std::tolower(c));
            } else {
                snake_case_name.push_back(c);
            }
        }
        output_file << "        create_" << snake_case_name << "_class(interpreter);" << std::endl;
    }

    output_file << "    }" << std::endl;
    output_file << "};" << std::endl;
    output_file << std::endl;
    output_file << "UVA_EXTENSION(" << "AndyLangExtension" << ")" << std::endl;
    output_file.flush();

    return 0;
}

int main(int argc, char* argv[]) {

    for(size_t i = 0; i < argc; i++) {
        //std::cout << "Arg " << i << ": " << argv[i] << std::endl;
    }

    if(argc < 2) {
        display_usage(argv[0]);
        return 1;
    }

    for(size_t i = 1; i < argc; i++) {
        std::string_view arg = argv[i];

        if(arg == "--debug" || arg == "-d") {
            debug = true;
        } else if(arg == "--folder" || arg == "-f") {
            if(i + 1 < argc) {
                folder_arg = argv[i + 1];
                ++i;
            } else {
                andy::console::log_error("Expected argument after --file flag");
                return 1;
            }
        } else if(arg == "--help") {
            display_usage(argv[0]);
            return 0;
        } else if(i > 1) {
            andy::console::log_error("Unknown argument: " + std::string(arg));
            return 1;
        }
    }

    if(extension) {
        return create_extension();
    }

    std::vector<std::string_view> input_files;

    std::string_view input_files_paths_str = argv[1];

    const char* start = input_files_paths_str.data();
    const char* end = input_files_paths_str.data();

    for(const char& c : input_files_paths_str) {
        if(c == ';') {
            std::string_view input_file_path(start, end - start);
            input_files.push_back(input_file_path);
            start = end + 1;
        }
        end++;
    }

    if(start != end) {
        std::string_view input_file_path(start, end - start);
        input_files.push_back(input_file_path);
    }

    bool is_any_file_changed = false;

    for(std::string_view input_file_path_str : input_files) {
        classes.clear();

        std::filesystem::path input_file_path = input_file_path_str;

        if(!std::filesystem::exists(input_file_path)) {
            andy::console::log_error("Input file does not exist");
            return 1;
        }

        if(!std::filesystem::is_regular_file(input_file_path)) {
            andy::console::log_error("Input file is not a regular file");
            return 1;
        }

        std::filesystem::path output_file_path;

        output_file_path = folder_arg;
        output_file_path /= (input_file_path.stem().string() + ".andy.cpp");

        bool output_file_exists = std::filesystem::exists(output_file_path);

        if(output_file_exists) {
            auto input_last_write = std::filesystem::last_write_time(input_file_path);
            auto output_last_write = std::filesystem::last_write_time(output_file_path);

            if(input_last_write < output_last_write) {
                continue;
            }
        }

        is_any_file_changed = true;

        CXIndex index = clang_createIndex(0, 0);

        CXTranslationUnit unit = clang_parseTranslationUnit(
            index,
            input_file_path.string().c_str(),
            nullptr,
            0,
            nullptr,
            0,
            CXTranslationUnit_None
        );

        if (unit == nullptr)
        {
            andy::console::log_error("Unable to parse translation unit. Quitting.");
            return 1;
        }

        CXCursor cursor = clang_getTranslationUnitCursor(unit);
        
        clang_visitChildren(cursor, [](CXCursor c, CXCursor parent, CXClientData client_data) {
            CXTranslationUnit unit = *(CXTranslationUnit*)client_data;
            CXTranslationUnit cursor_unit = clang_Cursor_getTranslationUnit(c);

            if(cursor_unit != unit) {
                return CXChildVisit_Continue;
            }

            if (clang_Location_isFromMainFile (clang_getCursorLocation(c)) == 0) {
                return CXChildVisit_Continue;
            }

            if(c.kind == CXCursorKind::CXCursor_ClassDecl) {
                std::string name = to_string(clang_getCursorSpelling(c));        
                if(clang_isCursorDefinition(c)) {
                    name[0] = std::toupper(name[0]);
                    andy::lang::structure cls(std::move(name));
                    all_classes.push_back(cls.name);

                    if(debug) {
                        std::cout << "Found class '" << cls.name << "'\n";
                    }

                    clang_visitChildren(c, [](CXCursor class_c, CXCursor paarent, CXClientData calient_data) {
                        //std::cout << class_c.kind << clang_getCursorKindSpelling(class_c.kind) << "\n";

                        if(class_c.kind == CXCursorKind::CXCursor_CXXMethod || class_c.kind == CXCursorKind::CXCursor_Constructor) {
                            andy::lang::structure* cls = (andy::lang::structure*)calient_data;

                            std::string name;
                            std::string return_type;

                            if(class_c.kind == CXCursorKind::CXCursor_Constructor) {
                                name = "new";
                            } else {
                                name = to_string(clang_getCursorSpelling(class_c));

                                CXSourceRange range = clang_getCursorExtent(class_c);
                                CXSourceLocation start = clang_getRangeStart(range);
                                CXSourceLocation end = clang_getRangeEnd(range);
    
                                CXFile file;
                                unsigned int line;
                                unsigned int column;
                                clang_getSpellingLocation(start, &file, &line, &column, nullptr);
    
                                CXTranslationUnit unit = clang_Cursor_getTranslationUnit(class_c);
                                CXToken *tokens;
                                unsigned int num_tokens;
                                CXSourceRange return_type_range = clang_getRange(start, end);
                                clang_tokenize(unit, return_type_range, &tokens, &num_tokens);
    
                                std::string everything_on_left;
    
                                for(unsigned int i = 0; i < num_tokens; i++) {
                                    CXToken token = tokens[i];
                                    // Check if is delimiter
                                    std::string token_str = to_string(clang_getTokenSpelling(unit, token));
                                    if(token_str == "(") {
                                        break;
                                    }
                                    if(everything_on_left.size() && token_str != "::" && !everything_on_left.ends_with("::")) {
                                        everything_on_left += " ";
                                    }
                                    everything_on_left += token_str;
                                }
    
                                // Remove function name
                                size_t pos = everything_on_left.find_last_of(" ");
                                if(pos != std::string::npos) {
                                    everything_on_left = everything_on_left.substr(0, pos);
                                }
    
                                std::cout << "'" << everything_on_left << "' ; ";
    
                                clang_disposeTokens(unit, tokens, num_tokens);

                                return_type = std::move(everything_on_left);
                            }

                            andy::lang::function m;
                            m.name = std::move(name);

                            if(return_type.size()) {
                                if(return_type != "void") {
                                    results[m.name] = return_type;
                                }
                            }
                            
                            if(debug) {
                                std::cout << "Found method '" << m.name << "'\n";
                            }

                            clang_visitChildren(class_c, [](CXCursor method_c, CXCursor parent, CXClientData client_data) {
                                switch(method_c.kind)
                                {
                                case CXCursorKind::CXCursor_ParmDecl:
                                    std::string name = to_string(clang_getCursorSpelling(method_c));
                                    if(debug) {
                                        std::cout << "Found parameter '" << name << "'\n";
                                    }
                                break;
                                }
                                return CXChildVisit_Continue;
                            }, nullptr);

                            if(m.storage_type == andy::lang::function_storage_type::instance_function) {
                                cls->functions[m.name] = std::make_shared<andy::lang::function>(std::move(m));
                            } else {
                                cls->functions[m.name] = std::make_shared<andy::lang::function>(std::move(m));
                            }
                        }

                        return CXChildVisit_Continue;
                    }, &cls);
                    
                    classes.push_back(std::move(cls));
                }
                
                return CXChildVisit_Continue;
            } else if(c.kind == CXCursorKind::CXCursor_FunctionDecl) {
                std::string name = to_string(clang_getCursorSpelling(c));
                if(debug) {
                    std::cout << "Found function '" << name << "'\n";
                }
            }

            return CXChildVisit_Continue;
        }, &unit);

        clang_disposeTranslationUnit(unit);
        clang_disposeIndex(index);

        std::ofstream output_file(output_file_path);

        output_file << "// This file was generated by andyc++" << std::endl;
        time_t now = time(0);
        struct tm tstruct;
        char buf[80];
        tstruct = *gmtime(&now);
        strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
        output_file << "// Generated at " << buf << " UTC" << std::endl;
        output_file << "// Changes to this file will be overwritten on next build" << std::endl;
        output_file << std::endl;
        output_file << "#include <andy/lang/api.hpp>" << std::endl;
        output_file << std::endl;
        output_file << "#include <" << input_file_path.filename().string() << ">" << std::endl;
        output_file << std::endl;


        for(auto& cls : classes) {
            std::string snake_case_name = cls.name;
            std::transform(snake_case_name.begin(), snake_case_name.end(), snake_case_name.begin(), [](unsigned char c) { return std::tolower(c); });

            output_file << "void create_" << snake_case_name << "_class(andy::lang::interpreter* interpreter)" << std::endl;
            output_file << "{" << std::endl;
            output_file << "\tauto " << snake_case_name << "_class = std::make_shared<andy::lang::structure>(" << "\"" << cls.name << "\"" << "); " << std::endl;

            if(cls.functions.size() > 0) {
                output_file  << std::endl;

                output_file  << "\t" << snake_case_name << "_class->methods = {" << std::endl;

                size_t method_iterator = 0;

                for(auto& [name, method] : cls.functions) {
                    if(method_iterator) {
                        output_file << "," << std::endl;
                    }
                    output_file << "\t\t{\"" << name << "\", andy::lang::function(\"" << name << "\", andy::lang::function_storage_type::instance_function, {}, [interpreter](std::shared_ptr<andy::lang::object> object, std::vector<std::shared_ptr<andy::lang::object>> params) {" << std::endl;

                    if(method.name == "new") {
                        output_file << "\t\t\tif constexpr(sizeof(" << snake_case_name << ") < andy::lang::max_native_size) {" << std::endl;
                        output_file << "\t\t\t\t" << snake_case_name << " native;" << std::endl;
                        output_file << "\t\t\t\t" << "object->set_native(std::move(native));" << std::endl;
                        output_file << "\t\t\t}" << std::endl;
                        output_file << "\t\t\telse {" << std::endl;
                        output_file << "\t\t\t\t" << snake_case_name << "* native = new " << snake_case_name << "();" << std::endl;
                        output_file << "\t\t\t\t" << "object->set_native(native);" << std::endl;
                        output_file << "\t\t\t}" << std::endl;
                        output_file << "\t\t\treturn nullptr;" << std::endl;
                    } else {

                        const std::string& return_type = results[method.name];
                        output_file << "\t\t\t";
                        if(return_type.size()) {
                            output_file << return_type << " result = ";
                        }
                        output_file << "object->as<" << snake_case_name << ">" << "()." << method.name << "();" << std::endl;
                        if(return_type.size()) {
                            output_file << "\t\t\treturn andy::lang::api::to_object(interpreter, result);" << std::endl;
                        }
                        else {
                            output_file << "\t\t\treturn nullptr;" << std::endl;
                        }
                    }

                    output_file << "\t\t})}";
                    ++method_iterator;
                }

                output_file << std::endl << "\t};" << std::endl << std::endl;

                output_file << "\tinterpreter->load(" << snake_case_name << "_class);" << std::endl;
            }

            output_file << "}" << std::endl;
            output_file.flush();
        }
    }

    if(is_any_file_changed) {
        create_extension();
    }

    return 0;
}