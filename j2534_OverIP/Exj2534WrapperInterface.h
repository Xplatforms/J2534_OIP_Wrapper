#ifndef EXJ2534WRAPPERINTERFACE_H
#define EXJ2534WRAPPERINTERFACE_H

#include "j2534.h"
#include <string>

class ExJ2534WrapperInterface
{
public:
    virtual ~ExJ2534WrapperInterface() = default;

    virtual std::string getIName() = 0;

    virtual long PassThruOpen(const void *pName, unsigned long *pDeviceID) = 0;
    virtual long PassThruClose(unsigned long DeviceID) = 0;
    virtual long PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID, unsigned long Flags, unsigned long Baudrate, unsigned long *pChannelID) = 0;
    virtual long PassThruDisconnect(unsigned long ChannelID) = 0;
    virtual long PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout) = 0;
    virtual long PassThruWriteMsgs(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout) = 0;
    virtual long PassThruStartPeriodicMsg(unsigned long ChannelID,const PASSTHRU_MSG *pMsg, unsigned long *pMsgID, unsigned long TimeInterval) = 0;
    virtual long PassThruStopPeriodicMsg(unsigned long ChannelID, unsigned long MsgID) = 0;
    virtual long PassThruStartMsgFilter(unsigned long ChannelID, unsigned long FilterType, const PASSTHRU_MSG *pMaskMsg, const PASSTHRU_MSG *pPatternMsg, const PASSTHRU_MSG *pFlowControlMsg, unsigned long *pMsgID) = 0;
    virtual long PassThruStopMsgFilter(unsigned long ChannelID, unsigned long MsgID) = 0;
    virtual long PassThruSetProgrammingVoltage(unsigned long DeviceID, unsigned long Pin, unsigned long Voltage) = 0;
    virtual long PassThruReadVersion(unsigned long DeviceID, char * pFirmwareVersion, char *pDllVersion, char *pApiVersion) = 0;
    virtual long PassThruGetLastError(char *pErrorDescription) = 0;
    virtual long PassThruIoctl(unsigned long ChannelID, unsigned long IoctlID, const void *pInput, void *pOutput) = 0;

    ///DREW Tech extra functions
    ///
    virtual long PassThruExConfigureWiFi ( void ) = 0;
    virtual long PassThruExDeviceWatchdog ( void ) = 0;
    virtual long PassThruExDownloadCypheredFlashData ( void ) = 0;
    virtual long PassThruExEraseFlash ( void ) = 0;
    virtual long PassThruExInitiateCypheredFlashDownload ( void ) = 0;
    virtual long PassThruExReadFlash ( void ) = 0;
    virtual long PassThruExResetFlash ( void ) = 0;
    virtual long PassThruExRunSelfTest ( void ) = 0;
    virtual long PassThruExWriteFlash ( void ) = 0;

    virtual long PassThruLoadFirmware(void) = 0;
    virtual long PassThruRecoverFirmware(void) = 0;
    virtual long PassThruReadIPSetup(unsigned long DeviceID, char *host_name, char *ip_addr, char *subnet_mask, char *gateway, char *dhcp_addr) = 0;
    virtual long PassThruWriteIPSetup(unsigned long DeviceID, char *host_name, char *ip_addr, char *subnet_mask, char *gateway, char *dhcp_addr) = 0;
    virtual long PassThruReadPCSetup(char *host_name, char *ip_addr) = 0;
    virtual long PassThruGetPointer(long vb_pointer) = 0;
    virtual long PassThruGetNextCarDAQ(char **name, unsigned long *version, char **addr) = 0;

    ///v05.00
    virtual long PassThruScanForDevices(unsigned long *pDeviceCount) = 0;
    virtual long PassThruGetNextDevice(SDEVICE *psDevice) = 0;

    virtual long PassThruSelect(SCHANNELSET *ChannelSetPtr, unsigned long SelectType, unsigned long Timeout) = 0;
    virtual long PassThruReadDetails(unsigned long* pName) = 0;

    virtual long PassThruQueueMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs) = 0;

};

#endif // EXJ2534WRAPPERINTERFACE_H
