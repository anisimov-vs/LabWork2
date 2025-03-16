#include "core/map.h"
#include <random>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <queue>
#include "util/logger.h"

namespace deckstiny {

GameMap::GameMap() : act_(1), currentRoomId_(-1), bossDefeated_(false) {
    // Initialize random number generator with a seed
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    rng_.seed(seed);
    mapSeed_ = seed;
}

bool GameMap::generate(int act) {
    // Clear existing map
    rooms_.clear();
    paths_.clear();
    currentRoomId_ = -1;
    bossDefeated_ = false;
    act_ = act;
    
    // Set up random seed for reproducibility
    mapSeed_ = std::chrono::system_clock::now().time_since_epoch().count();
    rng_.seed(mapSeed_);
    
    LOG_INFO("map", "Generating new map for act " + std::to_string(act_) + " with seed " + std::to_string(mapSeed_));
    
    // Define map parameters based on act
    int numFloors = 0;
    switch (act) {
        case 1:
            numFloors = 15;
            return generateAct1Map(numFloors);
        case 2:
            numFloors = 17;
            return generateAct2Map(numFloors);
        case 3:
            numFloors = 19;
            return generateAct3Map(numFloors);
        default:
            LOG_ERROR("map", "Invalid act number: " + std::to_string(act));
            numFloors = 15;
            return generateAct1Map(numFloors); // Default to Act 1 style
    }
}

bool GameMap::generateAct1Map(int numFloors) {
    LOG_INFO("map", "Generating Act 1 map with " + std::to_string(numFloors) + " floors");
    
    // Act 1 is simpler, more linear with fewer branches
    // It's designed to be easier for new players
    
    // Create starting room at floor 0
    int startRoomId = createRoom(0, 0, 0, numFloors, 0, PathType::NORMAL);
    Room& startRoom = rooms_[startRoomId];
    startRoom.type = RoomType::EVENT; // Start room is an event
    startRoom.visited = false; // Explicitly set to false (should be default but make sure)
    
    // Create boss room at the last floor
    int bossRoomId = createRoom(numFloors - 1, 0, numFloors - 1, 0, 0, PathType::NORMAL);
    Room& bossRoom = rooms_[bossRoomId];
    bossRoom.type = RoomType::BOSS;
    
    // Create normal path (always available)
    int mainPathId = createPath(PathType::NORMAL);
    
    // Add starting and boss rooms to main path
    paths_[mainPathId].roomIds.push_back(startRoomId);
    paths_[mainPathId].roomIds.push_back(bossRoomId);
    rooms_[startRoomId].pathId = mainPathId;
    rooms_[bossRoomId].pathId = mainPathId;
    
    // Create second path (more risky)
    int riskyPathId = createPath(PathType::RISKY);
    
    // Create a third path (safer with more rest sites)
    int safePathId = createPath(PathType::SAFE);
    
    // Generate main path first (one room per floor)
    int prevRoomId = startRoomId;
    int centerX = 0;
    
    // Create mandatory rest sites only at the middle of the act (floor numFloors/2)
    std::unordered_set<int> mandatoryRestFloors = {numFloors / 2};
    
    // Create rooms from floor 1 to numFloors-2 (omitting start and boss)
    for (int floor = 1; floor < numFloors - 1; ++floor) {
        // Main path slightly meanders from center
        std::uniform_int_distribution<> xDist(-1, 1);
        centerX += xDist(rng_);
        
        // Limit how far it can go from center
        centerX = std::max(-2, std::min(2, centerX));
        
        // Create a room at this floor for the main path
        int roomId = createRoom(floor, centerX, floor, numFloors - floor, mainPathId, PathType::NORMAL);
        Room& room = rooms_[roomId];
        
        // Add to main path
        paths_[mainPathId].roomIds.push_back(roomId);
        
        // Checkpoint rest sites only at the middle of the act
        if (mandatoryRestFloors.find(floor) != mandatoryRestFloors.end()) {
            room.type = RoomType::REST;
            room.isCheckpoint = true;
            LOG_INFO("map", "Placed mandatory rest site at floor " + std::to_string(floor));
        } else {
            // Set room type based on strategic rules (but don't randomly create REST sites)
            setRoomType(roomId);
            
            // Force non-rest site for all other floors
            if (room.type == RoomType::REST && !room.isCheckpoint) {
                room.type = RoomType::MONSTER;
            }
        }
        
        // Connect from previous room
        createRoomLink(prevRoomId, roomId);
        LOG_INFO("map", "Connected room #" + std::to_string(prevRoomId) + " to room #" + std::to_string(roomId));
        
        prevRoomId = roomId;
        
        // Create branching paths at specific floors (3, 6, 9, 12)
        if (floor == 3 || floor == 6 || floor == 9 || floor == 12) {
            // Create branch rooms (left and right)
            int leftX = centerX - 2;
            int rightX = centerX + 2;
            
            // Determine which path goes where (randomize)
            std::uniform_int_distribution<> pathChoice(0, 1);
            int leftPathId = (pathChoice(rng_) == 0) ? riskyPathId : safePathId;
            int rightPathId = (leftPathId == riskyPathId) ? safePathId : riskyPathId;
            
            // Create branch rooms
            int leftRoomId = createRoom(floor, leftX, floor, numFloors - floor, leftPathId, 
                                       (leftPathId == riskyPathId) ? PathType::RISKY : PathType::SAFE);
            int rightRoomId = createRoom(floor, rightX, floor, numFloors - floor, rightPathId, 
                                        (rightPathId == riskyPathId) ? PathType::RISKY : PathType::SAFE);
            
            // Add to respective paths
            paths_[leftPathId].roomIds.push_back(leftRoomId);
            paths_[rightPathId].roomIds.push_back(rightRoomId);
            
            // Set room types based on path type
            if (leftPathId == riskyPathId) {
                // Risky path: More monsters/elites but better rewards
                if (floor % 4 == 0) {
                    rooms_[leftRoomId].type = RoomType::ELITE;
                } else if (floor % 7 == 0) {
                    rooms_[leftRoomId].type = RoomType::TREASURE;
                } else {
                    rooms_[leftRoomId].type = RoomType::MONSTER;
                }
            } else {
                // Safe path: More events, no rest sites
                rooms_[leftRoomId].type = RoomType::EVENT;
            }
            
            if (rightPathId == riskyPathId) {
                // Risky path: More monsters/elites but better rewards
                if (floor % 4 == 0) {
                    rooms_[rightRoomId].type = RoomType::ELITE;
                } else if (floor % 7 == 0) {
                    rooms_[rightRoomId].type = RoomType::TREASURE;
                } else {
                    rooms_[rightRoomId].type = RoomType::MONSTER;
                }
            } else {
                // Safe path: More events, no rest sites
                rooms_[rightRoomId].type = RoomType::EVENT;
            }
            
            // Connect from previous room (connecting from main path room)
            createRoomLink(roomId, leftRoomId);
            createRoomLink(roomId, rightRoomId);
            
            // Always connect to the next floor (instead of only sometimes)
            int nextFloor = floor + 1;
            if (nextFloor < numFloors - 1) {
                // Create next rooms for the branches
                int nextMainRoomId = createRoom(nextFloor, centerX, nextFloor, numFloors - nextFloor, mainPathId, PathType::NORMAL);
                setRoomType(nextMainRoomId);
                paths_[mainPathId].roomIds.push_back(nextMainRoomId);
                
                // Important - connect current room to next floor's main room
                createRoomLink(roomId, nextMainRoomId);
                
                // Also connect the branch rooms to the next floor's main room
                createRoomLink(leftRoomId, nextMainRoomId);
                createRoomLink(rightRoomId, nextMainRoomId);
                
                // Update prevRoomId to the next floor's main room
                prevRoomId = nextMainRoomId;
            }
        }
    }
    
    // Connect the last main path room to boss
    createRoomLink(prevRoomId, bossRoomId);
    
    // Final map validation - ensure there are valid paths through the map
    if (!validateMap()) {
        LOG_ERROR("map", "Generated map failed validation");
        return false;
    }
    
    LOG_INFO("map", "Act 1 map generation complete. Created " + std::to_string(rooms_.size()) + " rooms.");
    
    // Set starting room as current room
    currentRoomId_ = startRoomId;
    LOG_INFO("map", "Set current room ID to starting room #" + std::to_string(startRoomId));
    
    // Mark the starting room as visited so the player can move to the next rooms
    markCurrentRoomVisited();
    LOG_INFO("map", "Marked starting room as visited");
    
    return rooms_.size() > 0;
}

bool GameMap::generateAct2Map(int numFloors) {
    LOG_INFO("map", "Generating Act 2 map with " + std::to_string(numFloors) + " floors");
    
    // Act 2 is wider with more horizontal branching
    // It has more complex path structures and room distributions
    
    // For now, use Act 1 style but with different parameters
    // This is a placeholder - you should implement a more complex generation algorithm
    return generateAct1Map(numFloors);
}

bool GameMap::generateAct3Map(int numFloors) {
    LOG_INFO("map", "Generating Act 3 map with " + std::to_string(numFloors) + " floors");
    
    // Act 3 is the most complex with interesting vertical branching
    // It has the most challenging paths and strategic decision points
    
    // For now, use Act 1 style but with different parameters
    // This is a placeholder - you should implement a more complex generation algorithm
    return generateAct1Map(numFloors);
}

int GameMap::createRoom(int floor, int x, int distFromStart, int distFromBoss, 
                       int pathId, PathType pathType) {
    // Create a new room
    static int nextRoomId = 0;
    
    Room room;
    room.id = nextRoomId++;
    room.y = floor;
    room.x = x;
    room.visited = false;
    room.pathId = pathId;
    room.pathType = pathType;
    room.distanceFromStart = distFromStart;
    room.distanceFromBoss = distFromBoss;
    
    // Store the room
    rooms_[room.id] = room;
    
    LOG_DEBUG("map", "Created room #" + std::to_string(room.id) + 
              " at floor " + std::to_string(floor) + ", x=" + std::to_string(x) +
              ", pathId=" + std::to_string(pathId));
    
    return room.id;
}

void GameMap::createRoomLink(int fromId, int toId) {
    // Check if rooms exist
    auto fromIt = rooms_.find(fromId);
    auto toIt = rooms_.find(toId);
    if (fromIt == rooms_.end() || toIt == rooms_.end()) {
        LOG_ERROR("map", "Cannot create link: room not found");
        return;
    }
    
    // Check if link already exists
    if (std::find(fromIt->second.nextRooms.begin(), 
                 fromIt->second.nextRooms.end(), 
                 toId) != fromIt->second.nextRooms.end()) {
        return; // Link already exists
    }
    
    // Add forward link
    fromIt->second.nextRooms.push_back(toId);
    
    // Add backward link for tracking
    toIt->second.prevRooms.push_back(fromId);
    
    LOG_DEBUG("map", "Created link from room #" + std::to_string(fromId) + 
              " to room #" + std::to_string(toId));
}

void GameMap::setRoomType(int roomId) {
    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) {
        return;
    }
    
