// Anisimov Vasiliy st129629@student.spbu.ru
// Laboratory Work 2

#include "core/map.h"
#include <random>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <queue>
#include "util/logger.h"

namespace deckstiny {

// Define constants for StS-style map generation
const int STS_NUM_FLOORS = 15;         // Number of regular floors (0-14)
const int STS_NUM_COLUMNS = 7;         // Width of the map (0-6)
const int STS_BOSS_FLOOR_Y = STS_NUM_FLOORS; // Boss is on the 16th "level" (y-coordinate 15)
const int STS_MIN_STARTING_PATHS = 2;  // Min distinct starting columns on floor 0
const int STS_PRE_BOSS_REST_FLOOR_Y = STS_NUM_FLOORS - 1; // Floor 14 (0-indexed)
const int STS_MID_ACT_TREASURE_FLOOR_Y = STS_NUM_FLOORS / 2; // e.g., Floor 7 (0-indexed)
const int STS_ELITE_MIN_FLOOR_Y = 5; // Elites can start appearing from floor 5 (0-indexed)
const size_t STS_MAX_INCOMING_CONNECTIONS_PER_NODE = 3; // Max incoming connections a typical node can receive

GameMap::GameMap() : act_(1), currentRoomId_(-1), bossDefeated_(false), nextRoomId_(0) {
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    rng_.seed(seed);
    mapSeed_ = seed;
}

bool GameMap::generate(int act) {
    rooms_.clear();
    currentRoomId_ = -1;
    bossDefeated_ = false;
    act_ = act;
    nextRoomId_ = 0; 
    
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    rng_.seed(seed);
    mapSeed_ = seed;
    
    LOG_INFO("map", "Generating new StS-style map for act " + std::to_string(act_) + " with seed " + std::to_string(mapSeed_));

    std::vector<std::vector<Room*>> grid_nodes(STS_NUM_FLOORS, std::vector<Room*>(STS_NUM_COLUMNS, nullptr));

    // 1. Create the Boss Room
    int boss_x_column = STS_NUM_COLUMNS / 2;
    int bossRoomId = createRoom(STS_BOSS_FLOOR_Y, boss_x_column);
    if (bossRoomId == -1) { 
        LOG_ERROR("map", "Failed to create boss room.");
        return false;
    }
    rooms_[bossRoomId].type = RoomType::BOSS;
    LOG_INFO("map", "Created Boss Room #" + std::to_string(bossRoomId) + " at (x:" + std::to_string(boss_x_column) + ", y:" + std::to_string(STS_BOSS_FLOOR_Y) + ")");

    std::vector<Room*> nodes_on_higher_floor_to_connect_from; 

    // 2a. Create Pre-Boss Rest Site(s) on STS_PRE_BOSS_REST_FLOOR_Y (e.g., floor 14)
    std::uniform_int_distribution<> pre_boss_rest_count_dist(2, 3); 
    int num_pre_boss_rests = pre_boss_rest_count_dist(rng_);
    std::vector<int> available_pre_boss_columns;
    for(int i=0; i<STS_NUM_COLUMNS; ++i) available_pre_boss_columns.push_back(i);
    std::shuffle(available_pre_boss_columns.begin(), available_pre_boss_columns.end(), rng_); 

    LOG_INFO("map", "Creating " + std::to_string(num_pre_boss_rests) + " rest sites on pre-boss floor y=" + std::to_string(STS_PRE_BOSS_REST_FLOOR_Y));
    for (int i = 0; i < num_pre_boss_rests && i < (int)available_pre_boss_columns.size(); ++i) {
        int col = available_pre_boss_columns[i];
        int rest_room_id = createRoom(STS_PRE_BOSS_REST_FLOOR_Y, col);
        rooms_[rest_room_id].type = RoomType::REST; 
        createRoomLink(rest_room_id, bossRoomId);
        nodes_on_higher_floor_to_connect_from.push_back(&rooms_[rest_room_id]);
        if (STS_PRE_BOSS_REST_FLOOR_Y < STS_NUM_FLOORS) { 
             grid_nodes[STS_PRE_BOSS_REST_FLOOR_Y][col] = &rooms_[rest_room_id];
        }
        LOG_DEBUG("map", "  Created pre-boss rest room #" + std::to_string(rest_room_id) + " at (x:" + std::to_string(col) + ", y:" + std::to_string(STS_PRE_BOSS_REST_FLOOR_Y) + ") linked to boss.");
    }

    if (nodes_on_higher_floor_to_connect_from.empty()) {
        LOG_ERROR("map", "Failed to create any pre-boss rest sites. Aborting generation.");
        return false;
    }

    // 2b. Iterate downwards from floor STS_PRE_BOSS_REST_FLOOR_Y - 1 (e.g., floor 13) down to 0
    for (int y = STS_PRE_BOSS_REST_FLOOR_Y - 1; y >= 0; --y) {
        LOG_DEBUG("map", "Generating paths for floor y=" + std::to_string(y) + ". Nodes on floor above (y+1) to connect from: " + std::to_string(nodes_on_higher_floor_to_connect_from.size()));
        std::vector<Room*> nodes_actually_created_on_this_floor_y;
        std::unordered_set<Room*> all_nodes_on_higher_floor_that_got_a_link;

        for (Room* room_on_higher_floor : nodes_on_higher_floor_to_connect_from) {
            std::uniform_int_distribution<> num_incoming_paths_dist(1, 2); 
            int num_paths_to_create_for_this_room_above = num_incoming_paths_dist(rng_);
            if (nodes_on_higher_floor_to_connect_from.size() == 1 && y > 0) num_paths_to_create_for_this_room_above = std::max(1, num_paths_to_create_for_this_room_above);
            
            LOG_DEBUG("map_detail", "  TargetRoom on y+1: #" + std::to_string(room_on_higher_floor->id) + 
                      " at (x:" + std::to_string(room_on_higher_floor->x) + ", y:" + std::to_string(room_on_higher_floor->y) + ") needs " + 
                      std::to_string(num_paths_to_create_for_this_room_above) + " incoming paths from floor y=" + std::to_string(y) );

            for (int i = 0; i < num_paths_to_create_for_this_room_above; ++i) {
                std::vector<int> possible_cols;
                possible_cols.push_back(room_on_higher_floor->x); 
                if (room_on_higher_floor->x > 0) possible_cols.push_back(room_on_higher_floor->x - 1);
                if (room_on_higher_floor->x < STS_NUM_COLUMNS - 1) possible_cols.push_back(room_on_higher_floor->x + 1);
                std::shuffle(possible_cols.begin(), possible_cols.end(), rng_);

                int chosen_x_for_new_room_on_floor_y = -1;
                Room* existing_room_on_floor_y_to_reuse = nullptr;

                for (int candidate_col : possible_cols) {
                    if (grid_nodes[y][candidate_col] == nullptr) { 
                        chosen_x_for_new_room_on_floor_y = candidate_col;
                        LOG_DEBUG("map_detail", "    Path #" + std::to_string(i+1) + " to TargetRoom #" + std::to_string(room_on_higher_floor->id) + ": Found empty spot at (x:" + std::to_string(candidate_col) + ", y:" + std::to_string(y) + ")");
                        break;
                    } else { 
                        Room* potential_reuse_room = grid_nodes[y][candidate_col];
                        bool already_linked = false;
                        for(int next_id : potential_reuse_room->nextRooms) {
                            if (next_id == room_on_higher_floor->id) { already_linked = true; break; }
                        }
                        if (!already_linked && potential_reuse_room->nextRooms.size() < 2) {
                            chosen_x_for_new_room_on_floor_y = candidate_col;
                            existing_room_on_floor_y_to_reuse = potential_reuse_room;
                            LOG_DEBUG("map_detail", "    Path #" + std::to_string(i+1) + " to TargetRoom #" + std::to_string(room_on_higher_floor->id) + ": Reusing existing room #" + std::to_string(existing_room_on_floor_y_to_reuse->id) + " at (x:" + std::to_string(candidate_col) + ", y:" + std::to_string(y) + ")");
                            break;
                        }
                    }
                }

                if (chosen_x_for_new_room_on_floor_y == -1) {
                    if (!possible_cols.empty()) {
                        chosen_x_for_new_room_on_floor_y = possible_cols[0];
                        if (grid_nodes[y][chosen_x_for_new_room_on_floor_y] != nullptr) {
                            existing_room_on_floor_y_to_reuse = grid_nodes[y][chosen_x_for_new_room_on_floor_y];
                            LOG_DEBUG("map_detail", "    Path #" + std::to_string(i+1) + " to TargetRoom #" + std::to_string(room_on_higher_floor->id) + ": Forced reuse/creation at (x:" + std::to_string(chosen_x_for_new_room_on_floor_y) + ", y:" + std::to_string(y) + ") existing: " + (existing_room_on_floor_y_to_reuse ? "Yes" : "No"));
                        } else {
                             LOG_DEBUG("map_detail", "    Path #" + std::to_string(i+1) + " to TargetRoom #" + std::to_string(room_on_higher_floor->id) + ": Forced creation at (x:" + std::to_string(chosen_x_for_new_room_on_floor_y) + ", y:" + std::to_string(y) + ")");
                        }
                    } else {
                        LOG_WARNING("map", "    Path #" + std::to_string(i+1) + " to TargetRoom #" + std::to_string(room_on_higher_floor->id) + ": No valid column on floor y=" + std::to_string(y) + ". Skipping path.");
                        continue; 
                    }
                }

                Room* room_on_floor_y_that_links_upward;
                if (existing_room_on_floor_y_to_reuse) {
                    room_on_floor_y_that_links_upward = existing_room_on_floor_y_to_reuse;
                } else {
                    int new_room_id = createRoom(y, chosen_x_for_new_room_on_floor_y);
                    room_on_floor_y_that_links_upward = &rooms_[new_room_id];
                    grid_nodes[y][chosen_x_for_new_room_on_floor_y] = room_on_floor_y_that_links_upward;
                    nodes_actually_created_on_this_floor_y.push_back(room_on_floor_y_that_links_upward); 
                }
                
                createRoomLink(room_on_floor_y_that_links_upward->id, room_on_higher_floor->id);
                all_nodes_on_higher_floor_that_got_a_link.insert(room_on_higher_floor);
            }
        }
        
        // Orphan Check for rooms on floor y+1
        for (Room* room_on_higher_floor_check : nodes_on_higher_floor_to_connect_from) {
            if (all_nodes_on_higher_floor_that_got_a_link.find(room_on_higher_floor_check) == all_nodes_on_higher_floor_that_got_a_link.end()) {
                LOG_WARNING("map", "  Orphaned room #" + std::to_string(room_on_higher_floor_check->id) + " at (x:"+std::to_string(room_on_higher_floor_check->x)+",y:"+std::to_string(room_on_higher_floor_check->y)+") found. Attempting emergency link from floor y=" + std::to_string(y));
                Room* emergency_link_source_on_floor_y = nullptr;
                int min_h_dist = STS_NUM_COLUMNS + 1;
                for(Room* candidate_on_y : nodes_actually_created_on_this_floor_y) { 
                    if(candidate_on_y->nextRooms.size() < 3) {
                       int h_dist = std::abs(candidate_on_y->x - room_on_higher_floor_check->x);
                       if (h_dist < min_h_dist) {min_h_dist = h_dist; emergency_link_source_on_floor_y = candidate_on_y;}
                    }
                }
                if (!emergency_link_source_on_floor_y) {
                     for(int temp_x = 0; temp_x < STS_NUM_COLUMNS; ++temp_x) {
                        if(grid_nodes[y][temp_x] != nullptr && grid_nodes[y][temp_x]->nextRooms.size() < 3) {
                            int h_dist = std::abs(grid_nodes[y][temp_x]->x - room_on_higher_floor_check->x);
                            if (h_dist < min_h_dist) {min_h_dist = h_dist; emergency_link_source_on_floor_y = grid_nodes[y][temp_x];}
                        }
                     }
                }

                if (emergency_link_source_on_floor_y) {
                    LOG_DEBUG("map", "    Emergency linking orphan #" + std::to_string(room_on_higher_floor_check->id) + " from #" + std::to_string(emergency_link_source_on_floor_y->id) + " on floor y="+std::to_string(y));
                    createRoomLink(emergency_link_source_on_floor_y->id, room_on_higher_floor_check->id);
        } else {
                    LOG_ERROR("map", "    COULD NOT FIX ORPHAN #" + std::to_string(room_on_higher_floor_check->id) + " on y+1=" + std::to_string(room_on_higher_floor_check->y) + ". Map might be invalid.");
                }
            }
        }

        nodes_on_higher_floor_to_connect_from.clear();
        for(int col_idx = 0; col_idx < STS_NUM_COLUMNS; ++col_idx) {
            if(grid_nodes[y][col_idx] != nullptr) {
                nodes_on_higher_floor_to_connect_from.push_back(grid_nodes[y][col_idx]);
            }
        }
        if (nodes_on_higher_floor_to_connect_from.empty() && y > 0) { 
            LOG_ERROR("map", "Path generation terminated: No nodes available on floor y=" + std::to_string(y) + " to continue paths downwards. Map invalid.");
            return false;
        }
        LOG_DEBUG("map", "Finished processing for floor y="+std::to_string(y)+". " + std::to_string(nodes_on_higher_floor_to_connect_from.size()) + " nodes on this floor will be connection targets for floor y-1.");
    }

    LOG_INFO("map", "Starting Path Diversification Pass...");
    for (int y = 0; y < STS_PRE_BOSS_REST_FLOOR_Y; ++y) {
        if (y >= STS_NUM_FLOORS) continue;

        LOG_DEBUG("map_diversify", "Diversifying paths FROM floor y=" + std::to_string(y));
        for (int x = 0; x < STS_NUM_COLUMNS; ++x) {
            Room* source_room_on_floor_y = grid_nodes[y][x];
            if (!source_room_on_floor_y) continue;

            size_t current_exits = source_room_on_floor_y->nextRooms.size();
            if (current_exits >= static_cast<size_t>(STS_MIN_STARTING_PATHS)) continue;

            size_t num_additional_paths_needed = static_cast<size_t>(STS_MIN_STARTING_PATHS) - current_exits;
            LOG_DEBUG("map_diversify", "  Room #" + std::to_string(source_room_on_floor_y->id) + " at (x:" + std::to_string(x) + ", y:" + std::to_string(y) + ") has " + std::to_string(current_exits) + " exits, needs " + std::to_string(num_additional_paths_needed) + " more.");

            std::vector<Room*> potential_targets_on_floor_y_plus_1;
            int target_cols_ordered[] = {source_room_on_floor_y->x, source_room_on_floor_y->x - 1, source_room_on_floor_y->x + 1};
            
            for (int target_x_offset_idx = 0; target_x_offset_idx < 3; ++target_x_offset_idx) {
                int target_x = target_cols_ordered[target_x_offset_idx];
                if (target_x < 0 || target_x >= STS_NUM_COLUMNS) continue;
                if ( (y + 1) >= STS_NUM_FLOORS && (y+1) != STS_BOSS_FLOOR_Y ) continue;

                Room* target_room_on_y_plus_1 = nullptr;
                if ((y + 1) == STS_BOSS_FLOOR_Y) {
                     for(auto& pair : rooms_) { if(pair.second.type == RoomType::BOSS) { target_room_on_y_plus_1 = &pair.second; break; } }
                } else if ((y+1) < STS_NUM_FLOORS) {
                    target_room_on_y_plus_1 = grid_nodes[y + 1][target_x];
                }

                if (target_room_on_y_plus_1) {
                    bool already_connected = false;
                    for (int next_id : source_room_on_floor_y->nextRooms) {
                        if (next_id == target_room_on_y_plus_1->id) {
                            already_connected = true;
                            break;
                        }
                    }
                    if (!already_connected && target_room_on_y_plus_1->prevRooms.size() < STS_MAX_INCOMING_CONNECTIONS_PER_NODE) {
                        potential_targets_on_floor_y_plus_1.push_back(target_room_on_y_plus_1);
                    }
                }
            }
            std::shuffle(potential_targets_on_floor_y_plus_1.begin(), potential_targets_on_floor_y_plus_1.end(), rng_);

            size_t added_count = 0;
            for (Room* target_node : potential_targets_on_floor_y_plus_1) {
                if (added_count >= num_additional_paths_needed) break;
                createRoomLink(source_room_on_floor_y->id, target_node->id);
                added_count++;
                LOG_DEBUG("map_diversify", "    Added emergency link from #" + std::to_string(source_room_on_floor_y->id) + " to #" + std::to_string(target_node->id) + " on floor y+1.");
            }
            if (added_count > 0) {
                 LOG_INFO("map_diversify", "  Room #" + std::to_string(source_room_on_floor_y->id) + " now has " + std::to_string(source_room_on_floor_y->nextRooms.size()) + " exits after diversification.");
            } else if (num_additional_paths_needed > 0) {
                 LOG_DEBUG("map_diversify", "  Could not add any new exits for Room #" + std::to_string(source_room_on_floor_y->id) + ". Still needs " + std::to_string(num_additional_paths_needed - added_count) + " exits.");
            }
        }
    }
    LOG_INFO("map", "Path Diversification Pass completed.");

    // 3. Set Starting Room (currentRoomId_)
    std::vector<Room*> floor0_rooms = nodes_on_higher_floor_to_connect_from; 
                                                                        
    std::vector<Room*> good_starting_rooms;
    for (Room* r_ptr : floor0_rooms) {
        if (r_ptr && r_ptr->nextRooms.size() >= static_cast<size_t>(STS_MIN_STARTING_PATHS)) {
            good_starting_rooms.push_back(r_ptr);
        }
    }

    if (!good_starting_rooms.empty()) {
        std::shuffle(good_starting_rooms.begin(), good_starting_rooms.end(), rng_);
        currentRoomId_ = good_starting_rooms[0]->id;
        LOG_INFO("map", "Selected start room #" + std::to_string(currentRoomId_) + " at (x:" + std::to_string(rooms_[currentRoomId_].x) + ", y:0) with " + std::to_string(rooms_[currentRoomId_].nextRooms.size()) + " exits.");
    } else {
        LOG_WARNING("map", "No rooms on floor 0 have at least " + std::to_string(STS_MIN_STARTING_PATHS) + " exits. Attempting emergency fix for start room.");
        if (floor0_rooms.empty()) {
            LOG_ERROR("map", "CRITICAL: No rooms on floor 0 at all. Cannot set start room. Map generation failed.");
            return false;
        }
        
        std::sort(floor0_rooms.begin(), floor0_rooms.end(), [](const Room* a, const Room* b) {
            return a->nextRooms.size() > b->nextRooms.size();
        });
        Room* start_room_ptr = floor0_rooms[0];
        currentRoomId_ = start_room_ptr->id;
        
        LOG_INFO("map", "Emergency fallback: selected start room #" + std::to_string(currentRoomId_) + " at (x:" + std::to_string(start_room_ptr->x) + ", y:0) with " + std::to_string(start_room_ptr->nextRooms.size()) + " exits initially.");

        size_t num_needed_exits = static_cast<size_t>(STS_MIN_STARTING_PATHS) - start_room_ptr->nextRooms.size();
        if (num_needed_exits > 0) {
            LOG_INFO("map", "Attempting to add " + std::to_string(num_needed_exits) + " more exits to start room #" + std::to_string(currentRoomId_));
            
            std::vector<Room*> potential_targets_on_floor1;
            for (auto& pair : rooms_) {
                Room& room_on_f1 = pair.second;
                if (room_on_f1.y == 1) {
                    bool already_connected = false;
                    for (int next_id : start_room_ptr->nextRooms) {
                        if (next_id == room_on_f1.id) {
                            already_connected = true;
                            break;
                        }
                    }
                    if (!already_connected) {
                        potential_targets_on_floor1.push_back(&room_on_f1);
                    }
                }
            }
            std::shuffle(potential_targets_on_floor1.begin(), potential_targets_on_floor1.end(), rng_);

            size_t added_count = 0;
            for (Room* target_room_on_floor1 : potential_targets_on_floor1) {
                if (added_count >= num_needed_exits) break;
                
                createRoomLink(start_room_ptr->id, target_room_on_floor1->id);
                added_count++;
                LOG_INFO("map", "Emergency: Added link from start #" + std::to_string(start_room_ptr->id) + " to floor 1 room #" + std::to_string(target_room_on_floor1->id));
            }
            if (added_count < num_needed_exits) {
                LOG_WARNING("map", "Emergency fix: Could only add " + std::to_string(added_count) + " of " + std::to_string(num_needed_exits) + " needed additional exits to start room.");
            }
             LOG_INFO("map", "Start room #" + std::to_string(currentRoomId_) + " now has " + std::to_string(start_room_ptr->nextRooms.size()) + " exits after emergency fix.");
        }
    }

    // 4. Assign Room Types (Complex Logic - initial pass in setRoomType)
    LOG_INFO("map", "Assigning room types (initial pass)...");
    for (auto& pair : rooms_) {
        Room& room = pair.second;
        if (room.type == RoomType::BOSS) continue;
        
        setRoomType(room.id, room.y, room.x, room.nextRooms.size());
    }

    LOG_INFO("map", "Applying GENERALIZED diversity pass for all rooms...");
    for (auto& pair : rooms_) {
        Room& parent_room = pair.second;

        if (parent_room.type == RoomType::BOSS || 
            parent_room.y == STS_PRE_BOSS_REST_FLOOR_Y || 
            parent_room.nextRooms.size() <= 1) {
            continue;
        }

        std::vector<Room*> child_shops;
        std::vector<Room*> child_rests;
        std::vector<Room*> child_events;

        for (int child_id : parent_room.nextRooms) {
            if (!rooms_.count(child_id)) continue;
            Room* child_room = &rooms_.at(child_id);
            
            if (parent_room.y == (STS_PRE_BOSS_REST_FLOOR_Y -1) && child_room->y == STS_PRE_BOSS_REST_FLOOR_Y) continue;
            if (child_room->type == RoomType::BOSS || child_room->y == STS_PRE_BOSS_REST_FLOOR_Y) continue;

            if (child_room->type == RoomType::SHOP)   child_shops.push_back(child_room);
            else if (child_room->type == RoomType::REST)  child_rests.push_back(child_room);
            else if (child_room->type == RoomType::EVENT) child_events.push_back(child_room);
        }

        if (child_shops.size() > 1) {
            LOG_DEBUG("map_diversity_gen", "Parent #" + std::to_string(parent_room.id) + " (y:" + std::to_string(parent_room.y) + ") has " + std::to_string(child_shops.size()) + " SHOP children. Changing extras to EVENT.");
            for (size_t i = 1; i < child_shops.size(); ++i) {
                child_shops[i]->type = RoomType::EVENT;
                LOG_DEBUG("map_diversity_gen", "  Changed child SHOP #" + std::to_string(child_shops[i]->id) + " to EVENT.");
            }
        }
        if (child_rests.size() > 1) {
            LOG_DEBUG("map_diversity_gen", "Parent #" + std::to_string(parent_room.id) + " (y:" + std::to_string(parent_room.y) + ") has " + std::to_string(child_rests.size()) + " REST children. Changing extras to EVENT.");
            for (size_t i = 1; i < child_rests.size(); ++i) {
                child_rests[i]->type = RoomType::EVENT;
                LOG_DEBUG("map_diversity_gen", "  Changed child REST #" + std::to_string(child_rests[i]->id) + " to EVENT.");
            }
        }
        if (child_events.size() > 1 && parent_room.type != RoomType::EVENT /* && parent_room.type != RoomType::SHOP && parent_room.type != RoomType::TREASURE */ ) {
             bool can_change_child_event_to_monster = true;
             if (parent_room.type == RoomType::SHOP || parent_room.type == RoomType::TREASURE || parent_room.type == RoomType::REST){
                 if(parent_room.type != RoomType::MONSTER && parent_room.type != RoomType::ELITE) {
                    can_change_child_event_to_monster = false;
                 }
             }

            if (can_change_child_event_to_monster) {
                LOG_DEBUG("map_diversity_gen", "Parent #" + std::to_string(parent_room.id) + " (y:" + std::to_string(parent_room.y) + ", type:" + getRoomTypeString(parent_room.type) + ") has " + std::to_string(child_events.size()) + " EVENT children. Changing extras to MONSTER.");
                for (size_t i = 1; i < child_events.size(); ++i) {
                    child_events[i]->type = RoomType::MONSTER;
                    LOG_DEBUG("map_diversity_gen", "  Changed child EVENT #" + std::to_string(child_events[i]->id) + " to MONSTER.");
                }
            }
        }
    }

    std::vector<int> candidate_treasure_rooms_ids;
    for (const auto& pair : rooms_) {
        const Room& room = pair.second;
        if (room.y == STS_MID_ACT_TREASURE_FLOOR_Y && room.type != RoomType::BOSS && room.type != RoomType::REST) {
            bool leads_to_boss_directly = false;
            for (int next_id : room.nextRooms) {
                if (rooms_.count(next_id) && rooms_.at(next_id).type == RoomType::BOSS) {
                    leads_to_boss_directly = true;
                    break;
                }
            }
            if (!leads_to_boss_directly) {
                candidate_treasure_rooms_ids.push_back(room.id);
            }
        }
    }
    std::shuffle(candidate_treasure_rooms_ids.begin(), candidate_treasure_rooms_ids.end(), rng_);
    int treasures_to_place = 1 + (rng_() % 2);
    LOG_DEBUG("map", "Attempting to place " + std::to_string(treasures_to_place) + " mid-act treasures on floor " + std::to_string(STS_MID_ACT_TREASURE_FLOOR_Y));
    
    int treasures_placed = 0;
    for (int room_id : candidate_treasure_rooms_ids) {
        if (treasures_placed >= treasures_to_place) break;
        if (rooms_.count(room_id)) {
            rooms_[room_id].type = RoomType::TREASURE;
            treasures_placed++;
            LOG_INFO("map", "Placed mid-act TREASURE at room #" + std::to_string(room_id) + " (y:" + std::to_string(rooms_[room_id].y) + ", x:" + std::to_string(rooms_[room_id].x) + ") overriding its previous type.");
        }
    }
    if (treasures_placed < treasures_to_place) {
        LOG_WARNING("map", "Wanted to place " + std::to_string(treasures_to_place) + " mid-act treasures, but only placed " + std::to_string(treasures_placed) + ".");
    }
    
    if (!validateMap()) {
        LOG_ERROR("map", "Generated StS-style map failed validation.");
        return false;
    }
    LOG_INFO("map", "Successfully generated StS-style map for act " + std::to_string(act_));
    return true;
}

int GameMap::createRoom(int y, int x) {
    int roomId = nextRoomId_++;
    Room newRoom;
    newRoom.id = roomId;
    newRoom.y = y;
    newRoom.x = x;
    newRoom.distanceFromStart = y;
    rooms_[roomId] = newRoom;
    LOG_DEBUG("map_detail", "Created room #" + std::to_string(roomId) + " at (x:" + std::to_string(x) + ", y:" + std::to_string(y) + ")");
    return roomId;
}

void GameMap::createRoomLink(int fromId, int toId) {
    rooms_[fromId].nextRooms.push_back(toId);
    rooms_[toId].prevRooms.push_back(fromId);
    LOG_DEBUG("map_detail", "Linked room #" + std::to_string(fromId) + " -> #" + std::to_string(toId));
}

void GameMap::setRoomType(int roomId, int y, int x, int numPathsFromNode) {
    (void)x;
    (void)numPathsFromNode;
    if (!rooms_.count(roomId)) return;
    Room& room = rooms_[roomId];

    if (room.type != RoomType::MONSTER && room.type != RoomType::BOSS) {
        if (y == STS_PRE_BOSS_REST_FLOOR_Y && room.type == RoomType::REST) {
             LOG_DEBUG("map_detail", "Room #" + std::to_string(roomId) + " y:" + std::to_string(y) + " set to REST (pre-boss area).");
            return;
        }
    }
    if (room.type == RoomType::BOSS) { 
        LOG_DEBUG("map_detail", "Room #" + std::to_string(roomId) + " is BOSS, type assignment skipped.");
        return;
    }
    
    // 1. Handle fixed types based on y-coordinate or flags
    if (y == STS_PRE_BOSS_REST_FLOOR_Y) {
        room.type = RoomType::REST;
        LOG_DEBUG("map_detail", "Room #" + std::to_string(roomId) + " y:" + std::to_string(y) + " set to REST (pre-boss area).");
        return;
    }
    if (y == 0) {
        room.type = RoomType::MONSTER;
        LOG_DEBUG("map_detail", "Room #" + std::to_string(roomId) + " y:" + std::to_string(y) + " set to MONSTER (floor 0).");
        return;
    }
    
    bool is_sole_child_of_start_monster = false;
    if (y == 1 && room.prevRooms.size() == 1) {
        if (rooms_.count(room.prevRooms[0])) {
            const Room& prevRoomOnFloor0 = rooms_.at(room.prevRooms[0]);
            if (prevRoomOnFloor0.y == 0 && prevRoomOnFloor0.type == RoomType::MONSTER) {
                is_sole_child_of_start_monster = true;
            }
        }
    }

    if (is_sole_child_of_start_monster) {
        std::vector<RoomType> restricted_types = {RoomType::MONSTER, RoomType::EVENT};
        std::vector<double> restricted_weights = {0.7, 0.3};
        std::discrete_distribution<> dist(restricted_weights.begin(), restricted_weights.end());
        room.type = restricted_types[dist(rng_)];
        LOG_DEBUG("map_detail", "Room #" + std::to_string(roomId) + " (y:1) is sole child of start MONSTER. Forced to " + getRoomTypeString(room.type));
        return;
    }
    
    // 2. Determine possible types based on constraints
    bool canBeElite = (y >= STS_ELITE_MIN_FLOOR_Y);
    bool canBeRest = (y != (STS_PRE_BOSS_REST_FLOOR_Y - 1));
    bool canBeShop = true;
    bool prevWasElite = false;
    bool prevWasEvent = false;

    bool canBeMonster = true;
    if (y == 1) {
        for (int prevId : room.prevRooms) {
            if (rooms_.count(prevId)) {
                const Room& prevRoomOnFloor0 = rooms_.at(prevId);
                if (prevRoomOnFloor0.y == 0) {
                    canBeMonster = false;
                    LOG_DEBUG("map_detail", "Room #" + std::to_string(roomId) + " (y:1) connected from floor 0. Cannot be MONSTER.");
                    break;
                }
            }
        }
    }

    if (!room.prevRooms.empty()) {
        int firstPrevRoomId = room.prevRooms[0];
        if (rooms_.count(firstPrevRoomId)) {
            const Room& prevRoom = rooms_.at(firstPrevRoomId);
            if (prevRoom.type == RoomType::ELITE) prevWasElite = true;
            if (prevRoom.type == RoomType::SHOP) { canBeShop = false; }
            if (prevRoom.type == RoomType::REST) { canBeRest = false; }
            if (prevRoom.type == RoomType::EVENT) { prevWasEvent = true; }
        }
    }

    // 3. Weighted Random Selection from allowed types
    std::vector<RoomType> possible_types;
    std::vector<double> weights;

    if (canBeMonster) {
        possible_types.push_back(RoomType::MONSTER);
        weights.push_back(prevWasElite ? 0.30 : (prevWasEvent ? 0.55 : 0.45)); // Increased weight if prev was Event
    }

    possible_types.push_back(RoomType::EVENT);
    weights.push_back(prevWasEvent ? 0.15 : 0.25);

    if (canBeShop) {
        possible_types.push_back(RoomType::SHOP);
        weights.push_back(0.15);
    }
    if (canBeElite && !prevWasElite) {
        possible_types.push_back(RoomType::ELITE);
        weights.push_back(0.10);
    }
    if (canBeRest && y > 0 && y < (STS_PRE_BOSS_REST_FLOOR_Y - 1)) {
        possible_types.push_back(RoomType::REST);
        weights.push_back(0.05);
    }

    if (possible_types.empty()) {
        LOG_WARNING("map", "Room #" + std::to_string(roomId) + " y:" + std::to_string(y) + ": No possible types in weighted list, defaulting to EVENT.");
            room.type = RoomType::EVENT;
        return;
    }

    std::discrete_distribution<> dist(weights.begin(), weights.end());
    room.type = possible_types[dist(rng_)];

    if (y == (STS_PRE_BOSS_REST_FLOOR_Y - 1) && room.type == RoomType::REST) {
        LOG_DEBUG("map_detail", "Room #" + std::to_string(roomId) + " on floor y=" + std::to_string(y) + " (pre-pre-boss) became REST, changing to MONSTER.");
        room.type = RoomType::MONSTER;
    }

    if ((room.type == RoomType::SHOP || room.type == RoomType::REST) && room.prevRooms.size() == 1) {
        if (rooms_.count(room.prevRooms[0])) {
            const Room& predecessor = rooms_.at(room.prevRooms[0]);
            if (predecessor.type == RoomType::MONSTER) {
                std::string roomTypeStr = getRoomTypeString(room.type);
                LOG_DEBUG("map_detail", "Room #" + std::to_string(roomId) + " (" + roomTypeStr + ") is sole child of MONSTER #" + std::to_string(predecessor.id) + ". Changing to EVENT.");
                room.type = RoomType::EVENT;
            }
        }
    }

    LOG_DEBUG("map_detail", "Room #" + std::to_string(roomId) + " y:" + std::to_string(y) + " final type: " + getRoomTypeString(room.type));
}

bool GameMap::validateMap() {
    if (rooms_.empty() || !rooms_.count(currentRoomId_)) {
        LOG_ERROR("map_validate", "Validation failed: No rooms or currentRoomId_ is invalid.");
        return false;
    }
    
    const Room* startNode = getRoom(currentRoomId_);
    if (!startNode) {
        LOG_ERROR("map_validate", "Validation failed: Start node is null.");
        return false;
    }
    
    int bossNodeId = -1;
    for(const auto& pair : rooms_) {
        if (pair.second.type == RoomType::BOSS) {
            bossNodeId = pair.first;
            break;
        }
    }
    if (bossNodeId == -1) {
        LOG_ERROR("map_validate", "Validation failed: No boss node found.");
        return false;
    }
    
    std::queue<int> q;
    std::unordered_set<int> visited_nodes;
    
    q.push(currentRoomId_);
    visited_nodes.insert(currentRoomId_);
    
    bool boss_reached = false;
    while(!q.empty()) {
        int u_id = q.front();
        q.pop();
        
        if (u_id == bossNodeId) {
            boss_reached = true;
            break;
        }
        
        const Room* u_room = getRoom(u_id);
        if (!u_room) continue;

        for (int v_id : u_room->nextRooms) {
            if (visited_nodes.find(v_id) == visited_nodes.end()) {
                visited_nodes.insert(v_id);
                q.push(v_id);
            }
        }
    }

    if (!boss_reached) {
        LOG_ERROR("map_validate", "Validation failed: Boss room #" + std::to_string(bossNodeId) + " is not reachable from start room #" + std::to_string(currentRoomId_));
        for(const auto& pair : rooms_) {
            std::string conns = "";
            for(int nid : pair.second.nextRooms) conns += std::to_string(nid) + ",";
            LOG_DEBUG("map_validate", "Room #" + std::to_string(pair.first) + " (y:"+std::to_string(pair.second.y)+",x:"+std::to_string(pair.second.x)+", type:"+getRoomTypeString(pair.second.type)+") -> [" + conns + "]");
        }
        return false;
    }
    LOG_INFO("map_validate", "Map validation successful: Boss is reachable.");
    return true;
}

bool GameMap::canMoveTo(int roomId) const {
    auto roomIt = rooms_.find(roomId);
    if (roomIt == rooms_.end()) {
        return false;
    }
    
    if (currentRoomId_ < 0) {
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
            
            if (it->second.type == RoomType::BOSS) {
                bossDefeated_ = true;
            }
        }
    }
}

bool GameMap::isMapCompleted() const {
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
    
    int floorRange = room.y;
    
    floorRange += (act_ - 1) * 3;
    
    floorRange = std::max(0, std::min(STS_NUM_FLOORS, floorRange)); 
    
    LOG_INFO("map", "Calculated enemy floor range: " + std::to_string(floorRange) + 
             " for room #" + std::to_string(room.id) +
             " at floor " + std::to_string(room.y) +
             " in act " + std::to_string(act_));
    
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