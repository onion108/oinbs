// OINBS - OINBS Is Not Build System
//
// Version: 0.1.0
// Author: 27Onion Nebell <zzy20080201@gmail.com>
//
// Copyright 2024 27Onion Nebell <zzy20080201@gmail.com>
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#pragma once

#if defined(__cplusplus)
#include <unordered_map>
#include <type_traits>
#include <string>
#include <source_location>
#include <filesystem>
#include <vector>
#include <format>
#include <stdexcept>
#include <cstring>
#include <exception>
#include <iostream>
#include <fstream>
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

#ifndef ENABLE_FEATURE_PKG_CONFIG
#define ENABLE_FEATURE_PKG_CONFIG 1
#endif

OINBS_NAMESPACE_BEGIN

// {{{ Global Variables
inline std::string g_build_script_name = "\\/\\/";
// }}}

// {{{ Utilities

// Escapes string
inline std::string escape_string(std::string_view str) {
    if (str.empty()) return "";

    using namespace std::string_literals;
    auto result = "\""s;
    for (auto ch : str) {
        if (ch == '\n') {
            result += "\\n";
            continue;
        }
        if (ch == '\t') {
            result += "\\t";
            continue;
        }
        if (ch == '\\') {
            result += "\\\\";
            continue;
        }
        if (ch == '\"') {
            result += "\\\"";
            continue;
        }

        result += ch;
    }
    result += "\"";
    return result;
}

// Checks if file a is newer than file b.
inline bool is_newer(std::string_view a, std::string_view b) {
    auto a_time = std::filesystem::last_write_time(a);
    auto b_time = std::filesystem::last_write_time(b);
    return a_time > b_time;
}

inline std::string get_cc() {
    auto cc = std::getenv("CC");
    return cc ? cc : "cc";
}

inline std::string get_cxx() {
    auto cxx= std::getenv("CXX");
    return cxx ? cxx : "cxx";
}

// Log stuff.
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

template <typename Fn>
inline void guard_exception(Fn&& f) requires std::is_invocable_v<Fn> {
    try {
        f();
    } catch (const std::exception& e) {
        log("ERROR", "Compilation stopped due to exception: {}", e.what());
        std::exit(1);
    }
}

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

inline std::vector<std::string> walk_dir(std::filesystem::path path) {
    std::vector<std::string> result;
    if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
        throw std::runtime_error(std::format("{} is not a valid path! ", path.string()));
    }
    for (auto i : std::filesystem::recursive_directory_iterator(path)) {
        result.push_back(i.path());
    }
    return result;
}

// }}}

// {{{ File extension

// Check if input is C++ source file.
inline bool is_cxx_source(std::string_view name) {
    return name.ends_with(".cc") || name.ends_with(".cxx") || name.ends_with(".cpp");
}

// Check if input is C source file.
inline bool is_c_source(std::string_view name) {
    return name.ends_with(".c");
}

// Check if input is C++ header file.
inline bool is_cxx_header(std::string name) {
    return name.ends_with(".h") || name.ends_with(".hh") || name.ends_with(".hpp");
}

// Check if input is C header file.
inline bool is_c_header(std::string name) {
    return name.ends_with(".h");
}

// Strip file extension.
inline std::string strip_file_extension(std::string_view filename) {
    auto idx = filename.find_last_of(".");
    if (idx == std::string_view::npos) {
        return std::string { filename };
    }
    return std::string { filename.substr(0, idx) };
}

// }}}

// {{{ Platform specific thingy

// Structure represents the result of a command execution.
struct CommandOutput {
    int ret_code;
    std::string stdout_content;
    std::string stderr_content;
};

// Load executable and replace current process.
inline void execute_nofork(char **argv) {
    if (execvp(argv[0], argv) == -1) {
        throw std::runtime_error("Failed to spawn process");
    }
}

