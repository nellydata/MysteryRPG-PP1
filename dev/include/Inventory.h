#pragma once

#include <vector>
#include "Clue.h"

class Inventory {
public:
	std::vector<Clue> clues;

	void AddClue(const Clue& clue);
};
