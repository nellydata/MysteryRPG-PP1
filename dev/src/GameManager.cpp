#include "GameManager.h"

void GameManager::Run() {
    bool running = true;

    while (running) {
        std::cout << "1. Inspect\n2. Quit\n";
        int choice;
        std::cin >> choice;

        if (choice == 2) {
            running = false;
        }
    }
}