// Execute command using parameter `argv`.
inline CommandOutput execute_command(const std::vector<std::string>& argv, bool redirect_output = true) {
    log("INFO", "Executing command: {}", render_command(argv));

    int pout[2], perr[2];
    if (pipe(pout)) throw std::runtime_error("Cannot create pipe for stdout");
    if (pipe(perr)) throw std::runtime_error("Cannot create pipe for stderr");
    auto child_pid = fork();
    if (child_pid == -1) throw std::runtime_error("Failed to fork child process, damn it! ");
    if (child_pid == 0) {
        if (redirect_output) {
            dup2(pout[1], STDOUT_FILENO);
            dup2(perr[1], STDERR_FILENO);
        }
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
        if (redirect_output) {
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
        } else {
            result.stdout_content = "<invalid>";
            result.stderr_content = "<invalid>";
        }
        waitpid(child_pid, &result.ret_code, 0);
        return result;
    }
}

// }}}

// {{{Raw compilation thingy

// Generates argv from a compilation call. Defaults to C and if `is_cxx` was set to `true` then C++.
inline std::vector<std::string> generate_compilation_argv(bool is_cxx, std::string_view src, std::string_view dest, const std::vector<std::string>& args, bool link_executable) {
    std::vector<std::string> compiler_args;
    compiler_args.push_back(is_cxx ? get_cxx() : get_cc());

    if (!link_executable) {
        compiler_args.push_back("-c");
    }
    compiler_args.push_back("-o");
    compiler_args.push_back(std::string(dest));

    for (const auto& arg : args) {
        compiler_args.push_back(arg);
    }

    for (const auto& arg : get_env_flags(is_cxx ? "CXXFLAGS" : "CFLAGS")) {
        compiler_args.push_back(arg);
    }

    if (link_executable) {
        for (const auto& arg : get_env_flags("LDFLAGS")) {
            compiler_args.push_back(arg);
        }
    }

    compiler_args.push_back(std::string(src));
    return compiler_args;
}

// Compile C source file `src` into artifact `dest`.
// Optional arguments includes `args` to pass extra flags to the compiler and `link_executable` that denotes whether the artifact is an executable.
inline void compile_c_source(std::string_view src, std::string_view dest, const std::vector<std::string>& args = {}, bool link_executable = true) {
    auto result = execute_command(generate_compilation_argv(false, src, dest, args, link_executable));
    if (result.ret_code != 0) {
        log("ERROR", "Compilation failed with: \n{}", result.stderr_content);
        throw std::runtime_error("Compilation failed");
    }
}

// Compile C++ source file `src` into artifact `dest`.
// Optional arguments includes `args` to pass extra flags to the compiler and `link_executable` that denotes whether the artifact is an executable.
inline void compile_cxx_source(std::string_view src, std::string_view dest, const std::vector<std::string>& args = {}, bool link_executable = true) {

    auto result = execute_command(generate_compilation_argv(true, src, dest, args, link_executable));
    if (result.ret_code != 0) {
        log("ERROR", "Compilation failed with: \n{}", result.stderr_content);
        throw std::runtime_error("Compilation failed");
    }
}

// }}}

// {{{Raw linking thingy

// Artifact type.
enum class ArtifactType {
    Executable,
    SharedLibrary,
    StaticLibrary,
};

inline std::string shared_library_name(std::string_view name) {
    #ifdef __MACH__
    return std::format("lib{}.dylib", name);
    #elif defined(_WIN32) || defined(_WIN64)
    return std::format("{}.dll", name);
    #else
    return std::format("lib{}.so", name);
    #endif
}

