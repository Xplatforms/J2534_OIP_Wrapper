#ifndef J2534EMULATOR_H
#define J2534EMULATOR_H

#include "Exj2534WrapperInterface.h"
#include "expipeclient.h"

class J2534Emulator: public ExJ2534WrapperInterface
{
public:
    J2534Emulator();

    std::string getIName();

    long PassThruOpen(const void *pName, unsigned long *pDeviceID);
    long PassThruClose(unsigned long DeviceID);
    long PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID, unsigned long Flags, unsigned long Baudrate, unsigned long *pChannelID);
    long PassThruDisconnect(unsigned long ChannelID);
    long PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout);
    long PassThruWriteMsgs(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout);
    long PassThruStartPeriodicMsg(unsigned long ChannelID,const PASSTHRU_MSG *pMsg, unsigned long *pMsgID, unsigned long TimeInterval);
    long PassThruStopPeriodicMsg(unsigned long ChannelID, unsigned long MsgID);
    long PassThruStartMsgFilter(unsigned long ChannelID, unsigned long FilterType, const PASSTHRU_MSG *pMaskMsg, const PASSTHRU_MSG *pPatternMsg, const PASSTHRU_MSG *pFlowControlMsg, unsigned long *pMsgID);
    long PassThruStopMsgFilter(unsigned long ChannelID, unsigned long MsgID);
    long PassThruSetProgrammingVoltage(unsigned long DeviceID, unsigned long Pin, unsigned long Voltage);
    long PassThruReadVersion(unsigned long DeviceID, char *pApiVersion, char *pDllVersion, char *pFirmwareVersion);
    long PassThruGetLastError(char *pErrorDescription);
    long PassThruIoctl(unsigned long ChannelID, unsigned long IoctlID, const void *pInput, void *pOutput);

    ///DREW Tech extra functions
    ///
    long PassThruExConfigureWiFi ( void );
    long PassThruExDeviceWatchdog ( void );
    long PassThruExDownloadCypheredFlashData ( void );
    long PassThruExEraseFlash ( void );
    long PassThruExInitiateCypheredFlashDownload ( void );
    long PassThruExReadFlash ( void );
    long PassThruExResetFlash ( void );
    long PassThruExRunSelfTest ( void );
    long PassThruExWriteFlash ( void );

    long PassThruLoadFirmware(void);
    long PassThruRecoverFirmware(void);
    long PassThruReadIPSetup(unsigned long DeviceID, char *host_name, char *ip_addr, char *subnet_mask, char *gateway, char *dhcp_addr);
    long PassThruWriteIPSetup(unsigned long DeviceID, char *host_name, char *ip_addr, char *subnet_mask, char *gateway, char *dhcp_addr);
    long PassThruReadPCSetup(char *host_name, char *ip_addr);
    long PassThruGetPointer(long vb_pointer);
    long PassThruGetNextCarDAQ(char **name, unsigned long *version, char **addr);

    ///v05.00
    long PassThruScanForDevices(unsigned long *pDeviceCount);
    long PassThruGetNextDevice(SDEVICE *psDevice);

    long PassThruSelect(SCHANNELSET *ChannelSetPtr, unsigned long SelectType, unsigned long Timeout);
    long PassThruReadDetails(unsigned long* pName);

    long PassThruQueueMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs);

private:
    ExPipeClient *          _p_pipe;
    std::string             p_path;
    uint32_t                p_id;
    uint32_t                p_type;
    uint32_t                p_data_type;
};

#endif // J2534EMULATOR_H
