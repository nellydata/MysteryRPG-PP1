#include "Win32UI.h"
#include "Automata.h"
#include "ContentCatalog.h"
#include "EnemyCatalog.h"
#include "Nun.h"
#include "ShimmerSkill.h"
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <array>
#include <cctype>
#include <tuple>
#include <memory>

namespace {
constexpr COLORREF Background = RGB(24, 24, 24);
constexpr COLORREF Panel = RGB(35, 35, 35);
constexpr COLORREF PanelEdge = RGB(108, 111, 105);
constexpr COLORREF Text = RGB(235, 235, 235);
constexpr COLORREF Muted = RGB(175, 175, 175);
constexpr COLORREF Brass = RGB(190, 157, 92);
constexpr COLORREF Verdigris = RGB(105, 157, 145);
constexpr COLORREF Shimmer = RGB(190, 222, 220);
constexpr COLORREF Danger = RGB(190, 76, 82);

unsigned RoomSeed(const std::string& roomName) {
	unsigned seed = 2166136261u;
	for (unsigned char character : roomName) {
		seed ^= character;
		seed *= 16777619u;
	}
	return seed;
}

bool IsSupportSkill(const std::string& effect) {
	return effect == "barrier" || effect == "heal_cleanse" || effect == "extra_turn" || effect == "stat_boost";
}

const std::vector<std::string>& BuiltInShimmerSkills() {
	static const std::vector<std::string> names = {
		"Pulsus Luminis", "Velum Radiāns", "Gradus Coruscus", "Lucerna Echo",
		"Fulgur Aurorae", "Flos Aeruginis", "Resonantia Chori", "Requiem Mechanicum",
		"Scutum Coruscum", "Nota Luminosa", "Revelatio Occulta", "Catena Aeruginis",
		"Onus Coruscum", "Lux Sanctorum", "Mutatio Horologii", "Ray of Mercy", "Prism Step", "Gleam Needle"
	};
	return names;
}
EnemyFaction RoomFaction(const std::string& roomName) {
	if (roomName == "Archive Room") return EnemyFaction::Automata;
	if (roomName == "Chapel") return EnemyFaction::Verdigris;
	return EnemyFaction::Devotee;
}

EnemyTier MaximumEncounterTier(int level) {
	if (level <= 1) return EnemyTier::Basic;
	if (level <= 3) return EnemyTier::Rare;
	if (level <= 5) return EnemyTier::Epic;
	return EnemyTier::Elite;
}

std::string ReplaceTokens(std::string value) {
	for (char& character : value) {
		if (character == '_') character = ' ';
	}
	return value;
}

COLORREF RarityColor(Rarity rarity) {
	switch (rarity) {
	case Rarity::Common: return RGB(225, 225, 225);
	case Rarity::Uncommon: return RGB(90, 190, 120);
	case Rarity::Rare: return RGB(90, 145, 230);
	case Rarity::Epic: return RGB(175, 100, 235);
	case Rarity::Legendary: return RGB(235, 175, 55);
	case Rarity::Artifact: return RGB(235, 75, 95);
	}
	return RGB(245, 245, 245);
}

COLORREF EnemyTierColor(EnemyTier tier) {
	switch (tier) {
	case EnemyTier::Basic: return RGB(225, 225, 225);
	case EnemyTier::Rare: return RGB(90, 145, 230);
	case EnemyTier::Epic: return RGB(175, 100, 235);
	case EnemyTier::Elite: return RGB(235, 175, 55);
	case EnemyTier::Boss: return RGB(235, 75, 95);
	}
	return Text;
}

int SpawnChance(EnemySpawnCondition condition) {
	switch (condition) {
	case EnemySpawnCondition::None: return 0;
	case EnemySpawnCondition::Low: return 20;
	case EnemySpawnCondition::Medium: return 50;
	case EnemySpawnCondition::High: return 80;
	}
	return 0;
}

void AddNamedItem(Player& player, const std::vector<ItemDefinition>& definitions,
				  const std::string& name) {
	for (const auto& definition : definitions) {
		if (definition.name == name) {
			player.AddItem(std::make_unique<Item>(definition));
			return;
		}
	}
}

std::string Lowercase(std::string value) {
	for (char& character : value) character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
	return value;
}

bool EnvironmentEquals(const char* name, const char* expected) {
	char* value = nullptr;
	std::size_t length = 0;
	if (_dupenv_s(&value, &length, name) != 0 || value == nullptr) return false;
	const bool result = std::string(value) == expected;
	std::free(value);
	return result;
}
}

void Win32UI::ChangeView(View nextView) {
	if (view_ == nextView) return;
	viewHistory_.push_back(view_);
	view_ = nextView;
}

void Win32UI::LeaveCombat() {
	KillTimer(window_, 1);
	enemyTurnPending_ = false;
	ResetCombatSelection();
	player_.ClearStatuses();
	if (activeEnemy_ != nullptr) activeEnemy_->ClearStatuses();
	activeEnemy_ = nullptr;
	combatTransition_ = false;
	combatDefeatRecovery_ = false;
	combat_.ClearEvents();
	if (selectedRoom_ >= 0 && selectedRoom_ < static_cast<int>(mapManager_.GetRooms().size())) {
		mapManager_.GetRooms()[selectedRoom_].ClearEnemies();
	}
}

void Win32UI::ResetCombatSelection() {
	combatAttackSelection_ = false;
	combatSkillSelection_ = false;
	combatItemSelection_ = false;
	combatTargetConfirmed_ = false;
	selectedCombatSkillName_.clear();
}

void Win32UI::ShowDefeatRecovery() {
	if (player_.isAlive() || combatDefeatRecovery_) return;
	AddLog(player_.getName() + " has fallen.");
	LeaveCombat();
	for (std::size_t index = 0; index < mapManager_.GetRooms().size(); ++index) {
		if (mapManager_.GetRooms()[index].getName() == "Fountain") {
			selectedRoom_ = static_cast<int>(index);
			break;
		}
	}
	player_.RestoreHealth(player_.getMaxHP());
	view_ = View::Room;
	viewHistory_.clear();
	statusMessage_ = "You awaken in the Fountain Room at full health.";
	AddLog("You return to the Fountain Room and recover to full health.");
}

std::wstring Win32UI::FormatDisplayText(const std::string& text) const {
	std::string formatted = ReplaceTokens(text);
	bool capitalize = true;
	for (char& character : formatted) {
		if (capitalize && std::isalpha(static_cast<unsigned char>(character))) {
			character = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
			capitalize = false;
		} else if (character == ' ' || character == '-' || character == '/') {
			capitalize = true;
		}
	}
	return ToWide(formatted);
}

Win32UI::Win32UI()
	: window_(nullptr), nameWindow_(nullptr), nameEdit_(nullptr), controls_{}, roomButtons_{}, inspectButtons_{}, font_(nullptr), headingFont_(nullptr), statusFont_(nullptr),
	  view_(View::Floor), selectedRoom_(0), player_("Adelaide", 20, 10),
	  mapManager_(), combat_(), progression_(), activeEnemy_(nullptr) {
	InitializePlayer();
	AddLog("The cathedral is silent except for the pulse of the Shimmer.");
}

Win32UI::~Win32UI() {
	if (font_ != nullptr) DeleteObject(font_);
	if (headingFont_ != nullptr) DeleteObject(headingFont_);
	if (statusFont_ != nullptr) DeleteObject(statusFont_);
}

bool Win32UI::ConsoleModeRequested() {
	return EnvironmentEquals("MICARE_CONSOLE_MODE", "1");
}

int Win32UI::Run() {
	HINSTANCE instance = GetModuleHandleW(nullptr);
	PromptForName(instance);
	if (completionMode_) InitializeCompletionState();
	CreateWindowShell(instance);
	if (window_ == nullptr) return 1;

	MSG message{};
	while (GetMessageW(&message, nullptr, 0, 0) > 0) {
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}
	return static_cast<int>(message.wParam);
}

void Win32UI::PromptForName(HINSTANCE instance) {
	const wchar_t* className = L"MicareNamePrompt";
	WNDCLASSW promptClass{};
	promptClass.hInstance = instance;
	promptClass.lpfnWndProc = NameProcedure;
	promptClass.lpszClassName = className;
	promptClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	promptClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
	RegisterClassW(&promptClass);

	nameWindow_ = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_TOPMOST, className,
								  L"Micare - Cathedral of Lies", WS_CAPTION | WS_SYSMENU,
								  CW_USEDEFAULT, CW_USEDEFAULT, 480, 190, nullptr, nullptr,
								  instance, this);
	if (nameWindow_ == nullptr) {
		player_.SetName("Adelaide");
		completionMode_ = true;
		return;
	}
	ShowWindow(nameWindow_, SW_SHOW);
	UpdateWindow(nameWindow_);

	MSG message{};
	while (IsWindow(nameWindow_) && GetMessageW(&message, nullptr, 0, 0) > 0) {
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}
}

void Win32UI::CreateWindowShell(HINSTANCE instance) {
	const wchar_t* className = L"MicareWin32Window";
	WNDCLASSW windowClass{};
	windowClass.hInstance = instance;
	windowClass.lpfnWndProc = WindowProcedure;
	windowClass.lpszClassName = className;
	windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	windowClass.hbrBackground = CreateSolidBrush(Background);
	RegisterClassW(&windowClass);

	window_ = CreateWindowExW(0, className, L"Micare - Cathedral of Lies",
							  WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
							  1280, 820, nullptr, nullptr, instance, this);
}