    Room& room = it->second;
    
    // Room type probabilities based on path type
    std::vector<double> roomProbs;
    
    // Define probabilities based on path type and distance
    // Order: MONSTER, ELITE, REST, EVENT, SHOP, TREASURE
    switch (room.pathType) {
        case PathType::NORMAL:
            // Weighted heavily toward MONSTER rooms
            if (room.distanceFromStart < 5) {
                // Early in the path: lots of monsters (80%)
                roomProbs = {0.85, 0.05, 0.0, 0.07, 0.03, 0.0};
            } else if (room.distanceFromBoss < 5) {
                // Near the boss: more monsters and elites
                roomProbs = {0.75, 0.15, 0.0, 0.05, 0.02, 0.03};
            } else {
                // Middle of the path: monsters dominate
                roomProbs = {0.80, 0.08, 0.0, 0.07, 0.03, 0.02};
            }
            break;
            
        case PathType::RISKY:
            // Risky path: Even more monsters and elites
            roomProbs = {0.75, 0.20, 0.0, 0.02, 0.0, 0.03};
            break;
            
        case PathType::SAFE:
            // Safe path: More events, but still mostly monsters
            roomProbs = {0.65, 0.05, 0.0, 0.20, 0.08, 0.02};
            break;
            
        default:
            // Default - heavily weighted toward monsters
            roomProbs = {0.80, 0.08, 0.0, 0.07, 0.03, 0.02};
            break;
    }
    
