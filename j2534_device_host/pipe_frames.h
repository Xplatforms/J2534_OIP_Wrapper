#ifndef PIPE_FRAMES_H
#define PIPE_FRAMES_H

#include <windows.h>
#include <cstdint>
#include <vector>

// Length-prefixed frames (little-endian int32 size, then the bytes) on a
// byte-mode named pipe, as used between the App and the device host.
namespace pipeframes
{

enum class ReadResult { Frame, NoData, Closed };

bool write(HANDLE pipe, const std::vector<uint8_t> &frame);

// Non-blocking: returns NoData unless a whole frame is already available.
ReadResult tryRead(HANDLE pipe, std::vector<uint8_t> &frame);

// Waits up to `timeoutMs` for a whole frame.
ReadResult read(HANDLE pipe, std::vector<uint8_t> &frame, DWORD timeoutMs);

constexpr uint32_t kMaxFrame = 1u << 20;   // 1 MB, far above the largest remote frame

} // namespace pipeframes

#endif // PIPE_FRAMES_H
