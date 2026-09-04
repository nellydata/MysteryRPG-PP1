#include "Area.h"
#include "Enemy.h"
#include <algorithm>

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

Enemy* Area::getFirstAliveEnemy() {
	for (auto& e : enemies_) {
		if (e && e->isAlive()) return e.get();
	}
	return nullptr;
}

void Area::removeDeadEnemies() {
	enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(), [](const std::unique_ptr<Enemy>& e) {
		return !e || !e->isAlive();
	}), enemies_.end());
}
