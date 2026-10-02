// panic.hpp
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <source_location>
#include <string>
#include <string_view>

#if defined(_WIN32)
// clang-format off
#  include <windows.h>
#  include <dbghelp.h>
#  pragma comment(lib, "dbghelp.lib")
// clang-format on
#else
#include <cxxabi.h>
#include <dlfcn.h>
#include <execinfo.h>
#endif

namespace serenkai {
namespace detail {

inline std::string demangle(const char* name) {
#if defined(__GNUG__)
    int status = 0;
    char* out = abi::__cxa_demangle(name, nullptr, nullptr, &status);
    if (status == 0 && out) {
        std::string s(out);
        std::free(out);
        return s;
    }
#endif
    return name;
}

inline void print_stacktrace(int skip = 2) {
#if defined(_WIN32)
    void* frames[64];
    USHORT n = CaptureStackBackTrace(skip, 64, frames, nullptr);

    HANDLE process = GetCurrentProcess();
    SymInitialize(process, nullptr, TRUE);

    char buf[sizeof(SYMBOL_INFO) + 256];
    auto* sym = reinterpret_cast<SYMBOL_INFO*>(buf);
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym->MaxNameLen = 255;

    for (USHORT i = 0; i < n; ++i) {
        DWORD64 addr = reinterpret_cast<DWORD64>(frames[i]);
        DWORD64 query_addr = (addr > 0) ? (addr - 1) : addr;
        DWORD64 disp = 0;
        if (SymFromAddr(process, query_addr, &disp, sym)) {
            std::fprintf(stderr, "  #%u %p %s +0x%llx\n", i, frames[i],
                         sym->Name, (unsigned long long)disp);
        } else {
            std::fprintf(stderr, "  #%u %p\n", i, frames[i]);
        }
    }
#else
    void* frames[64];
    int n = backtrace(frames, 64);

    for (int i = skip; i < n; ++i) {
        Dl_info info{};
        std::string sym = "???";
        auto uaddr = reinterpret_cast<uintptr_t>(frames[i]);
        void* query_addr =
            (uaddr > 0) ? reinterpret_cast<void*>(uaddr - 1) : frames[i];
        if (dladdr(query_addr, &info) && info.dli_sname) {
            sym = demangle(info.dli_sname);
        }
        std::fprintf(stderr, "  #%d %p %s\n", i - skip, frames[i], sym.c_str());
    }
#endif
}

} // namespace detail

[[noreturn]] inline void panic_at(std::string_view msg,
                                  std::source_location loc) {
    std::fprintf(stderr,
                 "panic: %.*s\n"
                 "  at %s:%u in %s\n"
                 "stacktrace:\n",
                 (int)msg.size(), msg.data(), loc.file_name(), loc.line(),
                 loc.function_name());
    detail::print_stacktrace(2);
    std::abort();
}

[[noreturn]] inline void
panic(std::string_view msg,
      std::source_location loc = std::source_location::current()) {
    panic_at(msg, loc);
}

} // namespace serenkai
