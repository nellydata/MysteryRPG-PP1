#pragma once

#include <string>
#include <vector>

class Area;
class CombatManager;
class Enemy;
class Player;
class ProgressionManager;

class ConsoleUI {
public:
	ConsoleUI();

	void Clear() const;
	void RenderDashboard(const Player& player, const ProgressionManager& progression,
						 const std::vector<Area>& rooms, const std::string& context) const;
	void RenderRoom(const Area& area, const Player& player,
					const ProgressionManager& progression) const;
	void RenderCombat(const Player& player, const ProgressionManager& progression,
					  const Enemy& enemy, const CombatManager& combat) const;
	void ShowInventory(const Player& player) const;
	void ShowMap(const std::vector<Area>& rooms) const;
	void ShowShimmerSkills(const CombatManager& combat) const;
	void ShowDeveloperPanel() const;
	void ShowMessage(const std::string& message) const;
	int Prompt(const std::string& prompt, int minimum, int maximum) const;
	bool IsDeveloperMode() const;

private:
	bool colorEnabled_;

	void PrintTitle(const std::string& title) const;
	void PrintPlayerPanel(const Player& player, const ProgressionManager& progression) const;
	void PrintStatusEffects(const Player& player) const;
	void PrintEquipment(const Player& player) const;
	void PrintRule() const;
	std::string StatusName(int statusType) const;
};
