#pragma once

#include <stdint.h>

typedef enum {
    DBG_LOG_LVL_NONE = 0,
    DBG_LOG_LVL_ERROR = 1,
    DBG_LOG_LVL_WARNING = 2,
    DBG_LOG_LVL_INFO = 3,
} DbgLogLevel;

void dbg_log(DbgLogLevel level, const char* tag, const char* format, ...);
void dbg_dump(DbgLogLevel level, const char* tag, const uint8_t* data, uint32_t len);
void dbg_log_init(DbgLogLevel level_max);

#define DBG_LOG_E(tag, format, ...) dbg_log(DBG_LOG_LVL_ERROR, tag, format, ##__VA_ARGS__)
#define DBG_LOG_W(tag, format, ...) dbg_log(DBG_LOG_LVL_WARNING, tag, format, ##__VA_ARGS__)
#define DBG_LOG_I(tag, format, ...) dbg_log(DBG_LOG_LVL_INFO, tag, format, ##__VA_ARGS__)

#define DBG_DUMP(tag, data, len) dbg_dump(DBG_LOG_LVL_INFO, tag, data, len)
