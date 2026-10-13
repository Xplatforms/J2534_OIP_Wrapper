// End to end over localhost, everything real except the J2534 driver:
//
//   J2534RemoteClient (x86, WinHTTP) --ws--> App --vehicle-server (x64)
//        --pipe--> j2534_device_host.exe (x86) --> fake_j2534_driver.dll
//
//   remote_e2e_test.exe <j2534_OverIP_App.exe> <j2534_device_host.exe> <fake_j2534_driver.dll>
//
// Exits 77 (skipped) when the App has not been built/installed.

#include <windows.h>

#include <cstdio>
#include <string>
#include <vector>

#include "j2534_defs.h"
#include "j2534remoteclient.h"

namespace {

int g_failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
            ++g_failures;                                                    \
        }                                                                    \
    } while (0)

std::wstring absolute(const wchar_t *path)
{
    wchar_t full[MAX_PATH] = {};
    GetFullPathNameW(path, MAX_PATH, full, nullptr);
    return full;
}

std::string lastError(J2534RemoteClient &client)
{
    char text[80] = {};
    client.PassThruGetLastError(text);
    return text;
}

PASSTHRU_MSG canMsg(uint32_t id, unsigned char tag)
{
    PASSTHRU_MSG msg{};
    msg.ProtocolID = 5;   // CAN
    msg.Data[2] = static_cast<unsigned char>(id >> 8);
    msg.Data[3] = static_cast<unsigned char>(id);
    msg.Data[4] = 0x02;
    msg.Data[5] = 0x3E;
    msg.Data[6] = tag;
    msg.DataSize = 7;
    return msg;
}

// Opens a device, retrying while the server is still starting.
long openWithRetry(J2534RemoteClient &client, unsigned long &device)
{
    long status = ERR_DEVICE_NOT_CONNECTED;
    for (int i = 0; i < 100 && status == ERR_DEVICE_NOT_CONNECTED; ++i) {
        status = client.PassThruOpen(nullptr, &device);
        if (status == ERR_DEVICE_NOT_CONNECTED && lastError(client).find("refused") != std::string::npos)
            break;   // reachable but refused: retrying won't help
        if (status == ERR_DEVICE_NOT_CONNECTED)
            Sleep(150);
    }
    return status;
}

void runCalls(J2534RemoteClient &client)
{
    unsigned long device = 0, channel = 0;
    CHECK(openWithRetry(client, device) == STATUS_NOERROR);
    CHECK(device == 1);
    CHECK(client.PassThruConnect(device, 5, 0, 500000, &channel) == STATUS_NOERROR);
    CHECK(channel == 7);

    // Config
    SCONFIG set[] = {{1, 500000}};
    SCONFIG_LIST setList{1, set};
    CHECK(client.PassThruIoctl(channel, SET_CONFIG, &setList, nullptr) == STATUS_NOERROR);
    SCONFIG get[] = {{1, 0}, {3, 0}};
    SCONFIG_LIST getList{2, get};
    CHECK(client.PassThruIoctl(channel, GET_CONFIG, &getList, nullptr) == STATUS_NOERROR);
    CHECK(get[0].Value == 10 && get[1].Value == 30);

    unsigned long vbatt = 0;
    CHECK(client.PassThruIoctl(device, READ_VBATT, nullptr, &vbatt) == STATUS_NOERROR);
    CHECK(vbatt == 12345);

    unsigned char in[] = {0x33, 0x01}, out[8] = {};
    SBYTE_ARRAY inBytes{2, in}, outBytes{sizeof(out), out};
    CHECK(client.PassThruIoctl(channel, FIVE_BAUD_INIT, &inBytes, &outBytes) == STATUS_NOERROR);
    CHECK(outBytes.NumOfBytes == 2 && out[0] == 0x01 && out[1] == 0x33);

    PASSTHRU_MSG fastIn = canMsg(0x7E0, 0x81), fastOut{};
    CHECK(client.PassThruIoctl(channel, FAST_INIT, &fastIn, &fastOut) == STATUS_NOERROR);
    CHECK(fastOut.DataSize == 7 && fastOut.Data[0] == 0x40);

    CHECK(client.PassThruIoctl(channel, 0x8001, nullptr, nullptr) == ERR_NOT_SUPPORTED);

    // Filter without flow control
    PASSTHRU_MSG mask = canMsg(0xFFFF, 0), pattern = canMsg(0x7E8, 0);
    unsigned long filter = 0;
    CHECK(client.PassThruStartMsgFilter(channel, 1, &mask, &pattern, nullptr, &filter) == STATUS_NOERROR);
    CHECK(filter == 5);

    // Nothing received yet
    std::vector<PASSTHRU_MSG> rx(100);
    unsigned long count = 10;
    CHECK(client.PassThruReadMsgs(channel, rx.data(), &count, 0) == ERR_BUFFER_EMPTY);
    CHECK(count == 0);

    // 70 messages: more than one frame (64), looped back by the fake driver
    // and pushed back to us as RX.
    std::vector<PASSTHRU_MSG> tx;
    for (int i = 0; i < 70; ++i)
        tx.push_back(canMsg(0x7E0, static_cast<unsigned char>(i)));
    count = static_cast<unsigned long>(tx.size());
    CHECK(client.PassThruWriteMsgs(channel, tx.data(), &count, 100) == STATUS_NOERROR);
    CHECK(count == 70);

    count = 70;
    CHECK(client.PassThruReadMsgs(channel, rx.data(), &count, 5000) == STATUS_NOERROR);
    CHECK(count == 70);
    if (count == 70) {
        CHECK(rx[0].RxStatus == 0x01);
        CHECK(rx[0].Data[6] == 0 && rx[69].Data[6] == 69);
    }

    // Timeout with fewer messages than requested
    count = 5;
    CHECK(client.PassThruReadMsgs(channel, rx.data(), &count, 50) == ERR_TIMEOUT);
    CHECK(count == 0);

    char firmware[80], dll[80], api[80];
    CHECK(client.PassThruReadVersion(device, firmware, dll, api) == STATUS_NOERROR);
    CHECK(std::string(api) == "04.04");

    CHECK(client.PassThruScanForDevices(&count) == ERR_NOT_SUPPORTED);
    CHECK(lastError(client).find("not available over the remote link") != std::string::npos);

    CHECK(client.PassThruDisconnect(channel) == STATUS_NOERROR);
    CHECK(client.PassThruClose(device) == STATUS_NOERROR);
}

} // namespace

