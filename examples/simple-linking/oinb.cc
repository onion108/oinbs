#include "../../oinbs.hpp"

int main(int argc, char **argv) {
    using namespace oinbs;
    namespace fs = std::filesystem;
    guard_exception([&argc, &argv] {
        go_rebuild_urself(argc, argv);

        if (!fs::exists("build")) {
            fs::create_directory("build");
        }
        compile_cxx_if_necessary(
            "main.cc",
            "build/main.o",
            { "-std=c++23" },
            false
        );
        compile_cxx_if_necessary(
            "header.cc",
            "build/header.o",
            { "-std=c++23" },
            false
        );
        link_artifact(
            { "build/main.o", "build/header.o" },
            "build/program"
        );
    });
}

