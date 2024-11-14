// This is build file of the project.
// Compile it with the following command:
// g++ -std=c++20 -o oinb oinb.cc
#include "../../oinbs.hpp"

int main(int argc, char **argv) {
    using namespace oinbs;
    using namespace std::string_literals;
    go_rebuild_urself(argc, argv);

    compile_cxx_if_necessary("main.cc", "main", { "-std=c++23"s });
}

