// Drives j2534_device_host.exe the way the App will: creates the named pipe,
// starts the host with the fake driver, sends remote::Frame requests and checks
// the responses and pushed RX batches.
//
//   device_host_test.exe <j2534_device_host.exe> <fake_j2534_driver.dll>

#include <windows.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "pipe_frames.h"
#include "remote_frame.h"

namespace {

int g_failures = 0;

#define CHECK(cond)                                                                    \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                \
            ++g_failures;                                                              \
        }                                                                              \
    } while (0)

std::wstring tempPath(const wchar_t *name)
{
    wchar_t dir[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, dir);
    return std::wstring(dir) + name;
}

std::string readFile(const std::wstring &path)
{
    std::ifstream file(path);
    std::stringstream text;
    text << file.rdbuf();
    return text.str();
}

class Session
{
public:
    ~Session() { stop(); }

    bool start(const std::wstring &hostExe, const std::wstring &driver)
    {
        m_name = L"\\\\.\\pipe\\j2534_device_host_test_" + std::to_wstring(GetCurrentProcessId());
        m_pipe = CreateNamedPipeW(m_name.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                                  1, 1 << 20, 1 << 20, 0, nullptr);
        if (m_pipe == INVALID_HANDLE_VALUE)
            return false;

        std::wstring command = L"\"" + hostExe + L"\" --pipe \"" + m_name + L"\" --driver \"" + driver + L"\"";
        STARTUPINFOW si{sizeof(si)};
        if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &m_process))
            return false;

        if (!ConnectNamedPipe(m_pipe, nullptr) && GetLastError() != ERROR_PIPE_CONNECTED)
            return false;
        return true;
    }

    // Sends a request and waits for its response; RX batches arriving in the
    // meantime are collected in `rx`.
    std::unique_ptr<remote::Frame> call(remote::Frame &request)
    {
        request.which_kind = j2534_remote_Frame_request_tag;
        request.kind.request.id = ++m_nextId;
        if (!pipeframes::write(m_pipe, remote::encode(request)))
            return nullptr;

        auto frame = remote::newFrame();
        std::vector<uint8_t> bytes;
        for (;;) {
            if (pipeframes::read(m_pipe, bytes, 5000) != pipeframes::ReadResult::Frame)
                return nullptr;
            if (!remote::decode(bytes.data(), bytes.size(), *frame))
                return nullptr;
            if (frame->which_kind == j2534_remote_Frame_rx_tag)
                collect(*frame);
            else if (frame->which_kind == j2534_remote_Frame_response_tag && frame->kind.response.id == m_nextId)
                return frame;
        }
    }

    // Waits until at least `count` RX messages have arrived.
    bool waitRx(size_t count, DWORD timeoutMs)
    {
        auto frame = remote::newFrame();
        std::vector<uint8_t> bytes;
        const ULONGLONG deadline = GetTickCount64() + timeoutMs;
        while (rx.size() < count && GetTickCount64() < deadline) {
            if (pipeframes::read(m_pipe, bytes, 50) == pipeframes::ReadResult::Frame
                && remote::decode(bytes.data(), bytes.size(), *frame)
                && frame->which_kind == j2534_remote_Frame_rx_tag)
                collect(*frame);
        }
        return rx.size() >= count;
    }

    // Drops the link without Disconnect/Close and returns the host's exit code.
    DWORD dropLink()
    {
        CloseHandle(m_pipe);
        m_pipe = INVALID_HANDLE_VALUE;
        DWORD code = 0xFFFFFFFF;
        if (WaitForSingleObject(m_process.hProcess, 5000) == WAIT_OBJECT_0)
            GetExitCodeProcess(m_process.hProcess, &code);
        return code;
    }

    std::vector<PASSTHRU_MSG> rx;
    std::vector<uint32_t> rxChannels;

private:
    void collect(const remote::Frame &frame)
    {
        for (pb_size_t i = 0; i < frame.kind.rx.msgs_count; ++i) {
            PASSTHRU_MSG msg;
            remote::fromWire(frame.kind.rx.msgs[i], msg);
            rx.push_back(msg);
            rxChannels.push_back(frame.kind.rx.channel_id);
        }
    }

    void stop()
    {
        if (m_pipe != INVALID_HANDLE_VALUE)
            CloseHandle(m_pipe);
        if (m_process.hProcess) {
            if (WaitForSingleObject(m_process.hProcess, 3000) != WAIT_OBJECT_0)
                TerminateProcess(m_process.hProcess, 1);
            CloseHandle(m_process.hProcess);
            CloseHandle(m_process.hThread);
        }
    }

    std::wstring m_name;
    HANDLE m_pipe = INVALID_HANDLE_VALUE;
    PROCESS_INFORMATION m_process{};
    uint32_t m_nextId = 0;
};

