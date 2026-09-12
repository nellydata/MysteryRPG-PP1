#pragma once

#include "Area.h"
#include <string>
#include <vector>

class MapManager {
public:
    MapManager();

    void GenerateFloor1();
    std::vector<Area>& GetRooms();
    const std::vector<Area>& GetRooms() const;
    Area& SelectRoom(int index);
    void DescribeRoom(const Area& area) const;
    const RoomDefinition* GetRoomDefinition(const Area& area) const;
    const RoomDefinition* GetRoomDefinition(const std::string& roomId) const;
    static const std::vector<RoomDefinition>& GetFloor1Definitions();

private:
    std::vector<Area> rooms_;
};
