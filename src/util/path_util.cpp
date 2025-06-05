// Anisimov Vasiliy st129629@student.spbu.ru
// Laboratory Work 2

#include "util/path_util.h"
#include <filesystem>
#include <cstdlib> // For getenv
#include "util/logger.h"

#ifdef __linux__
#include <unistd.h> // For readlink
#include <limits.h> // For PATH_MAX
#endif

// Helper function to get the directory of the current executable
std::filesystem::path get_executable_directory() {
#ifdef __linux__
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count != -1) {
        return std::filesystem::path(result).parent_path();
    }
#endif
    return "";
}

// Helper function to get the user-specific config/data directory for Deckstiny
std::filesystem::path get_user_deckstiny_config_dir_path() {
    const char* homeDirEnv = std::getenv("HOME");
    if (homeDirEnv) {
        return std::filesystem::path(homeDirEnv) / ".local" / "share" / "Deckstiny";
    }
    LOG_WARNING("path_util", "HOME environment variable not set. User-specific paths will be relative to CWD.");
    return std::filesystem::current_path() / ".deckstiny_user_data"; 
}

std::string get_data_path_prefix() {
    // 1. Check user-specific data directory (~/.local/share/Deckstiny/data)
    std::filesystem::path user_config_dir = get_user_deckstiny_config_dir_path();
    std::filesystem::path user_data_path = user_config_dir / "data";
    if (std::filesystem::exists(user_data_path) && std::filesystem::is_directory(user_data_path)) {
        LOG_INFO("path_util", "Using user-specific data path: " + user_config_dir.string() + "/");
        return user_config_dir.string() + "/"; // Prefix is the parent of 'data'
    }

    // 2. Check for data directory in Current Working Directory (./data)
    std::filesystem::path cwd_data_path = std::filesystem::current_path() / "data";
    if (std::filesystem::exists(cwd_data_path) && std::filesystem::is_directory(cwd_data_path)) {
        LOG_INFO("path_util", "Using CWD data path (./data). Prefix: (empty)");
        return ""; // Data is directly in ./data, so prefix is empty
    }
    
    // 3. Check APPDIR (for AppImage internal bundled data)
    const char* appDirEnv = std::getenv("APPDIR");
    if (appDirEnv) {
        LOG_DEBUG("path_util", "APPDIR environment variable found: " + std::string(appDirEnv));
        std::filesystem::path appdir_path(appDirEnv);
        std::filesystem::path appimage_resource_base = appdir_path / "usr" / "share" / "resources";
        std::filesystem::path appimage_bundled_data_path = appimage_resource_base / "data";

        if (std::filesystem::exists(appimage_bundled_data_path) && std::filesystem::is_directory(appimage_bundled_data_path)) {
            LOG_INFO("path_util", "Using AppImage internal data path. Prefix: " + appimage_resource_base.string() + "/");
            return appimage_resource_base.string() + "/"; // Prefix is parent of 'data'
        } else {
            LOG_WARNING("path_util", "APPDIR is set, but bundled data directory not found at " + appimage_bundled_data_path.string() + ".");
        }
    }

    // 4. Check for data directory next to the executable (less common for AppImage after user/APPDIR checks)
    std::filesystem::path exec_dir = get_executable_directory();
    if (!exec_dir.empty()) {
        std::filesystem::path exec_relative_data_path = exec_dir / "data";
        if (std::filesystem::exists(exec_relative_data_path) && std::filesystem::is_directory(exec_relative_data_path)) {
            LOG_INFO("path_util", "Using data path relative to executable: " + exec_dir.string() + "/");
            return exec_dir.string() + "/"; 
        }
    }

    // 5. Fallback to relative CWD checks (../data, ../../data)
    LOG_DEBUG("path_util", "Falling back to CWD-relative parent directory checks. CWD: " + std::filesystem::current_path().string());
    if (std::filesystem::exists("../data") && std::filesystem::is_directory("../data")) {
        LOG_INFO("path_util", "Using '../' prefix for data path (../data found).");
        return "../";
    }
    if (std::filesystem::exists("../../data") && std::filesystem::is_directory("../../data")) {
        LOG_INFO("path_util", "Using '../../' prefix for data path (../../data found).");
        return "../../";
    }

    LOG_WARNING("path_util", "Could not determine data path prefix. Defaulting to empty (./). This might lead to errors if data is not in CWD/data.");
    return ""; 
} 