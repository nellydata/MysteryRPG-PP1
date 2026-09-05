#include "GameManager.h"
#include "Win32UI.h"
#include <windows.h>
#include <cstdio>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
	if (Win32UI::ConsoleModeRequested()) {
		AllocConsole();
		FILE* stream = nullptr;
		freopen_s(&stream, "CONIN$", "r", stdin);
		freopen_s(&stream, "CONOUT$", "w", stdout);
		freopen_s(&stream, "CONOUT$", "w", stderr);
		GameManager game;
		game.Run();
		return 0;
	}

	Win32UI game;
	return game.Run();
}