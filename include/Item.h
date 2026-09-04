#pragma once

#include <string>

class Item {
public:
    Item(const std::string& name = "", const std::string& description = "");
    ~Item();

    const std::string& getName() const;
    const std::string& getDescription() const;

private:
    std::string name_;
    std::string description_;
};
