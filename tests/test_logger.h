#pragma once

#include "util/logger.h"

namespace deckstiny {
namespace testing {

/**
 * A test-specific logger that disables console output
 * This is used to prevent test output from being cluttered with log messages
 */
class TestLogger : public util::Logger {
public:
    TestLogger() : util::Logger() {
        // Always disable console output for tests
        setConsoleEnabled(false);
    }
    
    // Ensure setConsoleEnabled always returns to disabled for tests
    void setConsoleEnabled(bool enabled) override {
        // Ignore the parameter and always set to false
        util::Logger::setConsoleEnabled(false);
    }
    
    // Static method to initialize the test logger
    static void init() {
        if (!util::Logger::instance_) {
            util::Logger::instance_ = std::unique_ptr<util::Logger>(new TestLogger());
            util::Logger::instance_->setConsoleLevel(util::LogLevel::Fatal);
            util::Logger::instance_->setFileLevel(util::LogLevel::Debug);
            util::Logger::instance_->setFileEnabled(true);
            util::Logger::instance_->setLogDirectory("logs/deckstiny_tests");
        }
    }
};

} // namespace testing
} // namespace deckstiny 