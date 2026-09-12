#include "GameManager.h"
#include "ContentCatalog.h"
#include "ConsoleUI.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    for (int index = 1; index < argc; ++index) {
        const std::string option = argv[index];
        if (option == "--fast") ConsoleUI::SetFast(true);
        else if (option == "--validate") {
            std::string error;
            if (!ValidateMicareContent(&error)) { std::cerr << error << '\n'; return 1; }
            std::cout << "Micare content validation passed.\n";
            return 0;
        } else if (option == "--help") {
            std::cout << "Micare console RPG\nUsage: COS-PP1.exe [--fast] [--validate]\n";
            return 0;
        } else { std::cerr << "Unknown option: " << option << '\n'; return 2; }
    }
    GameManager game;
    game.Run();
    return 0;
}
