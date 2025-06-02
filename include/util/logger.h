#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <mutex>
#include <map>
#include <memory>

namespace deckstiny {
namespace util {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

class Logger {
public:
    /**
     * @brief Initialize the logger system
     */
    static void init();
    
    /**
     * @brief Get the singleton instance
     * @return Reference to the logger instance
     */
    static Logger& getInstance();
    
    /**
     * @brief Log a message
     * @param level Log level
     * @param category Log category
     * @param message Message to log
     */
    void log(LogLevel level, const std::string& category, const std::string& message);
    
    /**
     * @brief Set the minimum log level for console output
     * @param level Minimum log level
     */
    void setConsoleLevel(LogLevel level);
    
    /**
     * @brief Set the minimum log level for file output
     * @param level Minimum log level
     */
    void setFileLevel(LogLevel level);
    
    /**
     * @brief Enable or disable console output
     * @param enabled Whether console output is enabled
     */
    void setConsoleEnabled(bool enabled);
    
    /**
     * @brief Enable or disable file output
     * @param enabled Whether file output is enabled
     */
    void setFileEnabled(bool enabled);
    
    /**
     * @brief Set the log directory
     * @param directory Directory to store log files
     */
    void setLogDirectory(const std::string& directory);
    
    /**
     * @brief Set the testing mode
     * @param enabled Whether testing mode is enabled
     */
    static void setTestingMode(bool enabled);
    
    /**
     * @brief Check if the logger is initialized
     * @return True if initialized, false otherwise
     */
    static bool isInitialized();
    
    /**
     * @brief Check if testing mode is enabled
     * @return True if testing mode is enabled, false otherwise
     */
    static bool isTestingMode();
    
    /**
     * @brief Check if console output is enabled
     * @return True if console output is enabled, false otherwise
     */
    bool isConsoleEnabled() const;
    
    /**
     * @brief Get the log directory
     * @return The current log directory
     */
    std::string getLogDirectory() const;
    
protected:
    ~Logger();
    
private:
    friend class std::default_delete<Logger>;
    
    Logger();
    
    // Prevent copying
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // Get log level string
    std::string getLevelString(LogLevel level);
    
    // Get log level color
    std::string getLevelColor(LogLevel level);
    
    // Get timestamp
    std::string getTimestamp();
    
    // Open log file for category
    void openLogFile(const std::string& category);
    
    // Instance
    static std::unique_ptr<Logger> instance_;
    
    // Mutex for thread safety
    std::mutex mutex_;
    
    // Log settings
    LogLevel consoleLevel_ = LogLevel::Info;
    LogLevel fileLevel_ = LogLevel::Debug;
    bool consoleEnabled_ = true;
    bool fileEnabled_ = true;
    std::string logDirectory_ = "logs/deckstiny";
    bool testingMode_ = false;
    
    // Log files
    std::map<std::string, std::ofstream> logFiles_;
};

} // namespace util

// Convenience macros
#define LOG_DEBUG(category, message) ::deckstiny::util::Logger::getInstance().log(::deckstiny::util::LogLevel::Debug, category, message)
#define LOG_INFO(category, message) ::deckstiny::util::Logger::getInstance().log(::deckstiny::util::LogLevel::Info, category, message)
#define LOG_WARNING(category, message) ::deckstiny::util::Logger::getInstance().log(::deckstiny::util::LogLevel::Warning, category, message)
#define LOG_ERROR(category, message) ::deckstiny::util::Logger::getInstance().log(::deckstiny::util::LogLevel::Error, category, message)
#define LOG_FATAL(category, message) ::deckstiny::util::Logger::getInstance().log(::deckstiny::util::LogLevel::Fatal, category, message)

} // namespace deckstiny