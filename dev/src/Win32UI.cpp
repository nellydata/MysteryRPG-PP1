#include "Win32UI.h"
#include "Automata.h"
#include "ContentCatalog.h"
#include "Nun.h"
#include "ShimmerSkill.h"
#include <algorithm>
#include <cstdlib>
#include <cstdio>

namespace {
constexpr COLORREF Background = RGB(22, 17, 25);
constexpr COLORREF Panel = RGB(37, 27, 38);
constexpr COLORREF PanelEdge = RGB(137, 103, 105);
constexpr COLORREF Text = RGB(235, 224, 211);
constexpr COLORREF Muted = RGB(173, 151, 151);
constexpr COLORREF Brass = RGB(213, 163, 92);
constexpr COLORREF Verdigris = RGB(120, 188, 166);
constexpr COLORREF Shimmer = RGB(190, 170, 255);
constexpr COLORREF Danger = RGB(220, 105, 95);

unsigned RoomSeed(const std::string& roomName) {
	unsigned seed = 2166136261u;
	for (unsigned char character : roomName) {
		seed ^= character;
		seed *= 16777619u;
	}
	return seed;
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
	  view_(View::Floor), selectedRoom_(0), player_("Adelaide", 30, 10),
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
		player_.SetName("Investigator");
		return;
	}
	ShowWindow(nameWindow_, SW_SHOW);
	UpdateWindow(nameWindow_);

	MSG message{};
	while (IsWindow(nameWindow_) && GetMessageW(&message, nullptr, 0, 0) > 0) {
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}
	if (player_.getName() == "Adelaide") player_.SetName("Investigator");
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
		const DWORD style = WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON |
			(index < 4 ? BS_OWNERDRAW : 0);
		controls_[index] = CreateWindowW(L"BUTTON", text, style,
										 0, 0, 0, 0, window_, reinterpret_cast<HMENU>(id),
										 GetModuleHandleW(nullptr), nullptr);
		SendMessageW(controls_[index], WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
	};

	for (int i = 0; i < 4; ++i) createButton(i, RoomButtonBase + i, L"Room");
	for (int i = 0; i < 4; ++i) createButton(4 + i, InspectButtonBase + i, L"Inspect");
	for (int i = 0; i < 12; ++i) createButton(8 + i, InventoryButtonBase + i, L"Item");
	createButton(20, ButtonSearch, L"Search room");
	createButton(21, ButtonBack, L"Back");
	createButton(22, ButtonMap, L"Map");
	createButton(23, ButtonSkills, L"Shimmer skills");
	createButton(24, ButtonAttack, L"ATTACK");
	createButton(25, ButtonBlock, L"BLOCK");
	createButton(26, ButtonShimmer, L"SHIMMER");
	createButton(27, ButtonFlee, L"FLEE");
	createButton(28, ButtonQuit, L"Quit");
	createButton(29, ButtonInventory, L"Inventory");
	createButton(30, ButtonSort, L"Sort");
	createButton(31, ButtonLock, L"Lock / unlock");
	createButton(32, ButtonDiscard, L"Discard");
	createButton(33, ButtonEquip, L"Equip / unequip");
	RefreshControls();
}

void Win32UI::RefreshControls() {
	if (controls_[0] == nullptr) return;
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
		const auto& interactables = mapManager_.GetRooms()[selectedRoom_].GetVisibleInteractables();
		for (std::size_t i = 0; i < interactables.size() && i < 4; ++i) {
			SetWindowTextW(controls_[4 + i], ToWide(interactables[i]).c_str());
			ShowWindow(controls_[4 + i], SW_SHOW);
		}
		ShowWindow(controls_[20], SW_SHOW);
		ShowWindow(controls_[21], SW_SHOW);
		ShowWindow(controls_[22], SW_SHOW);
	} else if (view_ == View::Inventory) {
		const auto items = player_.GetInventory().GetSortedItems(inventorySort_);
		int buttonIndex = 0;
		for (const auto* item : items) {
			for (int quantity = 0; quantity < player_.GetInventory().GetQuantity(item->getName()) && buttonIndex < 12; ++quantity) {
				SetWindowTextW(controls_[8 + buttonIndex], ToWide(item->getName()).c_str());
				ShowWindow(controls_[8 + buttonIndex], SW_SHOW);
				++buttonIndex;
			}
		}
		ShowWindow(controls_[21], SW_SHOW);
		ShowWindow(controls_[30], SW_SHOW);
		ShowWindow(controls_[31], SW_SHOW);
		ShowWindow(controls_[32], SW_SHOW);
		ShowWindow(controls_[33], SW_SHOW);
	} else if (view_ == View::Combat) {
		ShowWindow(controls_[24], SW_SHOW);
		ShowWindow(controls_[25], SW_SHOW);
		ShowWindow(controls_[26], SW_SHOW);
		ShowWindow(controls_[27], SW_SHOW);
	} else {
		ShowWindow(controls_[21], SW_SHOW);
	}
	Invalidate();
}