void Win32UI::CreateControls() {
	font_ = CreateFontW(21, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
						OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
						DEFAULT_PITCH | FF_DONTCARE, L"IM FELL English");
	headingFont_ = CreateFontW(26, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
							   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
														   CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"IM FELL English");
	statusFont_ = CreateFontW(17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Emoji");

	auto createButton = [&](int index, int id, const wchar_t* text) {
		const DWORD style = WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON | BS_OWNERDRAW;
		HWND button = CreateWindowW(L"BUTTON", text, style,
										 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(id),
										 GetModuleHandleW(nullptr), nullptr);
		if (index >= 0) controls_[index] = button;
		SendMessageW(button, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
		return button;
	};

	for (int i = 0; i < 5; ++i) roomButtons_[i] = createButton(-1, RoomButtonBase + i, L"Room");
	for (int i = 0; i < 5; ++i) inspectButtons_[i] = createButton(-1, InspectButtonBase + i, L"Inspect");
	for (int i = 0; i < 12; ++i) createButton(8 + i, InventoryButtonBase + i, L"Item");
	for (int i = 0; i < 15; ++i) createButton(36 + i, SkillButtonBase + i, L"Skill");
	controls_[25] = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
		0, 0, 0, 0, window_, reinterpret_cast<HMENU>(InventorySearch), GetModuleHandleW(nullptr), nullptr);
	SendMessageW(controls_[25], WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
	createButton(20, ButtonSearch, L"Investigate");
	createButton(21, ButtonBack, L"Back");
	createButton(22, ButtonMap, L"Map");
	createButton(23, ButtonSkills, L"Skills");
	createButton(24, ButtonAttack, L"ATTACK");
	createButton(26, ButtonShimmer, L"SHIMMER");
	createButton(27, ButtonFlee, L"FLEE");
	createButton(28, ButtonQuit, L"Quit");
	createButton(29, ButtonInventory, L"Inventory");
	createButton(30, ButtonSort, L"Sort / filter");
	createButton(31, ButtonItem, L"ITEM");
	createButton(32, ButtonDiscard, L"Discard");
	createButton(33, ButtonEquip, L"Use");
	createButton(34, ButtonInventoryPrevious, L"<");
	createButton(35, ButtonInventoryNext, L">");
	RefreshControls();
}

void Win32UI::RefreshControls() {
	if (controls_[20] == nullptr || refreshingControls_) return;
	refreshingControls_ = true;
	for (HWND control : controls_) if (control != nullptr) ShowWindow(control, SW_HIDE);
	for (HWND control : roomButtons_) if (control != nullptr) ShowWindow(control, SW_HIDE);
	for (HWND control : inspectButtons_) if (control != nullptr) ShowWindow(control, SW_HIDE);
	EnableWindow(controls_[24], TRUE);
	EnableWindow(controls_[31], TRUE);
	EnableWindow(controls_[26], TRUE);
	EnableWindow(controls_[27], TRUE);
	RECT client{};
	GetClientRect(window_, &client);
	LayoutControls(client.right, client.bottom);

	if (view_ == View::Map) {
		const auto& rooms = mapManager_.GetRooms();
		visibleRoomIndices_.clear();
		for (std::size_t i = 0; i < rooms.size() && visibleRoomIndices_.size() < 5; ++i) {
			if (!rooms[i].IsUnlocked()) continue;
			visibleRoomIndices_.push_back(static_cast<int>(i));
			SetWindowTextW(roomButtons_[visibleRoomIndices_.size() - 1], ToWide(rooms[i].getName()).c_str());
			ShowWindow(roomButtons_[visibleRoomIndices_.size() - 1], SW_SHOW);
		}
		ShowWindow(controls_[22], SW_SHOW);
		ShowWindow(controls_[29], SW_SHOW);
		ShowWindow(controls_[23], SW_SHOW);
		ShowWindow(controls_[28], SW_SHOW);
		ShowWindow(controls_[21], SW_SHOW);
	} else if (view_ == View::Floor) {
		ShowWindow(controls_[22], SW_SHOW);
		ShowWindow(controls_[29], SW_SHOW);
		ShowWindow(controls_[23], SW_SHOW);
		ShowWindow(controls_[28], SW_SHOW);
	} else if (view_ == View::Room && selectedRoom_ >= 0) {
		if (selectedRoom_ >= static_cast<int>(mapManager_.GetRooms().size())) {
			view_ = View::Floor;
		} else if (activeEnemy_ != nullptr && (combatTransition_ || !combat_.IsPlayerTurn())) {
			ShowWindow(controls_[21], SW_HIDE);
		} else {
			const auto& interactables = mapManager_.GetRooms()[selectedRoom_].GetVisibleInteractables();
			for (std::size_t i = 0; i < interactables.size() && i < 5; ++i) {
				SetWindowTextW(inspectButtons_[i], ToWide(interactables[i]).c_str());
				ShowWindow(inspectButtons_[i], SW_SHOW);
			}
			ShowWindow(controls_[20], SW_SHOW);
			ShowWindow(controls_[21], SW_SHOW);
			ShowWindow(controls_[22], SW_SHOW);
			ShowWindow(controls_[29], SW_SHOW);
		}
	} else if (view_ == View::Inventory) {
		inventoryEntryItems_.clear();
		inventoryEntryDescriptions_.clear();
		const auto items = player_.GetInventory().GetSortedItems(inventorySort_, inventoryFocusStat_);
		for (const auto* item : items) {
			if (inventoryFilter_ >= 0 && static_cast<int>(item->getCategory()) != inventoryFilter_) continue;
			if (equipFilter_ >= 0 && (item->getCategory() != ItemCategory::Equippable || static_cast<int>(item->getSlot()) != equipFilter_)) continue;
			const std::string query = Lowercase(inventorySearchQuery_);
			if (!query.empty()) {
				const std::string searchable = Lowercase(item->getName() + " " + item->getDescription() + " " + item->getTheme() + " " + item->getPassive());
				if (searchable.find(query) == std::string::npos) continue;
			}
			inventoryEntryItems_.push_back(const_cast<Item*>(item));
			inventoryEntryDescriptions_.push_back(item->getDescription());
		}
		if (inventoryFilter_ < 0 || inventoryFilter_ == static_cast<int>(ItemCategory::Clue)) {
			for (const auto& clue : player_.GetInventory().GetClues()) {
				inventoryEntryItems_.push_back(nullptr);
				inventoryEntryDescriptions_.push_back(clue.description);
			}
		}
		const int pageSize = 12;
		const int pageCount = (static_cast<int>(inventoryEntryItems_.size()) + pageSize - 1) / pageSize;
		if (inventoryPage_ >= pageCount && pageCount > 0) inventoryPage_ = pageCount - 1;
		const int firstEntry = inventoryPage_ * pageSize;
		for (int button = 0; button < pageSize; ++button) {
			const int entry = firstEntry + button;
			if (entry < static_cast<int>(inventoryEntryItems_.size())) {
				const Item* item = inventoryEntryItems_[entry];
				const std::string label = item == nullptr ? "Clue" : item->getName() +
					(player_.GetInventory().GetQuantity(item->getName()) > 1
						? " x" + std::to_string(player_.GetInventory().GetQuantity(item->getName())) : "");
				SetWindowTextW(controls_[8 + button], FormatDisplayText(label).c_str());
				ShowWindow(controls_[8 + button], SW_SHOW);
			}
		}
		ShowWindow(controls_[21], SW_SHOW);
		ShowWindow(controls_[30], SW_SHOW);
		ShowWindow(controls_[25], SW_SHOW);
		ShowWindow(controls_[32], SW_SHOW);
		ShowWindow(controls_[33], SW_SHOW);
		ShowWindow(controls_[34], SW_SHOW);
		ShowWindow(controls_[35], SW_SHOW);
	} else if (view_ == View::Skills) {
		skillNames_.clear();
		for (const auto& name : progression_.GetUnlockedShimmerSkills()) {
			auto skill = ShimmerSkill::Create(name);
			if (skill == nullptr) continue;
			const bool support = IsSupportSkill(skill->getEffect());
			if ((skillFilter_ == 1 && support) || (skillFilter_ == 2 && !support)) continue;
			skillNames_.push_back(name);
		}
		std::sort(skillNames_.begin(), skillNames_.end(), [&](const std::string& left, const std::string& right) {
			if (skillSortMode_ == 1) return ShimmerSkill::Create(left)->getCost() < ShimmerSkill::Create(right)->getCost();
			if (skillSortMode_ == 2) return ShimmerSkill::Create(left)->getCost() > ShimmerSkill::Create(right)->getCost();
			return skillSortMode_ == 3 ? left > right : left < right;
		});
		if (selectedSkillIndex_ >= static_cast<int>(skillNames_.size())) selectedSkillIndex_ = -1;
		for (std::size_t index = 0; index < skillNames_.size() && index < 15; ++index) {
			SetWindowTextW(controls_[36 + index], FormatDisplayText(skillNames_[index]).c_str());
			ShowWindow(controls_[36 + index], SW_SHOW);
		}
		ShowWindow(controls_[21], SW_SHOW);
		ShowWindow(controls_[33], SW_SHOW);
		ShowWindow(controls_[30], SW_SHOW);
		SetWindowTextW(controls_[33], L"Equip / Unequip");
		SetWindowTextW(controls_[30], L"Sort / filter");
	} else if (view_ == View::Combat) {
			if (!combatDefeatRecovery_ && !combatTransition_ && !combat_.IsPlayerTurn() && activeEnemy_ != nullptr && activeEnemy_->isAlive() && player_.isAlive()) ScheduleEnemyTurn();
		if (combatDefeatRecovery_) {
			SetWindowTextW(controls_[24], L"RESPAWN (10% HP)");
			SetWindowTextW(controls_[31], L"END BATTLE (FULL HP)");
			ShowWindow(controls_[24], SW_SHOW);
			ShowWindow(controls_[31], SW_SHOW);
		} else if (combatTransition_ || !combat_.IsPlayerTurn()) {
		} else if (combatItemSelection_) {
			combatItemNames_.clear();
			for (const auto& item : player_.GetInventory().GetItems()) {
				if (item->getCategory() != ItemCategory::Consumable ||
					player_.GetInventory().GetQuantity(item->getName()) <= 0) continue;
				bool duplicate = false;
				for (const auto& name : combatItemNames_) if (name == item->getName()) duplicate = true;
				if (!duplicate && combatItemNames_.size() < 12) combatItemNames_.push_back(item->getName());
			}
			for (std::size_t index = 0; index < combatItemNames_.size(); ++index) {
				const Item* item = player_.FindItem(combatItemNames_[index]);
				if (item == nullptr) continue;
				SetWindowTextW(controls_[8 + index],
					(FormatDisplayText(item->getName()) + L"\n" + ToWide(CompactConsumableSummary(*item))).c_str());
				ShowWindow(controls_[8 + index], SW_SHOW);
			}
			ShowWindow(controls_[21], SW_SHOW);
		} else if (combatSkillSelection_) {
			equippedSkillNames_ = progression_.GetEquippedShimmerSkills();
			for (std::size_t index = 0; index < equippedSkillNames_.size() && index < 12; ++index) {
				SetWindowTextW(controls_[36 + index], FormatDisplayText(equippedSkillNames_[index]).c_str());
			auto skill = ShimmerSkill::Create(equippedSkillNames_[index]);
			if (skill != nullptr) SetWindowTextW(controls_[36 + index], (FormatDisplayText(equippedSkillNames_[index]) + L"\n" + FormatDisplayText(skill->getDescription())).c_str());
				ShowWindow(controls_[36 + index], SW_SHOW);
			}
			SetWindowTextW(controls_[21], L"CANCEL");
			ShowWindow(controls_[21], SW_SHOW);
		} else {
			targetEnemies_.clear();
			if (selectedRoom_ >= 0 && selectedRoom_ < static_cast<int>(mapManager_.GetRooms().size())) {
				for (const auto& enemy : mapManager_.GetRooms()[selectedRoom_].GetEnemies()) {
					if (enemy != nullptr && enemy->isAlive() && targetEnemies_.size() < 3) targetEnemies_.push_back(enemy.get());
				}
			}
			if (combatAttackSelection_ || !selectedCombatSkillName_.empty()) {
				for (std::size_t index = 0; index < targetEnemies_.size(); ++index) {
					const bool selected = combatTargetConfirmed_ && targetEnemies_[index] == activeEnemy_;
					const Enemy* enemy = targetEnemies_[index];
					const std::wstring label = (selected ? L"SELECTED: " : L"TARGET: ") + FormatDisplayText(enemy->getName());
					SetWindowTextW(controls_[8 + index], label.c_str());
					ShowWindow(controls_[8 + index], SW_SHOW);
				}
			}
			if (!selectedCombatSkillName_.empty() || combatAttackSelection_) SetWindowTextW(controls_[21], L"CANCEL");
			if (targetEnemies_.size() == 1 && (combatAttackSelection_ || !selectedCombatSkillName_.empty())) {
				SetWindowTextW(controls_[8], (L"TARGET: " + FormatDisplayText(targetEnemies_[0]->getName())).c_str());
				ShowWindow(controls_[8], SW_SHOW);
			}
			SetWindowTextW(controls_[24], L"ATTACK");
			SetWindowTextW(controls_[31], L"ITEM");
			ShowWindow(controls_[24], SW_SHOW);
			ShowWindow(controls_[31], SW_SHOW);
			ShowWindow(controls_[26], SW_SHOW);
			ShowWindow(controls_[27], SW_SHOW);
			const BOOL canAct = (combat_.IsPlayerTurn() && player_.isAlive()) ? TRUE : FALSE;
			EnableWindow(controls_[24], canAct);
			EnableWindow(controls_[31], canAct);
			EnableWindow(controls_[26], canAct);
			EnableWindow(controls_[27], canAct);
		}
	} else {
			SetWindowTextW(controls_[21], L"Back");
		ShowWindow(controls_[21], SW_SHOW);
	}
	Invalidate();
	refreshingControls_ = false;
}

void Win32UI::LayoutControls(int width, int height) {
	const int center = 425;
	const int panelRight = (std::max)(center + 620, width - 35);
	const int buttonWidth = 180;
	const int buttonHeight = 42;
	const int eventTop = (std::max)(120, height - 380);
	const int actionY = eventTop + 28;
	const int roomActionY = actionY + 124;
	const int inventoryActionY = eventTop + 260;
	const int inventoryWidth = (std::max)(150, (panelRight - center - 36) / 4);
	const int inventoryGap = 12;
	if (view_ == View::Inventory) {
		const int left = (std::min)(395, (std::max)(20, width / 3));
		const int right = (std::max)(left + 240, width - 20);
		const int top = 20;
		const int bottom = height - 20;
		const int split = left + (right - left) * 56 / 100;
		const int detailLeft = split + 16;
		const int listWidth = split - left;
		const int detailWidth = right - detailLeft;
		const int gridGap = 12;
		const int gridWidth = (std::max)(50, (listWidth - 40 - gridGap * 3) / 4);
		const int itemButtonHeight = 58;
		MoveWindow(controls_[25], left + 20, top + 48, listWidth - 40, 38, TRUE);
		MoveWindow(controls_[30], left + 20, top + 92, listWidth - 40, 40, TRUE);
		for (int i = 0; i < 12; ++i) {
			MoveWindow(controls_[8 + i], left + 20 + (i % 4) * (gridWidth + gridGap),
				top + 150 + (i / 4) * (itemButtonHeight + gridGap), gridWidth, itemButtonHeight, TRUE);
		}
		const int actionWidth = (detailWidth - 40 - gridGap * 2) / 3;
		const int actionY = bottom - 112;
		MoveWindow(controls_[33], detailLeft + 20, actionY, actionWidth, 44, TRUE);
		MoveWindow(controls_[32], detailLeft + 20 + actionWidth + gridGap, actionY, actionWidth, 44, TRUE);
		MoveWindow(controls_[21], detailLeft + 20 + (actionWidth + gridGap) * 2, actionY, actionWidth, 44, TRUE);
		MoveWindow(controls_[34], detailLeft + 20, bottom - 58, 70, 40, TRUE);
		MoveWindow(controls_[35], right - 90, bottom - 58, 70, 40, TRUE);
		return;
	}
	if (view_ == View::Combat && (combatItemSelection_ || combatSkillSelection_)) {
		const int combatButtonWidth = (std::max)(120, (panelRight - center - 48) / 5);
		const int combatButtonGap = 10;
		const int combatButtonY = actionY;
		for (int i = 0; i < 12; ++i) {
			MoveWindow(controls_[8 + i], center + (i % 5) * (combatButtonWidth + combatButtonGap),
				combatButtonY + (i / 5) * 50, combatButtonWidth, 42, TRUE);
			MoveWindow(controls_[36 + i], center + (i % 5) * (combatButtonWidth + combatButtonGap),
				combatButtonY + (i / 5) * 50, combatButtonWidth, 42, TRUE);
		}
		MoveWindow(controls_[21], center, combatButtonY + 160, 180, 42, TRUE);
		return;
	}
	if (view_ == View::Combat) {
		const int combatButtonWidth = (std::max)(130, (panelRight - center - 48) / 4);
		const int combatButtonY = height - 112;
		for (int i = 0; i < 12; ++i)
			MoveWindow(controls_[8 + i], center + (i % 4) * (combatButtonWidth + 10), actionY, combatButtonWidth, 52, TRUE);
		MoveWindow(controls_[24], center, combatButtonY, combatButtonWidth, 44, TRUE);
		MoveWindow(controls_[31], center + combatButtonWidth + 10, combatButtonY, combatButtonWidth, 44, TRUE);
		MoveWindow(controls_[26], center + (combatButtonWidth + 10) * 2, combatButtonY, combatButtonWidth, 44, TRUE);
		MoveWindow(controls_[27], center + (combatButtonWidth + 10) * 3, combatButtonY, combatButtonWidth, 44, TRUE);
	}
	if (view_ == View::Skills) {
		const int left = (std::min)(395, (std::max)(20, width / 3));
		const int right = (std::max)(left + 240, width - 20);
		const int top = 20;
		const int bottom = height - 20;
		const int split = left + (right - left) * 48 / 100;
		const int detailLeft = split + 16;
		const int listWidth = split - left;
		const int gridGap = 10;
		const int gridWidth = (std::max)(90, (listWidth - 40 - gridGap) / 2);
		const int skillButtonHeight = 54;
		for (int i = 0; i < 15; ++i) {
			MoveWindow(controls_[36 + i], left + 20 + (i % 2) * (gridWidth + gridGap),
				top + 125 + (i / 2) * (skillButtonHeight + gridGap), gridWidth, skillButtonHeight, TRUE);
		}
		const int actionWidth = (right - detailLeft - 40 - gridGap * 2) / 3;
		const int actionY = bottom - 64;
		MoveWindow(controls_[33], detailLeft + 20, actionY, actionWidth, 44, TRUE);
		MoveWindow(controls_[30], detailLeft + 20 + actionWidth + gridGap, actionY, actionWidth, 44, TRUE);
		MoveWindow(controls_[21], detailLeft + 20 + (actionWidth + gridGap) * 2, actionY, actionWidth, 44, TRUE);
		return;
	}

	for (int i = 0; i < 5; ++i) {
		MoveWindow(roomButtons_[i], center + (i % 2) * 225, actionY + (i / 2) * 50,
				   buttonWidth, buttonHeight, TRUE);
	}
	for (int i = 0; i < 5; ++i) {
		const int column = i % 2;
		const int row = i / 2;
		MoveWindow(inspectButtons_[i], center + column * (inventoryWidth * 2 + inventoryGap * 2),
				   actionY + row * 54, inventoryWidth * 2 + inventoryGap, buttonHeight, TRUE);
	}
	for (int i = 0; i < 12; ++i) {
		MoveWindow(controls_[8 + i], center + (i % 4) * (inventoryWidth + inventoryGap),
					132 + (i / 4) * 68, inventoryWidth, 58, TRUE);
		MoveWindow(controls_[36 + i], center + (i % 4) * (inventoryWidth + inventoryGap),
					132 + (i / 4) * 68, inventoryWidth, 58, TRUE);
	}
	MoveWindow(controls_[20], center, roomActionY, buttonWidth, buttonHeight, TRUE);
	MoveWindow(controls_[21], center + buttonWidth + 20, roomActionY, buttonWidth, buttonHeight, TRUE);
	MoveWindow(controls_[22], panelRight - 180, 40, 180, buttonHeight, TRUE);
	MoveWindow(controls_[29], panelRight - 180, 92, 180, buttonHeight, TRUE);
	MoveWindow(controls_[23], panelRight - 180, 144, 180, buttonHeight, TRUE);
	MoveWindow(controls_[28], panelRight - 180, 196, 180, buttonHeight, TRUE);
	MoveWindow(controls_[24], center, actionY, 180, 44, TRUE);
	MoveWindow(controls_[31], center + 190, actionY, 180, 44, TRUE);
	MoveWindow(controls_[26], center + 380, actionY, 180, 44, TRUE);
	MoveWindow(controls_[27], center + 570, actionY, 180, 44, TRUE);
	MoveWindow(controls_[33], center, inventoryActionY, inventoryWidth, 42, TRUE);
	MoveWindow(controls_[32], center + inventoryWidth + inventoryGap, inventoryActionY, inventoryWidth, 42, TRUE);
	MoveWindow(controls_[21], center + (inventoryWidth + inventoryGap) * 2, inventoryActionY, inventoryWidth, 42, TRUE);
	MoveWindow(controls_[34], center, inventoryActionY + 50, 70, 38, TRUE);
	MoveWindow(controls_[35], panelRight - 70, inventoryActionY + 50, 70, 38, TRUE);
}

void Win32UI::ShowInventoryMenu() {
	HMENU menu = CreatePopupMenu();
	HMENU sortMenu = CreatePopupMenu();
	HMENU statMenu = CreatePopupMenu();
	HMENU filterMenu = CreatePopupMenu();
	AppendMenuW(sortMenu, MF_STRING, SortModeBase + 0, L"A-Z");
	AppendMenuW(sortMenu, MF_STRING, SortModeBase + 1, L"Z-A");
	AppendMenuW(sortMenu, MF_STRING, SortModeBase + 2, L"Rarity: low to high");
	AppendMenuW(sortMenu, MF_STRING, SortModeBase + 3, L"Rarity: high to low");
	AppendMenuW(sortMenu, MF_STRING, SortModeBase + 4, L"Most recently obtained");
	AppendMenuW(sortMenu, MF_STRING, SortModeBase + 5, L"Least recently obtained");
	const std::pair<int, const wchar_t*> statOptions[] = {
		{SortModeBase + 10, L"Highest HP"}, {SortModeBase + 11, L"Highest ATK"},
		{SortModeBase + 12, L"Highest DEF"}, {SortModeBase + 13, L"Highest SPD"},
		{SortModeBase + 14, L"Highest Accuracy"}, {SortModeBase + 15, L"Highest Resistance"},
		{SortModeBase + 16, L"Highest Crit Rate"}, {SortModeBase + 17, L"Highest Evasion"}
	};
	for (const auto& option : statOptions) AppendMenuW(statMenu, MF_STRING, option.first, option.second);
	AppendMenuW(sortMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(statMenu), L"Highest selected stat");
	AppendMenuW(filterMenu, MF_STRING, FilterModeBase + 0, L"All items");
	AppendMenuW(filterMenu, MF_STRING, FilterModeBase + 1, L"Equipment");
	AppendMenuW(filterMenu, MF_STRING, FilterModeBase + 2, L"Consumables");
	AppendMenuW(filterMenu, MF_STRING, FilterModeBase + 3, L"Clues");
	AppendMenuW(filterMenu, MF_STRING, FilterModeBase + 4, L"Crux items");
	HMENU equipMenu = CreatePopupMenu();
	AppendMenuW(equipMenu, MF_STRING, EquipFilterBase, L"All equipment");
	AppendMenuW(equipMenu, MF_STRING, EquipFilterBase + 1, L"Head");
	AppendMenuW(equipMenu, MF_STRING, EquipFilterBase + 2, L"Torso");
	AppendMenuW(equipMenu, MF_STRING, EquipFilterBase + 3, L"Boots");
	AppendMenuW(equipMenu, MF_STRING, EquipFilterBase + 4, L"Hands");
	AppendMenuW(equipMenu, MF_STRING, EquipFilterBase + 5, L"Accessory");
	AppendMenuW(filterMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(equipMenu), L"Equipment slot");
	AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(sortMenu), L"Sort by");
	AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(filterMenu), L"Filter by type");
	POINT cursor{};
	GetCursorPos(&cursor);
	TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN, cursor.x, cursor.y, 0, window_, nullptr);
	DestroyMenu(menu);
}

