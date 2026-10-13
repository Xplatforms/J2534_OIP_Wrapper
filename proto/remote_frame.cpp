#include "remote_frame.h"

#include <algorithm>

#include <pb_decode.h>
#include <pb_encode.h>

namespace remote
{

std::unique_ptr<Frame> newFrame()
{
    return std::make_unique<Frame>();   // value-initialised: all zero
}

std::vector<uint8_t> encode(const Frame &frame)
{
    size_t size = 0;
    if (!pb_get_encoded_size(&size, j2534_remote_Frame_fields, &frame))
        return {};

    std::vector<uint8_t> buffer(size);
    pb_ostream_t stream = pb_ostream_from_buffer(buffer.data(), buffer.size());
    if (!pb_encode(&stream, j2534_remote_Frame_fields, &frame))
        return {};
    buffer.resize(stream.bytes_written);
    return buffer;
}

bool decode(const uint8_t *data, size_t size, Frame &frame)
{
    std::memset(&frame, 0, sizeof(frame));   // not `= Frame{}`: no 270 KB stack temporary
    pb_istream_t stream = pb_istream_from_buffer(data, size);
    return pb_decode(&stream, j2534_remote_Frame_fields, &frame);
}

void toWire(const PASSTHRU_MSG &in, j2534_remote_PassThruMsg &out)
{
    std::memset(&out, 0, sizeof(out));
    out.protocol_id = in.ProtocolID;
    out.rx_status = in.RxStatus;
    out.tx_flags = in.TxFlags;
    out.timestamp = in.Timestamp;
    out.extra_data_index = in.ExtraDataIndex;
    const size_t size = std::min<size_t>(in.DataSize, sizeof(out.data.bytes));
    out.data.size = static_cast<pb_size_t>(size);
    std::memcpy(out.data.bytes, in.Data, size);
}

void fromWire(const j2534_remote_PassThruMsg &in, PASSTHRU_MSG &out)
{
    std::memset(&out, 0, sizeof(out));
    out.ProtocolID = in.protocol_id;
    out.RxStatus = in.rx_status;
    out.TxFlags = in.tx_flags;
    out.Timestamp = in.timestamp;
    out.ExtraDataIndex = in.extra_data_index;
    const size_t size = std::min<size_t>(in.data.size, sizeof(out.Data));
    out.DataSize = static_cast<unsigned long>(size);
    std::memcpy(out.Data, in.data.bytes, size);
}

} // namespace remote
