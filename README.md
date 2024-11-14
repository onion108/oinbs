# OINBS: OINBS Is Not Build System

(WIP)

A build system for C++ inspired by Tsoding's [nob.h](https://github.com/tsoding/nob.h). This header-only library makes it possible for you to write build script in C++20. Here is an example to build `main.cc` into executable `main`.

```c++
#include "oinbs.hpp"
#include <string>

int main(int argc, char **argv) {
    using namespace oinbs;
    using namespace std::string_literals;
    go_rebuild_urself(argc, argv);

    compile_cxx_source("main.cc", "main", { "-std=c++20"s });
}

```

