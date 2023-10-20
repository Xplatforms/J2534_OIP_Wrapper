#include "ExDbg.h"
#include "j2534simplelogger.h"
#include "j2534_defs.h"

ExJ2534SimpleLogger::ExJ2534SimpleLogger()
{

}

std::string ExJ2534SimpleLogger::getIName()
{
    char pname[1024] = {0};
    sprintf(pname, "ExJ2534SimpleLogger %08x", (unsigned int)this);
    return std::string(pname);
}

long ExJ2534SimpleLogger::PassThruOpen(const void *pName, unsigned long *pDeviceID)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruClose(unsigned long DeviceID)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID, unsigned long Flags, unsigned long Baudrate, unsigned long *pChannelID)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruDisconnect(unsigned long ChannelID)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruQueueMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruWriteMsgs(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruStartPeriodicMsg(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pMsgID, unsigned long TimeInterval)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruStopPeriodicMsg(unsigned long ChannelID, unsigned long MsgID)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruStartMsgFilter(unsigned long ChannelID, unsigned long FilterType, const PASSTHRU_MSG *pMaskMsg, const PASSTHRU_MSG *pPatternMsg, const PASSTHRU_MSG *pFlowControlMsg, unsigned long *pMsgID)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruStopMsgFilter(unsigned long ChannelID, unsigned long MsgID)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruSetProgrammingVoltage(unsigned long DeviceID, unsigned long Pin, unsigned long Voltage)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruReadVersion(unsigned long DeviceID, char * pFirmwareVersion, char *pDllVersion, char * pApiVersion)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruGetLastError(char *pErrorDescription)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruIoctl(unsigned long ChannelID, unsigned long IoctlID, const void *pInput, void *pOutput)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

///DREW Tech extra functions
///
long ExJ2534SimpleLogger::PassThruExConfigureWiFi ( void )
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruExDeviceWatchdog ( void )
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruExDownloadCypheredFlashData ( void )
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruExEraseFlash ( void )
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruExInitiateCypheredFlashDownload ( void )
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruExReadFlash ( void )
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruExResetFlash ( void )
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruExRunSelfTest ( void )
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruExWriteFlash ( void )
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruLoadFirmware(void)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruRecoverFirmware(void)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruReadIPSetup(unsigned long DeviceID, char *host_name, char *ip_addr, char *subnet_mask, char *gateway, char *dhcp_addr)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruWriteIPSetup(unsigned long DeviceID, char *host_name, char *ip_addr, char *subnet_mask, char *gateway, char *dhcp_addr)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruReadPCSetup(char *host_name, char *ip_addr)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruGetPointer(long vb_pointer)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruGetNextCarDAQ(char **name, unsigned long *version, char **addr)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

///v05.00
long ExJ2534SimpleLogger::PassThruScanForDevices(unsigned long *pDeviceCount)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruGetNextDevice(SDEVICE *psDevice)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruSelect(SCHANNELSET *ChannelSetPtr, unsigned long SelectType, unsigned long Timeout)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}

long ExJ2534SimpleLogger::PassThruReadDetails(unsigned long* pName)
{
    EXDBG_LOG << __FUNCTION__ << EXDBG_END;
    return ERR_NOT_SUPPORTED;
}
