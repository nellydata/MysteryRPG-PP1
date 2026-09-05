#include "Win32UI.h"
#include "Automata.h"
#include "ContentCatalog.h"
#include "Nun.h"
#include "ShimmerSkill.h"
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <array>

namespace {
constexpr COLORREF Background = RGB(24, 24, 24);
constexpr COLORREF Panel = RGB(35, 35, 35);
constexpr COLORREF PanelEdge = RGB(155, 155, 155);
constexpr COLORREF Text = RGB(235, 235, 235);
constexpr COLORREF Muted = RGB(175, 175, 175);
constexpr COLORREF Brass = RGB(220, 220, 220);
constexpr COLORREF Verdigris = RGB(190, 190, 190);
constexpr COLORREF Shimmer = RGB(245, 245, 245);
constexpr COLORREF Danger = RGB(210, 210, 210);

unsigned RoomSeed(const std::string& roomName) {
	unsigned seed = 2166136261u;
	for (unsigned char character : roomName) {
		seed ^= character;
		seed *= 16777619u;
	}
	return seed;
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

bool EnvironmentEquals(const char* name, const char* expected) {
	char* value = nullptr;
	std::size_t length = 0;
	if (_dupenv_s(&value, &length, name) != 0 || value == nullptr) return false;
	const bool result = std::string(value) == expected;
	std::free(value);
	return result;
}
}

Win32UI::Win32UI()
	: window_(nullptr), nameWindow_(nullptr), nameEdit_(nullptr), controls_{}, font_(nullptr), headingFont_(nullptr),
	  view_(View::Floor), selectedRoom_(0), player_("Adelaide", 20, 10),
	  mapManager_(), combat_(), progression_(), activeEnemy_(nullptr) {
	InitializePlayer();
	AddLog("The cathedral is silent except for the pulse of the Shimmer.");
}

Win32UI::~Win32UI() {
	if (font_ != nullptr) DeleteObject(font_);
	if (headingFont_ != nullptr) DeleteObject(headingFont_);
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
	font_ = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
						OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
						DEFAULT_PITCH | FF_DONTCARE, L"IM FELL English");
	headingFont_ = CreateFontW(22, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
							   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
														   CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"IM FELL English");

