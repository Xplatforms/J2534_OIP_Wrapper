#ifndef EXDBG_H
#define EXDBG_H

#include <windows.h>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string.h>

#include "expipeclient.h"

#include <array>

class ExDbgEndMarker {};  // Special type for marking the end of a message


class ExDbg {
public:
    static ExDbg& getInstance() {
        static ExDbg instance;
        return instance;
    }

    static std::string getProcessPath() {
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        return std::string(buffer);
    }

    static int64_t crc32(const std::string& data) {
        std::array<int64_t, 256> crc_table;
        for (int64_t i = 0; i < crc_table.size(); i++) {
            int64_t crc = i;
            for (int64_t j = 0; j < 8; j++) {
                crc = (crc & 1) ? (0xEDB88320 ^ (crc >> 1)) : (crc >> 1);
            }
            crc_table[i] = crc;
        }

        int64_t crc = 0xFFFFFFFF;
        for (char c : data) {
            crc = crc_table[(crc ^ c) & 0xFF] ^ (crc >> 8);
        }
        return crc ^ 0xFFFFFFFF;
    }

    template <typename T>
    ExDbg& operator<<(const T& data) {
        std::lock_guard<std::mutex> lock(mutex_);
        buffer_ << data;  // Append to the buffer instead of sending immediately
        return *this;
    }

/*
    ExDbg& operator<<(std::ostream& (*os)(std::ostream&)) {
        if (os == static_cast<std::ostream& (*)(std::ostream&)>(std::endl))
        {
            // If endl is detected, send the buffered message
            const std::string crc32_dat = getProcessPath();
            _pipe_client->send({ crc32_dat, (long)crc32(crc32_dat), 0, buffer_.str(), {} });
            buffer_.str("");  // Clear the buffer
        }
        return *this;
    }*/

    // Overload operator<< to handle the end marker
    ExDbg& operator<<(const ExDbgEndMarker&) {
        // If the end marker is detected, send the buffered message
        _pipe_client->send({this->_crc32_id, this->_type_id, 0, 0, this->_client_path, buffer_.str(), {} });
        buffer_.str("");  // Clear the buffer
        return *this;
    }

    ~ExDbg() {
        if (!buffer_.str().empty()) {
            const std::string crc32_dat = getProcessPath();
            // If there is any data left in the buffer when the object is destroyed, send it
            _pipe_client->send({this->_crc32_id, this->_type_id, 0, 0, this->_client_path, buffer_.str(), {} });
        }
        _pipe_client->disconnect();
    }

private:
    ExDbg();
    ExDbg(const ExDbg&) = delete;
    ExDbg& operator=(const ExDbg&) = delete;
    ExDbg(ExDbg&&) = delete;
    ExDbg& operator=(ExDbg&&) = delete;

    std::ostringstream buffer_;
    std::mutex mutex_;

    std::string     _client_path;
    uint32_t            _crc32_id;
    const uint32_t      _type_id = 0; //0 = Logging


    ExPipeClient * _pipe_client;
};

#define EXDBG_LOG ExDbg::getInstance()
#define EXDBG_END ExDbgEndMarker()

#endif // EXDBG_H
