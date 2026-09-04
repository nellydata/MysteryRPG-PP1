#include "Item.h"

Item::Item(const std::string& name, const std::string& description)
	: name_(name), description_(description) {}

Item::~Item() = default;

const std::string& Item::getName() const { return name_; }
const std::string& Item::getDescription() const { return description_; }