// Link (or archive) objects into artifact.
// If `artifact_type` is set to `SharedLibrary` or `StaticLibrary`, file extension will be automatically added.
inline void link_artifact(const std::vector<std::string>& objects, std::string artifact, std::vector<std::string> flags = {}, ArtifactType artifact_type = ArtifactType::Executable, bool use_cxx_stdlib = true) {
    switch (artifact_type) {
        case ArtifactType::Executable: {
            auto linker = use_cxx_stdlib ? get_cxx() : get_cc();
            std::vector<std::string> cmd;
            cmd.push_back(linker);
            cmd.push_back("-o");
            cmd.push_back(artifact);
            for (const auto& i : flags) cmd.push_back(i);
            for (const auto& i : get_env_flags("LDFLAGS")) cmd.push_back(i);
            for (const auto& i : objects) cmd.push_back(i);
            auto result = execute_command(cmd);
            if (result.ret_code != 0) {
                log("ERROR", "Linking failed with: \n{}", result.stderr_content);
                throw std::runtime_error("Linking failed");
            }
        } break;

        case ArtifactType::SharedLibrary: {
            auto linker = use_cxx_stdlib ? get_cxx() : get_cc();
            std::vector<std::string> cmd;
            cmd.push_back(linker);
            cmd.push_back("-o");
            cmd.push_back(shared_library_name(artifact));
            cmd.push_back("-shared");
            for (const auto& i : flags) cmd.push_back(i);
            for (const auto& i : get_env_flags("LDFLAGS")) cmd.push_back(i);
            for (const auto& i : objects) cmd.push_back(i);
            auto result = execute_command(cmd);
            if (result.ret_code != 0) {
                log("ERROR", "Linking failed with: \n{}", result.stderr_content);
                throw std::runtime_error("Linking failed");
            }
        } break;

        case ArtifactType::StaticLibrary: {
            std::vector<std::string> cmd { "ar", "-rc" };
            for (const auto& i : objects) {
                cmd.push_back(i);
            }
            auto result = execute_command(cmd);
            if (result.ret_code != 0) {
                log("ERROR", "Archiving failed with: \n{}", result.stderr_content);
                throw std::runtime_error("Archiving failed");
            }
        } break;
    }
}

// }}}

// {{{ More compilation thingy

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
    g_build_script_name = argv[0];
    if (is_newer(loc.file_name(), argv[0])) {
        log("INFO", "Self-rebuilding... ");
        compile_cxx_source(loc.file_name(), argv[0], { "-std=c++20"s });
        execute_nofork(argv);
    }
}

// }}}

// {{{ Call other build scripts

// Call build script at `path`. `path` should be path to the source file of the build script.
// The bootstrapping will be done by the current build script if it haven't been done yet.
// Extra arguments could be passed by using `args` argument. Defaults to `{}`.
inline void invoke_build_script(std::filesystem::path path, std::vector<std::string> args = {}) {
    if (!path.has_parent_path()) {
        throw std::runtime_error(std::format("Path {} doesn't have parent path", path.string()));
    }
    auto cwd = std::filesystem::current_path();
    std::filesystem::current_path(path.parent_path());

    auto bs_build_name = path.filename().string();
    auto bs_build_src = bs_build_name;
    if (!is_cxx_source(bs_build_src)) {
        std::filesystem::current_path(cwd);
        throw std::runtime_error(std::format("{} is not a valid C++ source file", bs_build_name));
    }
    bs_build_name = strip_file_extension(bs_build_src);
    
    if (!std::filesystem::exists(bs_build_name)) {
        log("INFO", "Bootstrapping build script {}", path.string());
        try {
            compile_cxx_source(bs_build_src, bs_build_name, { "-std=c++20" });
        } catch (const std::runtime_error& e) {
            log("ERROR", "Failed to bootstrap build script: {}", e.what());
            std::filesystem::current_path(cwd);
            throw e;
        }
        log("INFO", "Build script successfully bootstrapped");
    }

    log("INFO", "Executing build script {}", path.string());
    std::vector<std::string> argv = args;
    args.insert(args.begin(), "./" + bs_build_name);
    auto result = execute_command(args, false);
    if (result.ret_code) {
        std::filesystem::current_path(cwd);
        throw std::runtime_error(std::format("Failed to execute build script {}", path.string()));
    }

    // Restore cwd.
    std::filesystem::current_path(cwd);
}

