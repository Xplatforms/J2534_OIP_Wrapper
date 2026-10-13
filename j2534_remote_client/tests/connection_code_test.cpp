// ConnectionCode::parse: every endpoint form the Vehicle PC or a user may paste.

#include <cstdio>
#include <string>

#include "connection_code.h"

namespace {

int g_failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
            ++g_failures;                                                    \
        }                                                                    \
    } while (0)

ConnectionCode parsed(const std::string &code)
{
    ConnectionCode result;
    std::string error;
    if (!ConnectionCode::parse(code, result, &error))
        std::printf("unexpected parse failure for '%s': %s\n", code.c_str(), error.c_str());
    return result;
}

bool rejects(const std::string &code)
{
    ConnectionCode result;
    std::string error;
    return !ConnectionCode::parse(code, result, &error) && !error.empty();
}

} // namespace

int main()
{
    // Short trycloudflare form, as the Vehicle PC shows it.
    auto c = parsed("brave-cat-xyz#K7P4-9QXM-2M8D");
    CHECK(c.secure);
    CHECK(c.host == L"brave-cat-xyz.trycloudflare.com");
    CHECK(c.port == 443);
    CHECK(c.path == L"/");
    CHECK(c.secret == "K7P4-9QXM-2M8D");
    CHECK(c.displayHost() == "brave-cat-xyz.trycloudflare.com");

    // Full URL pasted from cloudflared's output, with whitespace.
    c = parsed("  https://Brave-Cat-XYZ.trycloudflare.com#abc  ");
    CHECK(c.secure && c.host == L"brave-cat-xyz.trycloudflare.com" && c.port == 443 && c.secret == "abc");

    c = parsed("wss://relay.example.com:8443/j2534#s");
    CHECK(c.secure && c.host == L"relay.example.com" && c.port == 8443 && c.path == L"/j2534");
    CHECK(c.displayHost() == "relay.example.com:8443");

    c = parsed("http://10.0.0.5:8080#s");
    CHECK(!c.secure && c.port == 8080);

    // Loopback without scheme -> plain ws (tests, same-PC use).
    c = parsed("127.0.0.1:47534#secret");
    CHECK(!c.secure && c.host == L"127.0.0.1" && c.port == 47534);
    c = parsed("localhost:1234#x");
    CHECK(!c.secure && c.host == L"localhost");

    // The last '#' separates address and secret.
    c = parsed("host.example.com#a#b");
    CHECK(c.secret == "b");

    CHECK(rejects(""));
    CHECK(rejects("brave-cat-xyz"));         // no secret
    CHECK(rejects("brave-cat-xyz#"));        // empty secret
    CHECK(rejects("#secret"));               // no address
    CHECK(rejects("host:99999#s"));          // bad port
    CHECK(rejects("host:abc#s"));

    std::printf("%s (%d failure%s)\n", g_failures ? "FAILED" : "PASSED", g_failures, g_failures == 1 ? "" : "s");
    return g_failures ? 1 : 0;
}
