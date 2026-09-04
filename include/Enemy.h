#pragma once
#include <string>

class Enemy {
public:
	Enemy(const std::string& name, int hp, int atk);
	virtual ~Enemy();

	virtual void takeDamage(int amount);
	bool isAlive() const;
	const std::string& getName() const;
	int getHP() const;
	int getAtk() const;

protected:
	std::string name_;
	int hp_;
	int atk_;
};
