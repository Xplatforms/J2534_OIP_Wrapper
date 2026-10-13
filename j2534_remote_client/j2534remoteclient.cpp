#include "j2534remoteclient.h"

#include <algorithm>
#include <vector>

#include "j2534_defs.h"

namespace {

std::unique_ptr<remote::Frame> newRequest(pb_size_t tag)
{
    auto frame = remote::newFrame();
    frame->kind.request.which_call = tag;
    return frame;
}

} // namespace

J2534RemoteClient::J2534RemoteClient(CodeProvider codeProvider, std::string clientName, Logger logger)
    : m_codeProvider(std::move(codeProvider))
    , m_clientName(std::move(clientName))
    , m_logger(std::move(logger))
{
}

J2534RemoteClient::~J2534RemoteClient()
{
    m_session.disconnect();
}

void J2534RemoteClient::log(const std::string &text)
{
    if (m_logger)
        m_logger("[J2534Remote] " + text);
}

long J2534RemoteClient::localError(long status, const std::string &message)
{
    std::lock_guard<std::mutex> lock(m_errorMutex);
    m_lastError = message;
    m_lastErrorIsLocal = true;
    return status;
}

long J2534RemoteClient::notSupported(const char *function)
{
    return localError(ERR_NOT_SUPPORTED, std::string(function) + " is not available over the remote link");
}

long J2534RemoteClient::ensureConnected()
{
    std::lock_guard<std::mutex> lock(m_connectMutex);
    if (m_session.connected())
        return STATUS_NOERROR;

    ConnectionCode endpoint;
    std::string error;
    if (!ConnectionCode::parse(m_codeProvider ? m_codeProvider() : std::string(), endpoint, &error))
        return localError(ERR_DEVICE_NOT_CONNECTED, error);

    log("connecting to " + endpoint.displayHost());
    if (!m_session.connect(endpoint, m_clientName, kHelloTimeoutMs, &error)) {
        log(error);
        return localError(ERR_DEVICE_NOT_CONNECTED, error);
    }
    log("connected to " + endpoint.displayHost() + ", adapter \"" + m_session.adapterName() + "\"");
    return STATUS_NOERROR;
}

std::unique_ptr<remote::Frame> J2534RemoteClient::run(remote::Frame &request, DWORD extraTimeoutMs, long &status)
{
    std::string error;
    auto response = m_session.call(request, kResponseTimeoutMs + extraTimeoutMs, &error);
    if (!response) {
        log(error);
        status = localError(ERR_DEVICE_NOT_CONNECTED, error);
        return nullptr;
    }
    status = finish(*response);
    return response;
}

long J2534RemoteClient::finish(const remote::Frame &response)
{
    const auto &r = response.kind.response;
    std::lock_guard<std::mutex> lock(m_errorMutex);
    // Remote-side failures without a driver call (e.g. unsupported IOCTL) carry text.
    m_lastErrorIsLocal = r.error[0] != '\0';
    m_lastError = r.error;
    return r.status;
}

long J2534RemoteClient::PassThruOpen(const void *pName, unsigned long *pDeviceID)
{
    if (!pDeviceID)
        return localError(ERR_NULL_PARAMETER, "pDeviceID is NULL");
    if (const long status = ensureConnected(); status != STATUS_NOERROR)
        return status;

    auto request = newRequest(j2534_remote_Request_open_tag);
    remote::copyString(request->kind.request.call.open.name, static_cast<const char *>(pName));
    long status;
    if (auto response = run(*request, 0, status))
        *pDeviceID = response->kind.response.result.open.device_id;
    return status;
}

long J2534RemoteClient::PassThruClose(unsigned long DeviceID)
{
    auto request = newRequest(j2534_remote_Request_close_tag);
    request->kind.request.call.close.device_id = DeviceID;
    long status;
    run(*request, 0, status);
    return status;
}