void Win32UI::ShowSkillMenu() {
	HMENU menu = CreatePopupMenu();
	HMENU sortMenu = CreatePopupMenu();
	HMENU filterMenu = CreatePopupMenu();
	AppendMenuW(sortMenu, MF_STRING, SkillSortBase + 0, L"A-Z");
	AppendMenuW(sortMenu, MF_STRING, SkillSortBase + 1, L"Cost: low to high");
	AppendMenuW(sortMenu, MF_STRING, SkillSortBase + 2, L"Cost: high to low");
	AppendMenuW(sortMenu, MF_STRING, SkillSortBase + 3, L"Z-A");
	AppendMenuW(filterMenu, MF_STRING, SkillFilterBase + 0, L"All skills");
	AppendMenuW(filterMenu, MF_STRING, SkillFilterBase + 1, L"Support");
	AppendMenuW(filterMenu, MF_STRING, SkillFilterBase + 2, L"Offense");
	AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(sortMenu), L"Sort by");
	AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(filterMenu), L"Filter by type");
	POINT cursor{};
	GetCursorPos(&cursor);
	TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN, cursor.x, cursor.y, 0, window_, nullptr);
	DestroyMenu(menu);
}

void Win32UI::ShowCombatItems() {
	if (activeEnemy_ == nullptr || !activeEnemy_->isAlive()) return;
	HMENU menu = CreatePopupMenu();
	if (menu == nullptr) return;

	std::vector<std::string> consumables;
	for (const auto& item : player_.GetInventory().GetItems()) {
		if (item->getCategory() != ItemCategory::Consumable) continue;
		if (player_.GetInventory().GetQuantity(item->getName()) <= 0) continue;
		bool duplicate = false;
		for (const auto& name : consumables) {
			if (name == item->getName()) {
				duplicate = true;
				break;
			}
		}
		if (duplicate) continue;
		consumables.push_back(item->getName());
		AppendMenuW(menu, MF_STRING, 700 + static_cast<UINT>(consumables.size() - 1),
			FormatDisplayText(item->getName()).c_str());
	}

	if (consumables.empty()) {
		AppendMenuW(menu, MF_GRAYED, 0, L"No consumables available");
	} else if (combat_.HasUsedConsumableThisTurn()) {
		DestroyMenu(menu);
		menu = CreatePopupMenu();
		if (menu == nullptr) return;
		AppendMenuW(menu, MF_GRAYED, 0, L"One consumable already used this turn");
	}

	POINT cursor{};
	GetCursorPos(&cursor);
	const UINT command = TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD,
		cursor.x, cursor.y, 0, window_, nullptr);
	DestroyMenu(menu);

	if (command < 700 || command >= 700 + static_cast<UINT>(consumables.size())) return;
	if (combat_.UseConsumable(consumables[command - 700], player_, *activeEnemy_)) {
		statusMessage_ = "Consumable used without ending the turn.";
		AddLog("Combat: consumable used.");
	} else {
		statusMessage_ = "That consumable is unavailable.";
		AddLog("Combat: consumable use failed.");
	}
	if (!refreshPosted_) {
		refreshPosted_ = true;
		PostMessageW(window_, RefreshControlsMessage, 0, 0);
	}
}

std::string Win32UI::CompactConsumableSummary(const Item& item) const {
	const std::string& effect = item.getEffect();
	if (effect.find("restore_20") != std::string::npos) return "+20% HP";
	if (effect.find("restore_35") != std::string::npos) return "+35% HP";
	if (effect.find("restore_50") != std::string::npos) return "+50% HP";
	if (effect == "remove_all_debuffs") return "Cleanse all debuffs";
	if (effect == "remove_burn_and_poison") return "Remove Burn and Poison";
	if (effect.find("deal_flat_50") != std::string::npos) return "Inflict 50 damage";
	if (effect.find("apply_burn") != std::string::npos) return "Inflict Burn for 3 turns";
	return item.getDescription();
}

