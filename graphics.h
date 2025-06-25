#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOMINMAX
#include <Windows.h>
#include <WinUser.h>
#include <mmsystem.h>
#undef DrawText
#undef CloseWindow
#undef ShowCursor
#undef LoadImage
#undef PlaySound
#undef DrawTextEx

#include "raylib.h"

int graphrun()
{
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib + WinAPI coexistence");
    SetTargetFPS(60);

    MessageBox(NULL, L"WinAPI works too!", L"Info", MB_OK);

    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        ::DrawText("Raylib and WinAPI in harmony", 200, 200, 20, DARKGRAY);
        EndDrawing();
    }

    ::CloseWindow();

    return 0;
}
