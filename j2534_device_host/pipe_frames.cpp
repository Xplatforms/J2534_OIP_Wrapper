#include "pipe_frames.h"

namespace pipeframes
{

namespace {

bool readExact(HANDLE pipe, void *buffer, DWORD size)
{
    auto *out = static_cast<uint8_t *>(buffer);
    while (size > 0) {
        DWORD read = 0;
        if (!ReadFile(pipe, out, size, &read, nullptr) || read == 0)
            return false;
        out += read;
        size -= read;
    }
    return true;
}

} // namespace

bool write(HANDLE pipe, const std::vector<uint8_t> &frame)
{
    const uint32_t size = static_cast<uint32_t>(frame.size());
    DWORD written = 0;
    if (!WriteFile(pipe, &size, sizeof(size), &written, nullptr) || written != sizeof(size))
        return false;
    if (size == 0)
        return true;
    return WriteFile(pipe, frame.data(), size, &written, nullptr) && written == size;
}

ReadResult tryRead(HANDLE pipe, std::vector<uint8_t> &frame)
{
    DWORD available = 0;
    if (!PeekNamedPipe(pipe, nullptr, 0, nullptr, &available, nullptr))
        return ReadResult::Closed;
    if (available < sizeof(uint32_t))
        return ReadResult::NoData;

    uint32_t size = 0;
    DWORD peeked = 0;
    if (!PeekNamedPipe(pipe, &size, sizeof(size), &peeked, &available, nullptr) || peeked != sizeof(size))
        return ReadResult::Closed;
    if (size > kMaxFrame)
        return ReadResult::Closed;   // corrupt stream; treat as a broken link
    if (available < sizeof(size) + size)
        return ReadResult::NoData;

    if (!readExact(pipe, &size, sizeof(size)))
        return ReadResult::Closed;
    frame.resize(size);
    if (size > 0 && !readExact(pipe, frame.data(), size))
        return ReadResult::Closed;
    return ReadResult::Frame;
}

ReadResult read(HANDLE pipe, std::vector<uint8_t> &frame, DWORD timeoutMs)
{
    const ULONGLONG deadline = GetTickCount64() + timeoutMs;
    for (;;) {
        const ReadResult result = tryRead(pipe, frame);
        if (result != ReadResult::NoData || GetTickCount64() >= deadline)
            return result;
        Sleep(1);
    }
}

} // namespace pipeframes
