#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "raylib.h"

int graphrun()
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "Title");
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
		DrawFPS(10, 10);
        DrawText("Hello! World", 200, 200, 20, DARKGRAY);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