long J2534RemoteClient::PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID, unsigned long Flags,
                                        unsigned long Baudrate, unsigned long *pChannelID)
{
    if (!pChannelID)
        return localError(ERR_NULL_PARAMETER, "pChannelID is NULL");
    auto request = newRequest(j2534_remote_Request_connect_tag);
    request->kind.request.call.connect = {DeviceID, ProtocolID, Flags, Baudrate};
    long status;
    if (auto response = run(*request, 0, status)) {
        *pChannelID = response->kind.response.result.connect.channel_id;
        if (status == STATUS_NOERROR)
            m_session.clearRx(*pChannelID);
    }
    return status;
}

long J2534RemoteClient::PassThruDisconnect(unsigned long ChannelID)
{
    auto request = newRequest(j2534_remote_Request_disconnect_tag);
    request->kind.request.call.disconnect.channel_id = ChannelID;
    long status;
    run(*request, 0, status);
    m_session.clearRx(ChannelID);
    return status;
}

long J2534RemoteClient::PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs,
                                         unsigned long Timeout)
{
    if (!pMsg || !pNumMsgs)
        return localError(ERR_NULL_PARAMETER, "pMsg or pNumMsgs is NULL");
    if (!m_session.connected()) {
        *pNumMsgs = 0;
        return localError(ERR_DEVICE_NOT_CONNECTED, "Not connected to the Vehicle PC");
    }

    const unsigned long wanted = *pNumMsgs;
    const size_t got = m_session.readRx(ChannelID, pMsg, wanted, Timeout);
    *pNumMsgs = static_cast<unsigned long>(got);
    if (got == 0)
        return Timeout == 0 ? ERR_BUFFER_EMPTY : ERR_TIMEOUT;
    if (got < wanted && Timeout != 0)
        return ERR_TIMEOUT;
    return STATUS_NOERROR;
}

long J2534RemoteClient::PassThruWriteMsgs(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs,
                                          unsigned long Timeout)
{
    if (!pMsg || !pNumMsgs)
        return localError(ERR_NULL_PARAMETER, "pMsg or pNumMsgs is NULL");

    const unsigned long total = *pNumMsgs;
    unsigned long written = 0;
    long status = STATUS_NOERROR;
    auto request = remote::newFrame();

    // Frames carry at most remote::kMaxBatch messages.
    while (written < total) {
        const unsigned long batch = std::min<unsigned long>(total - written, static_cast<unsigned long>(remote::kMaxBatch));
        std::memset(request.get(), 0, sizeof(*request));
        request->kind.request.which_call = j2534_remote_Request_write_msgs_tag;
        auto &call = request->kind.request.call.write_msgs;
        call.channel_id = ChannelID;
        call.timeout = Timeout;
        call.msgs_count = static_cast<pb_size_t>(batch);
        for (unsigned long i = 0; i < batch; ++i)
            remote::toWire(pMsg[written + i], call.msgs[i]);

        auto response = run(*request, Timeout, status);
        const unsigned long sent = response ? response->kind.response.result.write_msgs.num_msgs_written : 0;
        written += std::min(sent, batch);
        if (status != STATUS_NOERROR || sent < batch)
            break;
    }

    *pNumMsgs = written;
    return status;
}

long J2534RemoteClient::PassThruStartPeriodicMsg(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pMsgID,
                                                 unsigned long TimeInterval)
{
    if (!pMsg || !pMsgID)
        return localError(ERR_NULL_PARAMETER, "pMsg or pMsgID is NULL");
    auto request = newRequest(j2534_remote_Request_start_periodic_msg_tag);
    auto &call = request->kind.request.call.start_periodic_msg;
    call.channel_id = ChannelID;
    call.has_msg = true;
    remote::toWire(*pMsg, call.msg);
    call.time_interval = TimeInterval;
    long status;
    if (auto response = run(*request, 0, status))
        *pMsgID = response->kind.response.result.start_periodic_msg.msg_id;
    return status;
}

long J2534RemoteClient::PassThruStopPeriodicMsg(unsigned long ChannelID, unsigned long MsgID)
{
    auto request = newRequest(j2534_remote_Request_stop_periodic_msg_tag);
    request->kind.request.call.stop_periodic_msg = {ChannelID, MsgID};
    long status;
    run(*request, 0, status);
    return status;
}

