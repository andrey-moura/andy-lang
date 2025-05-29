#include <iostream>
#include <chrono>

int main() {
    size_t checksum[2] = {0, 0};
    size_t elapsed[2] = {0, 0};
    {
        auto start = std::chrono::high_resolution_clock::now();
        char c = 'a';
        for(int i = 0; i < 1000000000; ++i) {
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
                checksum[0] += 1;
            }
            c++;
        }
        auto end = std::chrono::high_resolution_clock::now();
        elapsed[0] = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }
    const static uint64_t _is_alpha_lookup[] = {
        0, // 0x00 - 0x07
        0, // 0x08 - 0x0F
        0, // 0x10 - 0x17
        0, // 0x18 - 0x1F
        0, // 0x20 - 0x27
        0, // 0x28 - 0x2F
        0, // 0x30 - 0x37
        0, // 0x38 - 0x3F
        0x0000000000000000ULL,              // 0x40 - 0x47
        0x0101010101010100ULL,              // 0x48 - 0x4F ('H'-'O')
        0x0101010101010101ULL,              // 0x50 - 0x57 ('P'-'W')
        0x0000000001010101ULL,              // 0x58 - 0x5F ('X'-'Z')
        0x0000000000000000ULL,              // 0x60 - 0x67
        0x0101010101010101ULL,              // 0x68 - 0x6F ('h'-'o')
        0x0101010101010101ULL,              // 0x70 - 0x77 ('p'-'w')
        0x0000000001010101ULL,              // 0x78 - 0x7F ('x'-'z')
        0, // 0x80 - 0x87
        0, // 0x88 - 0x8F
        0, // 0x90 - 0x97
        0, // 0x98 - 0x9F
        0, // 0xA0 - 0xA7
        0, // 0xA8 - 0xAF
        0, // 0xB0 - 0xB7
        0, // 0xB8 - 0xBF
        0, // 0xC0 - 0xC7
        0, // 0xC8 - 0xCF
        0, // 0xD0 - 0xD7
        0, // 0xD8 - 0xDF
        0, // 0xE0 - 0xE7
        0, // 0xE8 - 0xEF
        0, // 0xF0 - 0xF7
        0  // 0xF8 - 0xFF
    };
    const static bool* is_alpha_lookup = (bool*)_is_alpha_lookup;
    {
        auto start = std::chrono::high_resolution_clock::now();
        char c = 'a';
        for(int i = 0; i < 1000000000; ++i) {
            if (is_alpha_lookup[(unsigned char)c]) {
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
    // Method 2 is 0,01% faster, it does not justify the lookup table.
}