void Win32UI::LayoutControls(int width, int height) {
	const int center = 425;
	const int buttonWidth = 200;
	const int buttonHeight = 42;
	const int eventTop = (std::max)(120, height - 260);
	const int actionY = eventTop + 28;
	const int safeRight = (std::max)(center + 620, width - 20);

	for (int i = 0; i < 4; ++i) {
		MoveWindow(controls_[i], center + (i % 2) * 225, actionY + (i / 2) * 50,
				   buttonWidth, buttonHeight, TRUE);
	}
	for (int i = 0; i < 4; ++i) {
		MoveWindow(controls_[4 + i], center + (i % 2) * 225, actionY + (i / 2) * 50,
				   buttonWidth, buttonHeight, TRUE);
	}
	for (int i = 0; i < 12; ++i) {
		MoveWindow(controls_[8 + i], center + (i % 3) * 215,
				   125 + (i / 3) * 48, 200, 38, TRUE);
	}
	MoveWindow(controls_[20], center, actionY + 104, buttonWidth, buttonHeight, TRUE);
	MoveWindow(controls_[21], center + 215, actionY + 104, buttonWidth, buttonHeight, TRUE);
	MoveWindow(controls_[22], safeRight - 180, 40, 180, buttonHeight, TRUE);
	MoveWindow(controls_[29], safeRight - 180, 92, 180, buttonHeight, TRUE);
	MoveWindow(controls_[23], safeRight - 180, 144, 180, buttonHeight, TRUE);
	MoveWindow(controls_[28], safeRight - 180, 196, 180, buttonHeight, TRUE);
	for (int i = 0; i < 4; ++i) MoveWindow(controls_[24 + i], center + i * 165, actionY, 150, 44, TRUE);
	MoveWindow(controls_[30], center, actionY, 180, 42, TRUE);
	MoveWindow(controls_[31], center + 190, actionY, 180, 42, TRUE);
	MoveWindow(controls_[32], center + 380, actionY, 180, 42, TRUE);
	MoveWindow(controls_[33], center + 570, actionY, 180, 42, TRUE);
}