    // Strategic rules that override probabilities
    
    // 1. If connected to an ELITE, make next room a monster (no REST or EVENT)
    bool connectedToElite = false;
    for (int prevId : room.prevRooms) {
        auto prevIt = rooms_.find(prevId);
        if (prevIt != rooms_.end() && prevIt->second.type == RoomType::ELITE) {
            connectedToElite = true;
            break;
        }
    }
    
    if (connectedToElite) {
        // After ELITE rooms, give monster room
        roomProbs[0] = 1.0; // 100% MONSTER
        roomProbs[1] = roomProbs[2] = roomProbs[3] = roomProbs[4] = roomProbs[5] = 0.0;
    }
    
    // 2. If at a floor that's a multiple of 4, increase ELITE chance
    if (room.y % 4 == 0 && room.y > 0) {
        // Boost ELITE chance
        roomProbs[1] += 0.15;
        
        // Normalize
        double sum = 0.0;
        for (double prob : roomProbs) {
            sum += prob;
        }
        for (double& prob : roomProbs) {
            prob /= sum;
        }
    }
    
    // 3. Ensure shops appear at reasonable intervals - but only after the 4th floor
    // This prevents the first room from always being a shop
    if (room.y >= 4) {
        int distanceFromLastShop = 999;
        for (const auto& [id, r] : rooms_) {
            if (r.type == RoomType::SHOP && r.y < room.y) {
                distanceFromLastShop = std::min(distanceFromLastShop, room.y - r.y);
            }
        }
        
        if (distanceFromLastShop > 8) {
            // Force a SHOP if too far from last one and we're not in the first few floors
            room.type = RoomType::SHOP;
            LOG_INFO("map", "Forced shop at floor " + std::to_string(room.y) + " due to distance from last shop: " + std::to_string(distanceFromLastShop));
            return;
        }
    }
    
