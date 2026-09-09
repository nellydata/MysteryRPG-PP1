#pragma once

#include "GameplayTypes.h"
#include <vector>
#include <string>

std::vector<ItemDefinition> BuildEquippableCatalog();
std::vector<ItemDefinition> BuildConsumableCatalog();
std::vector<ItemDefinition> BuildClueCatalog();
std::vector<ItemDefinition> BuildCruxCatalog();
std::vector<std::string> BuildLatinShimmerSkillNames();
std::string InvestigationConsumableName(ConsumableProfile profile, int playerLevel);
ItemDefinition BuildInvestigationConsumable(ConsumableProfile profile, int playerLevel);
std::vector<ItemDefinition> GenerateInvestigationLoot(ConsumableProfile profile, int playerLevel, const std::string& roomId, unsigned seed, bool repeat = false);
std::vector<ItemDefinition> GenerateLoot(const LootContext& context, unsigned seed);
ItemDefinition RollLootInstance(ItemDefinition item, unsigned seed);
std::pair<int, int> GetEquipmentMainStatRange(const std::string& mainStat, Rarity rarity);
int GetEquipmentSubstatCount(Rarity rarity);
bool ValidateEquipmentDefinition(const ItemDefinition& item, std::string* error = nullptr);
double BossArtifactChance(int playerLevel); // Percentage, from 0 through 5.
bool ValidateContentCatalog(std::string* error = nullptr);
bool ValidateMicareContent(std::string* error = nullptr);
