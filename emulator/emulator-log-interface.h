#pragma once

#include "td/utils/logging.h"
#include <memory>

namespace emulator {

/**
 * EmulatorLogContext - A class to manage logging context for the emulator
 * 
 * This class creates a scoped log context that respects the verbosity level
 * set by the user. It ensures that debug messages are properly suppressed
 * when verbosity_level is set to 0.
 */
class EmulatorLogContext {
public:
    /**
     * Constructor - Creates a new log context with the specified verbosity level
     * 
     * @param verbosity_level The verbosity level to use (0 = suppress debug messages)
     */
    explicit EmulatorLogContext(int verbosity_level) {
        // Store the original log options
        original_log_options_ = td::log_options;
        
        // Create a new log options with the specified verbosity level
        td::LogOptions new_options = td::log_options;
        new_options.level = VERBOSITY_NAME(FATAL) + verbosity_level;
        
        // Set the new log options
        td::log_options = new_options;
    }
    
    /**
     * Destructor - Restores the original log options
     */
    ~EmulatorLogContext() {
        // Restore the original log options
        td::log_options = original_log_options_;
    }
    
private:
    // The original log options to restore when this context is destroyed
    td::LogOptions original_log_options_;
};

} // namespace emulator