long J2534RemoteClient::PassThruStartMsgFilter(unsigned long ChannelID, unsigned long FilterType, const PASSTHRU_MSG *pMaskMsg,
                                               const PASSTHRU_MSG *pPatternMsg, const PASSTHRU_MSG *pFlowControlMsg,
                                               unsigned long *pMsgID)
{
    if (!pMsgID)
        return localError(ERR_NULL_PARAMETER, "pMsgID is NULL");
    auto request = newRequest(j2534_remote_Request_start_msg_filter_tag);
    auto &call = request->kind.request.call.start_msg_filter;
    call.channel_id = ChannelID;
    call.filter_type = FilterType;
    if ((call.has_mask = pMaskMsg != nullptr)) remote::toWire(*pMaskMsg, call.mask);
    if ((call.has_pattern = pPatternMsg != nullptr)) remote::toWire(*pPatternMsg, call.pattern);
    if ((call.has_flow_control = pFlowControlMsg != nullptr)) remote::toWire(*pFlowControlMsg, call.flow_control);
    long status;
    if (auto response = run(*request, 0, status))
        *pMsgID = response->kind.response.result.start_msg_filter.filter_id;
    return status;
}

long J2534RemoteClient::PassThruStopMsgFilter(unsigned long ChannelID, unsigned long MsgID)
{
    auto request = newRequest(j2534_remote_Request_stop_msg_filter_tag);
    request->kind.request.call.stop_msg_filter = {ChannelID, MsgID};
    long status;
    run(*request, 0, status);
    return status;
}

long J2534RemoteClient::PassThruSetProgrammingVoltage(unsigned long DeviceID, unsigned long Pin, unsigned long Voltage)
{
    auto request = newRequest(j2534_remote_Request_set_programming_voltage_tag);
    request->kind.request.call.set_programming_voltage = {DeviceID, Pin, Voltage};
    long status;
    run(*request, 0, status);
    return status;
}

long J2534RemoteClient::PassThruReadVersion(unsigned long DeviceID, char *pFirmwareVersion, char *pDllVersion,
                                            char *pApiVersion)
{
    if (!pFirmwareVersion || !pDllVersion || !pApiVersion)
        return localError(ERR_NULL_PARAMETER, "a version buffer is NULL");
    auto request = newRequest(j2534_remote_Request_read_version_tag);
    request->kind.request.call.read_version.device_id = DeviceID;
    long status;
    if (auto response = run(*request, 0, status)) {
        const auto &result = response->kind.response.result.read_version;
        strncpy_s(pFirmwareVersion, 80, result.firmware_version, _TRUNCATE);
        strncpy_s(pDllVersion, 80, result.dll_version, _TRUNCATE);
        strncpy_s(pApiVersion, 80, result.api_version, _TRUNCATE);
    }
    return status;
}

long J2534RemoteClient::PassThruGetLastError(char *pErrorDescription)
{
    if (!pErrorDescription)
        return ERR_NULL_PARAMETER;

    {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        if (m_lastErrorIsLocal || !m_session.connected()) {
            strncpy_s(pErrorDescription, 80, m_lastError.empty() ? "No error" : m_lastError.c_str(), _TRUNCATE);
            return STATUS_NOERROR;
        }
    }

    auto request = newRequest(j2534_remote_Request_get_last_error_tag);
    std::string error;
    auto response = m_session.call(*request, kResponseTimeoutMs, &error);
    if (!response) {
        strncpy_s(pErrorDescription, 80, error.c_str(), _TRUNCATE);
        return STATUS_NOERROR;
    }
    strncpy_s(pErrorDescription, 80, response->kind.response.result.get_last_error.error_description, _TRUNCATE);
    return response->kind.response.status;
}

