#pragma once

#include <memory>
#include <string>

class Area;

class MapManager {
public:
    MapManager();
    ~MapManager();

    std::unique_ptr<Area> generateArea(const std::string& name);
};