// }}}

// {{{ Apple thingy

// Modify install name of a dylib.
inline void modify_install_name(std::string_view bin, std::string_view old_dylib, std::string_view new_dylib) {
#if __APPLE__
    auto result = execute_command({ "install_name_tool", "-change", std::string(old_dylib), std::string(new_dylib) });
    if (result.ret_code) {
        throw std::runtime_error(std::format("Failed to change install name of {} to {} because of: {}", old_dylib, new_dylib, result.stderr_content));
    }
#endif
}

// Append `@rpath/` to the dynamic library file name.
inline void rpathify_lib(std::string_view bin, std::string_view dylib) {
    using namespace std::string_literals;
    modify_install_name(bin, dylib, "@rpath/"s + std::string(dylib));
}

// }}}

// {{{ PkgConfig stuff
#if ENABLE_FEATURE_PKG_CONFIG
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
#endif
// }}}

// {{{ Compilation Database stuff

class CompilationDatabase {
    struct Entry {
        std::vector<std::string> args;
        std::string dir;
        std::string file;
        std::string output;
    };

    struct Operation {
        std::string src;
        std::string dest;
        std::vector<std::string> args;
        bool link_executable;
        bool is_cxx;
    };


    std::vector<Operation> m_operations;
    bool m_use_lazy_compilation;
    bool m_dummy;
    std::string m_render_entry(const Entry& entry) {
        std::string args;
        if (entry.args.empty()) {
            args = "[]";
        } else {
            args = "[";
            args += escape_string(entry.args[0]);
            for (auto arg = entry.args.begin()+1; arg != entry.args.end(); arg++) {
                args += ",";
                args += escape_string(*arg);
            }
            args += "]";
        }
        std::string result = "{";
        result += "\"arguments\": ";
        result += args;
        result += ", \"directory\": ";
        result += escape_string(std::filesystem::current_path().string());
        result += ", \"file\": ";
        result += escape_string(std::filesystem::absolute(entry.file).string());
        result += ", \"output\": ";
        result += escape_string(std::filesystem::absolute(entry.output).string());
        result += "}";
        return result;
    }
    public:
    CompilationDatabase(bool lazy = true, bool dummy = false) : m_operations(), m_use_lazy_compilation(lazy), m_dummy(dummy) {}
    void compile_c_source(const std::string& src, const std::string& dest, const std::vector<std::string>& args = {}, bool link_executable = true) {
        m_operations.push_back({ src, dest, args, link_executable, false});
    }

    void compile_cxx_source(const std::string& src, const std::string& dest, const std::vector<std::string>& args = {}, bool link_executable = true) {
        m_operations.push_back({ src, dest, args, link_executable, true});
    }

    void perform() {
        auto cmp_c = oinbs::compile_c_source;
        auto cmp_cxx = oinbs::compile_cxx_source;
        if (m_use_lazy_compilation) {
            cmp_c = oinbs::compile_c_if_necessary;
            cmp_cxx = oinbs::compile_cxx_if_necessary;
        }
        for (auto operation : m_operations) {
            if (operation.is_cxx) {
                cmp_cxx(operation.src, operation.dest, operation.args, operation.link_executable);
            } else {
                cmp_c(operation.src, operation.dest, operation.args, operation.link_executable);
            }
        }
    }

    std::string generate_database() {

        // Dummy compdb doesn't generate anything.
        if (m_dummy) return "";

        // Use unordered_map to avoid duplicated items.
        std::unordered_map<std::string, Entry> db;
        for (auto operation : m_operations) {
            Entry e;
            e.args = generate_compilation_argv(operation.is_cxx, operation.src, operation.dest, operation.args, operation.link_executable);
            e.file = operation.src;
            e.output = operation.dest;
            db[e.file] = e;
        }

        if (db.size() == 0) {
            return "[]";
        }

        std::string result = "[";
        result += m_render_entry(db.begin()->second);
        for (auto entry = ++db.begin(); entry != db.end(); entry++) {
            result += ", ";
            result += m_render_entry(entry->second);
        }
        result += "]";
        return result;
    }