remote::Frame &request(std::unique_ptr<remote::Frame> &frame, pb_size_t tag)
{
    frame = remote::newFrame();
    frame->kind.request.which_call = tag;
    return *frame;
}

PASSTHRU_MSG canMsg(uint32_t id, std::initializer_list<unsigned char> payload)
{
    PASSTHRU_MSG msg{};
    msg.ProtocolID = 5;   // CAN
    msg.Data[0] = static_cast<unsigned char>(id >> 24);
    msg.Data[1] = static_cast<unsigned char>(id >> 16);
    msg.Data[2] = static_cast<unsigned char>(id >> 8);
    msg.Data[3] = static_cast<unsigned char>(id);
    size_t i = 4;
    for (auto b : payload)
        msg.Data[i++] = b;
    msg.DataSize = static_cast<unsigned long>(i);
    return msg;
}

void testCalls(const std::wstring &hostExe, const std::wstring &driver, const std::wstring &logPath)
{
    Session s;
    CHECK(s.start(hostExe, driver));
    std::unique_ptr<remote::Frame> req;

    // Open / Connect
    auto &open = request(req, j2534_remote_Request_open_tag);
    remote::copyString(open.kind.request.call.open.name, "J2534-1:127.0.0.1");
    auto resp = s.call(open);
    CHECK(resp && resp->kind.response.status == 0);
    CHECK(resp && resp->kind.response.result.open.device_id == 1);

    auto &connect = request(req, j2534_remote_Request_connect_tag);
    connect.kind.request.call.connect = {1, 5, 0, 500000};
    resp = s.call(connect);
    CHECK(resp && resp->kind.response.status == 0 && resp->kind.response.result.connect.channel_id == 7);

    // IOCTLs
    auto &setConfig = request(req, j2534_remote_Request_ioctl_tag);
    auto &set = setConfig.kind.request.call.ioctl;
    set.channel_id = 7;
    set.ioctl_id = 0x02;
    set.config_count = 1;
    set.config[0] = {1, 500000};
    resp = s.call(setConfig);
    CHECK(resp && resp->kind.response.status == 0);

    auto &getConfig = request(req, j2534_remote_Request_ioctl_tag);
    auto &get = getConfig.kind.request.call.ioctl;
    get.channel_id = 7;
    get.ioctl_id = 0x01;
    get.config_count = 2;
    get.config[0] = {1, 0};
    get.config[1] = {2, 0};
    resp = s.call(getConfig);
    CHECK(resp && resp->kind.response.result.ioctl.config_count == 2);
    CHECK(resp && resp->kind.response.result.ioctl.config[0].value == 10);
    CHECK(resp && resp->kind.response.result.ioctl.config[1].value == 20);

    auto &vbatt = request(req, j2534_remote_Request_ioctl_tag);
    vbatt.kind.request.call.ioctl = {};
    vbatt.kind.request.call.ioctl.channel_id = 1;
    vbatt.kind.request.call.ioctl.ioctl_id = 0x03;
    resp = s.call(vbatt);
    CHECK(resp && resp->kind.response.result.ioctl.output_value == 12345);

    auto &fiveBaud = request(req, j2534_remote_Request_ioctl_tag);
    auto &fb = fiveBaud.kind.request.call.ioctl;
    fb.channel_id = 7;
    fb.ioctl_id = 0x04;
    fb.input_bytes.size = 3;
    fb.input_bytes.bytes[0] = 1; fb.input_bytes.bytes[1] = 2; fb.input_bytes.bytes[2] = 3;
    resp = s.call(fiveBaud);
    CHECK(resp && resp->kind.response.result.ioctl.output_bytes.size == 3);
    CHECK(resp && resp->kind.response.result.ioctl.output_bytes.bytes[0] == 3);
    CHECK(resp && resp->kind.response.result.ioctl.output_bytes.bytes[2] == 1);

    auto &fastInit = request(req, j2534_remote_Request_ioctl_tag);
    auto &fi = fastInit.kind.request.call.ioctl;
    fi.channel_id = 7;
    fi.ioctl_id = 0x05;
    fi.has_input_msg = true;
    remote::toWire(canMsg(0x7E0, {0x81, 0x10}), fi.input_msg);
    resp = s.call(fastInit);
    CHECK(resp && resp->kind.response.result.ioctl.has_output_msg);
    CHECK(resp && resp->kind.response.result.ioctl.output_msg.data.size == 6);
    CHECK(resp && resp->kind.response.result.ioctl.output_msg.data.bytes[0] == 0x40);   // 0x00 | 0x40

    auto &unsupported = request(req, j2534_remote_Request_ioctl_tag);
    unsupported.kind.request.call.ioctl = {};
    unsupported.kind.request.call.ioctl.ioctl_id = 0x99;
    resp = s.call(unsupported);
    CHECK(resp && resp->kind.response.status == 0x01);   // ERR_NOT_SUPPORTED
    CHECK(resp && std::string(resp->kind.response.error).find("not supported") != std::string::npos);

    // Filter without flow control
    auto &filter = request(req, j2534_remote_Request_start_msg_filter_tag);
    auto &f = filter.kind.request.call.start_msg_filter;
    f.channel_id = 7;
    f.filter_type = 1;
    f.has_mask = f.has_pattern = true;
    remote::toWire(canMsg(0xFFFFFFFF, {}), f.mask);
    remote::toWire(canMsg(0x7E8, {}), f.pattern);
    resp = s.call(filter);
    CHECK(resp && resp->kind.response.result.start_msg_filter.filter_id == 5);

    // WriteMsgs loops back in the fake driver; the host must push it as RX.
    auto &write = request(req, j2534_remote_Request_write_msgs_tag);
    auto &w = write.kind.request.call.write_msgs;
    w.channel_id = 7;
    w.timeout = 100;
    w.msgs_count = 3;
    for (int i = 0; i < 3; ++i)
        remote::toWire(canMsg(0x7E0, {0x02, 0x10, static_cast<unsigned char>(0x81 + i)}), w.msgs[i]);
    resp = s.call(write);
    CHECK(resp && resp->kind.response.status == 0 && resp->kind.response.result.write_msgs.num_msgs_written == 3);
    CHECK(s.waitRx(3, 2000));
    CHECK(s.rx.size() == 3);
    if (s.rx.size() == 3) {
        CHECK(s.rxChannels[0] == 7);
        CHECK(s.rx[0].RxStatus == 0x01);
        CHECK(s.rx[0].DataSize == 7);
        CHECK(s.rx[2].Data[6] == 0x83);
    }

    // Strings
    auto &version = request(req, j2534_remote_Request_read_version_tag);
    version.kind.request.call.read_version.device_id = 1;
    resp = s.call(version);
    CHECK(resp && std::string(resp->kind.response.result.read_version.api_version) == "04.04");
    CHECK(resp && std::string(resp->kind.response.result.read_version.dll_version) == "Fake DLL 3.4");

    auto &lastError = request(req, j2534_remote_Request_get_last_error_tag);
    resp = s.call(lastError);
    CHECK(resp && std::string(resp->kind.response.result.get_last_error.error_description) == "fake driver error");

    // Losing the link must stop traffic to the vehicle.
    CHECK(s.dropLink() == 0);
    const std::string log = readFile(logPath);
    CHECK(log.find("Open J2534-1:127.0.0.1") != std::string::npos);
    CHECK(log.find("StartMsgFilter 7 1 mask=1 pattern=1 flow=0") != std::string::npos);
    CHECK(log.find("Disconnect 7") != std::string::npos);
    CHECK(log.find("Close 1") != std::string::npos);
}

