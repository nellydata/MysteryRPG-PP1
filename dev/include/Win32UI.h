#pragma once

#include "CombatManager.h"
#include "MapManager.h"
#include "Player.h"
#include "ProgressionManager.h"
#include "EncounterManager.h"
#include <windows.h>
#include <memory>
#include <string>
#include <vector>
#include <deque>

class Win32UI {
public:
	Win32UI();
	~Win32UI();

	int Run();
	static bool ConsoleModeRequested();

private:
	enum class View { Floor, Room, Combat, Inventory, Map, Skills, BattleLog };

	static constexpr int RoomButtonBase = 100;
	static constexpr int InspectButtonBase = 200;
	static constexpr int InventoryButtonBase = 300;
	static constexpr int ButtonSearch = 400;
	static constexpr int ButtonBack = 401;
	static constexpr int ButtonMap = 402;
	static constexpr int ButtonSkills = 403;
	static constexpr int ButtonAttack = 500;
	static constexpr int ButtonItem = 501;
	static constexpr int ButtonShimmer = 502;
	static constexpr int ButtonFlee = 503;
	static constexpr int ButtonRespawn = 507;
	static constexpr int ButtonQuit = 504;
	static constexpr int ButtonInventory = 505;
	static constexpr int ButtonSort = 506;
	static constexpr int ButtonDiscard = 508;
	static constexpr int ButtonEquip = 509;
	static constexpr int ButtonInventoryPrevious = 510;
	static constexpr int ButtonInventoryNext = 511;
	static constexpr int InventorySearch = 512;
	static constexpr int ButtonBattleLog = 513;
	static constexpr int ButtonCombatSpeed = 514;
	static constexpr int SkillButtonBase = 800;
	static constexpr int SkillSortBase = 740;
	static constexpr int SkillFilterBase = 750;
	static constexpr int SortModeBase = 600;
	static constexpr int FilterModeBase = 620;
	static constexpr int EquipFilterBase = 680;
	static constexpr UINT RefreshControlsMessage = WM_APP + 1;

	static LRESULT CALLBACK WindowProcedure(HWND window, UINT message,
											 WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK NameProcedure(HWND window, UINT message,
										  WPARAM wParam, LPARAM lParam);
	LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);

	void PromptForName(HINSTANCE instance);
	void CreateWindowShell(HINSTANCE instance);
	void CreateControls();
	void RefreshControls();
	void LayoutControls(int width, int height);
	void HandleCommand(int controlId);
	void ShowInventoryMenu();
	void ShowSkillMenu();
	void ChangeView(View nextView);
	void LeaveCombat();
	void ShowDefeatRecovery();
	void AcknowledgeDefeat();
	void ShowFullBattleLog();
	void EnterRoom(int roomIndex);
	void Inspect(int interactableIndex);
	void SearchRoom();
	void PerformCombatAction(CombatAction action);
	void ShowCombatItems();
	void ResetCombatSelection();
	std::string CompactConsumableSummary(const Item& item) const;
	void AddLog(const std::string& message);
	void AddLootLog(const Item& item);
	void SyncCombatEvents();
 void SyncProgressionRewards();
	void InitializePlayer();
	void InitializeCompletionState();
	void Invalidate();

	void Paint(HDC deviceContext);
	void DrawPanel(HDC deviceContext, RECT bounds, const std::wstring& title);
	void DrawHealthBar(HDC deviceContext, int x, int y, int width, int height, int current, int maximum, COLORREF fillColor) const;
	int DrawTextLine(HDC deviceContext, int x, int y, const std::wstring& text,
						 COLORREF color, int size = 18, bool bold = false, int maxWidth = 0, bool emoji = false) const;
	void DrawRainbowText(HDC deviceContext, int x, int y, const std::wstring& text,
					 int size = 18, bool bold = false) const;
	std::wstring ToWide(const std::string& text) const;
	std::wstring FormatDisplayText(const std::string& text) const;
	std::string ToNarrow(const std::wstring& text) const;
	std::wstring StatusName(StatusType type) const;
	std::wstring CombatStatusSymbol(StatusType type) const;

	HWND window_;
	HWND nameWindow_;
	HWND nameEdit_;
	HWND controls_[64];
	HWND roomButtons_[5];
	HWND inspectButtons_[5];
	HFONT font_;
	HFONT headingFont_;
	HFONT statusFont_;
	View view_;
	std::vector<View> viewHistory_;
	int selectedRoom_;
	Player player_;
	MapManager mapManager_;
	CombatManager combat_;
	ProgressionManager progression_;
	EncounterManager investigation_;
	Enemy* activeEnemy_;
	std::vector<std::string> log_;
	std::vector<Rarity> logRarities_;
	std::string statusMessage_;
	std::string inventorySearchQuery_;
	InventorySort inventorySort_ = InventorySort::RecentlyObtained;
	ItemStat inventoryFocusStat_ = ItemStat::ATK;
	int inventoryFilter_ = -1;
	int equipFilter_ = -1;
	std::vector<int> visibleRoomIndices_;
	int selectedInventoryIndex_ = -1;
	int selectedSkillIndex_ = -1;
	bool completionMode_ = false;
	std::vector<Item*> inventoryEntryItems_;
	std::vector<std::string> inventoryEntryDescriptions_;
	int inventoryPage_ = 0;
	bool refreshingControls_ = false;
	bool handlingCommand_ = false;
	bool refreshPosted_ = false;
	bool combatItemSelection_ = false;
	bool combatDefeatRecovery_ = false;
	std::vector<std::string> combatItemNames_;
	std::vector<std::string> skillNames_;
	std::vector<std::string> equippedSkillNames_;
	int skillSortMode_ = 0;
	int skillFilter_ = 0;
	std::vector<Enemy*> targetEnemies_;
	std::string selectedCombatSkillName_;
	bool combatSkillSelection_ = false;
	bool combatAttackSelection_ = false;
	bool combatTargetConfirmed_ = false;
	bool combatTransition_ = false;
	std::uint64_t loggedCombatSequence_ = 0;
	std::deque<CombatManager::CombatEvent> pendingCombatEvents_;
	std::vector<CombatManager::CombatEvent> presentedCombatEvents_;
	std::vector<CombatManager::CombatEvent> savedCombatHistory_;
	std::vector<CombatManager::CombatEvent> defeatSnapshot_;
	std::string defeatCause_;
	bool defeatPresentationPending_ = false;
	bool victoryPresentationPending_ = false;
	View battleLogReturnView_ = View::Room;
	UINT combatPresentationDelayMs_ = 350;
	int presentedPlayerHP_ = 0;
	bool enemyTurnPending_ = false;
	void ScheduleEnemyTurn();
};
