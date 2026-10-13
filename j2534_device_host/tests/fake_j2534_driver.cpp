// Minimal J2534 04.04 driver for the device host test. Deterministic:
//   Open -> device 1, Connect -> channel 7, WriteMsgs loops messages back as RX,
//   GET_CONFIG returns parameter*10, READ_VBATT 12345 mV, FIVE_BAUD_INIT
//   returns the input reversed, FAST_INIT echoes with data[0] | 0x40.
// Every call is appended to the file named by FAKE_J2534_LOG.

#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>

#include "PassThruStruct.h"

namespace {

std::mutex g_mutex;
std::deque<PASSTHRU_MSG> g_rx;
std::string g_lastError = "fake driver error";

constexpr long NOERROR_ = 0x00;
constexpr long BUFFER_EMPTY = 0x10;
constexpr long NULL_PARAMETER = 0x04;

void log(const char *format, ...)
{
    char path[MAX_PATH] = {};
    if (!GetEnvironmentVariableA("FAKE_J2534_LOG", path, MAX_PATH))
        return;
    FILE *file = nullptr;
    if (fopen_s(&file, path, "a") != 0 || !file)
        return;
    va_list args;
    va_start(args, format);
    std::vfprintf(file, format, args);
    va_end(args);
    std::fputc('\n', file);
    std::fclose(file);
}

} // namespace

extern "C" {

long __stdcall PassThruOpen(const void *pName, unsigned long *pDeviceID)
{
    if (!pDeviceID) return NULL_PARAMETER;
    *pDeviceID = 1;
    log("Open %s", pName ? static_cast<const char *>(pName) : "(null)");
    return NOERROR_;
}

long __stdcall PassThruClose(unsigned long DeviceID)
{
    log("Close %lu", DeviceID);
    return NOERROR_;
}

long __stdcall PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID, unsigned long Flags,
                               unsigned long Baudrate, unsigned long *pChannelID)
{
    if (!pChannelID) return NULL_PARAMETER;
    *pChannelID = 7;
    log("Connect %lu %lu %lu %lu", DeviceID, ProtocolID, Flags, Baudrate);
    return NOERROR_;
}

long __stdcall PassThruDisconnect(unsigned long ChannelID)
{
    log("Disconnect %lu", ChannelID);
    return NOERROR_;
}

long __stdcall PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    unsigned long count = 0;
    while (count < *pNumMsgs && !g_rx.empty()) {
        pMsg[count++] = g_rx.front();
        g_rx.pop_front();
    }
    *pNumMsgs = count;
    (void)ChannelID;
    return count ? NOERROR_ : BUFFER_EMPTY;
}

long __stdcall PassThruWriteMsgs(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    for (unsigned long i = 0; i < *pNumMsgs; ++i) {
        PASSTHRU_MSG echo = pMsg[i];
        echo.RxStatus = 0x01;   // TX_MSG_TYPE: loopback
        g_rx.push_back(echo);
    }
    log("WriteMsgs %lu %lu", ChannelID, *pNumMsgs);
    return NOERROR_;
}

long __stdcall PassThruStartPeriodicMsg(unsigned long ChannelID, const PASSTHRU_MSG *, unsigned long *pMsgID, unsigned long TimeInterval)
{
    *pMsgID = 42;
    log("StartPeriodicMsg %lu %lu", ChannelID, TimeInterval);
    return NOERROR_;
}

long __stdcall PassThruStopPeriodicMsg(unsigned long ChannelID, unsigned long MsgID)
{
    log("StopPeriodicMsg %lu %lu", ChannelID, MsgID);
    return NOERROR_;
}

long __stdcall PassThruStartMsgFilter(unsigned long ChannelID, unsigned long FilterType, const PASSTHRU_MSG *pMaskMsg,
                                      const PASSTHRU_MSG *pPatternMsg, const PASSTHRU_MSG *pFlowControlMsg, unsigned long *pMsgID)
{
    *pMsgID = 5;
    log("StartMsgFilter %lu %lu mask=%d pattern=%d flow=%d", ChannelID, FilterType,
        pMaskMsg != nullptr, pPatternMsg != nullptr, pFlowControlMsg != nullptr);
    return NOERROR_;
}

long __stdcall PassThruStopMsgFilter(unsigned long ChannelID, unsigned long MsgID)
{
    log("StopMsgFilter %lu %lu", ChannelID, MsgID);
    return NOERROR_;
}

long __stdcall PassThruSetProgrammingVoltage(unsigned long DeviceID, unsigned long Pin, unsigned long Voltage)
{
    log("SetProgrammingVoltage %lu %lu %lu", DeviceID, Pin, Voltage);
    return NOERROR_;
}

long __stdcall PassThruReadVersion(unsigned long, char *pFirmwareVersion, char *pDllVersion, char *pApiVersion)
{
    strcpy_s(pFirmwareVersion, 80, "FW 1.2");
    strcpy_s(pDllVersion, 80, "Fake DLL 3.4");
    strcpy_s(pApiVersion, 80, "04.04");
    return NOERROR_;
}

long __stdcall PassThruGetLastError(char *pErrorDescription)
{
    strcpy_s(pErrorDescription, 80, g_lastError.c_str());
    return NOERROR_;
}

long __stdcall PassThruIoctl(unsigned long ChannelID, unsigned long IoctlID, const void *pInput, void *pOutput)
{
    log("Ioctl %lu 0x%02lx", ChannelID, IoctlID);
    switch (IoctlID) {
    case 0x01: {   // GET_CONFIG
        auto *list = static_cast<const SCONFIG_LIST *>(pInput);
        for (unsigned long i = 0; i < list->NumOfParams; ++i)
            list->ConfigPtr[i].Value = list->ConfigPtr[i].Parameter * 10;
        return NOERROR_;
    }
    case 0x03:     // READ_VBATT
        *static_cast<unsigned long *>(pOutput) = 12345;
        return NOERROR_;
    case 0x04: {   // FIVE_BAUD_INIT
        auto *in = static_cast<const SBYTE_ARRAY *>(pInput);
        auto *out = static_cast<SBYTE_ARRAY *>(pOutput);
        out->NumOfBytes = in->NumOfBytes;
        for (unsigned long i = 0; i < in->NumOfBytes; ++i)
            out->BytePtr[i] = in->BytePtr[in->NumOfBytes - 1 - i];
        return NOERROR_;
    }
    case 0x05: {   // FAST_INIT
        auto *in = static_cast<const PASSTHRU_MSG *>(pInput);
        auto *out = static_cast<PASSTHRU_MSG *>(pOutput);
        *out = *in;
        if (out->DataSize > 0)
            out->Data[0] |= 0x40;
        return NOERROR_;
    }
    case 0x08: {   // CLEAR_RX_BUFFER
        std::lock_guard<std::mutex> lock(g_mutex);
        g_rx.clear();
        return NOERROR_;
    }
    default:
        return NOERROR_;
    }
}

} // extern "C"
