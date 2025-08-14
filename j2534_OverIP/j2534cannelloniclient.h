#ifndef J2534CANNELLONICLIENT_H
#define J2534CANNELLONICLIENT_H

#include <windows.h>
#include <iostream>
#include <memory>
#include <map>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "ExDbg.h"
#include "Exj2534WrapperInterface.h"
#include "j2534_dev_typedefs.h"
#include "j2534_defs.h"
#include "expipeclient.h"

//#define STATUS_NOERROR 0
#define ERR_FAILED 1

#define PROTOCOL_CAN 1
#define MAX_CAN_FRAME_DATA_LEN 8
#define CANNELLONI_PORT_125K 20000
#define CANNELLONI_PORT_500K_12_13 20002
#define CANNELLONI_PORT_500K_6_14 20001


struct ChannelContext
{
    int sockfd = -1;
    sockaddr_in targetAddr{};
    std::thread recvThread;
    std::atomic<bool> stopRequested{false};

    std::mutex msgQueueMutex;
    std::condition_variable msgQueueCv;
    std::queue<std::vector<uint8_t>> msgQueue;

    ChannelContext() = default;

    ChannelContext(ChannelContext&& other) noexcept
    {
        sockfd = other.sockfd;
        targetAddr = other.targetAddr;
        recvThread = std::move(other.recvThread);
        stopRequested.store(other.stopRequested.load());
        other.sockfd = -1;
    }

    ChannelContext& operator=(ChannelContext&& other) noexcept
    {
        if (this != &other)
        {
            sockfd = other.sockfd;
            targetAddr = other.targetAddr;
            recvThread = std::move(other.recvThread);
            stopRequested.store(other.stopRequested.load());
            other.sockfd = -1;
        }
        return *this;
    }

    ChannelContext(const ChannelContext&) = delete;
    ChannelContext& operator=(const ChannelContext&) = delete;
};

class J2534CannelloniClient : public ExJ2534WrapperInterface
{
public:
    J2534CannelloniClient();
    ~J2534CannelloniClient() override;

    std::string getIName() override;

    long PassThruOpen(const void *pName, unsigned long *pDeviceID) override;
    long PassThruClose(unsigned long DeviceID) override;
    long PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID, unsigned long Flags, unsigned long Baudrate, unsigned long *pChannelID) override;
    long PassThruDisconnect(unsigned long ChannelID) override;
    long PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout) override;
    long PassThruWriteMsgs(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout) override;
    long PassThruStartPeriodicMsg(unsigned long ChannelID,const PASSTHRU_MSG *pMsg, unsigned long *pMsgID, unsigned long TimeInterval) override;
    long PassThruStopPeriodicMsg(unsigned long ChannelID, unsigned long MsgID) override;
    long PassThruStartMsgFilter(unsigned long ChannelID, unsigned long FilterType, const PASSTHRU_MSG *pMaskMsg, const PASSTHRU_MSG *pPatternMsg, const PASSTHRU_MSG *pFlowControlMsg, unsigned long *pMsgID) override;
    long PassThruStopMsgFilter(unsigned long ChannelID, unsigned long MsgID) override;
    long PassThruSetProgrammingVoltage(unsigned long DeviceID, unsigned long Pin, unsigned long Voltage) override;
    long PassThruReadVersion(unsigned long DeviceID, char *pFirmwareVersion, char *pDllVersion, char *pApiVersion) override;
    long PassThruGetLastError(char *pErrorDescription) override;
    long PassThruIoctl(unsigned long ChannelID, unsigned long IoctlID, const void *pInput, void *pOutput) override;

    // DREW Tech extra functions
    long PassThruExConfigureWiFi() override;
    long PassThruExDeviceWatchdog() override;
    long PassThruExDownloadCypheredFlashData() override;
    long PassThruExEraseFlash() override;
    long PassThruExInitiateCypheredFlashDownload() override;
    long PassThruExReadFlash() override;
    long PassThruExResetFlash() override;
    long PassThruExRunSelfTest() override;
    long PassThruExWriteFlash() override;

    long PassThruLoadFirmware() override;
    long PassThruRecoverFirmware() override;
    long PassThruReadIPSetup(unsigned long DeviceID, char *host_name, char *ip_addr, char *subnet_mask, char *gateway, char *dhcp_addr) override;
    long PassThruWriteIPSetup(unsigned long DeviceID, char *host_name, char *ip_addr, char *subnet_mask, char *gateway, char *dhcp_addr) override;
    long PassThruReadPCSetup(char *host_name, char *ip_addr) override;
    long PassThruGetPointer(long vb_pointer) override;
    long PassThruGetNextCarDAQ(char **name, unsigned long *version, char **addr) override;

    long PassThruScanForDevices(unsigned long *pDeviceCount) override;
    long PassThruGetNextDevice(SDEVICE *psDevice) override;

    long PassThruSelect(SCHANNELSET *ChannelSetPtr, unsigned long SelectType, unsigned long Timeout) override;
    long PassThruReadDetails(unsigned long* pName) override;

    long PassThruQueueMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs) override;

private:
    void startReceiveThread(ChannelContext& ctx);
    bool popMessage(ChannelContext& ctx, std::vector<uint8_t>& outMsg, int timeoutMs);
    void pushMessage(ChannelContext& ctx, const std::vector<uint8_t>& msg);

private:
    std::mutex globalMutex;
    std::map<unsigned long, ChannelContext> channelMap;
    unsigned long nextDeviceId = 1;
    unsigned long nextChannelId = 100;

    void recvLoop(unsigned long chanId);
    std::string lastError = "No error";
    long lastErrorLong = 0;

private:
    ExPipeClient *          _p_pipe;
    std::string             p_path;
    uint32_t                p_id;
    uint32_t                p_type;
    uint32_t                p_data_type;
};
#endif // J2534CANNELLONICLIENT_H
