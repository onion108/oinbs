#include "../../oinbs.hpp"

int main(int argc, char **argv) {
    using namespace oinbs;
    guard_exception([&argc, &argv] {
        go_rebuild_urself(argc, argv);

        Target("st-test")
            .add_source_dir("src")
            .add_package("raylib")
            .build();
    });
}