void Win32UI::HandleCommand(int controlId) {
	if (handlingCommand_) return;
	handlingCommand_ = true;
	const bool combatActionControl = controlId == ButtonAttack || controlId == ButtonItem ||
		controlId == ButtonShimmer || controlId == ButtonFlee ||
		(controlId >= InventoryButtonBase && controlId < InventoryButtonBase + 12) ||
		(controlId >= SkillButtonBase && controlId < SkillButtonBase + 15);
	if (view_ == View::Combat && combatActionControl && !combat_.IsPlayerTurn()) {
		handlingCommand_ = false;
		return;
	}
	if (controlId >= SortModeBase && controlId < FilterModeBase) {
		if (controlId == SortModeBase + 0) inventorySort_ = InventorySort::AlphabeticalAscending;
		else if (controlId == SortModeBase + 1) inventorySort_ = InventorySort::AlphabeticalDescending;
		else if (controlId == SortModeBase + 2) inventorySort_ = InventorySort::RarityAscending;
		else if (controlId == SortModeBase + 3) inventorySort_ = InventorySort::RarityDescending;
		else if (controlId == SortModeBase + 4) inventorySort_ = InventorySort::RecentlyObtained;
		else if (controlId == SortModeBase + 5) inventorySort_ = InventorySort::LeastRecentlyObtained;
		else {
			inventorySort_ = InventorySort::HighestStat;
			switch (controlId) {
			case SortModeBase + 10: inventoryFocusStat_ = ItemStat::HP; break;
			case SortModeBase + 11: inventoryFocusStat_ = ItemStat::ATK; break;
			case SortModeBase + 12: inventoryFocusStat_ = ItemStat::DEF; break;
			case SortModeBase + 13: inventoryFocusStat_ = ItemStat::SPD; break;
			case SortModeBase + 14: inventoryFocusStat_ = ItemStat::Accuracy; break;
			case SortModeBase + 15: inventoryFocusStat_ = ItemStat::Resistance; break;
			case SortModeBase + 16: inventoryFocusStat_ = ItemStat::CritRate; break;
			case SortModeBase + 17: inventoryFocusStat_ = ItemStat::Evasion; break;
			}
		}
		selectedInventoryIndex_ = -1;
		inventoryPage_ = 0;
	} else if (controlId >= EquipFilterBase && controlId < EquipFilterBase + 6) {
		equipFilter_ = controlId == EquipFilterBase ? -1 : controlId - EquipFilterBase - 1;
		inventoryPage_ = 0;
	} else if (controlId >= SkillSortBase && controlId < SkillSortBase + 4) {
		if (view_ == View::Skills) {
			skillSortMode_ = controlId - SkillSortBase;
			selectedSkillIndex_ = -1;
		}
	} else if (controlId >= SkillFilterBase && controlId < SkillFilterBase + 3) {
		if (view_ == View::Skills) {
			skillFilter_ = controlId - SkillFilterBase;
			selectedSkillIndex_ = -1;
		}
	} else if (controlId >= FilterModeBase && controlId < FilterModeBase + 5) {
		inventoryFilter_ = controlId == FilterModeBase ? -1 : controlId - FilterModeBase - 1;
		selectedInventoryIndex_ = -1;
		inventoryPage_ = 0;
	} else if (controlId >= RoomButtonBase && controlId < RoomButtonBase + 5) {
		const int buttonIndex = controlId - RoomButtonBase;
		if (buttonIndex < static_cast<int>(visibleRoomIndices_.size())) EnterRoom(visibleRoomIndices_[buttonIndex]);
	} else if (controlId >= InspectButtonBase && controlId < InspectButtonBase + 5) {
		if (view_ == View::Room && selectedRoom_ >= 0 &&
			selectedRoom_ < static_cast<int>(mapManager_.GetRooms().size())) {
			const auto& interactables = mapManager_.GetRooms()[selectedRoom_].GetVisibleInteractables();
			const int index = controlId - InspectButtonBase;
			if (index < static_cast<int>(interactables.size())) Inspect(index);
		}
	} else if (controlId >= InventoryButtonBase && controlId < InventoryButtonBase + 12) {
		if (view_ == View::Combat && combatItemSelection_ && combat_.IsPlayerTurn()) {
			const int index = controlId - InventoryButtonBase;
			if (index < static_cast<int>(combatItemNames_.size())) {
				if (combat_.UseConsumable(combatItemNames_[index], player_, *activeEnemy_))
					statusMessage_ = "Consumable used without ending the turn.";
				else statusMessage_ = "That consumable is unavailable.";
				combatItemSelection_ = false;
			}
		} else if (view_ == View::Combat) {
			if (!combat_.IsPlayerTurn()) {
				statusMessage_ = "Wait for your turn.";
			} else {
				const int targetIndex = controlId - InventoryButtonBase;
				if (targetIndex < static_cast<int>(targetEnemies_.size())) {
					Enemy* target = targetEnemies_[targetIndex];
					if (combatAttackSelection_ || !selectedCombatSkillName_.empty()) {
						activeEnemy_ = target;
						combatTargetConfirmed_ = true;
						combat_.RecordInfo(selectedCombatSkillName_.empty()
							? player_.getName() + " attacks " + activeEnemy_->getName() + "."
							: selectedCombatSkillName_ + " targets " + activeEnemy_->getName() + ".");
						combatAttackSelection_ = false;
						PerformCombatAction(selectedCombatSkillName_.empty() ? CombatAction::Attack : CombatAction::Shimmer);
					} else {
						activeEnemy_ = target;
						statusMessage_ = activeEnemy_->getName() + " selected.";
					}
				}
			}
		} else {
		const int index = inventoryPage_ * 12 + controlId - InventoryButtonBase;
		if (index < static_cast<int>(inventoryEntryItems_.size())) {
			selectedInventoryIndex_ = index;
			statusMessage_ = inventoryEntryItems_[index] == nullptr ? "Clue selected." :
				inventoryEntryItems_[index]->getName() + " selected.";
		}
		}
	} else if (controlId >= SkillButtonBase && controlId < SkillButtonBase + 15) {
		const int index = controlId - SkillButtonBase;
		if (view_ == View::Skills && index < static_cast<int>(skillNames_.size())) {
			selectedSkillIndex_ = index;
			statusMessage_ = skillNames_[index] + " selected.";
		} else if (view_ == View::Combat && combatSkillSelection_ && index < static_cast<int>(equippedSkillNames_.size())) {
			selectedCombatSkillName_ = equippedSkillNames_[index];
			combatSkillSelection_ = false;
			auto selectedSkill = ShimmerSkill::Create(selectedCombatSkillName_);
			if (selectedSkill != nullptr && selectedSkill->getTargetType() != ShimmerSkill::TargetType::SingleEnemy) {
				statusMessage_ = selectedCombatSkillName_ + " selected. It resolves immediately.";
				combat_.RecordInfo(player_.getName() + " selected " + selectedCombatSkillName_ + ".");
				PerformCombatAction(CombatAction::Shimmer);
			} else {
				combatTargetConfirmed_ = false;
				const int cooldown = combat_.GetSkillCooldown(selectedCombatSkillName_);
				if (cooldown > 0) {
					statusMessage_ = selectedCombatSkillName_ + " is on cooldown for " + std::to_string(cooldown) + " more turn(s).";
					combat_.RecordInfo(selectedCombatSkillName_ + " is on cooldown for " + std::to_string(cooldown) + " more turn(s). Choose another skill or cancel.");
				} else {
					statusMessage_ = selectedCombatSkillName_ + " selected. Choose an enemy or cancel.";
					combat_.RecordInfo(selectedCombatSkillName_ + " selected. Choose an enemy or cancel.");
				}
			}
		}
	} else if (controlId == ButtonInventoryPrevious) {
		if (inventoryPage_ > 0) --inventoryPage_;
		selectedInventoryIndex_ = -1;
	} else if (controlId == ButtonInventoryNext) {
		const int pageCount = (static_cast<int>(inventoryEntryItems_.size()) + 11) / 12;
		if (inventoryPage_ + 1 < pageCount) ++inventoryPage_;
		selectedInventoryIndex_ = -1;
	} else if (controlId == ButtonSearch) {
		if (view_ == View::Room && player_.isAlive() && selectedRoom_ >= 0 &&
			selectedRoom_ < static_cast<int>(mapManager_.GetRooms().size())) SearchRoom();
	} else if (controlId == ButtonBack) {
		if (combatDefeatRecovery_) {
			handlingCommand_ = false;
			return;
		}
		if (view_ == View::Combat && combatItemSelection_) combatItemSelection_ = false;
		else if (view_ == View::Combat && combatSkillSelection_) {
			combatSkillSelection_ = false;
			selectedCombatSkillName_.clear();
			combatTargetConfirmed_ = false;
		}
		else if (view_ == View::Combat && (combatAttackSelection_ || !selectedCombatSkillName_.empty())) {
			combatAttackSelection_ = false;
			selectedCombatSkillName_.clear();
			combatTargetConfirmed_ = false;
			statusMessage_ = "Target selection canceled.";
		}
		else if (!viewHistory_.empty()) {
			view_ = viewHistory_.back();
			viewHistory_.pop_back();
		} else view_ = View::Floor;
	} else if (controlId == ButtonMap) {
		if (view_ != View::Combat && player_.isAlive() && !combatDefeatRecovery_) ChangeView(View::Map);
	} else if (controlId == ButtonInventory) {
		if (view_ != View::Combat && player_.isAlive() && !combatDefeatRecovery_) ChangeView(View::Inventory);
	} else if (controlId == ButtonSort) {
		if (view_ == View::Skills) ShowSkillMenu();
		else ShowInventoryMenu();
	} else if (controlId == ButtonDiscard) {
		if (selectedInventoryIndex_ >= 0 && selectedInventoryIndex_ < static_cast<int>(inventoryEntryItems_.size()) &&
			inventoryEntryItems_[selectedInventoryIndex_] != nullptr) {
			const std::string name = inventoryEntryItems_[selectedInventoryIndex_]->getName();
			if (player_.GetInventory().DiscardItem(name)) AddLog("Discarded " + name + ".");
			selectedInventoryIndex_ = -1;
		}
	} else if (controlId == ButtonEquip) {
		if (view_ == View::Skills && selectedSkillIndex_ >= 0 && selectedSkillIndex_ < static_cast<int>(skillNames_.size())) {
			if (activeEnemy_ != nullptr) {
				statusMessage_ = "Skills can only be equipped outside battle.";
				handlingCommand_ = false;
				return;
			}
			const std::string& skillName = skillNames_[selectedSkillIndex_];
			auto skill = ShimmerSkill::Create(skillName);
			if (skill != nullptr) {
				const auto equipped = progression_.GetEquippedShimmerSkills();
				const bool isEquipped = std::find(equipped.begin(), equipped.end(), skillName) != equipped.end();
				const bool changed = isEquipped ? progression_.UnequipShimmerSkill(skillName, skill->getCost()) : progression_.EquipShimmerSkill(skillName, skill->getCost());
				statusMessage_ = changed ? (isEquipped ? "Unequipped " : "Equipped ") + skillName + "." : "Not enough Shimmer points.";
			}
		} else if (selectedInventoryIndex_ >= 0 && selectedInventoryIndex_ < static_cast<int>(inventoryEntryItems_.size()) &&
			inventoryEntryItems_[selectedInventoryIndex_] != nullptr) {
			Item& item = *inventoryEntryItems_[selectedInventoryIndex_];
			if (item.isEquippable()) {
				const Item* equipped = player_.GetEquipped(item.getSlot());
				player_.ToggleEquip(item);
				AddLog(std::string(equipped == &item ? "Unequipped " : "Equipped ") + item.getName() + ".");
			} else if (item.getCategory() == ItemCategory::Consumable) {
				const std::string itemName = item.getName();
				const std::string itemDescription = item.getDescription();
				if (player_.UseConsumable(itemName)) {
					AddLog("Used " + itemName + ": " + itemDescription);
					statusMessage_ = itemDescription;
				}
				else AddLog("That consumable cannot be used right now.");
			} else if (item.getCategory() == ItemCategory::Crux) {
				const std::string itemName = item.getName();
				const std::string itemDescription = item.getDescription();
				if (progression_.UnlockCrux(itemName)) {
					player_.GetInventory().RemoveItem(itemName);
					AddLog("Crux activated: " + itemName + ". " + itemDescription);
					statusMessage_ = "Crux activated: " + itemDescription;
				} else AddLog("The conditions for that crux are not met.");
			} else {
				AddLog("That item has no usable action here.");
			}
		}
	} else if (controlId == ButtonSkills) {
		if (activeEnemy_ == nullptr) ChangeView(View::Skills);
	} else if (controlId == ButtonAttack) {
		if (combatDefeatRecovery_) {
			player_.RestoreHealth((std::max)(1, player_.getMaxHP() * 10 / 100));
			combatDefeatRecovery_ = false;
			statusMessage_ = "Respawned at the fountain with 10% HP. Continue the battle.";
			if (activeEnemy_ != nullptr && activeEnemy_->isAlive()) combat_.ResumeAfterRespawn(player_, *activeEnemy_);
			else ShowDefeatRecovery();
		} else if (player_.isAlive() && combat_.IsPlayerTurn()) {
			ResetCombatSelection();
			combatAttackSelection_ = true;
			combatTargetConfirmed_ = false;
			statusMessage_ = "Choose an enemy or cancel.";
		}
	} else if (controlId == ButtonItem) {
		if (combatDefeatRecovery_) {
			player_.heal(player_.getMaxHP());
			LeaveCombat();
			view_ = View::Floor;
			viewHistory_.clear();
			statusMessage_ = "Battle ended. You returned to the main floor with full HP.";
		} else if (player_.isAlive() && combat_.IsPlayerTurn()) {
			ResetCombatSelection();
			ShowCombatItems();
		}
	} else if (controlId == ButtonShimmer) {
		if (player_.isAlive() && activeEnemy_ != nullptr && combat_.IsPlayerTurn()) {
			ResetCombatSelection();
			equippedSkillNames_ = progression_.GetEquippedShimmerSkills();
			if (equippedSkillNames_.empty()) statusMessage_ = "No Shimmer skills are equipped.";
			else combatSkillSelection_ = true;
		}
	} else if (controlId == ButtonFlee) {
		if (player_.isAlive() && combat_.IsPlayerTurn()) PerformCombatAction(CombatAction::Flee);
	} else if (controlId == ButtonQuit) {
		PostMessageW(window_, WM_CLOSE, 0, 0);
		handlingCommand_ = false;
		return;
	}
	if (!refreshPosted_) {
		refreshPosted_ = true;
		PostMessageW(window_, RefreshControlsMessage, 0, 0);
	}
	handlingCommand_ = false;
}

