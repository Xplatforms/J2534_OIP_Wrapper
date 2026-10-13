// j2534_device_host.exe - Vehicle PC side of the remote J2534 link.
//
// 32-bit and Qt-free so it can load ordinary (32-bit) J2534 drivers. The App
// (Vehicle mode) creates the named pipe, starts this process and relays
// frames between the pipe and the WebSocket.
//
//   j2534_device_host.exe --pipe \\.\pipe\<name> --driver C:\path\driver.dll
//
// The first frame the host sends is a Response with id 0: status 0 when the
// driver is loaded and calls can be made, otherwise the error.
//
// Exit codes: 0 link closed normally, 2 bad arguments, 3 driver not loadable,
// 4 couldn't connect to the pipe.

#include <windows.h>
#include <timeapi.h>

#include <cstdio>
#include <string>

#include "device_host.h"
#include "j2534_defs.h"
#include "pipe_frames.h"

namespace {

std::string narrow(const std::wstring &text)
{
    if (text.empty())
        return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string out(size > 0 ? size - 1 : 0, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, out.data(), size, nullptr, nullptr);
    return out;
}

HANDLE connectPipe(const std::wstring &name)
{
    for (int attempt = 0; attempt < 100; ++attempt) {
        HANDLE pipe = CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (pipe != INVALID_HANDLE_VALUE)
            return pipe;
        if (GetLastError() == ERROR_PIPE_BUSY)
            WaitNamedPipeW(name.c_str(), 100);
        else
            Sleep(50);
    }
    return INVALID_HANDLE_VALUE;
}

void sendFrame(HANDLE pipe, const remote::Frame &frame)
{
    const auto bytes = remote::encode(frame);
    if (!bytes.empty())
        pipeframes::write(pipe, bytes);
}

} // namespace

int wmain(int argc, wchar_t *argv[])
{
    std::wstring pipeName, driverPath;
    for (int i = 1; i + 1 < argc; ++i) {
        const std::wstring arg = argv[i];
        if (arg == L"--pipe") pipeName = argv[++i];
        else if (arg == L"--driver") driverPath = argv[++i];
    }
    if (pipeName.empty() || driverPath.empty()) {
        std::fprintf(stderr, "usage: j2534_device_host --pipe <name> --driver <dll>\n");
        return 2;
    }

    HANDLE pipe = connectPipe(pipeName);
    if (pipe == INVALID_HANDLE_VALUE) {
        std::fprintf(stderr, "can't connect to %s\n", narrow(pipeName).c_str());
        return 4;
    }

    DriverApi api;
    std::string error;
    if (!api.load(driverPath, &error)) {
        // Tell the App why, then quit.
        auto frame = remote::newFrame();
        frame->which_kind = j2534_remote_Frame_response_tag;
        frame->kind.response.status = ERR_NOT_SUPPORTED;
        remote::copyString(frame->kind.response.error, error);
        sendFrame(pipe, *frame);
        CloseHandle(pipe);
        return 3;
    }

    // Ready: a Response with id 0 and status 0. (On a load failure above, the
    // id-0 response carries the error instead.)
    {
        auto ready = remote::newFrame();
        ready->which_kind = j2534_remote_Frame_response_tag;
        sendFrame(pipe, *ready);
    }

    timeBeginPeriod(1);   // 1 ms Sleep granularity for the poll loop

    DeviceHost host(api);
    auto in = remote::newFrame();
    auto out = remote::newFrame();
    std::vector<uint8_t> bytes;

    const DeviceHost::RxSink sendRx = [&](uint32_t channel, const PASSTHRU_MSG *msgs, size_t count) {
        std::memset(out.get(), 0, sizeof(*out));
        out->which_kind = j2534_remote_Frame_rx_tag;
        auto &rx = out->kind.rx;
        rx.channel_id = channel;
        rx.msgs_count = static_cast<pb_size_t>(count);
        for (size_t i = 0; i < count; ++i)
            remote::toWire(msgs[i], rx.msgs[i]);
        sendFrame(pipe, *out);
    };

    bool running = true;
    while (running) {
        bool busy = false;

        // Requests first, so writes and IOCTLs are not delayed by RX polling.
        for (;;) {
            const auto result = pipeframes::tryRead(pipe, bytes);
            if (result == pipeframes::ReadResult::Closed) {
                running = false;
                break;
            }
            if (result == pipeframes::ReadResult::NoData)
                break;

            busy = true;
            if (!remote::decode(bytes.data(), bytes.size(), *in) || in->which_kind != j2534_remote_Frame_request_tag)
                continue;

            std::memset(out.get(), 0, sizeof(*out));
            out->which_kind = j2534_remote_Frame_response_tag;
            host.handle(in->kind.request, out->kind.response);
            sendFrame(pipe, *out);
        }
        if (!running)
            break;

        busy |= host.pollRx(sendRx);
        if (!busy)
            Sleep(1);
    }

    // Link lost or closed: don't leave channels or periodic messages running.
    host.shutdown();
    timeEndPeriod(1);
    CloseHandle(pipe);
    return 0;
}
