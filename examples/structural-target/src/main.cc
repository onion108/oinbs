#include <raylib.h>

int main(void) {
    InitWindow(800, 600, "urmom");
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(WHITE);
        EndDrawing();
    }
    CloseWindow();
}