void Win32UI::InitializeCompletionState() {
	progression_.SetMaxLevel(player_);
	for (const auto& skillName : BuiltInShimmerSkills()) progression_.UnlockShimmerSkill(skillName);
	for (auto& room : mapManager_.GetRooms()) room.SetCleared(true);
	for (const auto& definition : BuildEquippableCatalog()) player_.AddItem(std::make_unique<Item>(definition));
	for (const auto& definition : BuildConsumableCatalog()) player_.AddItem(std::make_unique<Item>(definition));
	for (const auto& definition : BuildClueCatalog()) player_.AddItem(std::make_unique<Item>(definition));
	for (const auto& definition : BuildCruxCatalog()) player_.AddItem(std::make_unique<Item>(definition));
	for (const auto& definition : BuildEquippableCatalog()) {
		if (definition.rarity == Rarity::Artifact) {
			if (Item* item = player_.FindItem(definition.name)) player_.ToggleEquip(*item);
		}
	}
}

void Win32UI::EnterRoom(int roomIndex) {
	if (!player_.isAlive()) {
		ShowDefeatRecovery();
		return;
	}
	if (roomIndex < 0 || roomIndex >= static_cast<int>(mapManager_.GetRooms().size())) return;
	selectedRoom_ = roomIndex;
	Area& area = mapManager_.GetRooms()[roomIndex];
	if (!area.IsUnlocked()) return;
	LeaveCombat();
	ChangeView(View::Room);
	if (area.getName() == "Fountain" && area.IsHealingUnlocked()) {
		const int recovered = player_.getMaxHP() - player_.getHP();
		player_.RestoreHealth(player_.getMaxHP());
		statusMessage_ = "The fountain restores " + std::to_string(recovered) + " HP. You are fully healed.";
		AddLog("Fountain: " + player_.getName() + " recovered to full health.");
	} else if (area.getName() == "Fountain") {
		statusMessage_ = "The fountain is sealed. Investigate it to find its guardian.";
	}
	AddLog("Entered " + area.getName() + ". Investigate the visible interactions.");
}

void Win32UI::SyncCombatEvents() {
	const auto& events = combat_.GetEvents();
	if (loggedCombatEvents_ > events.size()) loggedCombatEvents_ = 0;
	while (loggedCombatEvents_ < events.size()) {
		AddLog(events[loggedCombatEvents_].message);
		++loggedCombatEvents_;
	}
}

void Win32UI::ScheduleEnemyTurn() {
	if (enemyTurnPending_ || activeEnemy_ == nullptr || !activeEnemy_->isAlive() || !player_.isAlive()) return;
	enemyTurnPending_ = true;
	combatTransition_ = true;
	AddLog(activeEnemy_->getName() + " is preparing its turn...");
	SyncCombatEvents();
	Invalidate();
	SetTimer(window_, 1, 1000, nullptr);
}

void Win32UI::Inspect(int interactableIndex) {
	if (!player_.isAlive()) return;
	if (selectedRoom_ < 0 || selectedRoom_ >= static_cast<int>(mapManager_.GetRooms().size())) return;
	Area& area = mapManager_.GetRooms()[selectedRoom_];
	combat_.ClearEvents();
	if (interactableIndex < 0 || interactableIndex >= static_cast<int>(area.GetVisibleInteractables().size())) return;
	const std::string object = area.GetVisibleInteractables()[interactableIndex];
	if (area.ConsumeClue()) {
		player_.AddClue({"A fragment recovered from the " + object + "."});
		progression_.GrantClueXP();
		progression_.CheckLevelUp(player_);
		AddLog("Investigation: clue recovered from " + object + ".");
		statusMessage_ = "Investigation complete: clue recovered.";
	} else {
		AddLog("Investigation: nothing new revealed at " + object + ".");
		statusMessage_ = "Investigation complete: nothing new revealed.";
	}
}

void Win32UI::SearchRoom() {
	if (!player_.isAlive()) return;
	if (selectedRoom_ < 0 || selectedRoom_ >= static_cast<int>(mapManager_.GetRooms().size())) return;
	statusMessage_ = "Investigating the room...";
	combat_.ClearEvents();
	Area& area = mapManager_.GetRooms()[selectedRoom_];
	area.RecordSearch();
	const unsigned searchSeed = RoomSeed(area.getName()) + static_cast<unsigned>(area.GetSearchCount() * 31);
	const int roll = static_cast<int>(searchSeed % 100);
	const bool cluesComplete = area.HasAllClues();
	const int bossThreshold = 18;
	const int playerLevel = progression_.GetLevel();
	const EnemyFaction roomFaction = RoomFaction(area.getName());
	const EnemyTier encounterTier = MaximumEncounterTier(playerLevel);
	const int levelDisparity = static_cast<int>((searchSeed / 7) % 8) - 2;
	const int enemyLevel = (std::max)(1, (std::min)(100, playerLevel + levelDisparity));
	const bool encounterAllowed = area.GetSpawnCondition() != EnemySpawnCondition::None &&
		roll < SpawnChance(area.GetSpawnCondition());
	const int enemyThreshold = cluesComplete ? 70 : 50;
	const int consumableThreshold = (std::max)(25, 62 - playerLevel / 3);
	if (area.getName() == "Fountain" && !area.IsHealingUnlocked() && activeEnemy_ == nullptr && !area.hasEnemies() && area.GetSearchCount() == 1) {
		if (auto enemy = CreateRandomCatalogueEnemy(searchSeed, 1, EnemyFaction::Devotee, EnemyTier::Basic)) {
			area.addEnemy(std::move(enemy));
			activeEnemy_ = area.getFirstAliveEnemy();
			combat_.BeginEncounter(*activeEnemy_);
			combat_.PrepareTurn(player_, *activeEnemy_);
			view_ = View::Combat;
			AddLog("Fountain investigation: a weak guardian appears. Defeat it to purify the fountain.");
		}
		return;
	}
	if (activeEnemy_ == nullptr && roll >= 90) {
		const int damage = (std::max)(1, player_.getMaxHP() * 5 / 100);
		player_.takeDamage(damage);
		AddLog("Investigation: environmental hazard caused " + std::to_string(damage) + " damage.");
		if (!player_.isAlive()) {
			if (activeEnemy_ != nullptr && activeEnemy_->isAlive()) ShowDefeatRecovery();
			else player_.RestoreHealth((std::max)(1, player_.getMaxHP() * 10 / 100));
		}
		return;
	}
	if (activeEnemy_ == nullptr && roll >= 84) {
		player_.heal((std::max)(1, player_.getMaxHP() * 10 / 100));
		AddLog("Shimmer Surge: the player gains a temporary restorative pulse.");
		if (roll % 5 == 0) {
			const int eliteLevel = progression_.GetLevel();
			if (static_cast<int>(encounterTier) >= static_cast<int>(EnemyTier::Elite)) {
				if (auto elite = CreateRandomCatalogueEnemy(searchSeed + 7, eliteLevel, roomFaction, EnemyTier::Elite)) {
				if (elite->getTier() == EnemyTier::Elite) {
					area.addEnemy(std::move(elite));
					activeEnemy_ = area.getFirstAliveEnemy();
					combat_.ContinueEncounter(*activeEnemy_);
					combat_.PrepareTurn(player_, *activeEnemy_);
					view_ = View::Combat;
			if (!player_.isAlive()) ShowDefeatRecovery();
					AddLog("Shimmer Anomaly: an elite enemy spawned.");
				}
			}
			}
		}
		return;
	}
	if (activeEnemy_ == nullptr && roll >= 78) {
		AddLog("Investigation: a Shimmer anomaly rippled through the room.");
		return;
	}
	if (activeEnemy_ == nullptr && playerLevel >= 6 && area.getName() == "Entrance Hall" && cluesComplete && area.HasAllItems() && !area.HasBossBeenDefeated() && roll < bossThreshold) {
		area.MarkBossEncountered();
		if (auto boss = CreateCatalogueEnemy("Matron of the Catacombs", enemyLevel)) area.addEnemy(std::move(boss));
		activeEnemy_ = area.getFirstAliveEnemy();
		combat_.BeginEncounter(*activeEnemy_);
		combat_.PrepareTurn(player_, *activeEnemy_);
		view_ = View::Combat;
		if (!player_.isAlive()) ShowDefeatRecovery();
		AddLog("Search: the Guard of the Catacombs emerges.");
		return;
	}
	if (activeEnemy_ == nullptr && roll >= 50 && roll < 55) {
		AddLog("Nothing Was Found.");
		statusMessage_ = "Nothing Was Found.";
		return;
	}
	if (activeEnemy_ == nullptr && area.HasAllClues() && area.HasAllItems() && area.HasBossBeenDefeated()) {
		const int repeatRoll = static_cast<int>(searchSeed % 100);
		if (repeatRoll < 10 && playerLevel >= 6) {
			const int enemyLevel = progression_.GetLevel();
			if (auto boss = CreateCatalogueEnemy("Overlord-Prime", enemyLevel)) area.addEnemy(std::move(boss));
			AddLog("Search: the defeated boss returns in a stronger form.");
		} else if (repeatRoll < 60) {
			const int enemyLevel = (std::max)(1, progression_.GetLevel() + static_cast<int>(searchSeed % 3) - 1);
			if (auto enemy = CreateRandomCatalogueEnemy(searchSeed, enemyLevel, roomFaction, encounterTier)) area.addEnemy(std::move(enemy));
			AddLog("Search: a Nun emerges from the cleared room.");
		} else {
			const int enemyLevel = (std::max)(1, progression_.GetLevel() + static_cast<int>(searchSeed % 3) - 1);
			if (auto enemy = CreateRandomCatalogueEnemy(searchSeed + 1, enemyLevel, roomFaction, encounterTier)) area.addEnemy(std::move(enemy));
			AddLog("Search: an Automata stirs in the cleared room.");
		}
		activeEnemy_ = area.getFirstAliveEnemy();
		if (activeEnemy_ == nullptr || !activeEnemy_->isAlive()) {
			activeEnemy_ = nullptr;
		if (selectedRoom_ >= 0 && selectedRoom_ < static_cast<int>(mapManager_.GetRooms().size()))
			mapManager_.GetRooms()[selectedRoom_].removeDeadEnemies();
		combat_.ClearEvents();
			view_ = View::Room;
			return;
		}
		combat_.BeginEncounter(*activeEnemy_);
	combat_.PrepareTurn(player_, *activeEnemy_);
		view_ = View::Combat;
		if (!player_.isAlive()) ShowDefeatRecovery();
		return;
	}
	if (activeEnemy_ == nullptr && encounterAllowed) {
		const int compositionRoll = static_cast<int>((searchSeed / 11) % 100);
		const int enemyCount = playerLevel >= 100 ? 2 + compositionRoll % 3
			: playerLevel >= 50 ? (compositionRoll < playerLevel ? 2 + compositionRoll % 2 : 1)
			: playerLevel >= 10 ? (compositionRoll < playerLevel * 2 ? 2 : 1) : 1;
		for (int count = 0; count < enemyCount && count < 4; ++count) {
			const unsigned enemySeed = searchSeed + static_cast<unsigned>(count * 17);
			const int rarityRoll = static_cast<int>((enemySeed / 13) % 100);
			EnemyTier rolledTier = encounterTier;
			if (rarityRoll < 8 && playerLevel >= 20) rolledTier = EnemyTier::Elite;
			else if (rarityRoll < 25 && playerLevel >= 6) rolledTier = EnemyTier::Epic;
			const int rolledLevel = (std::max)(1, (std::min)(100, playerLevel + static_cast<int>((enemySeed / 7) % 5) - 2));
			if (auto enemy = CreateRandomCatalogueEnemy(enemySeed, rolledLevel, roomFaction, rolledTier)) area.addEnemy(std::move(enemy));
		}
		activeEnemy_ = area.getFirstAliveEnemy();
		if (activeEnemy_ == nullptr) return;
		combat_.BeginEncounter(*activeEnemy_);
		combat_.PrepareTurn(player_, *activeEnemy_);
		view_ = View::Combat;
		if (!player_.isAlive()) ShowDefeatRecovery();
		AddLog("Search: an enemy ambushes you in " + area.getName() + ".");
		return;
	}
	if (activeEnemy_ == nullptr && roll >= consumableThreshold && roll < enemyThreshold) {
		if (!area.HasAllItems()) area.ConsumeItem();
		const std::string itemName = area.getName() == "Archive Room" ? "blessed_bandages" : "cellar_rations";
		AddNamedItem(player_, BuildConsumableCatalog(), itemName);
		AddLog("Search: a consumable item was added to the inventory.");
		return;
	}
	if (roll >= enemyThreshold && (cluesComplete || roll < 80)) {
		if (!area.HasAllItems()) area.ConsumeItem();
		AddNamedItem(player_, BuildConsumableCatalog(), "cellar_rations");
		AddLog("Search: a consumable item was added to the inventory.");
		return;
	}
	if (!cluesComplete && roll >= 80) {
		if (!area.HasAllClues()) {
			if (area.ConsumeClue()) {
				player_.AddClue({"A fragment recovered during the investigation."});
		progression_.GrantClueXP();
				progression_.CheckLevelUp(player_);
				AddLog("Search: a clue was recovered.");
			}
			return;
		}
	}
	if (area.RevealNextInteractable()) {
		AddLog("Search: a new interaction became visible.");
	} else if (area.HasNPC() && !area.HasDiscoveredNPC()) {
		area.DiscoverNPC();
		AddLog("Search: an NPC steps from the shadows.");
	} else if (area.ConsumeItem()) {
		if (area.getName() == "Archive Room") AddNamedItem(player_, BuildClueCatalog(), "choir_logbook");
		else AddNamedItem(player_, BuildConsumableCatalog(), "cellar_rations");
		AddLog("Search: an item was added to the inventory.");
	} else {
		AddLog(area.IsFullySearched() ? "Search: the room is fully investigated." : "Search: nothing useful found.");
	}
}

