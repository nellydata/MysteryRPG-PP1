#pragma once

#include "GameplayTypes.h"
#include <vector>

std::vector<ItemDefinition> BuildEquippableCatalog();
std::vector<ItemDefinition> BuildConsumableCatalog();
std::vector<ItemDefinition> BuildClueCatalog();
std::vector<ItemDefinition> BuildCruxCatalog();
std::vector<std::string> BuildLatinShimmerSkillNames();
