// Anisimov Vasiliy st129629@student.spbu.ru
// Laboratory Work 2

#ifndef DECKSTINY_UTIL_PATH_UTIL_H
#define DECKSTINY_UTIL_PATH_UTIL_H

#include <string>
#include <filesystem>
 
// Helper function to determine the data path prefix
std::string get_data_path_prefix();

// Helper function to get the user-specific config/data directory for Deckstiny
std::filesystem::path get_user_deckstiny_config_dir_path();

#endif // DECKSTINY_UTIL_PATH_UTIL_H