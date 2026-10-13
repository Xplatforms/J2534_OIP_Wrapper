#include "device_host.h"

#include <algorithm>
#include <cstdio>
#include <string>

#include "j2534_defs.h"

namespace {

void fail(j2534_remote_Response &response, long status, const std::string &error)
{
    response.status = static_cast<int32_t>(status);
    remote::copyString(response.error, error);
}

// PASSTHRU_MSG pointer for an optional wire message.
PASSTHRU_MSG *optionalMsg(bool has, const j2534_remote_PassThruMsg &in, PASSTHRU_MSG &storage)
{
    if (!has)
        return nullptr;
    remote::fromWire(in, storage);
    return &storage;
}

} // namespace

DeviceHost::DeviceHost(DriverApi &api)
    : m_api(api)
    , m_rx(remote::kMaxBatch)
{
}

void DeviceHost::handle(const j2534_remote_Request &request, j2534_remote_Response &response)
{
    std::memset(&response, 0, sizeof(response));
    response.id = request.id;

    // Shared scratch for message arguments; PASSTHRU_MSG is 4 KB each.
    std::vector<PASSTHRU_MSG> msgs;

    switch (request.which_call) {
    case j2534_remote_Request_open_tag: {
        const auto &call = request.call.open;
        unsigned long deviceId = 0;
        const void *name = call.name[0] ? call.name : nullptr;
        response.status = m_api.Open(name, &deviceId);
        if (response.status == STATUS_NOERROR)
            m_devices.insert(deviceId);
        response.which_result = j2534_remote_Response_open_tag;
        response.result.open.device_id = deviceId;
        break;
    }
    case j2534_remote_Request_close_tag:
        response.status = m_api.Close(request.call.close.device_id);
        if (response.status == STATUS_NOERROR)
            m_devices.erase(request.call.close.device_id);
        break;

    case j2534_remote_Request_connect_tag: {
        const auto &call = request.call.connect;
        unsigned long channelId = 0;
        response.status = m_api.Connect(call.device_id, call.protocol_id, call.flags, call.baudrate, &channelId);
        if (response.status == STATUS_NOERROR)
            m_channels.insert(channelId);
        response.which_result = j2534_remote_Response_connect_tag;
        response.result.connect.channel_id = channelId;
        break;
    }
    case j2534_remote_Request_disconnect_tag:
        response.status = m_api.Disconnect(request.call.disconnect.channel_id);
        if (response.status == STATUS_NOERROR)
            m_channels.erase(request.call.disconnect.channel_id);
        break;

    case j2534_remote_Request_write_msgs_tag: {
        const auto &call = request.call.write_msgs;
        msgs.resize(call.msgs_count);
        for (pb_size_t i = 0; i < call.msgs_count; ++i)
            remote::fromWire(call.msgs[i], msgs[i]);
        unsigned long count = call.msgs_count;
        response.status = m_api.WriteMsgs(call.channel_id, msgs.data(), &count, call.timeout);
        response.which_result = j2534_remote_Response_write_msgs_tag;
        response.result.write_msgs.num_msgs_written = count;
        break;
    }
    case j2534_remote_Request_start_periodic_msg_tag: {
        if (!m_api.StartPeriodicMsg) { fail(response, ERR_NOT_SUPPORTED, "PassThruStartPeriodicMsg is not exported by the driver"); break; }
        const auto &call = request.call.start_periodic_msg;
        msgs.resize(1);
        unsigned long msgId = 0;
        response.status = m_api.StartPeriodicMsg(call.channel_id, optionalMsg(call.has_msg, call.msg, msgs[0]),
                                                 &msgId, call.time_interval);
        response.which_result = j2534_remote_Response_start_periodic_msg_tag;
        response.result.start_periodic_msg.msg_id = msgId;
        break;
    }
    case j2534_remote_Request_stop_periodic_msg_tag:
        if (!m_api.StopPeriodicMsg) { fail(response, ERR_NOT_SUPPORTED, "PassThruStopPeriodicMsg is not exported by the driver"); break; }
        response.status = m_api.StopPeriodicMsg(request.call.stop_periodic_msg.channel_id,
                                                request.call.stop_periodic_msg.msg_id);
        break;

    case j2534_remote_Request_start_msg_filter_tag: {
        if (!m_api.StartMsgFilter) { fail(response, ERR_NOT_SUPPORTED, "PassThruStartMsgFilter is not exported by the driver"); break; }
        const auto &call = request.call.start_msg_filter;
        msgs.resize(3);
        unsigned long filterId = 0;
        response.status = m_api.StartMsgFilter(call.channel_id, call.filter_type,
                                               optionalMsg(call.has_mask, call.mask, msgs[0]),
                                               optionalMsg(call.has_pattern, call.pattern, msgs[1]),
                                               optionalMsg(call.has_flow_control, call.flow_control, msgs[2]),
                                               &filterId);
        response.which_result = j2534_remote_Response_start_msg_filter_tag;
        response.result.start_msg_filter.filter_id = filterId;
        break;
    }
    case j2534_remote_Request_stop_msg_filter_tag:
        if (!m_api.StopMsgFilter) { fail(response, ERR_NOT_SUPPORTED, "PassThruStopMsgFilter is not exported by the driver"); break; }
        response.status = m_api.StopMsgFilter(request.call.stop_msg_filter.channel_id,
                                              request.call.stop_msg_filter.filter_id);
        break;

    case j2534_remote_Request_set_programming_voltage_tag: {
        if (!m_api.SetProgrammingVoltage) { fail(response, ERR_NOT_SUPPORTED, "PassThruSetProgrammingVoltage is not exported by the driver"); break; }
        const auto &call = request.call.set_programming_voltage;
        response.status = m_api.SetProgrammingVoltage(call.device_id, call.pin, call.voltage);
        break;
    }
    case j2534_remote_Request_read_version_tag: {
        if (!m_api.ReadVersion) { fail(response, ERR_NOT_SUPPORTED, "PassThruReadVersion is not exported by the driver"); break; }
        char firmware[80] = {}, dll[80] = {}, api[80] = {};
        response.status = m_api.ReadVersion(request.call.read_version.device_id, firmware, dll, api);
        firmware[79] = dll[79] = api[79] = '\0';
        response.which_result = j2534_remote_Response_read_version_tag;
        remote::copyString(response.result.read_version.firmware_version, firmware);
        remote::copyString(response.result.read_version.dll_version, dll);
        remote::copyString(response.result.read_version.api_version, api);
        break;
    }
    case j2534_remote_Request_get_last_error_tag: {
        if (!m_api.GetLastError) { fail(response, ERR_NOT_SUPPORTED, "PassThruGetLastError is not exported by the driver"); break; }
        char description[80] = {};
        response.status = m_api.GetLastError(description);
        description[79] = '\0';
        response.which_result = j2534_remote_Response_get_last_error_tag;
        remote::copyString(response.result.get_last_error.error_description, description);
        break;
    }
    case j2534_remote_Request_ioctl_tag:
        ioctl(request.call.ioctl, response);
        break;

    default:
        fail(response, ERR_NOT_SUPPORTED, "Unknown request");
        break;
    }
}

