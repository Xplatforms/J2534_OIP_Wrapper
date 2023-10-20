#ifndef TIMESTAMP_UTIL_H
#define TIMESTAMP_UTIL_H

#include <string>
#include <chrono>

static inline int64_t get_timestamp()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

#endif // TIMESTAMP_UTIL_H