void Win32UI::HandleCommand(int controlId) {
	if (controlId >= RoomButtonBase && controlId < RoomButtonBase + 4) {
		const int buttonIndex = controlId - RoomButtonBase;
		if (buttonIndex < static_cast<int>(visibleRoomIndices_.size())) EnterRoom(visibleRoomIndices_[buttonIndex]);
	} else if (controlId >= InspectButtonBase && controlId < InspectButtonBase + 4) {
		Inspect(controlId - InspectButtonBase);
	} else if (controlId >= InventoryButtonBase && controlId < InventoryButtonBase + 12) {
		const int index = controlId - InventoryButtonBase;
		const auto items = player_.GetInventory().GetSortedItems(inventorySort_);
		std::vector<Item*> entries;
		for (Item* item : items)
			for (int quantity = 0; quantity < player_.GetInventory().GetQuantity(item->getName()); ++quantity)
				entries.push_back(item);
		if (index < static_cast<int>(entries.size())) {
			selectedInventoryIndex_ = index;
			AddLog("Selected " + entries[index]->getName() + ". Choose Equip / unequip below.");
		}
	} else if (controlId == ButtonSearch) {
		SearchRoom();
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
	} else if (controlId == ButtonLock || controlId == ButtonDiscard) {
		const auto items = player_.GetInventory().GetSortedItems(inventorySort_);
		if (!items.empty()) {
			const std::string name = items.front()->getName();
			if (controlId == ButtonLock) {
				player_.GetInventory().SetLocked(name, !player_.GetInventory().IsLocked(name));
				AddLog((player_.GetInventory().IsLocked(name) ? "Locked " : "Unlocked ") + name + ".");
			} else if (player_.GetInventory().DiscardItem(name)) AddLog("Discarded " + name + ".");
			selectedInventoryIndex_ = -1;
		}
	} else if (controlId == ButtonEquip) {
		const auto items = player_.GetInventory().GetSortedItems(inventorySort_);
		std::vector<Item*> entries;
		for (Item* item : items)
			for (int quantity = 0; quantity < player_.GetInventory().GetQuantity(item->getName()); ++quantity)
				entries.push_back(item);
		if (selectedInventoryIndex_ >= 0 && selectedInventoryIndex_ < static_cast<int>(entries.size())) {
			Item& item = *entries[selectedInventoryIndex_];
			const Item* equipped = player_.GetEquipped(item.getSlot());
			player_.ToggleEquip(item);
			AddLog(std::string(equipped == &item ? "Unequipped " : "Equipped ") + item.getName() + ".");
		}
	} else if (controlId == ButtonSkills) {
		view_ = View::Skills;
	} else if (controlId == ButtonAttack) {
		PerformCombatAction(CombatAction::Attack);
	} else if (controlId == ButtonBlock) {
		PerformCombatAction(CombatAction::Block);
	} else if (controlId == ButtonShimmer) {
		PerformCombatAction(CombatAction::Shimmer);
	} else if (controlId == ButtonFlee) {
		PerformCombatAction(CombatAction::Flee);
	} else if (controlId == ButtonQuit) {
		PostMessageW(window_, WM_CLOSE, 0, 0);
		return;
	}
	RefreshControls();
}

void Win32UI::EnterRoom(int roomIndex) {
	if (roomIndex < 0 || roomIndex >= static_cast<int>(mapManager_.GetRooms().size())) return;
	selectedRoom_ = roomIndex;
	Area& area = mapManager_.GetRooms()[roomIndex];
	if (!area.IsUnlocked()) return;
	activeEnemy_ = area.getFirstAliveEnemy();
	if (activeEnemy_ == nullptr && area.IsFullySearched() && !area.HasBossBeenEncountered() &&
		static_cast<int>(RoomSeed(area.getName()) % 100) < 50) {
		area.MarkBossEncountered();
		const int enemyLevel = progression_.GetLevel() + 3;
		area.addEnemy(std::make_unique<Enemy>("Cathedral Boss", 35 + enemyLevel * 5,
			10 + enemyLevel * 2, 100, enemyLevel));
		activeEnemy_ = area.getFirstAliveEnemy();
	} else if (activeEnemy_ == nullptr && static_cast<int>((RoomSeed(area.getName()) + area.GetSearchCount()) % 100) < SpawnChance(area.GetSpawnCondition())) {
		const int enemyLevel = progression_.GetLevel() + static_cast<int>(RoomSeed(area.getName()) % 6) - 2;
		const int boundedLevel = (std::max)(1, enemyLevel);
		switch (RoomSeed(area.getName()) % 3) {
		case 0: area.addEnemy(std::make_unique<Enemy>("Automata", 12 + boundedLevel * 2, 3 + boundedLevel, 15, boundedLevel)); break;
		case 1: area.addEnemy(std::make_unique<Enemy>("Nun", 18 + boundedLevel * 2, 4 + boundedLevel, 15, boundedLevel)); break;
		default: area.addEnemy(std::make_unique<Enemy>("Shimmer Husk", 10 + boundedLevel * 2, 3 + boundedLevel, 15, boundedLevel)); break;
		}
		activeEnemy_ = area.getFirstAliveEnemy();
	}
	if (activeEnemy_ != nullptr) {
		combat_.BeginEncounter(*activeEnemy_);
		view_ = View::Combat;
		AddLog("A hostile presence emerges in " + area.getName() + ".");
	} else {
		view_ = View::Room;
		AddLog("Entered " + area.getName() + ".");
	}
}