int wmain(int argc, wchar_t *argv[])
{
    if (argc < 4) {
        std::printf("usage: remote_e2e_test <app.exe> <j2534_device_host.exe> <fake_j2534_driver.dll>\n");
        return 2;
    }
    const std::wstring app = absolute(argv[1]);
    const std::wstring host = absolute(argv[2]);
    const std::wstring driver = absolute(argv[3]);
    if (GetFileAttributesW(app.c_str()) == INVALID_FILE_ATTRIBUTES) {
        std::printf("SKIPPED: App not found (build and install the x64 preset first)\n");
        return 77;
    }

    wchar_t logDir[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, logDir);
    SetEnvironmentVariableW(L"FAKE_J2534_LOG", (std::wstring(logDir) + L"fake_j2534_e2e.log").c_str());

    const int port = 41000 + static_cast<int>(GetCurrentProcessId() % 20000);
    const std::string secret = "E2E-SECRET-1234";

    std::wstring command = L"\"" + app + L"\" --vehicle-server --driver \"" + driver + L"\" --device-host \"" + host
                           + L"\" --port " + std::to_wstring(port) + L" --secret " + std::wstring(secret.begin(), secret.end())
                           + L" --adapter-name \"Fake adapter\"";
    STARTUPINFOW si{sizeof(si)};
    PROCESS_INFORMATION server{};
    if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &server)) {
        std::printf("FAIL: can't start the App (%lu)\n", GetLastError());
        return 1;
    }

    const std::string endpoint = "127.0.0.1:" + std::to_string(port);
    auto code = [](std::string value) { return [value] { return value; }; };

    {
        J2534RemoteClient a(code(endpoint + "#" + secret), "TEST-A");
        unsigned long device = 0;
        CHECK(openWithRetry(a, device) == STATUS_NOERROR);

        // Only one Diagnostic PC at a time.
        J2534RemoteClient busy(code(endpoint + "#" + secret), "TEST-BUSY");
        CHECK(busy.PassThruOpen(nullptr, &device) == ERR_DEVICE_NOT_CONNECTED);
        CHECK(lastError(busy).find("already connected") != std::string::npos);

        runCalls(a);
    }   // a disconnects here

    // The Vehicle PC accepts a new session after the previous one ended.
    {
        J2534RemoteClient again(code(endpoint + "#" + secret), "TEST-AGAIN");
        unsigned long device = 0;
        CHECK(openWithRetry(again, device) == STATUS_NOERROR);
        CHECK(again.PassThruClose(device) == STATUS_NOERROR);
    }

    // No code configured
    {
        J2534RemoteClient none(code(""), "TEST-NONE");
        unsigned long device = 0;
        CHECK(none.PassThruOpen(nullptr, &device) == ERR_DEVICE_NOT_CONNECTED);
        CHECK(lastError(none).find("No connection code") != std::string::npos);
    }

    // Wrong secrets: refused, and after three the Vehicle PC stops accepting
    // even the right code until sharing is restarted. Keep this last.
    for (int i = 0; i < 3; ++i) {
        J2534RemoteClient wrong(code(endpoint + "#WRONG-" + std::to_string(i)), "TEST-WRONG");
        unsigned long device = 0;
        CHECK(wrong.PassThruOpen(nullptr, &device) == ERR_DEVICE_NOT_CONNECTED);
        CHECK(lastError(wrong).find("secret") != std::string::npos);
    }
    {
        J2534RemoteClient locked(code(endpoint + "#" + secret), "TEST-LOCKED");
        unsigned long device = 0;
        CHECK(locked.PassThruOpen(nullptr, &device) == ERR_DEVICE_NOT_CONNECTED);
        CHECK(lastError(locked).find("rejected") != std::string::npos);
    }

    TerminateProcess(server.hProcess, 0);
    WaitForSingleObject(server.hProcess, 5000);
    CloseHandle(server.hProcess);
    CloseHandle(server.hThread);

    std::printf("%s (%d failure%s)\n", g_failures ? "FAILED" : "PASSED", g_failures, g_failures == 1 ? "" : "s");
    return g_failures ? 1 : 0;
}
