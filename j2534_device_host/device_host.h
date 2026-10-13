#ifndef DEVICE_HOST_H
#define DEVICE_HOST_H

#include <cstdint>
#include <functional>
#include <set>
#include <vector>

#include "driver_api.h"
#include "remote_frame.h"

// Executes remote J2534 requests on a local driver (Vehicle PC side).
// Not thread-safe: the host calls it from one thread only, so the driver never
// sees concurrent calls.
class DeviceHost
{
public:
    using RxSink = std::function<void(uint32_t channel, const PASSTHRU_MSG *msgs, size_t count)>;

    explicit DeviceHost(DriverApi &api);

    // Runs one call and fills `response` (id, status, error, result).
    void handle(const j2534_remote_Request &request, j2534_remote_Response &response);

    // Reads what the connected channels have received (timeout 0) and passes
    // it to `sink` in batches of at most remote::kMaxBatch. Returns true if
    // anything was read.
    bool pollRx(const RxSink &sink);

    // Link lost: disconnects channels and closes devices the remote side left
    // open, so nothing keeps transmitting to the vehicle.
    void shutdown();

    const std::set<unsigned long> &openDevices() const { return m_devices; }
    const std::set<unsigned long> &connectedChannels() const { return m_channels; }

private:
    void ioctl(const j2534_remote_IoctlRequest &request, j2534_remote_Response &response);

    DriverApi &m_api;
    std::set<unsigned long> m_devices;
    std::set<unsigned long> m_channels;
    std::vector<PASSTHRU_MSG> m_rx;     // ReadMsgs buffer, kMaxBatch messages
};

#endif // DEVICE_HOST_H
