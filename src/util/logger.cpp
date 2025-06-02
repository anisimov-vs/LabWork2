// Anisimov Vasiliy st129629@student.spbu.ru
// Laboratory Work 2

#include "util/logger.h"
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace deckstiny {
namespace util {

std::unique_ptr<Logger> Logger::instance_;

void Logger::init() {
    if (!instance_) {
        instance_ = std::unique_ptr<Logger>(new Logger());
        instance_->setConsoleLevel(LogLevel::Info);
        instance_->setFileLevel(LogLevel::Debug);
        instance_->setConsoleEnabled(false);
        instance_->setFileEnabled(true);
        instance_->setLogDirectory("logs/deckstiny");
    }
}

Logger& Logger::getInstance() {
    if (!instance_) {
        init();
    }
    return *instance_;
}

void Logger::setTestingMode(bool enabled) {
    if (!instance_) {
        init();
    }
    if (enabled) {
        instance_->setConsoleEnabled(false);
        instance_->setConsoleLevel(LogLevel::Fatal);
        instance_->testingMode_ = true;
    } else {
        instance_->testingMode_ = false;
    }
}

bool Logger::isInitialized() {
    return static_cast<bool>(instance_);
}

bool Logger::isTestingMode() {
    if (!instance_) {
        return false;
    }
    return instance_->testingMode_;
}

bool Logger::isConsoleEnabled() const {
    return consoleEnabled_;
}

Logger::Logger() {
    std::filesystem::create_directories(logDirectory_);
}

Logger::~Logger() {
    for (auto& file : logFiles_) {
        if (file.second.is_open()) {
            file.second.close();
        }
    }
}

void Logger::log(LogLevel level, const std::string& category, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string timestamp = getTimestamp();
    std::string levelStr = getLevelString(level);
    
    if (consoleEnabled_ && level >= consoleLevel_) {
        std::string colorCode = getLevelColor(level);
        std::cout << colorCode << timestamp << " [" << levelStr << "] " << category << ": " << message << "\033[0m" << std::endl;
    }
    
    if (fileEnabled_ && level >= fileLevel_) {
        openLogFile(category);
        logFiles_[category] << timestamp << " [" << levelStr << "] " << message << std::endl;
    }
}

void Logger::setConsoleLevel(LogLevel level) {
    consoleLevel_ = level;
}

void Logger::setFileLevel(LogLevel level) {
    fileLevel_ = level;
}

void Logger::setConsoleEnabled(bool enabled) {
    consoleEnabled_ = enabled;
}

void Logger::setFileEnabled(bool enabled) {
    fileEnabled_ = enabled;
}

void Logger::setLogDirectory(const std::string& directory) {
    logDirectory_ = directory;
    std::filesystem::create_directories(directory);
}

std::string Logger::getLogDirectory() const {
    return logDirectory_;
}

std::string Logger::getLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
        default:               return "UNKNOWN";
    }
}

std::string Logger::getLevelColor(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return "\033[37m"; // White
        case LogLevel::Info:    return "\033[32m"; // Green
        case LogLevel::Warning: return "\033[33m"; // Yellow
        case LogLevel::Error:   return "\033[31m"; // Red
        case LogLevel::Fatal:   return "\033[35m"; // Magenta
        default:               return "\033[0m";  // Reset
    }
}

std::string Logger::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

void Logger::openLogFile(const std::string& category) {
    if (logFiles_.find(category) == logFiles_.end() || !logFiles_[category].is_open()) {
        std::string filename = logDirectory_ + "/" + category + ".log";
        logFiles_[category].open(filename, std::ios::app);
    }
}

} // namespace util
} // namespace deckstiny