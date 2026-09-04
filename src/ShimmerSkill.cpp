#include "ShimmerSkill.h"

ShimmerSkill::ShimmerSkill(const std::string& name, int cost)
	: name_(name), cost_(cost) {}

ShimmerSkill::~ShimmerSkill() = default;

const std::string& ShimmerSkill::getName() const { return name_; }
int ShimmerSkill::getCost() const { return cost_; }
