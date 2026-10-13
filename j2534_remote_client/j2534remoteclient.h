#ifndef J2534REMOTECLIENT_H
#define J2534REMOTECLIENT_H

#include <functional>
#include <mutex>
#include <string>

#include "Exj2534WrapperInterface.h"
#include "remote_session.h"

// J2534Remote interface (Diagnostic PC): forwards PassThru calls to a Vehicle
// PC over the remote link. ReadMsgs is answered from messages the Vehicle PC
// pushes; everything else is a request/response round trip.
class J2534RemoteClient : public ExJ2534WrapperInterface
{
public:
    using CodeProvider = std::function<std::string()>;   // connection code, read at PassThruOpen
    using Logger = std::function<void(const std::string &)>;

    J2534RemoteClient(CodeProvider codeProvider, std::string clientName, Logger logger = {});
    ~J2534RemoteClient() override;

    std::string getIName() override { return "J2534Remote"; }

    long PassThruOpen(const void *pName, unsigned long *pDeviceID) override;
    long PassThruClose(unsigned long DeviceID) override;
    long PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID, unsigned long Flags, unsigned long Baudrate, unsigned long *pChannelID) override;
    long PassThruDisconnect(unsigned long ChannelID) override;
    long PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout) override;
    long PassThruWriteMsgs(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout) override;
    long PassThruStartPeriodicMsg(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pMsgID, unsigned long TimeInterval) override;
    long PassThruStopPeriodicMsg(unsigned long ChannelID, unsigned long MsgID) override;
    long PassThruStartMsgFilter(unsigned long ChannelID, unsigned long FilterType, const PASSTHRU_MSG *pMaskMsg, const PASSTHRU_MSG *pPatternMsg, const PASSTHRU_MSG *pFlowControlMsg, unsigned long *pMsgID) override;
    long PassThruStopMsgFilter(unsigned long ChannelID, unsigned long MsgID) override;
    long PassThruSetProgrammingVoltage(unsigned long DeviceID, unsigned long Pin, unsigned long Voltage) override;
    long PassThruReadVersion(unsigned long DeviceID, char *pFirmwareVersion, char *pDllVersion, char *pApiVersion) override;
    long PassThruGetLastError(char *pErrorDescription) override;
    long PassThruIoctl(unsigned long ChannelID, unsigned long IoctlID, const void *pInput, void *pOutput) override;

    // Not available over the remote link.
    long PassThruExConfigureWiFi() override { return notSupported("PassThruExConfigureWiFi"); }
    long PassThruExDeviceWatchdog() override { return notSupported("PassThruExDeviceWatchdog"); }
    long PassThruExDownloadCypheredFlashData() override { return notSupported("PassThruExDownloadCypheredFlashData"); }
    long PassThruExEraseFlash() override { return notSupported("PassThruExEraseFlash"); }
    long PassThruExInitiateCypheredFlashDownload() override { return notSupported("PassThruExInitiateCypheredFlashDownload"); }
    long PassThruExReadFlash() override { return notSupported("PassThruExReadFlash"); }
    long PassThruExResetFlash() override { return notSupported("PassThruExResetFlash"); }
    long PassThruExRunSelfTest() override { return notSupported("PassThruExRunSelfTest"); }
    long PassThruExWriteFlash() override { return notSupported("PassThruExWriteFlash"); }
    long PassThruLoadFirmware() override { return notSupported("PassThruLoadFirmware"); }
    long PassThruRecoverFirmware() override { return notSupported("PassThruRecoverFirmware"); }
    long PassThruReadIPSetup(unsigned long, char *, char *, char *, char *, char *) override { return notSupported("PassThruReadIPSetup"); }
    long PassThruWriteIPSetup(unsigned long, char *, char *, char *, char *, char *) override { return notSupported("PassThruWriteIPSetup"); }
    long PassThruReadPCSetup(char *, char *) override { return notSupported("PassThruReadPCSetup"); }
    long PassThruGetPointer(long) override { return notSupported("PassThruGetPointer"); }
    long PassThruGetNextCarDAQ(char **, unsigned long *, char **) override { return notSupported("PassThruGetNextCarDAQ"); }
    long PassThruScanForDevices(unsigned long *) override { return notSupported("PassThruScanForDevices"); }
    long PassThruGetNextDevice(SDEVICE *) override { return notSupported("PassThruGetNextDevice"); }
    long PassThruSelect(SCHANNELSET *, unsigned long, unsigned long) override { return notSupported("PassThruSelect"); }
    long PassThruReadDetails(unsigned long *) override { return notSupported("PassThruReadDetails"); }
    long PassThruQueueMsgs(unsigned long, PASSTHRU_MSG *, unsigned long *) override { return notSupported("PassThruQueueMsgs"); }

    // Waiting for a response, on top of the call's own J2534 timeout.
    static constexpr DWORD kResponseTimeoutMs = 15000;
    // Waiting for the Vehicle PC user to click Allow.
    static constexpr DWORD kHelloTimeoutMs = 120000;

private:
    long ensureConnected();
    // Runs `request`; on success returns the response, otherwise sets the
    // local error and returns null (status in `status`).
    std::unique_ptr<remote::Frame> run(remote::Frame &request, DWORD extraTimeoutMs, long &status);
    long finish(const remote::Frame &response);
    long notSupported(const char *function);
    long localError(long status, const std::string &message);
    void log(const std::string &text);

    CodeProvider m_codeProvider;
    std::string m_clientName;
    Logger m_logger;
    RemoteSession m_session;
    std::mutex m_connectMutex;

    std::mutex m_errorMutex;
    std::string m_lastError;         // set for failures that never reached the driver
    bool m_lastErrorIsLocal = false;
};

#endif // J2534REMOTECLIENT_H
