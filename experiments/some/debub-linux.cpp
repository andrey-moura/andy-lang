#include <elf.h>
#include <elfutils/libdw.h>
#include <fcntl.h>
#include <gelf.h>
#include <libelf.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/uio.h>

#include <cxxabi.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>


#include "../include/andy/lang/interpreter.hpp"

// ======================================================
// Utils
// ======================================================

std::string demangle_name(const char* mangled) {
    if (!mangled) return {};
    int status = 0;
    char* demangled = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
    if (status == 0 && demangled) {
        std::string result(demangled);
        std::free(demangled);
        return result;
    }
    return mangled;
}

uint64_t get_base_address(pid_t pid, const std::string& executable) {
    std::ifstream maps("/proc/" + std::to_string(pid) + "/maps");
    std::string line;

    while (std::getline(maps, line)) {
        if (line.find(executable) == std::string::npos) continue;

        std::istringstream iss(line);
        std::string range;
        iss >> range;

        auto dash = range.find('-');
        return std::stoull(range.substr(0, dash), nullptr, 16);
    }

    throw std::runtime_error("failed to determine base address");
}

std::optional<uint64_t> find_symbol_address(const std::string& path, const std::string& symbol_name) {
    if (elf_version(EV_CURRENT) == EV_NONE)
        throw std::runtime_error("libelf init failed");

    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0)
        throw std::runtime_error("open failed");

    Elf* elf = elf_begin(fd, ELF_C_READ, nullptr);
    if (!elf)
        throw std::runtime_error("elf_begin failed");

    Elf_Scn* scn = nullptr;
    while ((scn = elf_nextscn(elf, scn))) {
        GElf_Shdr shdr;
        gelf_getshdr(scn, &shdr);

        if (shdr.sh_type != SHT_SYMTAB && shdr.sh_type != SHT_DYNSYM)
            continue;

        Elf_Data* data = elf_getdata(scn, nullptr);
        size_t count = shdr.sh_size / shdr.sh_entsize;

        for (size_t i = 0; i < count; ++i) {
            GElf_Sym sym;
            gelf_getsym(data, i, &sym);

            const char* name = elf_strptr(elf, shdr.sh_link, sym.st_name);
            if (!name) continue;

            std::string dem = demangle_name(name);

            if (symbol_name == name || symbol_name == dem) {
                uint64_t addr = sym.st_value;
                elf_end(elf);
                close(fd);
                return addr;
            }
        }
    }

    elf_end(elf);
    close(fd);
    return std::nullopt;
}

// ======================================================
// Remote call
// ======================================================

uint64_t call_remote_function(pid_t pid, uint64_t func_addr) {
    user_regs_struct regs{}, backup{};

    ptrace(PTRACE_GETREGS, pid, nullptr, &regs);
    backup = regs;

    uint64_t return_addr = regs.rip;

    // colocar breakpoint no retorno
    long orig_ret = ptrace(PTRACE_PEEKTEXT, pid, return_addr, nullptr);
    ptrace(PTRACE_POKETEXT, pid, return_addr, (orig_ret & ~0xFF) | 0xCC);

    // preparar stack com retorno válido
    regs.rsp -= 8;
    ptrace(PTRACE_POKEDATA, pid, regs.rsp, return_addr);

    // chamar função
    regs.rip = func_addr;

    ptrace(PTRACE_SETREGS, pid, nullptr, &regs);
    ptrace(PTRACE_CONT, pid, nullptr, nullptr);

    int status;
    waitpid(pid, &status, 0);

    // capturar retorno
    ptrace(PTRACE_GETREGS, pid, nullptr, &regs);
    uint64_t ret = regs.rax;

    // restaurar código original
    ptrace(PTRACE_POKETEXT, pid, return_addr, orig_ret);

    // restaurar registradores
    ptrace(PTRACE_SETREGS, pid, nullptr, &backup);

    return ret;
}

// ======================================================
// Read C string
// ======================================================

std::string read_cstring(pid_t pid, uint64_t addr) {
    std::string result;
    char c;

    int limit = PATH_MAX; // evitar loops infinitos

    result.reserve(limit);

    while (result.size() < limit) {
        iovec local{ &c, 1 };
        iovec remote{ (void*)addr, 1 };

        if (process_vm_readv(pid, &local, 1, &remote, 1, 0) != 1)
            break;

        if (c == '\0') break;

        result += c;
        addr++;
    }

    if(result.size() == limit) {
        throw std::runtime_error("string too long or not null-terminated");
    }

    return result;
}