void Win32UI::PerformCombatAction(CombatAction action) {
	if (!player_.isAlive()) {
		ShowDefeatRecovery();
		return;
	}
	if (selectedRoom_ < 0 || selectedRoom_ >= static_cast<int>(mapManager_.GetRooms().size()) ||
		activeEnemy_ == nullptr || !activeEnemy_->isAlive()) {
		player_.ClearStatuses();
		activeEnemy_ = nullptr;
		view_ = View::Room;
		return;
	}
	if (action != CombatAction::Flee && !combat_.IsPlayerTurn()) {
		AddLog("Wait for your turn.");
		return;
	}
	if (action == CombatAction::Flee) {
		if (!player_.isAlive()) return;
		AddLog(player_.getName() + " withdrew from combat.");
		statusMessage_ = "Battle ended: " + player_.getName() + " fled.";
		mapManager_.GetRooms()[selectedRoom_].ClearEnemies();
		LeaveCombat();
		view_ = View::Room;
		return;
	}
	std::unique_ptr<ShimmerSkill> selectedSkill;
	if (action == CombatAction::Shimmer) selectedSkill = ShimmerSkill::Create(selectedCombatSkillName_);
	if (!combat_.resolve(action, player_, *activeEnemy_, selectedSkill.get(), &progression_)) {
		statusMessage_ = action == CombatAction::Shimmer ? "Shimmer is unavailable or still recharging." : "That combat action is unavailable.";
		AddLog("That action is unavailable.");
		return;
	}
	statusMessage_ = action == CombatAction::Shimmer ? "Shimmer effect resolved." : player_.getName() + "'s action resolved.";
	if (!activeEnemy_->isAlive()) {
		Area& area = mapManager_.GetRooms()[selectedRoom_];
		const std::string defeatedName = activeEnemy_->getName();
		const int defeatedLevel = activeEnemy_->getLevel();
		const int xpReward = activeEnemy_->getXPReward();
		const EnemyTier defeatedTier = activeEnemy_->getTier();
		const bool wasBoss = defeatedTier == EnemyTier::Boss;
		const bool allEnemiesDefeated = !area.hasEnemies();
		if (area.getName() == "Fountain" && !area.IsHealingUnlocked() && allEnemiesDefeated) {
			area.SetHealingUnlocked(true);
			AddLog("The Fountain is purified. Full healing is now available.");
		}
		statusMessage_ = defeatedName + " defeated. XP gained: " + std::to_string(xpReward) + ".";
		AddLog(defeatedName + " defeated. XP gained: " + std::to_string(xpReward) + ".");
		if (wasBoss) {
			if (progression_.UnlockShimmerFromTrigger(defeatedName)) AddLog("Shimmer skill unlocked from the boss: " + defeatedName + ".");
			area.SetBossDefeated(true);
			if (selectedRoom_ + 1 < static_cast<int>(mapManager_.GetRooms().size())) {
				mapManager_.GetRooms()[selectedRoom_ + 1].SetUnlocked(true);
				AddLog("The next area is now visible on the map.");
			}
		}
		const unsigned dropRoll = RoomSeed(defeatedName) + static_cast<unsigned>(defeatedLevel * 17 + area.GetSearchCount());
		const int dropChance = defeatedTier == EnemyTier::Elite ? 100 : (std::min)(90, 15 + defeatedLevel);
		const int dropCount = defeatedTier == EnemyTier::Boss ? 3 : defeatedTier == EnemyTier::Elite ? 2 : defeatedLevel >= 20 ? 2 : 1;
		for (int drop = 0; drop < dropCount; ++drop) {
		const unsigned currentRoll = dropRoll + static_cast<unsigned>(drop * 41);
		if (currentRoll % 100 < dropChance) {
			const auto catalog = BuildEquippableCatalog();
			const int index = static_cast<int>(currentRoll % catalog.size());
			const bool allowed = defeatedTier == EnemyTier::Boss ? catalog[index].rarity != Rarity::Artifact
				: defeatedTier == EnemyTier::Elite ? catalog[index].rarity == Rarity::Rare || catalog[index].rarity == Rarity::Epic || catalog[index].rarity == Rarity::Legendary
				: wasBoss ? catalog[index].rarity != Rarity::Legendary && catalog[index].rarity != Rarity::Artifact
				: catalog[index].rarity == Rarity::Common || catalog[index].rarity == Rarity::Uncommon || catalog[index].rarity == Rarity::Rare;
			if (allowed) {
				player_.AddItem(std::make_unique<Item>(catalog[index]));
				AddLog("Drop: " + catalog[index].name + ".");
				statusMessage_ += " Item dropped: " + catalog[index].name + ".";
			}
		}
		}
		if (allEnemiesDefeated) {
			LeaveCombat();
			view_ = View::Room;
		} else {
			activeEnemy_ = area.getFirstAliveEnemy();
			combatTargetConfirmed_ = false;
			combatAttackSelection_ = false;
			selectedCombatSkillName_.clear();
					combat_.BeginEncounter(*activeEnemy_);
			combat_.PrepareTurn(player_, *activeEnemy_);
					combat_.RecordInfo("Another enemy remains. Select a living enemy for the next action.");
					if (!combat_.IsPlayerTurn()) ScheduleEnemyTurn();
		}
		SyncCombatEvents();
		if (!refreshPosted_) {
			refreshPosted_ = true;
			PostMessageW(window_, RefreshControlsMessage, 0, 0);
		}
		return;
	}
	if (!combat_.IsPlayerTurn() && activeEnemy_->isAlive() && player_.isAlive()) ScheduleEnemyTurn();
	SyncCombatEvents();
	ResetCombatSelection();
	statusMessage_.clear();
	if (!player_.isAlive()) ShowDefeatRecovery();
}

void Win32UI::AddLog(const std::string& message) {
	log_.push_back(message);
	if (log_.size() > 12) log_.erase(log_.begin());
}

void Win32UI::InitializePlayer() {
	player_.SetLevelStats(1);
	player_.RestoreHealth(player_.getMaxHP());
	progression_.UnlockShimmerSkill("Pulsus Luminis");
	const auto equipment = BuildEquippableCatalog();
	AddNamedItem(player_, equipment, "plain_rosary");
	AddNamedItem(player_, equipment, "linen_veil");
	AddNamedItem(player_, BuildConsumableCatalog(), "cellar_rations");
	AddNamedItem(player_, BuildConsumableCatalog(), "purification_salts");
	AddNamedItem(player_, BuildClueCatalog(), "storage_key");
	AddNamedItem(player_, BuildCruxCatalog(), "first_nuns_diary");
	if (Item* hand = player_.FindItem("plain_rosary")) player_.Equip(*hand);
	player_.RestoreHealth(player_.getMaxHP());
}

void Win32UI::Invalidate() {
	InvalidateRect(window_, nullptr, TRUE);
}

LRESULT CALLBACK Win32UI::NameProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
	Win32UI* ui = reinterpret_cast<Win32UI*>(GetWindowLongPtrW(window, GWLP_USERDATA));
	if (message == WM_NCCREATE) {
		const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
		ui = static_cast<Win32UI*>(create->lpCreateParams);
		SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ui));
		ui->nameWindow_ = window;
	}
	if (ui == nullptr) return DefWindowProcW(window, message, wParam, lParam);

	switch (message) {
	case WM_CREATE:
		{
			HFONT promptFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
				DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
				DEFAULT_PITCH | FF_DONTCARE, L"IM FELL English");
			const std::array<const wchar_t*, 4> questions = {
				L"Who is this poor soul?",
				L"A name long forgotten...",
				L"What name survived the dark?",
				L"Who enters beneath the cathedral?"
			};
			const wchar_t* question = questions[GetTickCount() % questions.size()];
			HWND promptLabel = CreateWindowW(L"STATIC", question,
					  WS_CHILD | WS_VISIBLE, 24, 22, 410, 24, window, nullptr,
					  GetModuleHandleW(nullptr), nullptr);
			SendMessageW(promptLabel, WM_SETFONT, reinterpret_cast<WPARAM>(promptFont), TRUE);
			HWND promptHint = CreateWindowW(L"STATIC", L"Leave blank for developer mode.",
					  WS_CHILD | WS_VISIBLE, 24, 38, 410, 20, window, nullptr,
					  GetModuleHandleW(nullptr), nullptr);
			SendMessageW(promptHint, WM_SETFONT, reinterpret_cast<WPARAM>(promptFont), TRUE);
		ui->nameEdit_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
										WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
										24, 58, 410, 34, window, nullptr,
										GetModuleHandleW(nullptr), nullptr);
		SendMessageW(ui->nameEdit_, WM_SETFONT, reinterpret_cast<WPARAM>(promptFont), TRUE);
		HWND beginButton = CreateWindowW(L"BUTTON", L"Begin Investigation", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
					  24, 108, 190, 36, window, reinterpret_cast<HMENU>(1),
					  GetModuleHandleW(nullptr), nullptr);
		SendMessageW(beginButton, WM_SETFONT, reinterpret_cast<WPARAM>(promptFont), TRUE);
		}
		SetFocus(ui->nameEdit_);
		return 0;
	case WM_COMMAND:
		if (LOWORD(wParam) == 1 && HIWORD(wParam) == BN_CLICKED) {
			const int length = GetWindowTextLengthW(ui->nameEdit_);
			std::wstring name(static_cast<std::size_t>(length + 1), L'\0');
			if (length > 0) GetWindowTextW(ui->nameEdit_, name.data(), length + 1);
			if (!name.empty() && name.back() == L'\0') name.pop_back();
			if (name.empty()) {
				name = L"Adelaide";
				ui->completionMode_ = true;
				ui->AddLog("Developer mode is active. The cathedral remembers everything.");
			}
			ui->player_.SetName(ui->ToNarrow(name));
			DestroyWindow(window);
			return 0;
		}
		break;
	case WM_CLOSE:
		ui->player_.SetName("Adelaide");
		ui->completionMode_ = true;
		ui->AddLog("Developer mode is active. The cathedral remembers everything.");
		DestroyWindow(window);
		return 0;
	case WM_DESTROY:
		ui->nameWindow_ = nullptr;
		return 0;
	default:
		break;
	}
	return DefWindowProcW(window, message, wParam, lParam);
}

LRESULT CALLBACK Win32UI::WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
	Win32UI* ui = reinterpret_cast<Win32UI*>(GetWindowLongPtrW(window, GWLP_USERDATA));
	if (message == WM_NCCREATE) {
		const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
		ui = static_cast<Win32UI*>(create->lpCreateParams);
		SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ui));
		ui->window_ = window;
	}
	return ui == nullptr ? DefWindowProcW(window, message, wParam, lParam)
						 : ui->HandleMessage(message, wParam, lParam);
}

LRESULT Win32UI::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_CREATE:
		CreateControls();
		return 0;
	case WM_SIZE:
		LayoutControls(LOWORD(lParam), HIWORD(lParam));
		return 0;
	case RefreshControlsMessage:
		refreshPosted_ = false;
		RefreshControls();
		SyncCombatEvents();
		Invalidate();
		return 0;
	case WM_TIMER:
		if (wParam == 1 && enemyTurnPending_) {
			KillTimer(window_, 1);
			enemyTurnPending_ = false;
			combatTransition_ = false;
			if (view_ == View::Combat && activeEnemy_ != nullptr && activeEnemy_->isAlive() && player_.isAlive()) {
				combat_.enemyTurn(player_, *activeEnemy_);
				SyncCombatEvents();
				RefreshControls();
				UpdateWindow(window_);
				if (!combat_.IsPlayerTurn() && activeEnemy_->isAlive() && player_.isAlive()) ScheduleEnemyTurn();
				if (!player_.isAlive()) ShowDefeatRecovery();
				if (!refreshPosted_) {
					refreshPosted_ = true;
					PostMessageW(window_, RefreshControlsMessage, 0, 0);
				}
				Invalidate();
			}
		}
		return 0;
	case WM_COMMAND:
		if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == InventorySearch) {
			wchar_t query[256]{};
			GetWindowTextW(controls_[25], query, static_cast<int>(std::size(query)));
			inventorySearchQuery_ = ToNarrow(query);
			selectedInventoryIndex_ = -1;
			inventoryPage_ = 0;
			if (!refreshPosted_) {
				refreshPosted_ = true;
				PostMessageW(window_, RefreshControlsMessage, 0, 0);
			}
	SyncCombatEvents();
		} else if (HIWORD(wParam) == BN_CLICKED) HandleCommand(LOWORD(wParam));
		return 0;
	case WM_DRAWITEM: {
		const auto* draw = reinterpret_cast<const DRAWITEMSTRUCT*>(lParam);
		if (draw != nullptr && draw->CtlType == ODT_BUTTON) {
			const bool inventoryButton = draw->CtlID >= InventoryButtonBase && draw->CtlID < InventoryButtonBase + 12;
			const bool inventorySelected = inventoryButton &&
				inventoryPage_ * 12 + draw->CtlID - InventoryButtonBase == selectedInventoryIndex_;
			HBRUSH brush = CreateSolidBrush((draw->itemState & ODS_SELECTED) != 0 || inventorySelected ? RGB(52, 61, 62) : Background);
			FillRect(draw->hDC, &draw->rcItem, brush);
			DeleteObject(brush);
			const COLORREF borderColor = inventorySelected ? Shimmer : PanelEdge;
			HBRUSH border = CreateSolidBrush(borderColor);
			FrameRect(draw->hDC, &draw->rcItem, border);
			DeleteObject(border);
			wchar_t label[128]{};
			GetWindowTextW(draw->hwndItem, label, 128);
			SetBkMode(draw->hDC, TRANSPARENT);
			HFONT oldFont = static_cast<HFONT>(SelectObject(draw->hDC, font_));
			if (inventoryButton) {
				const int itemIndex = inventoryPage_ * 12 + draw->CtlID - InventoryButtonBase;
				const Item* item = itemIndex < static_cast<int>(inventoryEntryItems_.size())
					? inventoryEntryItems_[itemIndex] : nullptr;
				SetTextColor(draw->hDC, item == nullptr ? Muted : RarityColor(item->getRarity()));
				DrawTextW(draw->hDC, label, -1, const_cast<RECT*>(&draw->rcItem),
					DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_NOPREFIX);
			} else {
				SetTextColor(draw->hDC, Text);
				DrawTextW(draw->hDC, label, -1, const_cast<RECT*>(&draw->rcItem),
					DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_NOPREFIX);
			}
			SelectObject(draw->hDC, oldFont);
			return 1;
		}
		break;
	}
	case WM_PAINT: {
		PAINTSTRUCT paint{};
		HDC deviceContext = BeginPaint(window_, &paint);
		Paint(deviceContext);
		EndPaint(window_, &paint);
		return 0;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProcW(window_, message, wParam, lParam);
	}
}

