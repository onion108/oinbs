#pragma once

#if defined(__cplusplus)
#include <type_traits>
#include <string>
#include <source_location>
#include <filesystem>
#include <vector>
#include <unistd.h>
#include <sys/stat.h>
#include <format>
#include <stdexcept>
#include <cstring>
#include <exception>
#include <iostream>
#include <string_view>
#ifdef _WIN32
#error No windows support yet.
#else
#include <unistd.h>
#endif

#define OINBS_NAMESPACE_BEGIN namespace oinbs {
#define OINBS_NAMESPACE_END }

#define FEATURE_PKG_CONFIG_BEGIN namespace pkg_config {
#define FEATURE_PKG_CONFIG_END }


OINBS_NAMESPACE_BEGIN

inline void log(std::string_view level, std::string_view fmt, auto&&... args) {
    std::cerr << "[" << level << "] " << std::vformat(fmt, std::make_format_args(args...)) << "\n";
}

inline bool string_contains(std::string_view sv, char ch) {
    return sv.find(ch) != std::string_view::npos;
}

inline std::string render_command(const std::vector<std::string>& args) {
    std::string result;
    if (args.empty()) return "";
    result += args[0];
    bool drop = true;
    for (const auto& arg : args) {
        if (drop) {
            drop = false;
            continue;
        }
        if (string_contains(arg, ' ') || string_contains(arg, '\"' || string_contains(arg, '\''))) {
            result += " \"";
            for (auto ch : arg) {
                if (ch == '\"') {
                    result += "\\\"";
                } else if (ch == '\\') {
                    result += "\\\\";
                } else {
                    result += ch;
                }
            }
            result += "\"";
        } else {
            result += " " + arg;
        }
    }
    return result;
}

struct CommandOutput {
    int ret_code;
    std::string stdout_content;
    std::string stderr_content;
};

static std::vector<std::string> parse_flags(std::string_view env) {
    std::vector<std::string> result;
    std::string buffer;

    bool foundBackslash = false;
    bool isInQuote = false;
    char quoteChar = ' ';

    for (const char c : env) {
        if (foundBackslash) {
            buffer += c;
            foundBackslash = false;
        } else if (isInQuote) {
            if (c == '\\') {
                foundBackslash = true;
            } else if (c == quoteChar) {
                isInQuote = false;
            } else {
                buffer += c;
            }
        } else if (c == '\'' || c == '"') {
            isInQuote = true;
            quoteChar = c;
        } else if (c == '\\') {
            foundBackslash = true;
        } else if (std::isspace(c)) {
            if (!buffer.empty()) {
                result.push_back(buffer);
                buffer.clear();
            }
        } else {
            buffer += c;
        }
    }

    if (!buffer.empty()) {
        result.push_back(buffer);
    }

    return result;
}

static inline std::vector<std::string> get_env_flags(const char* name) {
    if (const char* env = std::getenv(name)) {
        return parse_flags(env);
    }
    return {};
}

// {{{ Platform specific thingy

// Load executable and replace current process.
inline void execute_nofork(char **argv) {
    if (execvp(argv[0], argv) == -1) {
        throw std::runtime_error("Failed to spawn process");
    }
}

// Execute command using parameter `argv`.
inline CommandOutput execute_command(const std::vector<std::string>& argv) {
    log("INFO", "Executing command: {}", render_command(argv));

    int pout[2], perr[2];
    if (pipe(pout)) throw std::runtime_error("Cannot create pipe for stdout");
    if (pipe(perr)) throw std::runtime_error("Cannot create pipe for stderr");
    auto child_pid = fork();
    if (child_pid == -1) throw std::runtime_error("Failed to fork child process, damn it! ");
    if (child_pid == 0) {
        dup2(pout[1], STDOUT_FILENO);
        dup2(perr[1], STDERR_FILENO);
        close(pout[0]); close(pout[1]);
        close(perr[0]); close(perr[1]);
        
        // Doing dirty C stuffs
        // A little memory leak here should be ok.
        char **c_argv = new char*[argv.size()+1];
        for (int i = 0; i < argv.size(); i++) {
            c_argv[i] = new char[argv[i].size()+1];
            std::strcpy(c_argv[i], argv[i].data());
        }
        c_argv[argv.size()] = 0;
        int res = execvp(c_argv[0], c_argv);
        if (res == -1) {
            std::cerr << "Failed to spawn child process" << std::endl;
            exit(1);
        }
        throw std::logic_error("unreachable!");
    } else {
        close(pout[1]);
        close(perr[1]);
        char buf[1024];
        std::size_t size;
        CommandOutput result;
        while ((size = read(pout[0], buf, 1024)) != 0) {
            for (std::size_t idx = 0; idx < size; idx++) {
                result.stdout_content.push_back(buf[idx]);
            }
        }
        while ((size = read(perr[0], buf, 1024)) != 0) {
            for (std::size_t idx = 0; idx < size; idx++) {
                result.stderr_content.push_back(buf[idx]);
            }
        }
        waitpid(child_pid, &result.ret_code, 0);
        return result;
    }
}

// }}}

// Checks if file a is newer than file b.
inline bool is_newer(std::string_view a, std::string_view b) {
    auto a_time = std::filesystem::last_write_time(a);
    auto b_time = std::filesystem::last_write_time(b);
    return a_time > b_time;
}

// {{{Raw compilation thingy

