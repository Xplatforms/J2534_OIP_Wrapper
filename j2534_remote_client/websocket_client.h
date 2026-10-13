#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <windows.h>
#include <winhttp.h>

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "connection_code.h"

// Binary WebSocket client on WinHTTP (TLS and system proxy settings included;
// no extra DLLs). One thread may receive while others send.
class WebSocketClient
{
public:
    WebSocketClient() = default;
    WebSocketClient(const WebSocketClient &) = delete;
    WebSocketClient &operator=(const WebSocketClient &) = delete;
    ~WebSocketClient();

    bool connect(const ConnectionCode &endpoint, DWORD timeoutMs, std::string *error);

    // Sends one binary message. Thread-safe.
    bool send(const std::vector<uint8_t> &message);

    enum class Received { Message, Closed };
    // Blocks for the next complete binary message.
    Received receive(std::vector<uint8_t> &message);

    // Asks the server to close; receive() then returns Closed.
    void shutdown();
    // Cancels a receive() still blocked after shutdown() and frees the handles.
    // Call only once the receiving thread has stopped or must be forced to.
    void close();

private:
    HINTERNET m_session = nullptr;
    HINTERNET m_connection = nullptr;
    HINTERNET m_socket = nullptr;
    std::mutex m_sendMutex;
};

#endif // WEBSOCKET_CLIENT_H