    // Roll for room type
    std::uniform_real_distribution<> dist(0.0, 1.0);
    double roll = dist(rng_);
    
    // Determine room type based on roll and probabilities
    double cumProb = 0.0;
    int typeIndex = 0;
    
    for (size_t i = 0; i < roomProbs.size(); ++i) {
        cumProb += roomProbs[i];
        if (roll <= cumProb) {
            typeIndex = i;
            break;
        }
    }
    
    // Set room type (follow RoomType enum order)
    switch (typeIndex) {
        case 0:
            room.type = RoomType::MONSTER;
            break;
        case 1:
            room.type = RoomType::ELITE;
            break;
        case 2:
            room.type = RoomType::REST;
            break;
        case 3:
            room.type = RoomType::EVENT;
            break;
        case 4:
            room.type = RoomType::SHOP;
            break;
        case 5:
            room.type = RoomType::TREASURE;
            break;
        default:
            room.type = RoomType::MONSTER; // Fallback
            break;
    }
    
    LOG_INFO("map", "Set room #" + std::to_string(roomId) + 
             " at floor " + std::to_string(room.y) + 
             " to type: " + getRoomTypeString(room.type) +
             " (roll: " + std::to_string(roll) + ")");
}

int GameMap::createPath(PathType pathType) {
    // Create a new path
    static int nextPathId = 1; // Start from 1 (0 is reserved for no path)
    
    Path path;
    path.id = nextPathId++;
    path.type = pathType;
    
    // Set difficulty/rewards based on path type
    switch (pathType) {
        case PathType::NORMAL:
            path.difficultyRating = 5;
            path.rewardsRating = 5;
            break;
        case PathType::ELITE:
            path.difficultyRating = 8;
            path.rewardsRating = 8;
            break;
        case PathType::SAFE:
            path.difficultyRating = 3;
            path.rewardsRating = 3;
            break;
        case PathType::RISKY:
            path.difficultyRating = 7;
            path.rewardsRating = 7;
            break;
        default:
            path.difficultyRating = 5;
            path.rewardsRating = 5;
            break;
    }
    
    // Store the path
    paths_[path.id] = path;
    
    LOG_DEBUG("map", "Created path #" + std::to_string(path.id) + 
              " of type " + std::to_string(static_cast<int>(pathType)));
    
    return path.id;
}