// Compile C source file `src` into artifact `dest`.
// Optional arguments includes `args` to pass extra flags to the compiler and `link_executable` that denotes whether the artifact is an executable.
inline void compile_c_source(std::string_view src, std::string_view dest, const std::vector<std::string>& args = {}, bool link_executable = true) {
    std::vector<std::string> compiler_args;
    if (auto env = std::getenv("CC")) {
        compiler_args.push_back(env);
    } else {
        compiler_args.push_back("cc");
    }

    if (!link_executable) {
        compiler_args.push_back("-c");
    }
    compiler_args.push_back("-o");
    compiler_args.push_back(std::string(dest));

    for (const auto& arg : args) {
        compiler_args.push_back(arg);
    }

    for (const auto& arg : get_env_flags("CFLAGS")) {
        compiler_args.push_back(arg);
    }

    if (link_executable) {
        for (const auto& arg : get_env_flags("LDFLAGS")) {
            compiler_args.push_back(arg);
        }
    }

    compiler_args.push_back(std::string(src));

    auto result = execute_command(compiler_args);
    if (result.ret_code != 0) {
        log("ERROR", "Compilation failed with: \n{}", result.stderr_content);
        throw std::runtime_error("Compilation failed");
    }
}

// Compile C++ source file `src` into artifact `dest`.
// Optional arguments includes `args` to pass extra flags to the compiler and `link_executable` that denotes whether the artifact is an executable.
inline void compile_cxx_source(std::string_view src, std::string_view dest, const std::vector<std::string>& args = {}, bool link_executable = true) {
    std::vector<std::string> compiler_args;
    if (auto env = std::getenv("CXX")) {
        compiler_args.push_back(env);
    } else {
        compiler_args.push_back("c++");
    }

    if (!link_executable) {
        compiler_args.push_back("-c");
    }
    compiler_args.push_back("-o");
    compiler_args.push_back(std::string(dest));

    for (const auto& arg : args) {
        compiler_args.push_back(arg);
    }

    for (const auto& arg : get_env_flags("CXXFLAGS")) {
        compiler_args.push_back(arg);
    }

    if (link_executable) {
        for (const auto& arg : get_env_flags("LDFLAGS")) {
            compiler_args.push_back(arg);
        }
    }

    compiler_args.push_back(std::string(src));

    auto result = execute_command(compiler_args);
    if (result.ret_code != 0) {
        log("ERROR", "Compilation failed with: \n{}", result.stderr_content);
        throw std::runtime_error("Compilation failed");
    }
}

// }}}

// If the `dest` doesn't exist or older than `src`, call `compile_cxx_source` with given arguments.
inline void compile_cxx_if_necessary(std::string_view src, std::string_view dest, const std::vector<std::string>& args = {}, bool link_executable = true) {
    if (std::filesystem::exists(dest) && is_newer(dest, src)) {
        return;
    }

    compile_cxx_source(src, dest, args, link_executable);
}

// If the `dest` doesn't exist or older than `src`, call `compile_c_source` with given arguments.
inline void compile_c_if_necessary(std::string_view src, std::string_view dest, const std::vector<std::string>& args = {}, bool link_executable = true) {
    if (std::filesystem::exists(dest) && is_newer(dest, src)) {
        return;
    }

    compile_c_source(src, dest, args, link_executable);
}

// Rebuild the build script if source has been modified.
inline void go_rebuild_urself(int argc, char **argv, std::source_location loc = std::source_location::current()) {
    using namespace std::string_literals;
    if (is_newer(loc.file_name(), argv[0])) {
        log("INFO", "Self-rebuilding... ");
        compile_cxx_source(loc.file_name(), argv[0], { "-std=c++20"s });
        execute_nofork(argv);
    }
}

template <typename Fn>
inline void guard_exception(Fn&& f) requires std::is_invocable_v<Fn> {
    try {
        f();
    } catch (const std::exception& e) {
        log("ERROR", "Compilation stopped due to exception: {}", e.what());
    }
}

// {{{ PkgConfig stuff
FEATURE_PKG_CONFIG_BEGIN

class PackageNotFoundError : public std::exception {
    private:
    std::string name;
    std::string msg;
    public:
    PackageNotFoundError(const std::string& package_name) : name(package_name) {
        msg = std::format("Cannot find pacakge {} via pkg-config. ", name);
    }
    const char * what() const noexcept override {
        return msg.c_str();
    }
    const std::string& get_name() const noexcept {
        return name;
    }
};

struct Package {
    std::vector<std::string> cflags;
    std::vector<std::string> libs;

    void add_cflags_to(std::vector<std::string>& flags) {
        for (const auto& flag : cflags) {
            flags.push_back(flag);
        }
    }

    void add_libs_to(std::vector<std::string>& flags) {
        for (const auto& flag : libs) {
            flags.push_back(flag);
        }
    }
};

// Check whether `pkg-config` is installed.
inline bool has_pkg_config() {
    try {
        auto result = execute_command({ "pkg-config", "--version" });
        return result.ret_code == 0;
    } catch (const std::runtime_error& e) {
        log("WARNING", "Runtime Error caught: {}", e.what());
        return false;
    }
}

// Find package using `pkg-config`.
inline Package find_package(std::string_view name) {
    log("INFO", "Finding package {}", name);
    if (!has_pkg_config()) {
        throw std::runtime_error("No pkg-config found! ");
    }
    std::string name_s(name);

    auto cflags_res = execute_command({ "pkg-config", "--cflags", name_s });
    auto libs_res = execute_command({ "pkg-config", "--libs", name_s });
    if (cflags_res.ret_code == -1) {
        throw PackageNotFoundError(name_s);
    }

    Package result;
    result.cflags = parse_flags(cflags_res.stdout_content);
    result.libs = parse_flags(libs_res.stdout_content);
    return result;
}

FEATURE_PKG_CONFIG_END
// }}}

OINBS_NAMESPACE_END

#else
#error This is a C++ library and should not be used in a C program.
#endif

