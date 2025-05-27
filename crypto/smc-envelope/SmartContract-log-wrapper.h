#pragma once

#include "emulator/emulator-log-interface.h"
#include "td/utils/logging.h"

/**
 * SmartContractLogWrapper - A wrapper to manage logging for SmartContract.cpp
 * 
 * This class provides a way to temporarily suppress debug messages from SmartContract.cpp
 * when used within the emulator with verbosity level 0.
 */
class SmartContractLogWrapper {
public:
/**
 * Create a new instance that will suppress debug messages if needed
 * 
 * @param verbosity_level The verbosity level (0 = suppress debug messages)
 */
static std::unique_ptr<::emulator::EmulatorLogContext> create(int verbosity_level = 0) {
    if (verbosity_level == 0) {
        return std::make_unique<::emulator::EmulatorLogContext>(verbosity_level);
    }
    return nullptr;
}
};
