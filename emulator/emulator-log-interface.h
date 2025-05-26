/*
    This file is part of TON Blockchain Library.

    TON Blockchain Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    TON Blockchain Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with TON Blockchain Library.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#include "td/utils/logging.h"
#include <memory>

namespace emulator {

/**
 * Custom LogInterface that filters debug messages based on emulator verbosity level.
 * This avoids modifying global logging state and prevents symbol duplication issues.
 */
class EmulatorLogInterface : public td::LogInterface {
private:
    td::LogInterface* original_log_;
    int verbosity_level_;
    
public:
    EmulatorLogInterface(td::LogInterface* original, int verbosity) 
        : original_log_(original), verbosity_level_(verbosity) {}
    
    int get_verbosity_level() const {
        return verbosity_level_;
    }
    
    void append(td::CSlice slice, int log_level) override {
        // Filter DEBUG messages when verbosity is 0
        if (verbosity_level_ == 0 && log_level >= VERBOSITY_NAME(DEBUG)) {
            return; // Suppress debug messages
        }
        if (original_log_) {
            original_log_->append(slice, log_level);
        }
    }
    
    void append(td::CSlice slice) override {
        append(slice, -1);
    }
    
    void rotate() override { 
        if (original_log_) {
            original_log_->rotate(); 
        }
    }
    
    std::vector<std::string> get_file_paths() override { 
        if (original_log_) {
            return original_log_->get_file_paths(); 
        }
        return {};
    }
};

/**
 * RAII class to manage emulator-specific logging context.
 * Automatically restores original log interface when destroyed.
 */
class EmulatorLogContext {
private:
    td::LogInterface* original_log_;
    std::unique_ptr<EmulatorLogInterface> emulator_log_;
    
public:
    EmulatorLogContext(int verbosity_level) 
        : original_log_(td::log_interface) {
        emulator_log_ = std::make_unique<EmulatorLogInterface>(original_log_, verbosity_level);
        td::log_interface = emulator_log_.get();
    }
    
    ~EmulatorLogContext() {
        td::log_interface = original_log_;
    }
    
    int get_verbosity_level() const {
        return emulator_log_->get_verbosity_level();
    }
    
    // Non-copyable, non-movable
    EmulatorLogContext(const EmulatorLogContext&) = delete;
    EmulatorLogContext& operator=(const EmulatorLogContext&) = delete;
    EmulatorLogContext(EmulatorLogContext&&) = delete;
    EmulatorLogContext& operator=(EmulatorLogContext&&) = delete;
};

} // namespace emulator