void Win32UI::Inspect(int interactableIndex) {
	Area& area = mapManager_.GetRooms()[selectedRoom_];
	if (interactableIndex < 0 || interactableIndex >= static_cast<int>(area.GetVisibleInteractables().size())) return;
	const std::string object = area.GetVisibleInteractables()[interactableIndex];
	if (area.ConsumeClue()) {
		player_.AddClue({"A fragment recovered from the " + object + "."});
		progression_.GrantXP(10);
		progression_.CheckLevelUp(player_);
		AddLog("Investigation: clue recovered from " + object + ".");
	} else {
		AddLog("Investigation: nothing new revealed at " + object + ".");
	}
}

void Win32UI::SearchRoom() {
	Area& area = mapManager_.GetRooms()[selectedRoom_];
	area.RecordSearch();
	const unsigned searchSeed = RoomSeed(area.getName()) + static_cast<unsigned>(area.GetSearchCount() * 31);
	if (activeEnemy_ == nullptr && area.IsFullySearched() && !area.HasBossBeenEncountered() &&
		(searchSeed % 100) < 50) {
		area.MarkBossEncountered();
		const int enemyLevel = progression_.GetLevel() + 3;
		area.addEnemy(std::make_unique<Enemy>("Cathedral Boss", 35 + enemyLevel * 5,
			10 + enemyLevel * 2, 100, enemyLevel));
		activeEnemy_ = area.getFirstAliveEnemy();
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
	if (activeEnemy_ == nullptr) return;
	if (action == CombatAction::Flee) {
		AddLog("Adelaide withdrew from combat.");
		view_ = View::Room;
		return;
	}
	if (!combat_.resolve(action, player_, *activeEnemy_, nullptr, &progression_)) {
		AddLog("That action is unavailable.");
		return;
	}
	if (!activeEnemy_->isAlive()) {
		AddLog("The enemy was defeated.");
		Area& area = mapManager_.GetRooms()[selectedRoom_];
		const bool wasBoss = activeEnemy_->getName() == "Cathedral Boss";
		area.removeDeadEnemies();
		if (wasBoss) {
			area.SetBossDefeated(true);
			if (selectedRoom_ + 1 < static_cast<int>(mapManager_.GetRooms().size())) {
				mapManager_.GetRooms()[selectedRoom_ + 1].SetUnlocked(true);
				AddLog("The next area is now visible on the map.");
			}
		}
		activeEnemy_ = nullptr;
		view_ = View::Room;
		return;
	}
	combat_.enemyTurn(player_, *activeEnemy_);
	AddLog("The enemy takes its turn.");
	if (!player_.isAlive()) AddLog("Adelaide has fallen.");
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
		CreateWindowW(L"STATIC", L"What should the investigator be called?",
					  WS_CHILD | WS_VISIBLE, 24, 22, 410, 24, window, nullptr,
					  GetModuleHandleW(nullptr), nullptr);
		ui->nameEdit_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
										WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
										24, 58, 410, 34, window, nullptr,
										GetModuleHandleW(nullptr), nullptr);
		CreateWindowW(L"BUTTON", L"Begin Investigation", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
					  24, 108, 190, 36, window, reinterpret_cast<HMENU>(1),
					  GetModuleHandleW(nullptr), nullptr);
		SetFocus(ui->nameEdit_);
		return 0;
	case WM_COMMAND:
		if (LOWORD(wParam) == 1 && HIWORD(wParam) == BN_CLICKED) {
			const int length = GetWindowTextLengthW(ui->nameEdit_);
			std::wstring name(static_cast<std::size_t>(length + 1), L'\0');
			if (length > 0) GetWindowTextW(ui->nameEdit_, name.data(), length + 1);
			if (!name.empty() && name.back() == L'\0') name.pop_back();
			if (name.empty()) name = L"Investigator";
			ui->player_.SetName(ui->ToNarrow(name));
			DestroyWindow(window);
			return 0;
		}
		break;
	case WM_CLOSE:
		ui->player_.SetName("Investigator");
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
	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED) HandleCommand(LOWORD(wParam));
		return 0;
	case WM_DRAWITEM: {
		const auto* draw = reinterpret_cast<const DRAWITEMSTRUCT*>(lParam);
		if (draw != nullptr && draw->CtlType == ODT_BUTTON && draw->CtlID >= RoomButtonBase &&
			draw->CtlID < RoomButtonBase + 4) {
			const int buttonIndex = draw->CtlID - RoomButtonBase;
			const bool complete = buttonIndex < static_cast<int>(visibleRoomIndices_.size()) &&
				mapManager_.GetRooms()[visibleRoomIndices_[buttonIndex]].IsFullySearched();
			HBRUSH brush = CreateSolidBrush((draw->itemState & ODS_SELECTED) ? RGB(210, 205, 198) : RGB(245, 242, 238));
			FillRect(draw->hDC, &draw->rcItem, brush);
			DeleteObject(brush);
			FrameRect(draw->hDC, &draw->rcItem, reinterpret_cast<HBRUSH>(GetStockObject(GRAY_BRUSH)));
			wchar_t label[128]{};
			GetWindowTextW(draw->hwndItem, label, 128);
			SetBkMode(draw->hDC, TRANSPARENT);
			if (!complete) {
				SetTextColor(draw->hDC, RGB(35, 30, 35));
				DrawTextW(draw->hDC, label, -1, const_cast<RECT*>(&draw->rcItem), DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			} else {
				const COLORREF colors[] = {RGB(210, 55, 65), RGB(220, 130, 45), RGB(190, 170, 45), RGB(50, 150, 90), RGB(65, 120, 210), RGB(145, 75, 180)};
				const int length = lstrlenW(label);
				RECT segment = draw->rcItem;
				for (int index = 0; index < length; ++index) {
					wchar_t character[2] = {label[index], L'\0'};
					SetTextColor(draw->hDC, colors[index % 6]);
					DrawTextW(draw->hDC, character, 1, &segment, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
				}
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
	DrawTextLine(deviceContext, 42, 110, ToWide(player_.getName()) + L"   LEVEL " + std::to_wstring(progression_.GetLevel()), Text, 19, true);
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
		DrawTextLine(deviceContext, 48, y, std::wstring(slot.second) + L": " + (item == nullptr ? L"--" : ToWide(item->getName())), Text, 14);
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
		DrawTextLine(deviceContext, 425, contentY + 38, L"Map and Shimmer Skills stay available in the upper-right.", Muted, 16);
	} else if (view_ == View::Room) {
		const Area& room = mapManager_.GetRooms()[selectedRoom_];
		DrawTextLine(deviceContext, 425, contentY, ToWide(room.getName()), Shimmer, 25, true);
		DrawTextLine(deviceContext, 425, contentY + 42, L"The cathedral holds its breath. Investigate an object below.", Muted);
		contentY += 100;
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
	} else if (view_ == View::Inventory) {
		DrawTextLine(deviceContext, 425, contentY, L"INVENTORY // SELECT AN ITEM, THEN EQUIP", Shimmer, 23, true);
		DrawTextLine(deviceContext, 425, contentY + 42, L"Each button represents one held item.", Muted, 16);
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
		DrawTextLine(deviceContext, 425, contentY, L"SHIMMER SKILLS // LATIN RESONANCE", Shimmer, 23, true);
		const std::vector<std::wstring> skills = {L"Pulsus Luminis   CD 3", L"Velum Radiāns    CD 4", L"Gradus Coruscus  CD 2", L"Fulgur Aurorae   CD 5", L"Lux Sanctorum    CD 4"};
		contentY += 55;
		for (const auto& skill : skills) { DrawTextLine(deviceContext, 440, contentY, skill, Text, 18); contentY += 34; }
	}

	DrawPanel(deviceContext, {395, eventTop, client.right - 20, client.bottom - 20}, L"EVENT LOG");
	int logY = eventTop + 38;
	const int start = (std::max)(0, static_cast<int>(log_.size()) - 3);
	for (int i = start; i < static_cast<int>(log_.size()); ++i) {
		DrawTextLine(deviceContext, 420, logY, ToWide(log_[i]), Muted, 14);
		logY += 22;
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
								 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
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
