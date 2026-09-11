// Runnable entry point for the progression regression headers.
//
// The game's progression rules live in ProgressionManager.cpp. The two headers
// exercised here (ProgressionRepairTests.h, ProgressionOwnershipTests.h) were
// previously compiled by nothing, so this target makes them part of the build.
#include "ContentCatalog.h"
#include "MapManager.h"
#include "Player.h"
#include "ProgressionManager.h"
#include "TestDiagnostics.h"
// The regression headers below use symbols from the whole gameplay stack; each
// header is pulled in explicitly so the test headers need not be self-contained.
#include "Area.h"
#include "Clue.h"
#include "CombatManager.h"
#include "CombatMath.h"
#include "ConsumableEffects.h"
#include "EncounterManager.h"
#include "Enemy.h"
#include "EnemyCatalog.h"
#include "EquipmentStats.h"
#include "Inventory.h"
#include "Item.h"
#include "ShimmerSkill.h"
#include "ProgressionOwnershipTests.h"
#include "ProgressionRepairTests.h"

#include <iostream>

int main()
{
    UseConsoleTestDiagnostics();

    TestProgressionOwnership();
    ProgressionRepairValidation::Run();

    std::cout << "Progression tests passed.\n";
    return 0;
}