void DeviceHost::ioctl(const j2534_remote_IoctlRequest &request, j2534_remote_Response &response)
{
    if (!m_api.Ioctl) {
        fail(response, ERR_NOT_SUPPORTED, "PassThruIoctl is not exported by the driver");
        return;
    }

    response.which_result = j2534_remote_Response_ioctl_tag;
    auto &result = response.result.ioctl;
    const unsigned long channel = request.channel_id;

    switch (request.ioctl_id) {
    case GET_CONFIG:
    case SET_CONFIG: {
        std::vector<SCONFIG> params(request.config_count);
        for (pb_size_t i = 0; i < request.config_count; ++i)
            params[i] = SCONFIG{request.config[i].parameter, request.config[i].value};
        SCONFIG_LIST list{static_cast<unsigned long>(params.size()), params.data()};
        response.status = m_api.Ioctl(channel, request.ioctl_id, &list, nullptr);
        if (request.ioctl_id == GET_CONFIG) {
            result.config_count = request.config_count;
            for (pb_size_t i = 0; i < request.config_count; ++i)
                result.config[i] = j2534_remote_ConfigParam{params[i].Parameter, params[i].Value};
        }
        break;
    }
    case READ_VBATT:
    case READ_PROG_VOLTAGE: {
        unsigned long value = 0;
        response.status = m_api.Ioctl(channel, request.ioctl_id, nullptr, &value);
        result.output_value = value;
        break;
    }
    case FIVE_BAUD_INIT: {
        unsigned char input[sizeof(request.input_bytes.bytes)] = {};
        unsigned char output[sizeof(result.output_bytes.bytes)] = {};
        std::memcpy(input, request.input_bytes.bytes, request.input_bytes.size);
        SBYTE_ARRAY in{request.input_bytes.size, input};
        SBYTE_ARRAY out{sizeof(output), output};
        response.status = m_api.Ioctl(channel, request.ioctl_id, &in, &out);
        const size_t size = std::min<size_t>(out.NumOfBytes, sizeof(output));
        result.output_bytes.size = static_cast<pb_size_t>(size);
        std::memcpy(result.output_bytes.bytes, output, size);
        break;
    }
    case FAST_INIT: {
        std::vector<PASSTHRU_MSG> msgs(2);
        remote::fromWire(request.input_msg, msgs[0]);
        response.status = m_api.Ioctl(channel, request.ioctl_id, &msgs[0], &msgs[1]);
        result.has_output_msg = true;
        remote::toWire(msgs[1], result.output_msg);
        break;
    }
    case ADD_TO_FUNCT_MSG_LOOKUP_TABLE:
    case DELETE_FROM_FUNCT_MSG_LOOKUP_TABLE: {
        unsigned char input[sizeof(request.input_bytes.bytes)] = {};
        std::memcpy(input, request.input_bytes.bytes, request.input_bytes.size);
        SBYTE_ARRAY in{request.input_bytes.size, input};
        response.status = m_api.Ioctl(channel, request.ioctl_id, &in, nullptr);
        break;
    }
    case CLEAR_TX_BUFFER:
    case CLEAR_RX_BUFFER:
    case CLEAR_PERIODIC_MSGS:
    case CLEAR_MSG_FILTERS:
    case CLEAR_FUNCT_MSG_LOOKUP_TABLE:
        response.status = m_api.Ioctl(channel, request.ioctl_id, nullptr, nullptr);
        break;

    default: {
        char text[96];
        std::snprintf(text, sizeof(text), "IOCTL 0x%02lx is not supported over the remote link",
                      static_cast<unsigned long>(request.ioctl_id));
        response.which_result = 0;
        fail(response, ERR_NOT_SUPPORTED, text);
        break;
    }
    }
}

bool DeviceHost::pollRx(const RxSink &sink)
{
    bool any = false;
    for (unsigned long channel : m_channels) {
        unsigned long count = static_cast<unsigned long>(m_rx.size());
        m_api.ReadMsgs(channel, m_rx.data(), &count, 0);
        // Drivers report ERR_BUFFER_EMPTY / ERR_TIMEOUT with whatever they had;
        // only the count matters here.
        count = std::min<unsigned long>(count, static_cast<unsigned long>(m_rx.size()));
        if (count > 0) {
            sink(channel, m_rx.data(), count);
            any = true;
        }
    }
    return any;
}

void DeviceHost::shutdown()
{
    for (unsigned long channel : m_channels)
        m_api.Disconnect(channel);
    m_channels.clear();
    for (unsigned long device : m_devices)
        m_api.Close(device);
    m_devices.clear();
}
