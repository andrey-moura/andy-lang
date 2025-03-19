#ifndef ANDYLANG_API_H
#define ANDYLANG_API_H

typedef int andy_lang_object;

#ifdef __cplusplus
extern "C" {
#endif
/// @brief Initialize the andy lang api.
void andy_lang_api_init();
/// @brief Executes the code in a file and return the result.
/// @param path The path to the source code.
andy_lang_object andy_lang_api_evaluate_file(const char* path);
/// @brief Executes the code in a string and return the result.
/// @param source The source code.
andy_lang_object andy_lang_api_evaluate_string(const char* source);
/// @brief Create an object.
andy_lang_object andy_lang_api_create_object(void* obj = nullptr);
/// @brief Destroy an object.
/// @param object The object to destroy.
void andy_lang_api_destroy_object(andy_lang_object object);
/// @brief Destroy the andy lang api.
void andy_lang_api_destroy();
#ifdef __cplusplus
}
#endif

#endif // ANDYLANG_API_H
