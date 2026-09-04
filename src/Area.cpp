#include "Area.h"
#include "Enemy.h"

Area::Area(const std::string& name) : name_(name) {}
Area::~Area() = default;

void Area::addEnemy(std::unique_ptr<Enemy> enemy) {
	enemies_.push_back(std::move(enemy));
}

bool Area::hasEnemies() const {
	for (const auto& e : enemies_) {
		if (e && e->isAlive()) return true;
	}
	return false;
}

const std::string& Area::getName() const { return name_; }
