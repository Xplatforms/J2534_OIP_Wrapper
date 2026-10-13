#ifndef REMOTE_FRAME_H
#define REMOTE_FRAME_H

// Helpers for j2534.remote.Frame (see j2534_remote.proto). Qt-free; used by the
// PassThru DLL (J2534Remote interface), the x86 device host and the tests.
//
// Frames are ~270 KB because of nanopb's fixed-size arrays, so they are only
// ever created on the heap with newFrame().

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "j2534_remote.pb.h"
#include "PassThruStruct.h"

namespace remote
{

using Frame = j2534_remote_Frame;

std::unique_ptr<Frame> newFrame();

// Serialized frame; empty on failure.
std::vector<uint8_t> encode(const Frame &frame);
bool decode(const uint8_t *data, size_t size, Frame &frame);

// PASSTHRU_MSG <-> wire message. DataSize is clamped to 4128.
void toWire(const PASSTHRU_MSG &in, j2534_remote_PassThruMsg &out);
void fromWire(const j2534_remote_PassThruMsg &in, PASSTHRU_MSG &out);

// Copies a C string into a fixed nanopb char array, truncating and terminating.
template <size_t N>
void copyString(char (&dest)[N], const char *src)
{
    dest[0] = '\0';
    if (src)
        strncpy_s(dest, N, src, _TRUNCATE);
}

template <size_t N>
void copyString(char (&dest)[N], const std::string &src)
{
    copyString(dest, src.c_str());
}

// Largest batch a single frame carries (RxBatch, WriteMsgsRequest).
constexpr size_t kMaxBatch = sizeof(j2534_remote_RxBatch::msgs) / sizeof(j2534_remote_PassThruMsg);
constexpr size_t kMaxConfig = sizeof(j2534_remote_IoctlRequest::config) / sizeof(j2534_remote_ConfigParam);

} // namespace remote

#endif // REMOTE_FRAME_H