long J2534RemoteClient::PassThruIoctl(unsigned long ChannelID, unsigned long IoctlID, const void *pInput, void *pOutput)
{
    auto request = newRequest(j2534_remote_Request_ioctl_tag);
    auto &call = request->kind.request.call.ioctl;
    call.channel_id = ChannelID;
    call.ioctl_id = IoctlID;

    // Marshal the input.
    switch (IoctlID) {
    case GET_CONFIG:
    case SET_CONFIG: {
        auto *list = static_cast<const SCONFIG_LIST *>(pInput);
        if (!list || (list->NumOfParams && !list->ConfigPtr))
            return localError(ERR_NULL_PARAMETER, "SCONFIG_LIST is NULL");
        if (list->NumOfParams > remote::kMaxConfig)
            return localError(ERR_FAILED, "Too many config parameters for one remote IOCTL");
        call.config_count = static_cast<pb_size_t>(list->NumOfParams);
        for (unsigned long i = 0; i < list->NumOfParams; ++i)
            call.config[i] = {list->ConfigPtr[i].Parameter, list->ConfigPtr[i].Value};
        break;
    }
    case READ_VBATT:
    case READ_PROG_VOLTAGE:
        if (!pOutput)
            return localError(ERR_NULL_PARAMETER, "pOutput is NULL");
        break;
    case FIVE_BAUD_INIT:
    case ADD_TO_FUNCT_MSG_LOOKUP_TABLE:
    case DELETE_FROM_FUNCT_MSG_LOOKUP_TABLE: {
        auto *bytes = static_cast<const SBYTE_ARRAY *>(pInput);
        if (!bytes || (bytes->NumOfBytes && !bytes->BytePtr))
            return localError(ERR_NULL_PARAMETER, "SBYTE_ARRAY is NULL");
        if (bytes->NumOfBytes > sizeof(call.input_bytes.bytes))
            return localError(ERR_FAILED, "Too many bytes for one remote IOCTL");
        call.input_bytes.size = static_cast<pb_size_t>(bytes->NumOfBytes);
        std::memcpy(call.input_bytes.bytes, bytes->BytePtr, bytes->NumOfBytes);
        if (IoctlID == FIVE_BAUD_INIT && !pOutput)
            return localError(ERR_NULL_PARAMETER, "pOutput is NULL");
        break;
    }
    case FAST_INIT:
        if (!pInput || !pOutput)
            return localError(ERR_NULL_PARAMETER, "pInput or pOutput is NULL");
        call.has_input_msg = true;
        remote::toWire(*static_cast<const PASSTHRU_MSG *>(pInput), call.input_msg);
        break;
    case CLEAR_RX_BUFFER:
        m_session.clearRx(ChannelID);   // also drop what was already pushed to us
        break;
    case CLEAR_TX_BUFFER:
    case CLEAR_PERIODIC_MSGS:
    case CLEAR_MSG_FILTERS:
    case CLEAR_FUNCT_MSG_LOOKUP_TABLE:
        break;
    default:
        return localError(ERR_NOT_SUPPORTED, "This IOCTL is not available over the remote link");
    }

    long status;
    auto response = run(*request, 0, status);
    if (!response)
        return status;

    // Copy the output back.
    const auto &result = response->kind.response.result.ioctl;
    switch (IoctlID) {
    case GET_CONFIG: {
        auto *list = static_cast<const SCONFIG_LIST *>(pInput);
        const unsigned long count = std::min<unsigned long>(list->NumOfParams, result.config_count);
        for (unsigned long i = 0; i < count; ++i)
            list->ConfigPtr[i].Value = result.config[i].value;
        break;
    }
    case READ_VBATT:
    case READ_PROG_VOLTAGE:
        *static_cast<unsigned long *>(pOutput) = result.output_value;
        break;
    case FIVE_BAUD_INIT: {
        auto *out = static_cast<SBYTE_ARRAY *>(pOutput);
        out->NumOfBytes = result.output_bytes.size;
        if (out->BytePtr)
            std::memcpy(out->BytePtr, result.output_bytes.bytes, result.output_bytes.size);
        break;
    }
    case FAST_INIT:
        if (result.has_output_msg)
            remote::fromWire(result.output_msg, *static_cast<PASSTHRU_MSG *>(pOutput));
        break;
    default:
        break;
    }
    return status;
}
