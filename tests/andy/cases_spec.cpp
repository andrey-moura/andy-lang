#include <andy/tests.hpp>

#include <filesystem>
#include <fstream>

using namespace andy::tests;

describe of("Running cases", []() {
  std::filesystem::path cases_path = std::filesystem::path(ANDYLANG_PROJECT_DIR) / "tests" / "andy" / "cases";
  std::filesystem::path tmp_file = std::filesystem::temp_directory_path() / "andy_tests_output.txt";

  std::function<void(const std::filesystem::path&)> recurse_directory;

  auto run = [&](const std::string& command) {
    std::string command_with_output = command + " > " + tmp_file.string() + " 2>&1";
    int result = system(command_with_output.c_str());
    return WEXITSTATUS(result);
  };

  auto cout = [&](const std::string& command) {
    int result = run(command);
    std::string output;
    std::ifstream file(tmp_file);
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    output.resize(size);
    file.read(output.data(), size);
    std::filesystem::remove(tmp_file);
    return output;
  };

  recurse_directory = [&](const std::filesystem::path& path) {
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
      const std::filesystem::path& file_path = entry.path();
      if (entry.is_directory()) {
        context(entry.path().filename().string(), [&]() {
          recurse_directory(entry.path());
        });
      } else {
        if(entry.path().extension() == ".andy") {
          std::string command = "./build/andy '" + file_path.string() + "'";
          std::string stem = file_path.stem().string();
          if(isdigit(stem[0])) {
            int ret = std::stoi(stem);
            it(stem + ".andy should return " + std::to_string(ret), [&]() {
              int result = run(command);
              expect(result).to<eq>(ret);
            });
          } else {
            std::string expected;
            std::filesystem::path expected_path = file_path;
            expected_path.replace_extension(".cout");
            std::ifstream file(expected_path);
            file.seekg(0, std::ios::end);
            size_t size = file.tellg();
            file.seekg(0, std::ios::beg);
            expected.resize(size);
            file.read(expected.data(), size);
            std::string expected_safe;
            expected_safe.reserve(expected.size()*2);
            for (size_t i = 0; i < expected.size(); i++) {
              if (expected[i] == '\n') {
                expected_safe += "\\n";
              } else if (expected[i] == '\r') {
                expected_safe += "\\r";
              } else if (expected[i] == '\t') {
                expected_safe += "\\t";
              } else {
                expected_safe += expected[i];
              }
            }
            it(stem + ".andy should print '" + expected_safe + "'", [&]() {
              std::string result = cout(command);
              expect(result).to<eq>(expected);
            });
          }
        }
      }
    }
  };

  recurse_directory(cases_path);
});