    void build() {
        perform();

        if (m_dummy) return;
        auto db = generate_database();
        std::ofstream comp_db("compile_commands.json");
        comp_db << db << '\n';
    }

};

// }}}

// {{{ Structural Representation (facny stuff)

// {{{ Target

// Class that represents a target.
class Target {
    ArtifactType m_atype;
    std::vector<std::string> m_c_files;
    std::vector<std::string> m_cflags;
    std::vector<std::string> m_cxx_files;
    std::vector<std::string> m_cxxflags;
    std::vector<std::string> m_ldflags;
    std::filesystem::path m_build_dir;
    std::string m_target_name;

    // Generates object file name from a path.
    // This generates a unique name for every path, and always generates same name for the same path.
    std::string m_generate_obj_name(std::string_view path) {
        std::string result;
        for (auto ch : path) {
            switch (ch) {
                case '/': {
                    result += "$P";
                } break;
                case '$': {
                    result += "$$";
                } break;
                case '.': {
                    result += "$d";
                } break;
                default: {
                    result += ch;
                } break;
            }
        }
        return result + ".o";
    }

    public:
    Target(const std::string& target_name = "program", const std::string& build_dir = "./build") : m_atype(ArtifactType::Executable), m_build_dir(build_dir), m_target_name(target_name) {}

    // Set target type to executable.
    Target& executable() {
        m_atype = ArtifactType::Executable;
        return *this;
    }

    // Set target type to static library.
    Target& static_library() {
        m_atype = ArtifactType::StaticLibrary;
        return *this;
    }

    // Set target type to shared library.
    Target& shared_library() {
        m_atype = ArtifactType::SharedLibrary;
        return *this;
    }

    // Add C source file.
    Target& add_c_source(std::string_view src) {
        log("INFO", "Adding C source {}", src);
        m_c_files.push_back(std::string { src });
        return *this;
    }

    // Add C++ source file.
    Target& add_cxx_source(std::string_view src) {
        log("INFO", "Adding C++ source {}", src);
        m_cxx_files.push_back(std::string { src });
        return *this;
    }

    // Add multiple C source files.
    Target& add_c_sources(std::vector<std::string> srcs) {
        for (auto src : srcs) {
            add_c_source(src);
        }
        return *this;
    }

    // Add multiple C++ source files.
    Target& add_cxx_sources(std::vector<std::string> srcs) {
        for (auto src : srcs) {
            add_cxx_source(src);
        }
        return *this;
    }

    // Add compiler flag to C compiler.
    Target& add_c_flag(std::string_view flag) {
        m_cflags.push_back(std::string { flag });
        return *this;
    }

    // Add compiler flag to C++ compiler.
    Target& add_cxx_flag(std::string_view flag) {
        m_cxxflags.push_back(std::string { flag });
        return *this;
    }

    // Add compiler flags to C compiler.
    Target& add_c_flags(const std::vector<std::string>& flags) {
        for (auto flag : flags) {
            add_c_flag(flag);
        }
        return *this;
    }

    // Add compiler flags to C++ compiler.
    Target& add_cxx_flags(const std::vector<std::string>& flags) {
        for (auto flag : flags) {
            add_cxx_flag(flag);
        }
        return *this;
    }

    // Set C++ standard.
    Target& set_cxx_standard(std::string_view ver) {
        m_cxxflags.push_back(std::format("-std={}", ver));
        return *this;
    }

    // Set C standard.
    Target& set_c_standard(std::string_view ver) {
        m_cflags.push_back(std::format("-std={}", ver));
        return *this;
    }

    // Set C++ optimization level.
    Target& set_cxx_optimization(std::string_view level) {
        m_cxxflags.push_back(std::format("-O{}", level));
        return *this;
    }

