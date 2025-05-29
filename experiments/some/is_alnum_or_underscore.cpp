#include <iostream>
#include <chrono>

int main() {
    size_t checksum[2] = {0, 0};
    size_t elapsed[2] = {0, 0};

    {
        auto start = std::chrono::high_resolution_clock::now();
        char c = 'a';
        for(int i = 0; i < 1000000000; ++i) {
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9') || (c == '_')) {
                checksum[0] += 1;
            }
            c++;
        }
        auto end = std::chrono::high_resolution_clock::now();
        elapsed[0] = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }

    const static uint64_t _is_alnum_or_underscore_lookup[] = {
        0,                                  // 0x00 - 0x07
        0,                                  // 0x08 - 0x0F
        0,                                  // ...
        0,                                  // 0x18 - 0x1F
        0,                                  // 0x20 - 0x27
        0,                                  // 0x28 - 0x2F
        0x03FF000000000000ULL,              // 0x30 - 0x3F  ('0'-'9' + '_')
        0x07FFFFFE00000000ULL,              // 0x40 - 0x47  ('A'-'O')
        0x00000001FFFFFFFFULL,              // 0x48 - 0x4F  ('P'-'W')
        0x0000000001FFFFFFULL,              // 0x50 - 0x57  ('X'-'Z')
        0,                                  // 0x58 - 0x5F
        0,                                  // 0x60 - 0x67
        0x07FFFFFE00000000ULL,              // 0x68 - 0x6F  ('a'-'o')
        0x00000001FFFFFFFFULL,              // 0x70 - 0x77  ('p'-'w')
        0x0000000001FFFFFFULL,              // 0x78 - 0x7F  ('x'-'z')
        0, 0, 0, 0, 0, 0, 0, 0,              // 0x80 - 0xBF
        0, 0, 0, 0, 0, 0, 0, 0               // 0xC0 - 0xFF
    };

    const static bool* is_alnum_or_underscore_lookup = (bool*)_is_alnum_or_underscore_lookup;

    {
        auto start = std::chrono::high_resolution_clock::now();
        char c = 'a';
        for(int i = 0; i < 1000000000; ++i) {
            if (is_alnum_or_underscore_lookup[(unsigned char)c]) {
                checksum[1] += 1;
            }
            c++;
        }
        auto end = std::chrono::high_resolution_clock::now();
        elapsed[1] = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }

    for(size_t i = 0; i < 2; ++i) {
        std::cout << "Method " << i + 1 << ": " << elapsed[i] << "ms" << std::endl;
        std::cout << "Checksum: " << checksum[i] << std::endl;
        std::cout << std::endl;
    }

    // Results:
    // Method 1 is slightly faster than method 2 in release mode.
    // Method 2 is faster in debug mode.
    // Method 1 is more readable.
}
