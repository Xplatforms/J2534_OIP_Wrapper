#include "websocket_client.h"

namespace {

bool fail(std::string *error, const std::string &message, DWORD code = GetLastError())
{
    if (error)
        *error = message + " (WinHTTP error " + std::to_string(code) + ")";
    return false;
}

} // namespace

WebSocketClient::~WebSocketClient()
{
    close();
}

bool WebSocketClient::connect(const ConnectionCode &endpoint, DWORD timeoutMs, std::string *error)
{
    close();

    m_session = WinHttpOpen(L"J2534OverIP/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!m_session)
        return fail(error, "Can't initialise WinHTTP");
    const int timeout = static_cast<int>(timeoutMs);
    WinHttpSetTimeouts(m_session, timeout, timeout, timeout, timeout);

    m_connection = WinHttpConnect(m_session, endpoint.host.c_str(), endpoint.port, 0);
    if (!m_connection)
        return fail(error, "Can't reach " + endpoint.displayHost());

    HINTERNET request = WinHttpOpenRequest(m_connection, L"GET", endpoint.path.c_str(), nullptr,
                                           WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                           endpoint.secure ? WINHTTP_FLAG_SECURE : 0);
    if (!request)
        return fail(error, "Can't create the request to " + endpoint.displayHost());

    bool ok = WinHttpSetOption(request, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, nullptr, 0)
              && WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
              && WinHttpReceiveResponse(request, nullptr);
    if (!ok) {
        const DWORD code = GetLastError();
        WinHttpCloseHandle(request);
        return fail(error, "Can't connect to " + endpoint.displayHost(), code);
    }

    DWORD status = 0;
    DWORD size = sizeof(status);
    WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);
    if (status != 101) {
        WinHttpCloseHandle(request);
        if (error) {
            *error = endpoint.displayHost() + " answered HTTP " + std::to_string(status)
                     + (status == 530 || status == 404 ? " - the Vehicle PC is not sharing (or the code is outdated)." : ".");
        }
        return false;
    }

    m_socket = WinHttpWebSocketCompleteUpgrade(request, 0);
    const DWORD code = GetLastError();
    WinHttpCloseHandle(request);
    if (!m_socket)
        return fail(error, "WebSocket upgrade failed", code);
    return true;
}

bool WebSocketClient::send(const std::vector<uint8_t> &message)
{
    std::lock_guard<std::mutex> lock(m_sendMutex);
    if (!m_socket)
        return false;
    return WinHttpWebSocketSend(m_socket, WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE,
                                const_cast<uint8_t *>(message.data()), static_cast<DWORD>(message.size())) == NO_ERROR;
}

WebSocketClient::Received WebSocketClient::receive(std::vector<uint8_t> &message)
{
    message.clear();
    uint8_t chunk[64 * 1024];
    for (;;) {
        DWORD read = 0;
        WINHTTP_WEB_SOCKET_BUFFER_TYPE type{};
        if (!m_socket || WinHttpWebSocketReceive(m_socket, chunk, sizeof(chunk), &read, &type) != NO_ERROR)
            return Received::Closed;

        switch (type) {
        case WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE:
            message.insert(message.end(), chunk, chunk + read);
            break;
        case WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE:
            message.insert(message.end(), chunk, chunk + read);
            return Received::Message;
        case WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE:
            return Received::Closed;
        default:   // text messages are not part of the protocol
            message.clear();
            break;
        }
    }
}

void WebSocketClient::shutdown()
{
    std::lock_guard<std::mutex> lock(m_sendMutex);
    if (m_socket)
        WinHttpWebSocketShutdown(m_socket, WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, nullptr, 0);
}

void WebSocketClient::close()
{
    std::lock_guard<std::mutex> lock(m_sendMutex);
    for (HINTERNET *handle : {&m_socket, &m_connection, &m_session}) {
        if (*handle) {
            WinHttpCloseHandle(*handle);
            *handle = nullptr;
        }
    }
}
