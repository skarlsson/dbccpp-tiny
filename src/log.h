#ifndef DBCPPP_TINY_LOG_H
#define DBCPPP_TINY_LOG_H

// Platform-specific logging abstraction for dbcppp-tiny
// Uses ESP_LOG on ESP32, Google Log on Linux/other platforms

#ifdef ESP_PLATFORM
    // ESP32/ESP-IDF platform
    #include "esp_log.h"

    #define LOG_TAG "dbcppp"

    #define LOG_ERROR(fmt, ...) ESP_LOGE(LOG_TAG, fmt, ##__VA_ARGS__)
    #define LOG_WARNING(fmt, ...) ESP_LOGW(LOG_TAG, fmt, ##__VA_ARGS__)
    #define LOG_INFO(fmt, ...) ESP_LOGI(LOG_TAG, fmt, ##__VA_ARGS__)
    #define LOG_DEBUG(fmt, ...) ESP_LOGD(LOG_TAG, fmt, ##__VA_ARGS__)
    #define LOG_VERBOSE(fmt, ...) ESP_LOGV(LOG_TAG, fmt, ##__VA_ARGS__)

#else
    // Linux/other platforms - use Google Log
    #include <glog/logging.h>
    #include <sstream>
    #include <cstdio>

    // Helper class to format printf-style strings for glog
    class LogFormatter {
    public:
        template<typename... Args>
        static std::string format(const char* fmt, Args... args) {
            char buffer[1024];
            snprintf(buffer, sizeof(buffer), fmt, args...);
            return std::string(buffer);
        }
    };

    #define LOG_ERROR(fmt, ...) LOG(ERROR) << LogFormatter::format(fmt, ##__VA_ARGS__)
    #define LOG_WARNING(fmt, ...) LOG(WARNING) << LogFormatter::format(fmt, ##__VA_ARGS__)
    #define LOG_INFO(fmt, ...) LOG(INFO) << LogFormatter::format(fmt, ##__VA_ARGS__)
    #define LOG_DEBUG(fmt, ...) VLOG(1) << LogFormatter::format(fmt, ##__VA_ARGS__)
    #define LOG_VERBOSE(fmt, ...) VLOG(2) << LogFormatter::format(fmt, ##__VA_ARGS__)
#endif

// Utility macro to disable logging entirely if needed
#ifdef DBCPPP_NO_LOGGING
    #undef LOG_ERROR
    #undef LOG_WARNING
    #undef LOG_INFO
    #undef LOG_DEBUG
    #undef LOG_VERBOSE

    #define LOG_ERROR(...) ((void)0)
    #define LOG_WARNING(...) ((void)0)
    #define LOG_INFO(...) ((void)0)
    #define LOG_DEBUG(...) ((void)0)
    #define LOG_VERBOSE(...) ((void)0)
#endif

#endif // DBCPPP_TINY_LOG_H