bool GameMap::validateMap() {
    // Check if map is empty
    if (rooms_.empty()) {
        LOG_ERROR("map", "Map validation failed: No rooms");
        return false;
    }
    
    // Find start room
    int startRoomId = -1;
    for (const auto& [id, room] : rooms_) {
        if (room.y == 0) {
            startRoomId = id;
            break;
        }
    }
    
    if (startRoomId == -1) {
        LOG_ERROR("map", "Map validation failed: No start room");
        return false;
    }
    
    // Find boss room
    int bossRoomId = -1;
    int maxFloor = 0;
    for (const auto& [id, room] : rooms_) {
        if (room.y > maxFloor) {
            maxFloor = room.y;
        }
        if (room.type == RoomType::BOSS) {
            bossRoomId = id;
        }
    }
    
    if (bossRoomId == -1) {
        LOG_ERROR("map", "Map validation failed: No boss room");
        return false;
    }
    
    // Log detailed map structure for debugging
    LOG_INFO("map", "===== DEBUG: Map Structure =====");
    for (const auto& [id, room] : rooms_) {
        LOG_INFO("map", "Room #" + std::to_string(id) + 
                 ": Type=" + std::to_string(static_cast<int>(room.type)) + 
                 ", Floor=" + std::to_string(room.y) + 
                 ", X=" + std::to_string(room.x) +
                 ", Visited=" + (room.visited ? "true" : "false"));
        
        LOG_INFO("map", "  Connected to next rooms: ");
        for (int nextId : room.nextRooms) {
            auto nextIt = rooms_.find(nextId);
            if (nextIt != rooms_.end()) {
                LOG_INFO("map", "    -> Room #" + std::to_string(nextId) + 
                        ": Type=" + std::to_string(static_cast<int>(nextIt->second.type)) + 
                        ", Floor=" + std::to_string(nextIt->second.y));
            } else {
                LOG_INFO("map", "    -> Room #" + std::to_string(nextId) + " [NOT FOUND]");
            }
        }
        
        LOG_INFO("map", "  Connected from previous rooms: ");
        for (int prevId : room.prevRooms) {
            auto prevIt = rooms_.find(prevId);
            if (prevIt != rooms_.end()) {
                LOG_INFO("map", "    <- Room #" + std::to_string(prevId) + 
                        ": Type=" + std::to_string(static_cast<int>(prevIt->second.type)) + 
                        ", Floor=" + std::to_string(prevIt->second.y));
            } else {
                LOG_INFO("map", "    <- Room #" + std::to_string(prevId) + " [NOT FOUND]");
            }
        }
    }
    LOG_INFO("map", "================================");
    
    // Check if boss room is reachable (breadth-first search)
    std::unordered_set<int> visited;
    std::queue<int> queue;
    
    // Start from the start room
    queue.push(startRoomId);
    visited.insert(startRoomId);
    
    bool canReachBoss = false;
    while (!queue.empty()) {
        int currentId = queue.front();
        queue.pop();
        
        if (currentId == bossRoomId) {
            canReachBoss = true;
            break;
        }
        
        // Add all connected rooms
        auto it = rooms_.find(currentId);
        if (it != rooms_.end()) {
            for (int nextId : it->second.nextRooms) {
                if (visited.find(nextId) == visited.end()) {
                    visited.insert(nextId);
                    queue.push(nextId);
                }
            }
        }
    }
    
    if (!canReachBoss) {
        LOG_ERROR("map", "Map validation failed: Boss not reachable from start");
        return false;
    }
    
    LOG_INFO("map", "Map validation passed");
    return true;
}

bool GameMap::canMoveTo(int roomId) const {
    // Check if room exists
    auto roomIt = rooms_.find(roomId);
    if (roomIt == rooms_.end()) {
        return false;
    }
    
    // Check if current room has a link to the target room
    if (currentRoomId_ < 0) {
        // No current room, can only move to starting room
        // Find the first room on the first floor
        for (const auto& room : rooms_) {
            if (room.second.y == 0) {
                return roomId == room.first;
            }
        }
        return false;
    }
    
    auto currentRoomIt = rooms_.find(currentRoomId_);
    if (currentRoomIt == rooms_.end()) {
        return false;
    }
    
    // Check if target room is in the next rooms list
    return std::find(currentRoomIt->second.nextRooms.begin(),
                    currentRoomIt->second.nextRooms.end(),
                    roomId) != currentRoomIt->second.nextRooms.end();
}

bool GameMap::moveToRoom(int roomId) {
    if (!canMoveTo(roomId)) {
        return false;
    }
    
    currentRoomId_ = roomId;
    return true;
}

const Room* GameMap::getCurrentRoom() const {
    if (currentRoomId_ < 0) {
        return nullptr;
    }
    
    auto it = rooms_.find(currentRoomId_);
    return (it != rooms_.end()) ? &it->second : nullptr;
}

const Room* GameMap::getRoom(int roomId) const {
    auto it = rooms_.find(roomId);
    return (it != rooms_.end()) ? &it->second : nullptr;
}