void Win32UI::Paint(HDC deviceContext) {
	RECT client{};
	GetClientRect(window_, &client);
	HBRUSH background = CreateSolidBrush(Background);
	FillRect(deviceContext, &client, background);
	DeleteObject(background);

	DrawPanel(deviceContext, {20, 20, 365, client.bottom - 20}, ToWide(player_.getName()) + L" // CHARACTER");
	DrawTextLine(deviceContext, 42, 65, L"MICARE", Brass, 28, true);
	if (progression_.GetLevel() == 100) DrawRainbowText(deviceContext, 42, 110, ToWide(player_.getName()), 19, true);
	else DrawTextLine(deviceContext, 42, 110, ToWide(player_.getName()), Text, 19, true);
	DrawTextLine(deviceContext, 250, 110, L"LEVEL " + std::to_wstring(progression_.GetLevel()), Text, 19, true);
	DrawTextLine(deviceContext, 42, 150, L"HP", Muted); DrawHealthBar(deviceContext, 80, 154, 145, 12, player_.getHP(), player_.getMaxHP(), Verdigris); DrawTextLine(deviceContext, 235, 150, std::to_wstring(player_.getHP()) + L" / " + std::to_wstring(player_.getMaxHP()), Text, 15);
	DrawTextLine(deviceContext, 42, 182, L"XP", Muted); DrawTextLine(deviceContext, 150, 182,
		std::to_wstring(progression_.GetXP()) + L" / " + std::to_wstring(progression_.GetXPToNextLevel()), Text);
	DrawTextLine(deviceContext, 42, 214, L"DEF   " + std::to_wstring(player_.getDefense()) + L"     SPD   " + std::to_wstring(player_.getSpeed()), Text);
	DrawTextLine(deviceContext, 42, 246, L"ATK   " + std::to_wstring(player_.getAttack()) + L"     ACC   " + std::to_wstring(player_.getDisplayedAccuracy()) + L"%", Text);
	DrawTextLine(deviceContext, 42, 278, L"RES   " + std::to_wstring(player_.getDisplayedResistance()) + L"%    CRIT  " + std::to_wstring(player_.getDisplayedCritRate()) + L"% / " + std::to_wstring(player_.getCritDamage()) + L"%", Text, 15);
	DrawTextLine(deviceContext, 42, 318, L"STATUS", Brass, 17, true);
	int y = 348;
	bool displayedStatus = false;
	for (const auto& status : player_.GetStatuses()) {
		if (status.type == StatusType::VerdigrisMark) continue;
		displayedStatus = true;
		const bool healing = status.type == StatusType::HealOverTime;
		const bool beneficial = status.type == StatusType::Barrier || status.type == StatusType::DamageReduction;
		const COLORREF statusColor = healing ? RGB(90, 210, 110) : beneficial ? RGB(90, 160, 245) : Danger;
		DrawTextLine(deviceContext, 48, y, StatusName(status.type) + L" (" + std::to_wstring(status.turns) + L")", statusColor, 15);
		y += 25;
	}
	if (!displayedStatus) DrawTextLine(deviceContext, 48, y, L"No active effects", Muted, 15);
	const auto activeSets = player_.GetActiveSets();
	if (!activeSets.empty()) {
		if (!displayedStatus) y += 25;
		for (const auto& setName : activeSets) {
			DrawTextLine(deviceContext, 48, y, L"Set active: " + FormatDisplayText(setName), Shimmer, 15, false, 300);
			y += 25;
		}
	}
	DrawTextLine(deviceContext, 42, 430, L"EQUIPMENT", Brass, 17, true);
	const std::pair<EquipSlot, const wchar_t*> slots[] = {{EquipSlot::Head, L"Head"}, {EquipSlot::Torso, L"Torso"}, {EquipSlot::Boots, L"Boots"}, {EquipSlot::Hand, L"Hand"}, {EquipSlot::Accessory, L"Accessory"}};
	y = 460;
	for (const auto& slot : slots) {
		const Item* item = player_.GetEquipped(slot.first);
		DrawTextLine(deviceContext, 48, y, std::wstring(slot.second) + L": ", Muted, 14);
		if (item != nullptr) DrawTextLine(deviceContext, 105, y, FormatDisplayText(item->getName()), RarityColor(item->getRarity()), 14, false, 235);
		else DrawTextLine(deviceContext, 105, y, L"--", Muted, 14);
		y += 25;
	}
	DrawTextLine(deviceContext, 42, y + 12, L"SHIMMER LOADOUT", Brass, 17, true);
	y += 42;
	const auto equippedShimmer = progression_.GetEquippedShimmerSkills();
	if (equippedShimmer.empty()) DrawTextLine(deviceContext, 48, y, L"--", Muted, 14);
	for (const auto& skillName : equippedShimmer) {
		DrawTextLine(deviceContext, 48, y, FormatDisplayText(skillName), Shimmer, 14, false, 275);
		y += 25;
	}

	if (view_ == View::Inventory) {
		const int left = client.right / 3 < 395 ? (client.right / 3 < 20 ? 20 : client.right / 3) : 395;
		const int right = client.right - 20 > left + 240 ? client.right - 20 : left + 240;
		const int top = 20;
		const int bottom = client.bottom - 20;
		const int split = left + (right - left) * 56 / 100;
		const int detailLeft = split + 16;
		DrawPanel(deviceContext, {left, top, split, bottom}, L"INVENTORY");
		DrawPanel(deviceContext, {detailLeft, top, right, bottom}, L"ITEM INFORMATION");
		DrawTextLine(deviceContext, left + 20, top + 30, L"Search items", Muted, 18);
		DrawTextLine(deviceContext, left + 20, top + 134, L"Select an item", Muted, 18);
		DrawTextLine(deviceContext, detailLeft + 20, top + 50, L"Choose an item to inspect its complete effects.", Muted, 18);
		if (selectedInventoryIndex_ >= 0 && selectedInventoryIndex_ < static_cast<int>(inventoryEntryItems_.size())) {
			const Item* selected = inventoryEntryItems_[selectedInventoryIndex_];
			const std::string description = inventoryEntryDescriptions_[selectedInventoryIndex_];
			const int detailX = detailLeft + 20;
			int detailY = top + 92;
			const int detailWidth = right - detailX - 20;
			detailY += DrawTextLine(deviceContext, detailX, detailY, FormatDisplayText(selected == nullptr ? "Clue" : selected->getName()),
				selected == nullptr ? Muted : RarityColor(selected->getRarity()), 22, true, detailWidth) + 10;
			if (selected == nullptr) {
				DrawTextLine(deviceContext, detailX, detailY, FormatDisplayText(description), Text, 18, false, detailWidth);
			} else {
				if (!selected->getDescription().empty()) {
					detailY += DrawTextLine(deviceContext, detailX, detailY, FormatDisplayText(selected->getDescription()), Text, 18, false, detailWidth) + 8;
				}
				if (selected->getCategory() == ItemCategory::Equippable) {
					const std::string mainValue = selected->getMainStatPercent() > 0 ? "+" + std::to_string(selected->getMainStatPercent()) + "%" : "+" + std::to_string(selected->getMainStatValue());
					detailY += DrawTextLine(deviceContext, detailX, detailY, L"Main stat: " + FormatDisplayText(selected->getMainStat()) + L" " + ToWide(mainValue), Shimmer, 17, false, detailWidth) + 6;
				}
				for (const auto& substat : selected->getSubstats()) {
					const char* label = "Stat";
					switch (substat.first) {
					case ItemStat::HP: label = "HP"; break; case ItemStat::DEF: label = "DEF"; break; case ItemStat::SPD: label = "SPD"; break;
					case ItemStat::ATK: label = "ATK"; break; case ItemStat::Accuracy: label = "Accuracy"; break; case ItemStat::Resistance: label = "Resistance"; break;
					case ItemStat::CritRate: label = "Crit rate"; break; case ItemStat::CritDamage: label = "Crit damage"; break; case ItemStat::Evasion: label = "Evasion"; break;
					case ItemStat::HealingPotency: label = "Healing potency"; break; case ItemStat::BarrierPotency: label = "Barrier potency"; break;
					case ItemStat::DotPotency: label = "DOT potency"; break; case ItemStat::Lifesteal: label = "Lifesteal"; break;
					case ItemStat::DebuffDurationReduction: label = "Debuff reduction"; break;
					}
					detailY += DrawTextLine(deviceContext, detailX, detailY, L"Substat: " + ToWide(label) + L" +" + std::to_wstring(substat.second) + L"%", Text, 17, false, detailWidth) + 6;
				}
				std::string effect = selected->getEffect();
				if (!selected->getPassive().empty()) effect += (effect.empty() ? "" : " ") + selected->getPassive();
				if (!selected->getUniqueEffect().empty()) effect += (effect.empty() ? "" : " ") + selected->getUniqueEffect();
				if (!effect.empty()) {
					detailY += DrawTextLine(deviceContext, detailX, detailY, L"Effect: " + FormatDisplayText(effect), Shimmer, 17, false, detailWidth) + 6;
				}
				if (!selected->getSetName().empty()) {
					detailY += DrawTextLine(deviceContext, detailX, detailY, L"Set: " + FormatDisplayText(selected->getSetName()), Brass, 17, false, detailWidth) + 6;
					if (!selected->getSetPassive().empty()) detailY += DrawTextLine(deviceContext, detailX, detailY, L"Set bonus: " + FormatDisplayText(selected->getSetPassive()), Brass, 17, false, detailWidth) + 6;
				}
			}
		}
		return;
	}
	if (view_ == View::Skills) {
		const int left = client.right / 3 < 395 ? (client.right / 3 < 20 ? 20 : client.right / 3) : 395;
		const int right = client.right - 20 > left + 240 ? client.right - 20 : left + 240;
		const int top = 20;
		const int bottom = client.bottom - 20;
		const int split = left + (right - left) * 48 / 100;
		const int detailLeft = split + 16;
		DrawPanel(deviceContext, {left, top, split, bottom}, L"SKILL INVENTORY");
		DrawPanel(deviceContext, {detailLeft, top, right, bottom}, L"SKILL INFORMATION");
		DrawTextLine(deviceContext, left + 20, top + 90,
			L"Shimmer points: " + std::to_wstring(progression_.GetShimmerPoints()), Muted, 16);
		const auto equipped = progression_.GetEquippedShimmerSkills();
		if (selectedSkillIndex_ >= 0 && selectedSkillIndex_ < static_cast<int>(skillNames_.size())) {
			const std::string& name = skillNames_[selectedSkillIndex_];
			auto skill = ShimmerSkill::Create(name);
			if (skill != nullptr) {
				const bool active = std::find(equipped.begin(), equipped.end(), name) != equipped.end();
				int infoY = top + 55;
				infoY += DrawTextLine(deviceContext, detailLeft + 20, infoY, FormatDisplayText(name), Shimmer, 22, true, right - detailLeft - 40) + 12;
				DrawTextLine(deviceContext, detailLeft + 20, infoY, active ? L"EQUIPPED" : L"NOT EQUIPPED", active ? RGB(90, 210, 110) : Muted, 16, true);
				infoY += 32;
				DrawTextLine(deviceContext, detailLeft + 20, infoY, L"Cost: " + std::to_wstring(skill->getCost()) + L"   Cooldown: " + std::to_wstring(skill->getCooldown()), Text, 17);
				infoY += 34;
				infoY += DrawTextLine(deviceContext, detailLeft + 20, infoY, L"Effect: " + FormatDisplayText(skill->getEffect()), Shimmer, 17, false, right - detailLeft - 40) + 10;
				DrawTextLine(deviceContext, detailLeft + 20, infoY, FormatDisplayText(skill->getDescription()), Text, 17, false, right - detailLeft - 40);
			}
		} else {
			DrawTextLine(deviceContext, detailLeft + 20, top + 70, L"Select a skill to view its information.", Muted, 17, false, right - detailLeft - 40);
		}
		return;
	}

	const int eventTop = client.bottom > 500 ? static_cast<int>(client.bottom) - 380 : 120;
	DrawPanel(deviceContext, {395, 20, client.right - 20, eventTop - 15},
			  view_ == View::Combat ? L"COMBAT VIEW" : L"GAMEPLAY VIEW");
	int contentY = 70;
	if (view_ == View::Floor) {
		DrawTextLine(deviceContext, 425, contentY, L"FLOOR I // THE CATHEDRAL", Shimmer, 25, true);
		DrawTextLine(deviceContext, 425, contentY + 45, L"Choose a room or open a panel using the buttons.", Muted);
		contentY += 100;
		DrawTextLine(deviceContext, 425, contentY, L"Select a room from the navigation buttons.", Text, 19);
		DrawTextLine(deviceContext, 425, contentY + 38, L"Map and Skills stay available in the upper-right.", Muted, 16);
	} else if (view_ == View::Room) {
		if (selectedRoom_ < 0 || selectedRoom_ >= static_cast<int>(mapManager_.GetRooms().size())) {
			DrawTextLine(deviceContext, 425, contentY, L"No room is currently selected.", Muted, 19);
			return;
		}
		const Area& room = mapManager_.GetRooms()[selectedRoom_];
		DrawTextLine(deviceContext, 425, contentY, ToWide(room.getName()), Shimmer, 25, true);
		DrawTextLine(deviceContext, 425, contentY + 42, L"The cathedral holds its breath. Investigate an object below.", Muted);
		contentY += 100;
		if (!statusMessage_.empty()) {
			DrawTextLine(deviceContext, 425, contentY, ToWide(statusMessage_), Shimmer, 16);
			contentY += 32;
		}
		for (const auto& object : room.GetVisibleInteractables()) {
			DrawTextLine(deviceContext, 440, contentY, L"[ ]  " + ToWide(object), Text, 18);
			contentY += 34;
		}
	} else if (view_ == View::Combat && activeEnemy_ != nullptr) {
		if (combatTransition_ || !combat_.IsPlayerTurn()) {
			DrawTextLine(deviceContext, 425, contentY, L"ENEMY TURN — AI ACTING", Muted, 18, true);
			contentY += 34;
		}
		int cardX = 425;
		for (Enemy* target : targetEnemies_) {
			if (target == nullptr || !target->isAlive()) continue;
			DrawTextLine(deviceContext, cardX, contentY, ToWide(target->getName()), EnemyTierColor(target->getTier()), 18, true, 170);
			DrawHealthBar(deviceContext, cardX, contentY + 28, 170, 12, target->getHP(), target->getMaxHP(), Danger);
			DrawTextLine(deviceContext, cardX, contentY + 46, L"HP " + std::to_wstring(target->getHP()) + L" / " + std::to_wstring(target->getMaxHP()), Text, 14);
			DrawTextLine(deviceContext, cardX, contentY + 66, L"LV " + std::to_wstring(target->getLevel()), Text, 15);
			std::wstring statusText;
			for (const auto& status : target->GetStatuses()) {
				statusText += CombatStatusSymbol(status.type);
				statusText += std::to_wstring(status.turns) + L"  ";
			}
			if (!statusText.empty()) {
				DrawTextLine(deviceContext, cardX, contentY + 88, statusText, Text, 17, false, 170);
			}
			int enemyStatusY = contentY + 110;
			if (target->getTier() == EnemyTier::Elite || target->getTier() == EnemyTier::Boss) {
				for (std::size_t skillIndex = 0; skillIndex < target->GetSkills().size(); ++skillIndex) {
					const int cooldown = target->GetSkillCooldown(skillIndex);
					if (cooldown > 0) {
						DrawTextLine(deviceContext, cardX, enemyStatusY, L"Skill " + std::to_wstring(skillIndex + 1) + L" CD " + std::to_wstring(cooldown), Muted, 14);
						enemyStatusY += 22;
					}
				}
			}
			cardX += 205;
		}
		DrawTextLine(deviceContext, 425, contentY + 145, L"Battle event log", Brass, 18, true);
		int eventY = contentY + 180;
		const auto& events = combat_.GetEvents();
		const int firstEvent = events.size() > 10 ? static_cast<int>(events.size()) - 10 : 0;
		for (int index = firstEvent; index < static_cast<int>(events.size()); ++index) {
			const auto& event = events[static_cast<std::size_t>(index)];
			COLORREF eventColor = Text;
			if (event.kind == CombatManager::CombatEvent::Kind::Damage || event.kind == CombatManager::CombatEvent::Kind::Attack || event.kind == CombatManager::CombatEvent::Kind::Skill || event.kind == CombatManager::CombatEvent::Kind::DotTick || event.kind == CombatManager::CombatEvent::Kind::Death) eventColor = Danger;
			else if (event.kind == CombatManager::CombatEvent::Kind::Heal) eventColor = RGB(90, 210, 110);
			else if (event.kind == CombatManager::CombatEvent::Kind::Shield || event.kind == CombatManager::CombatEvent::Kind::Buff) eventColor = RGB(90, 160, 245);
			else if (event.kind == CombatManager::CombatEvent::Kind::Debuff || event.kind == CombatManager::CombatEvent::Kind::DotTick) eventColor = Danger;
			else if (event.kind == CombatManager::CombatEvent::Kind::Status || event.kind == CombatManager::CombatEvent::Kind::PhaseTransition || event.kind == CombatManager::CombatEvent::Kind::Mutation) eventColor = Shimmer;
			eventY += DrawTextLine(deviceContext, 425, eventY, FormatDisplayText(event.message), eventColor, 16, false,
				client.right - 449) + 5;
			if (eventY > eventTop - 30) break;
		}
	} else if (view_ == View::Map) {
		DrawTextLine(deviceContext, 425, contentY, L"FLOOR I // MAP", Shimmer, 25, true);
		DrawTextLine(deviceContext, 425, contentY + 35, L"Select Back to return.", Muted, 16);
		int mapY = contentY + 55;
		for (const auto& room : mapManager_.GetRooms()) {
			if (room.IsUnlocked()) {
				DrawTextLine(deviceContext, 440, mapY, L"[ " + ToWide(room.getName()) + L" ]", Text, 18);
				mapY += 32;
			}
		}
	}

	if (view_ == View::Floor) {
		DrawPanel(deviceContext, {395, eventTop, client.right - 20, client.bottom - 20}, L"EVENT LOG");
		int logY = eventTop + 38;
		const int start = (std::max)(0, static_cast<int>(log_.size()) - 3);
		for (int i = start; i < static_cast<int>(log_.size()); ++i) {
			DrawTextLine(deviceContext, 420, logY, ToWide(log_[i]), Muted, 14);
			logY += 22;
		}
	}
}

