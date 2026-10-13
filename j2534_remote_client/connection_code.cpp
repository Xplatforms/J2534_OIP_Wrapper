#include "connection_code.h"

#include <algorithm>
#include <cctype>

namespace {

std::string trim(const std::string &s)
{
    const auto begin = s.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos)
        return {};
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(begin, end - begin + 1);
}

std::string lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::wstring widen(const std::string &s)
{
    return std::wstring(s.begin(), s.end());   // host names and paths are ASCII
}

bool fail(std::string *error, const std::string &message)
{
    if (error)
        *error = message;
    return false;
}

bool isLoopback(const std::string &host)
{
    return host == "localhost" || host == "127.0.0.1" || host == "::1" || host == "[::1]";
}

} // namespace

std::string ConnectionCode::displayHost() const
{
    std::string text;
    for (wchar_t c : host)
        text += static_cast<char>(c);   // host names are ASCII (see parse())
    const bool defaultPort = (secure && port == 443) || (!secure && port == 80);
    if (!defaultPort)
        text += ":" + std::to_string(port);
    return text;
}

bool ConnectionCode::parse(const std::string &code, ConnectionCode &out, std::string *error)
{
    const std::string text = trim(code);
    const auto hash = text.rfind('#');
    if (text.empty())
        return fail(error, "No connection code set. Paste the code shown on the Vehicle PC.");
    if (hash == std::string::npos || hash + 1 >= text.size())
        return fail(error, "The connection code has no secret (expected <address>#<secret>).");

    ConnectionCode result;
    result.secret = text.substr(hash + 1);
    std::string endpoint = text.substr(0, hash);

    // Scheme
    const std::string lowered = lower(endpoint);
    bool schemeGiven = true;
    if (lowered.rfind("wss://", 0) == 0)        { result.secure = true;  endpoint = endpoint.substr(6); }
    else if (lowered.rfind("https://", 0) == 0) { result.secure = true;  endpoint = endpoint.substr(8); }
    else if (lowered.rfind("ws://", 0) == 0)    { result.secure = false; endpoint = endpoint.substr(5); }
    else if (lowered.rfind("http://", 0) == 0)  { result.secure = false; endpoint = endpoint.substr(7); }
    else schemeGiven = false;

    // Path
    std::string path = "/";
    const auto slash = endpoint.find('/');
    if (slash != std::string::npos) {
        path = endpoint.substr(slash);
        endpoint = endpoint.substr(0, slash);
    }

    // Port
    std::string host = endpoint;
    int port = -1;
    const auto colon = endpoint.rfind(':');
    if (colon != std::string::npos && endpoint.find(']') == std::string::npos) {
        host = endpoint.substr(0, colon);
        const std::string portText = endpoint.substr(colon + 1);
        if (portText.empty() || portText.find_first_not_of("0123456789") != std::string::npos)
            return fail(error, "Invalid port in the connection code.");
        port = std::stoi(portText);
        if (port <= 0 || port > 65535)
            return fail(error, "Invalid port in the connection code.");
    }
    host = lower(host);
    if (host.empty())
        return fail(error, "The connection code has no address.");

    // Short trycloudflare name: "brave-cat-xyz"
    if (!schemeGiven && host.find('.') == std::string::npos && !isLoopback(host))
        host += ".trycloudflare.com";

    if (!schemeGiven)
        result.secure = !isLoopback(host);

    result.host = widen(host);
    result.port = static_cast<uint16_t>(port > 0 ? port : (result.secure ? 443 : 80));
    result.path = widen(path);
    out = result;
    return true;
}