    // Set C optimization level.
    Target& set_c_optimization(std::string_view level) {
        m_cflags.push_back(std::format("-O{}", level));
        return *this;
    }

    // Set optimization level.
    Target& set_optimization(std::string_view level) {
        set_c_optimization(level);
        return set_cxx_optimization(level);
    }

    // Enable debug information.
    Target& debug() {
        m_cflags.push_back("-g");
        m_cxxflags.push_back("-g");
        return *this;
    }

    // Add include directory for C.
    Target& add_include_directory_c(std::string_view dir) {
        m_cflags.push_back(std::format("-I{}", dir));
        return *this;
    }

    // Add include directory for C++.
    Target& add_include_directory_cxx(std::string_view dir) {
        m_cxxflags.push_back(std::format("-I{}", dir));
        return *this;
    }

    // Add include directory.
    Target& add_include_directory(std::string_view dir) {
        m_cxxflags.push_back(std::format("-I{}", dir));
        m_cflags.push_back(std::format("-I{}", dir));
        return *this;
    }

    // Add link directory.
    Target& add_link_directory(std::string_view dir) {
        m_ldflags.push_back(std::format("-L{}", dir));
        return *this;
    }

    // Add library.
    Target& add_library(std::string_view lib) {
        m_ldflags.push_back(std::format("-l{}", lib));
        return *this;
    }

    // Add runtime path.
    Target& add_rpath(std::string_view rpath) {
        m_ldflags.push_back("-rpath");
        m_ldflags.push_back(std::string { rpath });
        return *this;
    }

    // Add linker flag.
    Target& add_linker_flag(std::string_view flag) {
        m_ldflags.push_back(std::string { flag });
        return *this;
    }

    // Add linker flags.
    Target& add_linker_flags(const std::vector<std::string>& flags) {
        for (auto flag : flags) {
            m_ldflags.push_back(flag);
        }
        return *this;
    }

    // Add bunch of sources in a directory.
    Target& add_source_dir(std::string_view path) {
        log("INFO", "Adding source directory {}", path);
        if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
            throw std::runtime_error(std::format("{} is not a valid directory. ", path));
        }

        for (auto entry : std::filesystem::recursive_directory_iterator(path, std::filesystem::directory_options::follow_directory_symlink)) {
            if (entry.is_directory()) continue;

            auto path = entry.path().string();
            if (is_c_source(path)) {
                add_c_source(path);
            } else if (is_cxx_source(path)) {
                add_cxx_source(path);
            }
        }

        return *this;
    }

#if ENABLE_FEATURE_PKG_CONFIG
    // Add a package from pkg-config for C sources.
    Target& add_package_c(std::string_view pkg) {
        if (!pkg_config::has_pkg_config()) {
            throw std::runtime_error("No pkg-config found");
        }

        try {
            auto package = pkg_config::find_package(pkg);
            add_c_flags(package.cflags);
            add_linker_flags(package.libs);
        } catch (const pkg_config::PackageNotFoundError& e) {
            log("ERROR", "Cannot find package {}", pkg);
            throw e;
        }
        return *this;
    }

    // Add a package from pkg-config for C++ sources.
    Target& add_package_cxx(std::string_view pkg) {
        if (!pkg_config::has_pkg_config()) {
            throw std::runtime_error("No pkg-config found");
        }

        try {
            auto package = pkg_config::find_package(pkg);
            add_cxx_flags(package.cflags);
            add_linker_flags(package.libs);
        } catch (const pkg_config::PackageNotFoundError& e) {
            log("ERROR", "Cannot find package {}", pkg);
            throw e;
        }
        return *this;
    }

    // Add a package from pkg-config for entire target.
    Target& add_package(std::string_view pkg) {
        if (!pkg_config::has_pkg_config()) {
            throw std::runtime_error("No pkg-config found");
        }

        try {
            auto package = pkg_config::find_package(pkg);
            add_c_flags(package.cflags);
            add_cxx_flags(package.cflags);
            add_linker_flags(package.libs);
        } catch (const pkg_config::PackageNotFoundError& e) {
            log("ERROR", "Cannot find package {}", pkg);
            throw e;
        }
        return *this;
    }