void Win32UI::DrawRainbowText(HDC deviceContext, int x, int y, const std::wstring& text,
							 int size, bool bold) const {
	const COLORREF colors[] = {RGB(220, 60, 70), RGB(220, 130, 45), RGB(205, 190, 50), RGB(60, 165, 95), RGB(65, 125, 220), RGB(150, 75, 190)};
	int offset = x;
	for (std::size_t index = 0; index < text.size(); ++index) {
		const std::wstring character(1, text[index]);
		DrawTextLine(deviceContext, offset, y, character, colors[index % 6], size, bold);
		HFONT measureFont = bold && size >= 24 ? headingFont_ : font_;
		HFONT oldFont = static_cast<HFONT>(SelectObject(deviceContext, measureFont));
		SIZE characterSize{};
		GetTextExtentPoint32W(deviceContext, character.c_str(), 1, &characterSize);
		SelectObject(deviceContext, oldFont);
		offset += characterSize.cx;
	}
}

void Win32UI::DrawPanel(HDC deviceContext, RECT bounds, const std::wstring& title) {
	HBRUSH brush = CreateSolidBrush(Panel);
	FillRect(deviceContext, &bounds, brush);
	DeleteObject(brush);
	HPEN pen = CreatePen(PS_SOLID, 2, PanelEdge);
	HGDIOBJ oldPen = SelectObject(deviceContext, pen);
	HGDIOBJ oldBrush = SelectObject(deviceContext, GetStockObject(HOLLOW_BRUSH));
	Rectangle(deviceContext, bounds.left, bounds.top, bounds.right, bounds.bottom);
	SelectObject(deviceContext, oldBrush);
	SelectObject(deviceContext, oldPen);
	DeleteObject(pen);
	DrawTextLine(deviceContext, bounds.left + 18, bounds.top + 14, title, Brass, 17, true);
}

void Win32UI::DrawHealthBar(HDC deviceContext, int x, int y, int width, int height,
	int current, int maximum, COLORREF fillColor) const {
	if (maximum <= 0) return;
	RECT bounds{x, y, x + width, y + height};
	HBRUSH background = CreateSolidBrush(RGB(55, 55, 55));
	FillRect(deviceContext, &bounds, background);
	DeleteObject(background);
	const int filledWidth = (std::max)(0, (std::min)(width, width * current / maximum));
	if (filledWidth > 0) {
		RECT filled{x, y, x + filledWidth, y + height};
		HBRUSH fill = CreateSolidBrush(fillColor);
		FillRect(deviceContext, &filled, fill);
		DeleteObject(fill);
	}
	FrameRect(deviceContext, &bounds, reinterpret_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH)));
}

int Win32UI::DrawTextLine(HDC deviceContext, int x, int y, const std::wstring& text,
						  COLORREF color, int size, bool bold, int maxWidth) const {
	const bool emojiText = text.find(L"🔥") != std::wstring::npos || text.find(L"🩸") != std::wstring::npos ||
		text.find(L"☠") != std::wstring::npos || text.find(L"⚡") != std::wstring::npos ||
		text.find(L"🐌") != std::wstring::npos || text.find(L"☣") != std::wstring::npos ||
		text.find(L"🛡") != std::wstring::npos || text.find(L"🔰") != std::wstring::npos ||
		text.find(L"💚") != std::wstring::npos;
	HFONT selected = emojiText ? statusFont_ : bold && size >= 24 ? headingFont_ : font_;
	HGDIOBJ oldFont = SelectObject(deviceContext, selected);
	SetTextColor(deviceContext, color);
	SetBkMode(deviceContext, TRANSPARENT);
	RECT client{};
	GetClientRect(window_, &client);
	const int right = maxWidth > 0 ? x + maxWidth : (client.right > x + 1 ? static_cast<int>(client.right) - 24 : x + 1);
	RECT textBounds{x, y, right, static_cast<int>(client.bottom)};
	const int height = DrawTextW(deviceContext, text.c_str(), static_cast<int>(text.size()), &textBounds,
			  DT_NOPREFIX | DT_WORDBREAK | DT_CALCRECT);
	RECT drawBounds{x, y, right, y + (height > 0 ? height : size + 4)};
	DrawTextW(deviceContext, text.c_str(), static_cast<int>(text.size()), &drawBounds,
			  DT_NOPREFIX | DT_WORDBREAK);
	SelectObject(deviceContext, oldFont);
	return height > 0 ? height : size + 4;
}

std::wstring Win32UI::ToWide(const std::string& text) const {
	if (text.empty()) return {};
	const int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
	std::wstring result(static_cast<std::size_t>(length), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, result.data(), length);
	if (!result.empty() && result.back() == L'\0') result.pop_back();
	return result;
}

std::string Win32UI::ToNarrow(const std::wstring& text) const {
	if (text.empty()) return {};
	const int length = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string result(static_cast<std::size_t>(length), '\0');
	WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, result.data(), length, nullptr, nullptr);
	if (!result.empty() && result.back() == '\0') result.pop_back();
	return result;
}

std::wstring Win32UI::StatusName(StatusType type) const {
	switch (type) {
	case StatusType::Stun: return L"Stun";
	case StatusType::Bleed: return L"Bleed";
	case StatusType::Burn: return L"Burn";
	case StatusType::Poison: return L"Poison";
	case StatusType::HealOverTime: return L"Healing";
	case StatusType::Slow: return L"Slow";
	case StatusType::VerdigrisMark: return L"Verdigris Mark";
	case StatusType::DamageReduction: return L"Damage Reduction";
	case StatusType::Barrier: return L"Barrier";
	}
	return L"Unknown";
}

std::wstring Win32UI::CombatStatusSymbol(StatusType type) const {
	switch (type) {
	case StatusType::Burn: return L"🔥";
	case StatusType::Bleed: return L"🩸";
	case StatusType::Poison: return L"☠️";
	case StatusType::Stun: return L"⚡";
	case StatusType::Slow: return L"🐌";
	case StatusType::VerdigrisMark: return L"☣️";
	case StatusType::Barrier: return L"🛡️";
	case StatusType::DamageReduction: return L"🔰";
	case StatusType::HealOverTime: return L"💚";
	}
	return L"•";
}
