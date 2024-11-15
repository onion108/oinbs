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

## How to use

It's a header-only library, so just download the `oinbs.hpp` in the repository and put in your project directory. Then, create a C++ source (typically with the name `oinb.cc`) in the same directory as `oinbs.hpp` and write your building code. You may want to write it like this:
```c++
#include "oinbs.hpp"

int main(int argc, char **argv) {
    using namespace oinbs;
    guard_exception([&argc, &argv] {
        go_rebuild_urself(argc, argv);

        // Your build code here! 
    });
}

```

The Go Rebuild Urself™ technology brought by `oinbs::go_rebuild_urself(int argc, char **argv)` automatically rebuilds the build script on changes and runs the newest build of the build script. Hence once you build the source you will never need to build it again.

Here is the simplest way to bootstrap the thing:
```shell
clang++ -std=c++20 -o oinb ./oinb.cc
```

Replace `clang++` with the compiler you like (e.g. `g++`), as long as the compiler supports C++20. Then you just execute the build script by:

```shell
./oinb
```

And you're ready to go! 


## Compilation Database

Currently there is no way to generate compilation database from the build script using the library. However you can use [Bear](https://github.com/rizsotto/Bear) to make `compile_commands.json` while compiling. It also works while with Mr.Azozin's [nob.h](https://github.com/tsoding/nob.h), by the way.

## Roadmap

- [ ] Support structural representation of targets (`class Target`) and `compile_commands.json` generation from it.
- [ ] Support structural representation of a project (`class Project`) (which basically contains multiple targets hence should be easy to implement).
- [ ] Windows Support.


