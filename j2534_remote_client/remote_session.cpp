#include "remote_session.h"

#include <chrono>

namespace {

using Clock = std::chrono::steady_clock;

const char *helloStatusText(j2534_remote_HelloResult_Status status)
{
    switch (status) {
    case j2534_remote_HelloResult_Status_STATUS_BAD_SECRET: return "the connection code's secret is wrong (codes change every session)";
    case j2534_remote_HelloResult_Status_STATUS_REJECTED: return "the Vehicle PC rejected the connection";
    case j2534_remote_HelloResult_Status_STATUS_BUSY: return "another Diagnostic PC is already connected";
    case j2534_remote_HelloResult_Status_STATUS_VERSION_MISMATCH: return "the Vehicle PC runs an incompatible version";
    default: return "the Vehicle PC refused the connection";
    }
}

} // namespace

RemoteSession::~RemoteSession()
{
    disconnect();
}

bool RemoteSession::connect(const ConnectionCode &endpoint, const std::string &clientName, DWORD helloTimeoutMs,
                            std::string *error)
{
    disconnect();

    if (!m_socket.connect(endpoint, 15000, error))
        return false;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_helloResult.reset();
        m_responses.clear();
        m_rx.clear();
    }
    m_open = true;
    m_receiver = std::thread(&RemoteSession::receiveLoop, this);

    auto hello = remote::newFrame();
    hello->which_kind = j2534_remote_Frame_hello_tag;
    hello->kind.hello.version = j2534_remote_Version_VERSION_CURRENT;
    remote::copyString(hello->kind.hello.secret, endpoint.secret);
    remote::copyString(hello->kind.hello.client_name, clientName);
    if (!m_socket.send(remote::encode(*hello))) {
        if (error) *error = "Lost the connection to " + endpoint.displayHost();
        disconnect();
        return false;
    }

    std::unique_lock<std::mutex> lock(m_mutex);
    const bool answered = m_changed.wait_for(lock, std::chrono::milliseconds(helloTimeoutMs),
                                             [this] { return m_helloResult || !m_open; });
    if (!answered || !m_helloResult) {
        lock.unlock();
        if (error) *error = answered ? "The Vehicle PC closed the connection" : "No answer from the Vehicle PC (was the connection allowed?)";
        disconnect();
        return false;
    }

    const auto &result = m_helloResult->kind.hello_result;
    if (result.status != j2534_remote_HelloResult_Status_STATUS_ACCEPTED) {
        std::string text = std::string("Connection refused: ") + helloStatusText(result.status);
        if (result.message[0])
            text += std::string(" (") + result.message + ")";
        lock.unlock();
        if (error) *error = text;
        disconnect();
        return false;
    }
    m_adapterName = result.adapter_name;
    return true;
}

std::string RemoteSession::adapterName() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_adapterName;
}

std::unique_ptr<remote::Frame> RemoteSession::call(remote::Frame &request, DWORD timeoutMs, std::string *error)
{
    if (!m_open) {
        if (error) *error = "Not connected to the Vehicle PC";
        return nullptr;
    }

    uint32_t id;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        id = ++m_nextId;
        m_responses.erase(id);
    }
    request.which_kind = j2534_remote_Frame_request_tag;
    request.kind.request.id = id;

    const auto bytes = remote::encode(request);
    if (bytes.empty()) {
        if (error) *error = "Can't encode the request";
        return nullptr;
    }
    if (!m_socket.send(bytes)) {
        if (error) *error = "Lost the connection to the Vehicle PC";
        return nullptr;
    }

    std::unique_lock<std::mutex> lock(m_mutex);
    const bool answered = m_changed.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                                             [&] { return m_responses.count(id) || !m_open; });
    auto it = m_responses.find(id);
    if (it == m_responses.end()) {
        if (error) *error = answered ? "Lost the connection to the Vehicle PC" : "The Vehicle PC did not answer in time";
        return nullptr;
    }
    auto response = std::move(it->second);
    m_responses.erase(it);
    return response;
}

size_t RemoteSession::readRx(uint32_t channel, PASSTHRU_MSG *out, size_t max, DWORD timeoutMs)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (timeoutMs > 0) {
        m_changed.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                           [&] { return m_rx[channel].size() >= max || !m_open; });
    }
    auto &queue = m_rx[channel];
    const size_t count = std::min(max, queue.size());
    for (size_t i = 0; i < count; ++i) {
        out[i] = queue.front();
        queue.pop_front();
    }
    return count;
}

void RemoteSession::clearRx(uint32_t channel)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_rx.erase(channel);
}

void RemoteSession::disconnect()
{
    if (m_receiver.joinable()) {
        m_socket.shutdown();
        // Give the server a moment to answer the close; then cancel.
        for (int i = 0; i < 40 && m_open; ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        m_socket.close();
        m_receiver.join();
    }
    m_socket.close();
    m_open = false;
    m_changed.notify_all();
}

void RemoteSession::receiveLoop()
{
    std::vector<uint8_t> bytes;
    auto frame = remote::newFrame();

    while (m_socket.receive(bytes) == WebSocketClient::Received::Message) {
        if (!remote::decode(bytes.data(), bytes.size(), *frame))
            continue;

        std::lock_guard<std::mutex> lock(m_mutex);
        switch (frame->which_kind) {
        case j2534_remote_Frame_hello_result_tag:
            m_helloResult = std::move(frame);
            frame = remote::newFrame();
            break;
        case j2534_remote_Frame_response_tag: {
            const uint32_t id = frame->kind.response.id;
            m_responses[id] = std::move(frame);
            frame = remote::newFrame();
            break;
        }
        case j2534_remote_Frame_rx_tag: {
            auto &queue = m_rx[frame->kind.rx.channel_id];
            for (pb_size_t i = 0; i < frame->kind.rx.msgs_count; ++i) {
                if (queue.size() >= kMaxQueued) {
                    queue.pop_front();
                    ++m_dropped;
                }
                queue.emplace_back();
                remote::fromWire(frame->kind.rx.msgs[i], queue.back());
            }
            break;
        }
        default:
            break;
        }
        m_changed.notify_all();
    }

    m_open = false;
    m_changed.notify_all();
}