uint64_t find_address_by_file_line(
    const std::string& executable,
    const std::string& target_file,
    int target_line
) {
    int fd = open(executable.c_str(), O_RDONLY);
    if (fd < 0) throw std::runtime_error("open failed");

    Dwarf* dbg = dwarf_begin(fd, DWARF_C_READ);
    if (!dbg) throw std::runtime_error("dwarf_begin failed");

    Dwarf_Off off = 0, next_off = 0;
    size_t hsize;

    while (dwarf_nextcu(dbg, off, &next_off, &hsize, nullptr, nullptr, nullptr) == 0) {

        Dwarf_Die cu_die;
        if (!dwarf_offdie(dbg, off + hsize, &cu_die)) {
            off = next_off;
            continue;
        }

        Dwarf_Lines* lines;
        size_t nlines;

        if (dwarf_getsrclines(&cu_die, &lines, &nlines) != 0) {
            off = next_off;
            continue;
        }

        for (size_t i = 0; i < nlines; ++i) {
            Dwarf_Line* line = dwarf_onesrcline(lines, i);

            int lineno;
            dwarf_lineno(line, &lineno);

            const char* src = dwarf_linesrc(line, nullptr, nullptr);

            if (!src) continue;

            if(!std::string_view(src).ends_with(target_file)) continue;

            if (lineno == target_line)
            {
                Dwarf_Addr addr;
                dwarf_lineaddr(line, &addr);

                dwarf_end(dbg);
                close(fd);
                return addr;
            }
        }

        off = next_off;
    }

    dwarf_end(dbg);
    close(fd);

    throw std::runtime_error("file:line not found");
}

// ======================================================
// MAIN
// ======================================================

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: <exe> <breakpoint_symbol>\n";
        return 1;
    }

    std::string executable = argv[1];
    std::string breakpoint_symbol = argv[2];
    std::string helper_symbol = "andy_interpreter_current_node_path";

    auto helper_sym = find_symbol_address(executable, helper_symbol);

    if (!helper_sym)
        throw std::runtime_error("symbol not found");

    pid_t child = fork();

    if (child == 0) {
        ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
        execl(executable.c_str(), executable.c_str(), nullptr);
        _exit(1);
    }

    int status;
    waitpid(child, &status, 0);

    uint64_t base = get_base_address(child, executable);

    auto bp_addr = find_address_by_file_line(executable, "andy-lang/src/interpreter.cpp", 942);
    bp_addr += base;

    uint64_t helper_addr = base + *helper_sym;

    long orig = ptrace(PTRACE_PEEKTEXT, child, bp_addr, nullptr);
    auto set_breakpoint = [&]() {
        ptrace(PTRACE_POKETEXT, child, bp_addr, (orig & ~0xFF) | 0xCC);
    };

    set_breakpoint();

    while (true) {
        ptrace(PTRACE_CONT, child, nullptr, nullptr);
        waitpid(child, &status, 0);

        // 🔴 processo terminou
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            break;
        }

        // 🔴 não é parada válida
        if (!WIFSTOPPED(status)) {
            continue;
        }

        int sig = WSTOPSIG(status);

        // 🔴 repassar sinais que não são SIGTRAP
        if (sig != SIGTRAP) {
            ptrace(PTRACE_CONT, child, nullptr, sig);
            continue;
        }

        user_regs_struct regs{};
        ptrace(PTRACE_GETREGS, child, nullptr, &regs);

    #if defined(__x86_64__)
        uint64_t rip = regs.rip - 1;
    #else
        uint64_t rip = regs.rip;
    #endif

        // 🔴 não é nosso breakpoint
        if (rip != bp_addr) {
            continue;
        }

        // corrigir RIP
        regs.rip = rip;
        ptrace(PTRACE_SETREGS, child, nullptr, &regs);

        // restaurar instrução original
        ptrace(PTRACE_POKETEXT, child, bp_addr, orig);

        // executar instrução original
        ptrace(PTRACE_SINGLESTEP, child, nullptr, nullptr);
        waitpid(child, &status, 0);

        // reinstalar breakpoint
        ptrace(PTRACE_POKETEXT, child, bp_addr, (orig & ~0xFF) | 0xCC);

        // 🔥 chamada remota
        uint64_t str_ptr = call_remote_function(child, helper_addr);

        std::string result = read_cstring(child, str_ptr);

        std::cout << "input_file_path: " << result << "\n";
    }

    return 0;
}