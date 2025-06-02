// Anisimov Vasiliy st129629@student.spbu.ru
// Laboratory Work 2

#include "util/path_util.h"
#include <filesystem>
#include "util/logger.h"

std::string get_data_path_prefix() {
    std::filesystem::path current_path = std::filesystem::current_path();

    bool exists_dd_data = std::filesystem::exists("../../data");
    bool is_dir_dd_data = exists_dd_data && std::filesystem::is_directory("../../data");
    if (exists_dd_data && is_dir_dd_data) {
        LOG_DEBUG("path_util", "Using '../../' prefix for data path (likely test/build env).");
        return "../../"; 
    }

    bool exists_d_data = std::filesystem::exists("../data");
    bool is_dir_d_data = exists_d_data && std::filesystem::is_directory("../data");
    if (exists_d_data && is_dir_d_data) {
        LOG_DEBUG("path_util", "Using '../' prefix for data path (likely build env).");
        return "../";
    }
    
    bool exists_data = std::filesystem::exists("data");
    bool is_dir_data = exists_data && std::filesystem::is_directory("data");
    if (exists_data && is_dir_data) {
        LOG_DEBUG("path_util", "Using empty prefix for data path (likely project root).");
        return ""; 
    }

    LOG_WARNING("path_util", "Could not determine data path prefix. CWD: " + current_path.string() + ". Defaulting to empty.");
    return "";
} 