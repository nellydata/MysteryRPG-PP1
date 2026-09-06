#include "GameManager.h"
#include "Win32UI.h"
#include <windows.h>
#include <cstdio>
#include "ContentCatalog.h"
#include <iostream>
#include <cstdlib>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
	char* validationMode = nullptr;
	size_t validationLength = 0;
	_dupenv_s(&validationMode, &validationLength, "MICARE_VALIDATE");
	const bool shouldValidate = validationMode != nullptr;
	free(validationMode);
	if (shouldValidate) {
		std::string error;
		const bool valid = ValidateMicareContent(&error);
		if (!valid) std::cerr << "Micare content validation failed: " << error << "\n";
		else std::cout << "Micare content validation passed.\n";
		return valid ? 0 : 1;
	}
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