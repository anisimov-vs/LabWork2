#include <gtest/gtest.h>
#include "core/map.h"
#include <queue>
#include <set>
#include <algorithm>

namespace deckstiny {
namespace testing {

class MapTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a map instance for testing
        map = std::make_unique<GameMap>();
        
        // Generate map for Act 1
        map->generate(1);
    }

    void TearDown() override {
        // Clean up resources
        map.reset();
    }

    std::unique_ptr<GameMap> map;
    
    // Helper method to find a room of a specific type
    int findRoomOfType(RoomType type) {
        const auto& rooms = map->getAllRooms();
        for (const auto& [id, room] : rooms) {
            if (room.type == type) {
                return id;
            }
        }
        return -1;
    }
};

// Test map generation
TEST_F(MapTest, Generation) {
    // Verify map was generated
    EXPECT_EQ(map->getAct(), 1);
    
    // Verify rooms were created
    const auto& rooms = map->getAllRooms();
    EXPECT_GT(rooms.size(), 0);
    
    // Verify there is at least one of each important room type
    EXPECT_NE(findRoomOfType(RoomType::MONSTER), -1);
    EXPECT_NE(findRoomOfType(RoomType::ELITE), -1);
    EXPECT_NE(findRoomOfType(RoomType::BOSS), -1);
    EXPECT_NE(findRoomOfType(RoomType::REST), -1);
    
    // Verify boss room is on the last floor
    int bossRoomId = findRoomOfType(RoomType::BOSS);
    ASSERT_NE(bossRoomId, -1);
    
    const Room* bossRoom = map->getRoom(bossRoomId);
    ASSERT_NE(bossRoom, nullptr);
    
    // Find max floor
    int maxFloor = 0;
    for (const auto& [id, room] : rooms) {
        maxFloor = std::max(maxFloor, room.y);
    }
    
    EXPECT_EQ(bossRoom->y, maxFloor);
}

// Test map structure
TEST_F(MapTest, Structure) {
    const auto& rooms = map->getAllRooms();
    
    // Verify each room has valid connections
    for (const auto& [id, room] : rooms) {
        // Skip the last floor rooms
        if (room.type == RoomType::BOSS) {
            continue;
        }
        
        // Each room should connect to at least one next room
        EXPECT_GT(room.nextRooms.size(), 0);
        
        // Verify all connections are valid room IDs
        for (int nextRoomId : room.nextRooms) {
            EXPECT_NE(rooms.find(nextRoomId), rooms.end());
            
            // The next room should have this room as a prev room
            const auto& nextRoom = rooms.at(nextRoomId);
            EXPECT_NE(std::find(nextRoom.prevRooms.begin(), nextRoom.prevRooms.end(), id), nextRoom.prevRooms.end());
            
            // The next room should be on a higher floor
            EXPECT_GT(nextRoom.y, room.y);
        }
    }
}

// Test map navigation
TEST_F(MapTest, Navigation) {
    // Find the starting room
    int startRoomId = -1;
    const auto& rooms = map->getAllRooms();
    for (const auto& [id, room] : rooms) {
        if (room.y == 0) {
            startRoomId = id;
            break;
        }
    }
    
    ASSERT_NE(startRoomId, -1);
    
    // Verify we can get available rooms
    std::vector<int> availableRooms = map->getAvailableRooms();
    EXPECT_GT(availableRooms.size(), 0);
    
    // Verify we can move to an available room
    int nextRoomId = availableRooms[0];
    EXPECT_TRUE(map->canMoveTo(nextRoomId));
    EXPECT_TRUE(map->moveToRoom(nextRoomId));
    
    // Verify current room was updated
    EXPECT_EQ(map->getCurrentRoom()->id, nextRoomId);
    
    // Mark room as visited
    map->markCurrentRoomVisited();
    
    // Available rooms should now be the next rooms from the current room
    availableRooms = map->getAvailableRooms();
    EXPECT_GT(availableRooms.size(), 0);
    
    // Verify we can't move to an unavailable room
    std::set<int> availableSet(availableRooms.begin(), availableRooms.end());
    for (const auto& [id, room] : rooms) {
        if (availableSet.find(id) == availableSet.end() && id != nextRoomId) {
            EXPECT_FALSE(map->canMoveTo(id));
        }
    }
}

// Test path to boss exists
TEST_F(MapTest, PathToBoss) {
    // Find the starting room
    int startRoomId = -1;
    const auto& rooms = map->getAllRooms();
    for (const auto& [id, room] : rooms) {
        if (room.y == 0) {
            startRoomId = id;
            break;
        }
    }
    
    ASSERT_NE(startRoomId, -1);
    
    // Find the boss room
    int bossRoomId = findRoomOfType(RoomType::BOSS);
    ASSERT_NE(bossRoomId, -1);
    
    // Verify there is a path from start to boss
    std::set<int> visited;
    std::queue<int> queue;
    
    queue.push(startRoomId);
    visited.insert(startRoomId);
    
    bool pathFound = false;
    while (!queue.empty() && !pathFound) {
        int currentRoomId = queue.front();
        queue.pop();
        
        if (currentRoomId == bossRoomId) {
            pathFound = true;
            break;
        }
        
        const Room* currentRoom = map->getRoom(currentRoomId);
        for (int nextRoomId : currentRoom->nextRooms) {
            if (visited.find(nextRoomId) == visited.end()) {
                visited.insert(nextRoomId);
                queue.push(nextRoomId);
            }
        }
    }
    
    EXPECT_TRUE(pathFound);
}

// Test map completion
TEST_F(MapTest, Completion) {
    // Initially map should not be completed
    EXPECT_FALSE(map->isMapCompleted());
    EXPECT_FALSE(map->isBossDefeated());
    
    // Find and move to boss room
    int bossRoomId = findRoomOfType(RoomType::BOSS);
    ASSERT_NE(bossRoomId, -1);
    
    // We need to navigate to the boss room (this is a shortcut for testing completion logic)
    map->setCurrentRoomId_TestHelper(bossRoomId);
    EXPECT_EQ(map->getCurrentRoom()->id, bossRoomId);
    
    // Mark boss room as visited (defeating the boss)
    map->markCurrentRoomVisited();
    
    // Map should now be completed
    EXPECT_TRUE(map->isBossDefeated());
    EXPECT_TRUE(map->isMapCompleted());
}

// Test room type string conversion
TEST_F(MapTest, RoomTypeString) {
    // Verify room type string conversions
    EXPECT_EQ(map->getRoomTypeString(RoomType::MONSTER), "MONSTER");
    EXPECT_EQ(map->getRoomTypeString(RoomType::ELITE), "ELITE");
    EXPECT_EQ(map->getRoomTypeString(RoomType::BOSS), "BOSS");
    EXPECT_EQ(map->getRoomTypeString(RoomType::REST), "REST");
    EXPECT_EQ(map->getRoomTypeString(RoomType::EVENT), "EVENT");
    EXPECT_EQ(map->getRoomTypeString(RoomType::SHOP), "SHOP");
    EXPECT_EQ(map->getRoomTypeString(RoomType::TREASURE), "TREASURE");
}

} // namespace testing
} // namespace deckstiny 