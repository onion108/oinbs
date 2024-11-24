#include "oinbs.hpp"

int main(int argc, char **argv) {
    using namespace std::string_literals;
    oinbs::go_rebuild_urself(argc, argv);

    oinbs::guard_exception([] {
        auto result = oinbs::execute_command({ "pkg-config"s, "--cflags"s, "--libs"s, "raylib"s });
        oinbs::log("INFO", "pkg-config gives out: {}", result.stdout_content);

        for (const auto& entry : std::filesystem::directory_iterator("examples")) {
            oinbs::log("DEBUG", "Get path: {}", entry.path().string());
            if (entry.is_directory()) {
                oinbs::invoke_build_script(entry.path() / "oinb.cc");
            }
        }
    });
}