std::vector<int> GameMap::getAvailableRooms() const {
    std::vector<int> availableRooms;
    
    if (currentRoomId_ < 0) {
        LOG_INFO("map", "No current room set, returning empty list");
        return availableRooms;
    }
    
    auto currentRoomIt = rooms_.find(currentRoomId_);
    if (currentRoomIt == rooms_.end()) {
        LOG_INFO("map", "Current room ID not found in rooms map, returning empty list");
        return availableRooms;
    }
    
    LOG_INFO("map", "Current room #" + std::to_string(currentRoomId_) + 
             " has " + std::to_string(currentRoomIt->second.nextRooms.size()) + " connected rooms");
    
    // Add all connected rooms that haven't been visited yet
    for (int nextRoomId : currentRoomIt->second.nextRooms) {
        auto nextRoomIt = rooms_.find(nextRoomId);
        if (nextRoomIt != rooms_.end()) {
            LOG_INFO("map", "  Connected room #" + std::to_string(nextRoomId) + 
                    ", visited: " + (nextRoomIt->second.visited ? "true" : "false"));
            
            if (!nextRoomIt->second.visited) {
                availableRooms.push_back(nextRoomId);
                LOG_INFO("map", "  Adding room #" + std::to_string(nextRoomId) + " to available rooms");
            } else {
                LOG_INFO("map", "  Skipping visited room #" + std::to_string(nextRoomId));
            }
        } else {
            LOG_INFO("map", "  Connected room #" + std::to_string(nextRoomId) + " not found in rooms map");
        }
    }
    
    LOG_INFO("map", "Returning " + std::to_string(availableRooms.size()) + " available rooms");
    return availableRooms;
}

const std::unordered_map<int, Room>& GameMap::getAllRooms() const {
    return rooms_;
}

int GameMap::getAct() const {
    return act_;
}

void GameMap::markCurrentRoomVisited() {
    if (currentRoomId_ >= 0) {
        auto it = rooms_.find(currentRoomId_);
        if (it != rooms_.end()) {
            it->second.visited = true;
            
            // Check if it's a boss room
            if (it->second.type == RoomType::BOSS) {
                bossDefeated_ = true;
            }
        }
    }
}

bool GameMap::isMapCompleted() const {
    // Map is completed if all rooms have been visited or if the boss has been defeated
    if (bossDefeated_) {
        return true;
    }
    
    for (const auto& room : rooms_) {
        if (!room.second.visited) {
            return false;
        }
    }
    
    return true;
}

bool GameMap::isBossDefeated() const {
    return bossDefeated_;
}

void GameMap::markBossDefeated() {
    bossDefeated_ = true;
}

int GameMap::getEnemyFloorRange() const {
    if (currentRoomId_ < 0) {
        return 0;
    }
    
    auto currentRoomIt = rooms_.find(currentRoomId_);
    if (currentRoomIt == rooms_.end()) {
        return 0;
    }
    
    const Room& room = currentRoomIt->second;
    
    // Base the floor range on a combination of:
    // - Current floor/distance from start
    // - Act number
    // - Path difficulty
    
    int floorRange = room.distanceFromStart;
    
    // Factor 1: Act number (each act increases difficulty)
    floorRange += (act_ - 1) * 3;
    
    // Factor 2: Path difficulty
    switch (room.pathType) {
        case PathType::SAFE:
            // Easier enemies on safe path
            floorRange -= 1;
            break;
        case PathType::NORMAL:
            // No adjustment for normal path
            break;
        case PathType::RISKY:
            // Harder enemies on risky path
            floorRange += 1;
            break;
        case PathType::ELITE:
            // Much harder enemies on elite path
            floorRange += 2;
            break;
        default:
            break;
    }
    
    // Ensure the floor range is within reasonable bounds (0-15)
    floorRange = std::max(0, std::min(15, floorRange));
    
    LOG_INFO("map", "Calculated enemy floor range: " + std::to_string(floorRange) + 
             " for room at floor " + std::to_string(room.y) + 
             " on path type " + std::to_string(static_cast<int>(room.pathType)));
    
    return floorRange;
}

std::string GameMap::getRoomTypeString(RoomType type) const {
    switch (type) {
        case RoomType::MONSTER: return "MONSTER";
        case RoomType::ELITE: return "ELITE";
        case RoomType::REST: return "REST";
        case RoomType::EVENT: return "EVENT";
        case RoomType::SHOP: return "SHOP";
        case RoomType::TREASURE: return "TREASURE";
        case RoomType::BOSS: return "BOSS";
        default: return "UNKNOWN";
    }
}

} // namespace deckstiny 