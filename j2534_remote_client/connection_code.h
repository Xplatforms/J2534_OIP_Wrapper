#ifndef CONNECTION_CODE_H
#define CONNECTION_CODE_H

#include <cstdint>
#include <string>

// The code the Vehicle PC shows and the Diagnostic PC pastes:
//
//   <endpoint>#<secret>
//
// <endpoint> may be
//   brave-cat-xyz                         -> wss://brave-cat-xyz.trycloudflare.com/
//   brave-cat-xyz.trycloudflare.com       -> wss://…:443/
//   https://host[:port][/path]            -> wss://…
//   http://host[:port][/path]             -> ws://…
//   wss://… / ws://…                      -> as given
//   127.0.0.1:47534, localhost:47534      -> ws:// (loopback only; tests and LAN use)
struct ConnectionCode
{
    bool secure = true;
    std::wstring host;
    uint16_t port = 443;
    std::wstring path = L"/";
    std::string secret;

    // Human-readable endpoint for logs and error messages ("brave-cat-xyz.trycloudflare.com").
    std::string displayHost() const;

    // Returns false and sets `error` for malformed codes.
    static bool parse(const std::string &code, ConnectionCode &out, std::string *error);
};

#endif // CONNECTION_CODE_H
