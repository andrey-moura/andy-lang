#include <andy/lang/config.hpp>

namespace andy
{
    namespace lang
    {
        namespace config
        {
            std::filesystem::path src_dir()
            {
                std::filesystem::path src_dir;
#ifdef __UVA_DEBUG__
                return std::filesystem::absolute(ANDYLANG_PROJECT_DIR);
#endif
#ifdef __linux__
                src_dir = std::filesystem::path("/usr/local/src/andy-lang");
#elif defined(__wasm__)
                src_dir = std::filesystem::path("/");
#elif defined(_WIN32)
                src_dir = std::filesystem::path("C:/Program Files (x86)/andy-lang/src");
#else
                throw std::runtime_error("unsupported OS");
#endif
                return src_dir;
            }
        };
    };
};