	auto createButton = [&](int index, int id, const wchar_t* text) {
		const DWORD style = WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON | BS_OWNERDRAW;
		controls_[index] = CreateWindowW(L"BUTTON", text, style,
										 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(id),
										 GetModuleHandleW(nullptr), nullptr);
		SendMessageW(controls_[index], WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
	};

	for (int i = 0; i < 4; ++i) createButton(i, RoomButtonBase + i, L"Room");
	for (int i = 0; i < 4; ++i) createButton(4 + i, InspectButtonBase + i, L"Inspect");
	for (int i = 0; i < 12; ++i) createButton(8 + i, InventoryButtonBase + i, L"Item");
	createButton(20, ButtonSearch, L"Investigate");
	createButton(21, ButtonBack, L"Back");
	createButton(22, ButtonMap, L"Map");
	createButton(23, ButtonSkills, L"Skills");
	createButton(24, ButtonAttack, L"ATTACK");
	createButton(26, ButtonShimmer, L"SHIMMER");
	createButton(27, ButtonFlee, L"FLEE");
	createButton(28, ButtonQuit, L"Quit");
	createButton(29, ButtonInventory, L"Inventory");
	createButton(30, ButtonSort, L"Sort");
	createButton(31, ButtonLock, L"Lock / unlock");
	createButton(32, ButtonDiscard, L"Discard");
	createButton(33, ButtonEquip, L"Use");
	createButton(34, ButtonInventoryPrevious, L"<");
	createButton(35, ButtonInventoryNext, L">");
	RefreshControls();
}

void Win32UI::RefreshControls() {
	if (controls_[0] == nullptr || refreshingControls_) return;
	refreshingControls_ = true;
	for (HWND control : controls_) if (control != nullptr) ShowWindow(control, SW_HIDE);

	if (view_ == View::Map) {
		const auto& rooms = mapManager_.GetRooms();
		visibleRoomIndices_.clear();
		for (std::size_t i = 0; i < rooms.size() && visibleRoomIndices_.size() < 4; ++i) {
			if (!rooms[i].IsUnlocked()) continue;
			visibleRoomIndices_.push_back(static_cast<int>(i));
			SetWindowTextW(controls_[visibleRoomIndices_.size() - 1], ToWide(rooms[i].getName()).c_str());
			ShowWindow(controls_[visibleRoomIndices_.size() - 1], SW_SHOW);
		}
		ShowWindow(controls_[22], SW_SHOW);
		ShowWindow(controls_[29], SW_SHOW);
		ShowWindow(controls_[23], SW_SHOW);
		ShowWindow(controls_[28], SW_SHOW);
	} else if (view_ == View::Floor) {
		ShowWindow(controls_[22], SW_SHOW);
		ShowWindow(controls_[29], SW_SHOW);
		ShowWindow(controls_[23], SW_SHOW);
		ShowWindow(controls_[28], SW_SHOW);
	} else if (view_ == View::Room && selectedRoom_ >= 0) {
		if (selectedRoom_ >= static_cast<int>(mapManager_.GetRooms().size())) {
			view_ = View::Floor;
		} else {
			const auto& interactables = mapManager_.GetRooms()[selectedRoom_].GetVisibleInteractables();
			for (std::size_t i = 0; i < interactables.size() && i < 4; ++i) {
				SetWindowTextW(controls_[4 + i], ToWide(interactables[i]).c_str());
				ShowWindow(controls_[4 + i], SW_SHOW);
			}
			ShowWindow(controls_[20], SW_SHOW);
			ShowWindow(controls_[21], SW_SHOW);
			ShowWindow(controls_[22], SW_SHOW);
		}
	} else if (view_ == View::Inventory) {
		inventoryEntryItems_.clear();
		inventoryEntryDescriptions_.clear();
		const auto items = player_.GetInventory().GetSortedItems(inventorySort_);
		for (const auto* item : items) {
			for (int quantity = 0; quantity < player_.GetInventory().GetQuantity(item->getName()); ++quantity) {
				inventoryEntryItems_.push_back(const_cast<Item*>(item));
				inventoryEntryDescriptions_.push_back(item->getDescription());
			}
		}
		for (const auto& clue : player_.GetInventory().GetClues()) {
			inventoryEntryItems_.push_back(nullptr);
			inventoryEntryDescriptions_.push_back(clue.description);
		}
		const int pageSize = 12;
		const int pageCount = (static_cast<int>(inventoryEntryItems_.size()) + pageSize - 1) / pageSize;
		if (inventoryPage_ >= pageCount && pageCount > 0) inventoryPage_ = pageCount - 1;
		const int firstEntry = inventoryPage_ * pageSize;
		for (int button = 0; button < pageSize; ++button) {
			const int entry = firstEntry + button;
			if (entry < static_cast<int>(inventoryEntryItems_.size())) {
				const Item* item = inventoryEntryItems_[entry];
				const std::string label = item == nullptr ? "Clue" : item->getName();
				SetWindowTextW(controls_[8 + button], ToWide(label).c_str());
				ShowWindow(controls_[8 + button], SW_SHOW);
			}
		}
		ShowWindow(controls_[21], SW_SHOW);
		ShowWindow(controls_[30], SW_SHOW);
		ShowWindow(controls_[31], SW_SHOW);
		ShowWindow(controls_[32], SW_SHOW);
		ShowWindow(controls_[33], SW_SHOW);
		ShowWindow(controls_[34], SW_SHOW);
		ShowWindow(controls_[35], SW_SHOW);
	} else if (view_ == View::Combat) {
		ShowWindow(controls_[24], SW_SHOW);
		ShowWindow(controls_[26], SW_SHOW);
		ShowWindow(controls_[27], SW_SHOW);
	} else {
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
	const int inventoryActionY = eventTop + 190;
	const int inventoryWidth = (std::max)(150, (panelRight - center - 36) / 4);
	const int inventoryGap = 12;

	for (int i = 0; i < 4; ++i) {
		MoveWindow(controls_[i], center + (i % 2) * 225, actionY + (i / 2) * 50,
				   buttonWidth, buttonHeight, TRUE);
	}
	for (int i = 0; i < 4; ++i) {
		const int column = i % 2;
		const int row = i / 2;
		MoveWindow(controls_[4 + i], center + column * (inventoryWidth * 2 + inventoryGap * 2),
				   actionY + row * 54, inventoryWidth * 2 + inventoryGap, buttonHeight, TRUE);
	}
	for (int i = 0; i < 12; ++i) {
		MoveWindow(controls_[8 + i], center + (i % 3) * (inventoryWidth + inventoryGap),
				   132 + (i / 3) * 48, inventoryWidth, 38, TRUE);
	}
	MoveWindow(controls_[20], center, roomActionY, buttonWidth, buttonHeight, TRUE);
	MoveWindow(controls_[21], center + buttonWidth + 20, roomActionY, buttonWidth, buttonHeight, TRUE);
	MoveWindow(controls_[22], panelRight - 180, 40, 180, buttonHeight, TRUE);
	MoveWindow(controls_[29], panelRight - 180, 92, 180, buttonHeight, TRUE);
	MoveWindow(controls_[23], panelRight - 180, 144, 180, buttonHeight, TRUE);
	MoveWindow(controls_[28], panelRight - 180, 196, 180, buttonHeight, TRUE);
	MoveWindow(controls_[24], center, actionY, 180, 44, TRUE);
	MoveWindow(controls_[26], center + 190, actionY, 180, 44, TRUE);
	MoveWindow(controls_[27], center + 380, actionY, 180, 44, TRUE);
	for (int i = 0; i < 4; ++i) {
		MoveWindow(controls_[30 + i], center + i * (inventoryWidth + inventoryGap), inventoryActionY,
				   inventoryWidth, 42, TRUE);
	}
	MoveWindow(controls_[34], center, inventoryActionY + 50, 70, 38, TRUE);
	MoveWindow(controls_[35], panelRight - 70, inventoryActionY + 50, 70, 38, TRUE);
}

void Win32UI::HandleCommand(int controlId) {
	if (handlingCommand_) return;
	handlingCommand_ = true;
	if (controlId >= RoomButtonBase && controlId < RoomButtonBase + 4) {
		const int buttonIndex = controlId - RoomButtonBase;
		if (buttonIndex < static_cast<int>(visibleRoomIndices_.size())) EnterRoom(visibleRoomIndices_[buttonIndex]);
	} else if (controlId >= InspectButtonBase && controlId < InspectButtonBase + 4) {
		if (view_ == View::Room && selectedRoom_ >= 0 &&
			selectedRoom_ < static_cast<int>(mapManager_.GetRooms().size())) {
			const auto& interactables = mapManager_.GetRooms()[selectedRoom_].GetVisibleInteractables();
			const int index = controlId - InspectButtonBase;
			if (index < static_cast<int>(interactables.size())) Inspect(index);
		}
	} else if (controlId >= InventoryButtonBase && controlId < InventoryButtonBase + 12) {
		const int index = inventoryPage_ * 12 + controlId - InventoryButtonBase;
		if (index < static_cast<int>(inventoryEntryItems_.size())) {
			selectedInventoryIndex_ = index;
			statusMessage_ = inventoryEntryItems_[index] == nullptr ? "Clue selected." :
				inventoryEntryItems_[index]->getName() + " selected.";
		}
	} else if (controlId == ButtonInventoryPrevious) {
		if (inventoryPage_ > 0) --inventoryPage_;
		selectedInventoryIndex_ = -1;
	} else if (controlId == ButtonInventoryNext) {
		const int pageCount = (static_cast<int>(inventoryEntryItems_.size()) + 11) / 12;
		if (inventoryPage_ + 1 < pageCount) ++inventoryPage_;
		selectedInventoryIndex_ = -1;
	} else if (controlId == ButtonSearch) {
		if (view_ == View::Room && selectedRoom_ >= 0 &&
			selectedRoom_ < static_cast<int>(mapManager_.GetRooms().size())) SearchRoom();
	} else if (controlId == ButtonBack) {
		view_ = View::Floor;
	} else if (controlId == ButtonMap) {
		view_ = View::Map;
	} else if (controlId == ButtonInventory) {
		view_ = View::Inventory;
	} else if (controlId == ButtonSort) {
		inventorySort_ = inventorySort_ == InventorySort::RecentlyObtained ? InventorySort::Alphabetical
			: inventorySort_ == InventorySort::Alphabetical ? InventorySort::Amount : InventorySort::RecentlyObtained;
		selectedInventoryIndex_ = -1;
		inventoryPage_ = 0;
	} else if (controlId == ButtonLock || controlId == ButtonDiscard) {
		if (selectedInventoryIndex_ >= 0 && selectedInventoryIndex_ < static_cast<int>(inventoryEntryItems_.size()) &&
			inventoryEntryItems_[selectedInventoryIndex_] != nullptr) {
			const std::string name = inventoryEntryItems_[selectedInventoryIndex_]->getName();
			if (controlId == ButtonLock) {
				player_.GetInventory().SetLocked(name, !player_.GetInventory().IsLocked(name));
				AddLog((player_.GetInventory().IsLocked(name) ? "Locked " : "Unlocked ") + name + ".");
			} else if (player_.GetInventory().DiscardItem(name)) AddLog("Discarded " + name + ".");
			selectedInventoryIndex_ = -1;
		}
	} else if (controlId == ButtonEquip) {
		if (selectedInventoryIndex_ >= 0 && selectedInventoryIndex_ < static_cast<int>(inventoryEntryItems_.size()) &&
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
		view_ = View::Skills;
	} else if (controlId == ButtonAttack) {
		PerformCombatAction(CombatAction::Attack);
	} else if (controlId == ButtonShimmer) {
		PerformCombatAction(CombatAction::Shimmer);
	} else if (controlId == ButtonFlee) {
		PerformCombatAction(CombatAction::Flee);
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
	if (roomIndex < 0 || roomIndex >= static_cast<int>(mapManager_.GetRooms().size())) return;
	selectedRoom_ = roomIndex;
	Area& area = mapManager_.GetRooms()[roomIndex];
	if (!area.IsUnlocked()) return;
	activeEnemy_ = area.getFirstAliveEnemy();
	view_ = activeEnemy_ != nullptr && activeEnemy_->isAlive() ? View::Combat : View::Room;
	if (view_ == View::Room) activeEnemy_ = nullptr;
	AddLog("Entered " + area.getName() + ". Investigate the visible interactions.");
}

void Win32UI::Inspect(int interactableIndex) {
	if (selectedRoom_ < 0 || selectedRoom_ >= static_cast<int>(mapManager_.GetRooms().size())) return;
	Area& area = mapManager_.GetRooms()[selectedRoom_];
	if (interactableIndex < 0 || interactableIndex >= static_cast<int>(area.GetVisibleInteractables().size())) return;
	const std::string object = area.GetVisibleInteractables()[interactableIndex];
	if (area.ConsumeClue()) {
		player_.AddClue({"A fragment recovered from the " + object + "."});
		progression_.GrantXP(10);
		progression_.CheckLevelUp(player_);
		AddLog("Investigation: clue recovered from " + object + ".");
		statusMessage_ = "Investigation complete: clue recovered.";
	} else {
		AddLog("Investigation: nothing new revealed at " + object + ".");
		statusMessage_ = "Investigation complete: nothing new revealed.";
	}
}

void Win32UI::SearchRoom() {
	if (selectedRoom_ < 0 || selectedRoom_ >= static_cast<int>(mapManager_.GetRooms().size())) return;
	statusMessage_ = "Investigating the room...";
	Area& area = mapManager_.GetRooms()[selectedRoom_];
	area.RecordSearch();
	const unsigned searchSeed = RoomSeed(area.getName()) + static_cast<unsigned>(area.GetSearchCount() * 31);
	if (activeEnemy_ == nullptr && area.IsFullySearched() && area.HasBossBeenDefeated()) {
		const unsigned roll = searchSeed % 100;
		if (roll < 10) {
			const int enemyLevel = progression_.GetLevel() + 5;
			area.addEnemy(std::make_unique<Enemy>("Cathedral Boss", 70 + enemyLevel * 8,
				20 + enemyLevel * 3, 200, enemyLevel));
			AddLog("Search: the defeated boss returns in a stronger form.");
		} else if (roll < 60) {
			const int enemyLevel = (std::max)(1, progression_.GetLevel() + static_cast<int>(searchSeed % 6) - 2);
			area.addEnemy(std::make_unique<Enemy>("Nun", 18 + enemyLevel * 2, 4 + enemyLevel, 15, enemyLevel));
			AddLog("Search: a Nun emerges from the cleared room.");
		} else {
			const int enemyLevel = (std::max)(1, progression_.GetLevel() + static_cast<int>(searchSeed % 6) - 2);
			area.addEnemy(std::make_unique<Enemy>("Automata", 12 + enemyLevel * 2, 3 + enemyLevel, 15, enemyLevel));
			AddLog("Search: an Automata stirs in the cleared room.");
		}
		activeEnemy_ = area.getFirstAliveEnemy();
		if (activeEnemy_ == nullptr || !activeEnemy_->isAlive()) {
			activeEnemy_ = nullptr;
			view_ = View::Room;
			return;
		}
		combat_.BeginEncounter(*activeEnemy_);
		view_ = View::Combat;
		return;
	}
	if (activeEnemy_ == nullptr && area.IsFullySearched() && !area.HasBossBeenEncountered() &&
		(searchSeed % 100) < 50) {
		area.MarkBossEncountered();
		const int enemyLevel = progression_.GetLevel() + 3;
		area.addEnemy(std::make_unique<Enemy>("Cathedral Boss", 35 + enemyLevel * 5,
			10 + enemyLevel * 2, 100, enemyLevel));
		activeEnemy_ = area.getFirstAliveEnemy();
		if (activeEnemy_ == nullptr) return;
		combat_.BeginEncounter(*activeEnemy_);
		view_ = View::Combat;
		AddLog("Search: the Cathedral Boss emerges from the investigated room.");
		return;
	}
	if (activeEnemy_ == nullptr && (searchSeed % 100) < SpawnChance(area.GetSpawnCondition())) {
		const int enemyLevel = (std::max)(1, progression_.GetLevel() + static_cast<int>(searchSeed % 6) - 2);
		switch (searchSeed % 2) {
		case 0: area.addEnemy(std::make_unique<Enemy>("Automata", 12 + enemyLevel * 2, 3 + enemyLevel, 15, enemyLevel)); break;
		default: area.addEnemy(std::make_unique<Enemy>("Nun", 18 + enemyLevel * 2, 4 + enemyLevel, 15, enemyLevel)); break;
		}
		activeEnemy_ = area.getFirstAliveEnemy();
		if (activeEnemy_ == nullptr) return;
		combat_.BeginEncounter(*activeEnemy_);
		view_ = View::Combat;
		AddLog("Search: an enemy ambushes you in " + area.getName() + ".");
		return;
	}
	if (area.IsFullySearched() && !area.HasBossBeenEncountered() &&
		static_cast<int>((RoomSeed(area.getName()) + area.GetSearchCount()) % 100) < 50) {
		area.MarkBossEncountered();
		const int enemyLevel = progression_.GetLevel() + 3;
		area.addEnemy(std::make_unique<Enemy>("Cathedral Boss", 35 + enemyLevel * 5,
			10 + enemyLevel * 2, 100, enemyLevel));
		activeEnemy_ = area.getFirstAliveEnemy();
		if (activeEnemy_ == nullptr) return;
		combat_.BeginEncounter(*activeEnemy_);
		view_ = View::Combat;
		AddLog("Search: the Cathedral Boss emerges from the investigated room.");
		return;
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
	if (selectedRoom_ < 0 || selectedRoom_ >= static_cast<int>(mapManager_.GetRooms().size()) ||
		activeEnemy_ == nullptr || !activeEnemy_->isAlive()) {
		activeEnemy_ = nullptr;
		view_ = View::Room;
		return;
	}
	if (action != CombatAction::Flee && !combat_.IsPlayerTurn()) {
		AddLog("Wait for your turn.");
		return;
	}
	if (action == CombatAction::Flee) {
		AddLog(player_.getName() + " withdrew from combat.");
		statusMessage_ = "Battle ended: " + player_.getName() + " fled.";
		activeEnemy_ = nullptr;
		view_ = View::Room;
		return;
	}
	if (!combat_.resolve(action, player_, *activeEnemy_, nullptr, &progression_)) {
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
		const bool wasBoss = defeatedName == "Cathedral Boss";
		area.removeDeadEnemies();
		statusMessage_ = "Battle ended: " + defeatedName + " defeated. XP gained: " + std::to_string(xpReward) + ".";
		AddLog("Battle ended: " + defeatedName + " defeated. XP gained: " + std::to_string(xpReward) + ".");
		if (wasBoss) {
			area.SetBossDefeated(true);
			if (selectedRoom_ + 1 < static_cast<int>(mapManager_.GetRooms().size())) {
				mapManager_.GetRooms()[selectedRoom_ + 1].SetUnlocked(true);
				AddLog("The next area is now visible on the map.");
			}
		}
		const unsigned dropRoll = RoomSeed(defeatedName) + static_cast<unsigned>(defeatedLevel * 17 + area.GetSearchCount());
		const bool canDropEpic = wasBoss;
		if (dropRoll % 100 < (std::min)(80, 10 + defeatedLevel)) {
			const auto catalog = BuildEquippableCatalog();
			const int index = static_cast<int>(dropRoll % catalog.size());
			const bool allowed = canDropEpic ? catalog[index].rarity != Rarity::Legendary && catalog[index].rarity != Rarity::Artifact
				: catalog[index].rarity == Rarity::Common || catalog[index].rarity == Rarity::Uncommon || catalog[index].rarity == Rarity::Rare;
			if (allowed) {
				player_.AddItem(std::make_unique<Item>(catalog[index]));
				AddLog("Drop: " + catalog[index].name + ".");
				statusMessage_ += " Item dropped: " + catalog[index].name + ".";
			}
		}
		activeEnemy_ = nullptr;
		view_ = View::Room;
		return;
	}
	combat_.enemyTurn(player_, *activeEnemy_);
	statusMessage_ = player_.isAlive() ? "The enemy acted. " + player_.getName() + "'s turn." : player_.getName() + " has fallen.";
	if (!player_.isAlive()) AddLog(player_.getName() + " has fallen.");
	if (!player_.isAlive()) {
		activeEnemy_ = nullptr;
		view_ = View::Room;
	}
}

void Win32UI::AddLog(const std::string& message) {
	log_.push_back(message);
	if (log_.size() > 12) log_.erase(log_.begin());
}

void Win32UI::InitializePlayer() {
	const auto equipment = BuildEquippableCatalog();
	AddNamedItem(player_, equipment, "plain_rosary");
	AddNamedItem(player_, equipment, "linen_veil");
	AddNamedItem(player_, BuildConsumableCatalog(), "cellar_rations");
	AddNamedItem(player_, BuildConsumableCatalog(), "purification_salts");
	AddNamedItem(player_, BuildClueCatalog(), "storage_key");
	AddNamedItem(player_, BuildCruxCatalog(), "first_nuns_diary");
	if (Item* hand = player_.FindItem("plain_rosary")) player_.Equip(*hand);
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
		return 0;
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED) HandleCommand(LOWORD(wParam));
		return 0;
	case WM_DRAWITEM: {
		const auto* draw = reinterpret_cast<const DRAWITEMSTRUCT*>(lParam);
		if (draw != nullptr && draw->CtlType == ODT_BUTTON) {
			const bool inventoryButton = draw->CtlID >= InventoryButtonBase && draw->CtlID < InventoryButtonBase + 12;
			HBRUSH brush = CreateSolidBrush(((draw->itemState & ODS_SELECTED) != 0) ? RGB(45, 45, 45) : Background);
			FillRect(draw->hDC, &draw->rcItem, brush);
			DeleteObject(brush);
			FrameRect(draw->hDC, &draw->rcItem, reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
			wchar_t label[128]{};
			GetWindowTextW(draw->hwndItem, label, 128);
			SetBkMode(draw->hDC, TRANSPARENT);
			if (inventoryButton) {
				const int itemIndex = inventoryPage_ * 12 + draw->CtlID - InventoryButtonBase;
				const Item* item = itemIndex < static_cast<int>(inventoryEntryItems_.size())
					? inventoryEntryItems_[itemIndex] : nullptr;
				SetTextColor(draw->hDC, item == nullptr ? Muted : RarityColor(item->getRarity()));
				DrawTextW(draw->hDC, label, -1, const_cast<RECT*>(&draw->rcItem),
					DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			} else {
				SetTextColor(draw->hDC, Text);
				DrawTextW(draw->hDC, label, -1, const_cast<RECT*>(&draw->rcItem),
					DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
			}
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
	DrawRainbowText(deviceContext, 42, 110, ToWide(player_.getName()), 19, true);
	DrawTextLine(deviceContext, 250, 110, L"LEVEL " + std::to_wstring(progression_.GetLevel()), Text, 19, true);
	DrawTextLine(deviceContext, 42, 150, L"HP", Muted); DrawTextLine(deviceContext, 150, 150, std::to_wstring(player_.getHP()) + L" / " + std::to_wstring(player_.getMaxHP()), Text);
	DrawTextLine(deviceContext, 42, 182, L"DEF   " + std::to_wstring(player_.getDefense()) + L"     SPD   " + std::to_wstring(player_.getSpeed()), Text);
	DrawTextLine(deviceContext, 42, 214, L"ATK   " + std::to_wstring(player_.getAttack()) + L"     ACC   " + std::to_wstring(player_.getAccuracy()) + L"%", Text);
	DrawTextLine(deviceContext, 42, 246, L"RES   " + std::to_wstring(player_.getResistance()) + L"%    CRIT  " + std::to_wstring(player_.getCritRate()) + L"% / " + std::to_wstring(player_.getCritDamage()) + L"%", Text, 15);
	DrawTextLine(deviceContext, 42, 290, L"STATUS", Brass, 17, true);
	int y = 320;
	for (const auto& status : player_.GetStatuses()) {
		DrawTextLine(deviceContext, 48, y, StatusName(status.type) + L" (" + std::to_wstring(status.turns) + L")", Danger, 15);
		y += 25;
	}
	if (player_.GetStatuses().empty()) DrawTextLine(deviceContext, 48, y, L"No active effects", Muted, 15);
	DrawTextLine(deviceContext, 42, 430, L"EQUIPMENT", Brass, 17, true);
	const std::pair<EquipSlot, const wchar_t*> slots[] = {{EquipSlot::Head, L"Head"}, {EquipSlot::Torso, L"Torso"}, {EquipSlot::Boots, L"Boots"}, {EquipSlot::Hand, L"Hand"}};
	y = 460;
	for (const auto& slot : slots) {
		const Item* item = player_.GetEquipped(slot.first);
		DrawTextLine(deviceContext, 48, y, std::wstring(slot.second) + L": ", Muted, 14);
		if (item != nullptr) DrawTextLine(deviceContext, 105, y, ToWide(item->getName()), RarityColor(item->getRarity()), 14);
		else DrawTextLine(deviceContext, 105, y, L"--", Muted, 14);
		y += 25;
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
		DrawTextLine(deviceContext, 425, contentY, L"HOSTILE: " + ToWide(activeEnemy_->getName()), Danger, 26, true);
		DrawTextLine(deviceContext, 425, contentY + 48, L"HP  " + std::to_wstring(activeEnemy_->getHP()) + L" / " + std::to_wstring(activeEnemy_->getMaxHP()), Text, 20);
		DrawTextLine(deviceContext, 425, contentY + 84, L"ATK " + std::to_wstring(activeEnemy_->getAtk()) + L"   DEF " + std::to_wstring(activeEnemy_->getDefense()) + L"   SPD " + std::to_wstring(activeEnemy_->getSpeed()), Muted, 17);
		DrawTextLine(deviceContext, 425, contentY + 145, L"Turn order", Brass, 18, true);
		DrawTextLine(deviceContext, 425, contentY + 180, ToWide(player_.getName()) + L"  >  " + ToWide(activeEnemy_->getName()), Text, 18);
		if (!statusMessage_.empty()) DrawTextLine(deviceContext, 425, contentY + 225, ToWide(statusMessage_), Shimmer, 16);
	} else if (view_ == View::Inventory) {
		DrawPanel(deviceContext, {395, eventTop, client.right - 20, client.bottom - 20}, L"ITEM INFORMATION");
		DrawTextLine(deviceContext, 425, contentY, L"INVENTORY // SELECT AN ITEM, THEN EQUIP", Shimmer, 23, true);
		DrawTextLine(deviceContext, 425, contentY + 42, L"Select an item to view its details below.", Muted, 16);
		if (!statusMessage_.empty()) DrawTextLine(deviceContext, 425, contentY + 70, ToWide(statusMessage_), Shimmer, 15);
		if (selectedInventoryIndex_ >= 0 && selectedInventoryIndex_ < static_cast<int>(inventoryEntryItems_.size())) {
			const Item* selected = inventoryEntryItems_[selectedInventoryIndex_];
			const std::string description = inventoryEntryDescriptions_[selectedInventoryIndex_];
			const Rarity rarity = selected == nullptr ? Rarity::Common : selected->getRarity();
			DrawTextLine(deviceContext, 425, eventTop + 42, ToWide(selected == nullptr ? "Clue" : selected->getName()), RarityColor(rarity), 20, true);
			if (selected == nullptr) {
				DrawTextLine(deviceContext, 425, eventTop + 74, L"Type: Clue", Muted, 16);
				DrawTextLine(deviceContext, 425, eventTop + 108, ToWide(description), Text, 16);
			} else {
				std::wstring type = L"Type: ";
				switch (selected->getCategory()) {
				case ItemCategory::Consumable: type += L"Consumable"; break;
				case ItemCategory::Crux: type += L"Crux"; break;
				case ItemCategory::Clue: type += L"Clue"; break;
				case ItemCategory::ShimmerSkill: type += L"Shimmer skill"; break;
				case ItemCategory::Equippable:
					switch (selected->getSlot()) {
					case EquipSlot::Head: type += L"Head"; break;
					case EquipSlot::Torso: type += L"Torso"; break;
					case EquipSlot::Boots: type += L"Feet"; break;
					case EquipSlot::Hand: type += L"Hands"; break;
					default: type += L"Equipment"; break;
					}
					break;
				}
				DrawTextLine(deviceContext, 425, eventTop + 74, type, Muted, 16);
				const Stats& stats = selected->getStats();
				const bool hasStats = stats.hp != 0 || stats.def != 0 || stats.spd != 0 || stats.atk != 0 ||
					stats.acc != 0 || stats.res != 0 || stats.critRate != 0 || stats.critDamage != 0;
				int detailY = eventTop + 108;
				if (hasStats) {
					DrawTextLine(deviceContext, 425, detailY,
						L"Stats: HP " + std::to_wstring(stats.hp) + L"  DEF " + std::to_wstring(stats.def) +
						L"  ATK " + std::to_wstring(stats.atk) + L"  SPD " + std::to_wstring(stats.spd), Text, 16);
					detailY += 32;
				}
				if (selected->getCategory() == ItemCategory::Consumable) {
					DrawTextLine(deviceContext, 425, detailY, L"Effect: " + ToWide(selected->getEffect()), Text, 16);
					detailY += 32;
				} else if (selected->getCategory() == ItemCategory::Crux) {
					DrawTextLine(deviceContext, 425, detailY, L"Lore: " + ToWide(selected->getDescription()), Text, 16);
					detailY += 32;
				} else {
					DrawTextLine(deviceContext, 425, detailY, ToWide(description), Text, 16);
				}
				if (!selected->getUniqueEffect().empty()) {
					DrawTextLine(deviceContext, 425, detailY + 32, L"Special: " + ToWide(selected->getUniqueEffect()), Muted, 15);
				}
			}
		}
	} else if (view_ == View::Map) {
		DrawTextLine(deviceContext, 425, contentY, L"FLOOR I // MAP", Shimmer, 25, true);
		int mapY = contentY + 55;
		for (const auto& room : mapManager_.GetRooms()) {
			if (room.IsUnlocked()) {
				DrawTextLine(deviceContext, 440, mapY, L"[ " + ToWide(room.getName()) + L" ]", Text, 18);
				mapY += 32;
			}
		}
	} else if (view_ == View::Skills) {
		DrawTextLine(deviceContext, 425, contentY, L"SKILLS // LATIN RESONANCE", Shimmer, 23, true);
		const std::vector<std::wstring> skills = {L"Pulsus Luminis   CD 3", L"Velum Radiāns    CD 4", L"Gradus Coruscus  CD 2", L"Fulgur Aurorae   CD 5", L"Lux Sanctorum    CD 4"};
		contentY += 55;
		for (const auto& skill : skills) { DrawTextLine(deviceContext, 440, contentY, skill, Text, 18); contentY += 34; }
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
		HFONT measureFont = CreateFontW(size, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
			DEFAULT_PITCH | FF_DONTCARE, L"IM FELL English");
		HFONT oldFont = static_cast<HFONT>(SelectObject(deviceContext, measureFont));
		SIZE characterSize{};
		GetTextExtentPoint32W(deviceContext, character.c_str(), 1, &characterSize);
		SelectObject(deviceContext, oldFont);
		DeleteObject(measureFont);
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

void Win32UI::DrawTextLine(HDC deviceContext, int x, int y, const std::wstring& text,
						   COLORREF color, int size, bool bold) const {
	HFONT selected = CreateFontW(size, 0, 0, 0, bold ? FW_SEMIBOLD : FW_NORMAL, FALSE, FALSE, FALSE,
								 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
								  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"IM FELL English");
	HGDIOBJ oldFont = SelectObject(deviceContext, selected);
	SetTextColor(deviceContext, color);
	SetBkMode(deviceContext, TRANSPARENT);
	RECT client{};
	GetClientRect(window_, &client);
	const int right = client.right > x + 1 ? static_cast<int>(client.right) - 24 : x + 1;
	const int bottom = client.bottom < y + size + 10 ? static_cast<int>(client.bottom) - 8 : y + size + 10;
	RECT textBounds{x, y, right, bottom};
	DrawTextW(deviceContext, text.c_str(), static_cast<int>(text.size()), &textBounds,
			  DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS | DT_VCENTER);
	SelectObject(deviceContext, oldFont);
	DeleteObject(selected);
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
