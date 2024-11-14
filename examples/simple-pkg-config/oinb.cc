#include "../../oinbs.hpp"

int main(int argc, char **argv) {
    using namespace oinbs;
    go_rebuild_urself(argc, argv);

    guard_exception([] {
        std::vector<std::string> flags;
        auto raylib = pkg_config::find_package("raylib");

        flags.push_back("-std=c++23");
        raylib.add_cflags_to(flags);
        raylib.add_libs_to(flags);
        compile_cxx_if_necessary("main.cc", "main", flags);
    });
}

