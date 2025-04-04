#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <random>  // Add back for RNG

namespace deckstiny {

/**
 * @enum RoomType
 * @brief Represents the type of a room on the map
 */
enum class RoomType {
    MONSTER,
    ELITE,
    BOSS,
    REST,
    EVENT,
    SHOP,
    TREASURE
};

/**
 * @enum PathType
 * @brief Represents the type/difficulty of a path
 */
enum class PathType {
    NORMAL,   // Standard path
    ELITE,    // Path with more elites and better rewards
    SAFE,     // Path with more rest sites
    RISKY     // Path with more monsters but better treasures
};

/**
 * @struct Room
 * @brief Represents a single room on the map
 */
struct Room {
    int id = 0;                     ///< Unique room ID
    RoomType type = RoomType::MONSTER; ///< Room type
    std::vector<int> nextRooms;     ///< IDs of connected rooms
    std::vector<int> prevRooms;     ///< IDs of rooms that lead to this one
    bool visited = false;           ///< Whether room has been visited
    int x = 0;                      ///< X position for display
    int y = 0;                      ///< Y position for display (floor level)
    std::string data;               ///< Additional room data (JSON)
    
    // Properties for enhanced map generation
    int pathId = 0;                  ///< ID of the path this room belongs to
    PathType pathType = PathType::NORMAL; ///< Type of path this room is on
    int distanceFromStart = 0;       ///< Distance from starting room (used for enemy selection)
    int distanceFromBoss = 0;        ///< Distance from boss room (used for difficulty scaling)
    bool isCheckpoint = false;       ///< Whether this room is a mandatory checkpoint
};

/**
 * @struct Path
 * @brief Represents a path through the map
 */
struct Path {
    int id = 0;                      ///< Unique path ID
    PathType type = PathType::NORMAL;///< Type of path
    std::vector<int> roomIds;        ///< IDs of rooms on this path
    int difficultyRating = 0;        ///< Overall difficulty rating
    int rewardsRating = 0;           ///< Overall rewards rating
};

/**
 * @class GameMap
 * @brief Represents the progression map in the game
 */
class GameMap {
public:
    /**
     * @brief Default constructor
     */
    GameMap();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~GameMap() = default;
    
    /**
     * @brief Generate a new map
     * @param act Current act number
     * @return True if generation succeeded, false otherwise
     */
    bool generate(int act);
    
    /**
     * @brief Check if player can move to a specific room
     * @param roomId ID of the target room
     * @return True if movement is possible, false otherwise
     */
    bool canMoveTo(int roomId) const;
    
    /**
     * @brief Move to a specific room
     * @param roomId ID of the target room
     * @return True if movement succeeded, false otherwise
     */
    bool moveToRoom(int roomId);
    
    /**
     * @brief Get the current room
     * @return Pointer to the current room, nullptr if not on a room
     */
    const Room* getCurrentRoom() const;
    
    /**
     * @brief Get a specific room
     * @param roomId ID of the room to get
     * @return Pointer to the room, nullptr if not found
     */
    const Room* getRoom(int roomId) const;
    
    /**
     * @brief Get all available (connected and unvisited) rooms
     * @return Vector of available room IDs
     */
    std::vector<int> getAvailableRooms() const;
    
    /**
     * @brief Get all rooms
     * @return Map of all rooms
     */
    const std::unordered_map<int, Room>& getAllRooms() const;
    
    /**
     * @brief Get current act number
     * @return Current act
     */
    int getAct() const;
    
    /**
     * @brief Mark the current room as visited
     */
    void markCurrentRoomVisited();
    
    /**
     * @brief Check if all rooms have been visited
     * @return True if all rooms visited, false otherwise
     */
    bool isMapCompleted() const;
    
    /**
     * @brief Check if the final boss has been defeated
     * @return True if boss defeated, false otherwise
     */
    bool isBossDefeated() const;
    
    /**
     * @brief Mark the boss as defeated
     */
    void markBossDefeated();
    
    /**
     * @brief Get the enemy floor range for the current position
     * @return The floor range value for enemy selection
     */
    int getEnemyFloorRange() const;
    
    /**
     * @brief Get the floor range for enemy selection
     * @return Pair of min and max floor values
     */
    std::pair<int, int> getFloorRange() const;
    
    /**
     * @brief Convert a RoomType enum to a string representation
     * @param type The room type to convert
     * @return String representation of the room type
     */
    std::string getRoomTypeString(RoomType type) const;

private:
    int act_ = 0;                               ///< Current act
    int currentRoomId_ = -1;                    ///< ID of the current room
    std::unordered_map<int, Room> rooms_;       ///< Map of rooms by ID
    std::unordered_map<int, Path> paths_;       ///< Map of paths by ID
    bool bossDefeated_ = false;                 ///< Whether the boss has been defeated
    unsigned mapSeed_;                          ///< Random seed for map generation
    std::mt19937 rng_;                          ///< RNG for map generation
    
    /**
     * @brief Generate an Act 1 map (simpler, more linear)
     * @param numFloors Number of floors in the act
     * @return True if generation succeeded
     */
    bool generateAct1Map(int numFloors);
    
    /**
     * @brief Generate an Act 2 map (more branching, wider)
     * @param numFloors Number of floors in the act
     * @return True if generation succeeded
     */
    bool generateAct2Map(int numFloors);
    
    /**
     * @brief Generate an Act 3 map (complex with interesting paths)
     * @param numFloors Number of floors in the act
     * @return True if generation succeeded
     */
    bool generateAct3Map(int numFloors);
    
    /**
     * @brief Create a new room
     * @param floor Floor number
     * @param x Horizontal position
     * @param distFromStart Distance from start
     * @param distFromBoss Distance from boss
     * @param pathId Path ID this room belongs to
     * @param pathType Type of path
     * @return Room ID
     */
    int createRoom(int floor, int x, int distFromStart, int distFromBoss, 
                  int pathId, PathType pathType);
    
    /**
     * @brief Create a link between two rooms
     * @param fromId Source room ID
     * @param toId Target room ID
     */
    void createRoomLink(int fromId, int toId);
    
    /**
     * @brief Set room type based on position and strategic rules
     * @param roomId Room ID to set type for
     */
    void setRoomType(int roomId);
    
    /**
     * @brief Create a path through the map
     * @param pathType Type of path to create
     * @return Path ID
     */
    int createPath(PathType pathType);
    
    /**
     * @brief Validate map to ensure it's completable
     * @return True if map is valid
     */
    bool validateMap();
};

} // namespace deckstiny 