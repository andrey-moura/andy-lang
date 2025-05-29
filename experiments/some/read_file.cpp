#include <iostream>
#include <filesystem>
#include <chrono>
#include <vector>
#include <fstream>

int main(int argc, char** argv) {
    std::filesystem::path folder = argv[1];
    std::vector<std::filesystem::path> files;
    for(auto& p : std::filesystem::recursive_directory_iterator(folder)) {
        if(p.is_regular_file()) {
            size_t file_size = std::filesystem::file_size(p.path());
            // Maximum file size is 5KB
            if(file_size > 1024*1*1024) {
                continue;
            }
            files.push_back(p.path());
        }
    }
    size_t durations[4];
    size_t checksums[4] = {0, 0};
    // Why the buffer is kept?
    // Simply because the file will be loaded into vm memory, no allocation is needed.
    {
        auto start = std::chrono::high_resolution_clock::now();
        for(const auto& file_path : files) {
            std::unique_ptr<FILE, decltype(&fclose)> file(fopen(file_path.string().c_str(), "r"), fclose);
            if(!file) {
                throw std::runtime_error("could not open file " + file_path.string());
            }
            // Beliave it or not, this is faster than reading the file in one go.
            // Why 32? Increasingg the size makes it faster, untill 
            fseek(file.get(), 0, SEEK_END);
            int size = ftell(file.get());
            fseek(file.get(), 0, SEEK_SET);
            char temp[8];
            bool should_copy = false;
            size_t buffer_ptr = 0;
            while(size >= 8) {
                fread(temp, 1, 8, file.get());
                for(int i = 0; i < 8; ++i) {
                    char c = temp[i];
                    checksums[0] += c;
                }
                size -= 8;
            }
            if(size > 0) {
                fread(temp, 1, size, file.get());
                for(int i = 0; i < size; ++i) {
                    char c = temp[i];
                    checksums[0] += c;
                }
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        durations[0] = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }

    {
        std::string buffer;
        auto start = std::chrono::high_resolution_clock::now();
        for(const auto& file_path : files) {
            std::unique_ptr<FILE, decltype(&fclose)> file(fopen(file_path.string().c_str(), "r"), fclose);
            if(!file) {
                throw std::runtime_error("could not open file " + file_path.string());
            }
            fseek(file.get(), 0, SEEK_END);
            size_t size = ftell(file.get());
            buffer.resize(size);
            fseek(file.get(), 0, SEEK_SET);
            fread(buffer.data(), 1, size, file.get());
            for(size_t i = 0; i < size; ++i) {
                char c = buffer[i];
                checksums[1] += c;
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        durations[1] = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }

    for(int i = 0; i < 4; ++i) {
        if(i) {
            std::cout << std::endl;
        }
        std::cout << "Method " << i + 1 << ":" << std::endl;
        std::cout << "Files: " << files.size() << std::endl;
        std::cout << "Duration: " << durations[i] << "ms" << std::endl;
        std::cout << "Checksum: " << checksums[i] << std::endl;
    }

    // Winner is method 3
}