#endif

    // Start the build process.
    void build() {
        CompilationDatabase db;
        build(db);
    }

    // Ready. Set. Go!
    // Start the build process using given compilation database.
    void build(CompilationDatabase& compdb) {
        if (g_build_script_name == "\\/\\/") {
            log("WARNING", "Did you forget to call go_rebuild_urself? g_build_script_name doesn't detected. ");
        }

        log("INFO", "Building target {}", m_target_name);
        if (!std::filesystem::exists(m_build_dir)) {
            std::filesystem::create_directories(m_build_dir);
        }

        if (!std::filesystem::exists(m_build_dir / "obj")) {
            std::filesystem::create_directories(m_build_dir / "obj");
        }

        if (!std::filesystem::exists(get_build_artifact_dir())) {
            std::filesystem::create_directories(get_build_artifact_dir());
        }

        // Create dummy file to indicate build time.
        if (!std::filesystem::exists(m_build_dir / "dummy")) {
            std::ofstream ofs("dummy");
            ofs << "DUMMY FILE DONT TOUCH";
            ofs.close();
        } else {
            if (g_build_script_name != "\\/\\/" && is_newer(g_build_script_name, (m_build_dir / "dummy").string())) {
                log("INFO", "Build script changed detected");
                clean();
            }
        }

        // Compilation stage
        log("INFO", "Compiling target {}", m_target_name);
        std::unordered_map<std::string, std::string> src_to_obj;
        for (auto cxxsrc : m_cxx_files) {
            src_to_obj[cxxsrc] = m_generate_obj_name(cxxsrc);
            compdb.compile_cxx_source(cxxsrc, m_build_dir / "obj" / src_to_obj[cxxsrc], m_cxxflags, false);
        }

        for (auto csrc : m_c_files) {
            src_to_obj[csrc] = m_generate_obj_name(csrc);
            compdb.compile_c_source(csrc, m_build_dir / "obj" / src_to_obj[csrc], m_cxxflags, false);
        }

        compdb.build();

        // Linking stage
        log("INFO", "Linking or archiving target {}", m_target_name);
        std::vector<std::string> objs;
        for (auto kv : src_to_obj) {
            objs.push_back(m_build_dir / "obj" / kv.second);
        }
        link_artifact(objs, get_build_artifact(), m_ldflags, m_atype, !m_cxx_files.empty());
    }

    // Clean the build directory
    void clean() {
        log("INFO", "Cleaning target {}", m_target_name);
        if (std::filesystem::exists(m_build_dir))
            std::filesystem::remove_all(m_build_dir);
    }

    // Get build artifact (may not exist).
    std::filesystem::path get_build_artifact() {
        return get_build_artifact_dir() / m_target_name;
    }

    // Get build artifact directory.
    std::filesystem::path get_build_artifact_dir() {
        return m_build_dir / "dest";
    }

    // Get build directory.
    std::filesystem::path get_build_dir() {
        return m_build_dir;
    }

    // Get compiler flags for C.
    std::vector<std::string> get_cflags() {
        return m_cflags;
    }

    // Get compile flags for C++.
    std::vector<std::string> get_cxxflags() {
        return m_cxxflags;
    }

    // Get linker flags.
    std::vector<std::string> get_ldflags() {
        return m_ldflags;
    }

    // Get artifact type.
    ArtifactType get_artifact_type() {
        return m_atype;
    }

    // Get Target name.
    std::string get_name() {
        return m_target_name;
    }

};

// }}}

// }}}

OINBS_NAMESPACE_END

#else
#error This is a C++ library and should not be used in a C program.
#endif

