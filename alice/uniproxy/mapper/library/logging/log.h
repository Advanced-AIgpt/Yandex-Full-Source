#pragma once

#define LOG_DEBUG(...)                                       \
    if (Logger != nullptr) {                                 \
        Log(*Logger, ELogPriority::TLOG_DEBUG, __VA_ARGS__); \
    }

#define LOG_INFO(...)                                       \
    if (Logger != nullptr) {                                \
        Log(*Logger, ELogPriority::TLOG_INFO, __VA_ARGS__); \
    }
