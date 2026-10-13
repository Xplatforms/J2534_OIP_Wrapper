#ifndef REMOTE_SESSION_H
#define REMOTE_SESSION_H

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "connection_code.h"
#include "remote_frame.h"
#include "websocket_client.h"

// Diagnostic PC side of the link: one WebSocket to the Vehicle PC, request /
// response matching, and per-channel queues of pushed RX messages.
class RemoteSession
{
public:
    RemoteSession() = default;
    RemoteSession(const RemoteSession &) = delete;
    RemoteSession &operator=(const RemoteSession &) = delete;
    ~RemoteSession();

    // Connects and sends Hello. Waits up to `helloTimeoutMs` for the Vehicle
    // PC to accept (its user may have to click Allow). Returns false and sets
    // `error` if the link can't be used.
    bool connect(const ConnectionCode &endpoint, const std::string &clientName, DWORD helloTimeoutMs,
                 std::string *error);
    bool connected() const { return m_open; }
    std::string adapterName() const;

    // Sends `request` (kind.request filled in, id assigned here) and waits for
    // the matching response. Null on timeout or lost link (`error` says which).
    std::unique_ptr<remote::Frame> call(remote::Frame &request, DWORD timeoutMs, std::string *error);

    // Takes up to `max` received messages for `channel`, waiting up to
    // `timeoutMs` for that many to arrive. Returns how many were copied.
    size_t readRx(uint32_t channel, PASSTHRU_MSG *out, size_t max, DWORD timeoutMs);
    void clearRx(uint32_t channel);
    // Messages dropped because a queue was full (oldest first).
    uint64_t droppedRx() const { return m_dropped; }

    void disconnect();

    // Per-channel queue limit; ~4 KB per message.
    static constexpr size_t kMaxQueued = 20000;

private:
    void receiveLoop();

    WebSocketClient m_socket;
    std::thread m_receiver;
    std::atomic<bool> m_open{false};
    std::atomic<uint64_t> m_dropped{0};

    mutable std::mutex m_mutex;
    std::condition_variable m_changed;
    uint32_t m_nextId = 0;
    std::map<uint32_t, std::unique_ptr<remote::Frame>> m_responses;   // by request id
    std::unique_ptr<remote::Frame> m_helloResult;
    std::map<uint32_t, std::deque<PASSTHRU_MSG>> m_rx;
    std::string m_adapterName;
};

#endif // REMOTE_SESSION_H
