/*
 * Add this code to the file that has main(), then add a call to
 * printsymbols() as the first line in main(). This will result in the
 * program printing a list of all the symbols and their addresses suitable
 * for reading by dis.cc, which will let us make a full disassembly of the
 * program DLLs and all
 *
 * There's definitely a better way to do this. This is a hack for now.
 *
 * This code snippet was written by ChatGPT.
 */
#include <iostream>
#include <vector>
#include <link.h>
#include <dlfcn.h>
#include <elf.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

void list_symbols_from_library(const char* libname) {
    int fd = open(libname, O_RDONLY);
    if (fd < 0) {
        std::cerr << "Error opening " << libname << std::endl;
        return;
    }

    Elf64_Ehdr ehdr;
    read(fd, &ehdr, sizeof(ehdr));
    if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
        std::cerr << libname << " is not a valid ELF file." << std::endl;
        close(fd);
        return;
    }

    lseek(fd, ehdr.e_shoff, SEEK_SET);
    std::vector<Elf64_Shdr> sections(ehdr.e_shnum);
    read(fd, sections.data(), ehdr.e_shnum * sizeof(Elf64_Shdr));

    Elf64_Shdr strtab, symtab;
    for (const auto& sec : sections) {
        if (sec.sh_type == SHT_DYNSYM) {
            symtab = sec;
        } else if (sec.sh_type == SHT_STRTAB && &sec != &sections[ehdr.e_shstrndx]) {
            strtab = sec;
        }
    }

    if (symtab.sh_size == 0 || strtab.sh_size == 0) {
        std::cerr << "Symbol table not found in " << libname << std::endl;
        close(fd);
        return;
    }

    lseek(fd, symtab.sh_offset, SEEK_SET);
    std::vector<Elf64_Sym> symbols(symtab.sh_size / sizeof(Elf64_Sym));
    read(fd, symbols.data(), symtab.sh_size);

    std::vector<char> strtab_data(strtab.sh_size);
    lseek(fd, strtab.sh_offset, SEEK_SET);
    read(fd, strtab_data.data(), strtab.sh_size);

    for (const auto& sym : symbols) {
        if (sym.st_name != 0 && ELF64_ST_TYPE(sym.st_info) == STT_FUNC) {
            void* handle = dlopen(libname, RTLD_NOW);
            if (!handle) {
                std::cerr << "Failed to open: " << libname << " Error: " << dlerror() << std::endl;
                continue;
            }
            void* sym_addr = dlsym(handle, &strtab_data[sym.st_name]);
            if (sym_addr) {
                std::cout << "  Symbol: " << &strtab_data[sym.st_name] 
                          << " at address: " << sym_addr << std::endl;
            }
            dlclose(handle);
        }
    }
    close(fd);
    fflush (stdout);
    printf ("end symbols\n");
}

int callback(struct dl_phdr_info *info, size_t size, void *data) {
    if (!info->dlpi_name || info->dlpi_name[0] == '\0') {
        return 0; // Skip empty entries
    }
    
    std::cout << "Library: " << info->dlpi_name << " Base Address: " << std::hex << info->dlpi_addr << std::dec << std::endl;
    list_symbols_from_library(info->dlpi_name);
    return 0;
}

void list_symbols() {
    dl_iterate_phdr(callback, nullptr);
}

int printsymbols() {
    std::cout << "Listing symbols for the running process:" << std::endl;
    list_symbols();
    return 0;
}