void testBadDriver(const std::wstring &hostExe)
{
    Session s;
    CHECK(s.start(hostExe, L"C:\\does\\not\\exist\\driver.dll"));
    // The host reports why and exits with code 3.
    CHECK(s.dropLink() == 3);
}

} // namespace

int wmain(int argc, wchar_t *argv[])
{
    if (argc < 3) {
        std::printf("usage: device_host_test <j2534_device_host.exe> <fake_j2534_driver.dll>\n");
        return 2;
    }

    const std::wstring logPath = tempPath(L"fake_j2534_driver_test.log");
    DeleteFileW(logPath.c_str());
    SetEnvironmentVariableW(L"FAKE_J2534_LOG", logPath.c_str());

    // CreateProcess and LoadLibrary need absolute paths here.
    auto absolute = [](const wchar_t *path) {
        wchar_t full[MAX_PATH] = {};
        GetFullPathNameW(path, MAX_PATH, full, nullptr);
        return std::wstring(full);
    };
    const std::wstring hostExe = absolute(argv[1]);
    const std::wstring driver = absolute(argv[2]);

    testCalls(hostExe, driver, logPath);
    testBadDriver(hostExe);

    std::printf("%s (%d failure%s)\n", g_failures ? "FAILED" : "PASSED", g_failures, g_failures == 1 ? "" : "s");
    return g_failures ? 1 : 0;
}
