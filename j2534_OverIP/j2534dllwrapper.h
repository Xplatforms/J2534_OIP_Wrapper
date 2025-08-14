#ifndef J2534DLLWRAPPER_H
#define J2534DLLWRAPPER_H


#include <windows.h>
#include <iostream>
#include <memory>
#include "ExDbg.h"
#include "Exj2534WrapperInterface.h"
#include "j2534_dev_typedefs.h"
#include "j2534_defs.h"
#include "expipeclient.h"
#include "exj2534wrapperInterface.h"

// Custom deleter
struct DllDeleter {
    typedef HMODULE pointer;
    void operator()(HMODULE hModule) const {
        FreeLibrary(hModule);
    }
};

#define DECLARE_AND_DEFINE_WRAPPER_FUNCTION(FunctionType, FunctionName) \
private: \
    FunctionType m_##FunctionName = nullptr; \
    public: \
    template<typename... Args> \
    long FunctionName(Args... args) { \
        /*EXDBG_LOG << "Function called: " #FunctionName << EXDBG_END;*/ \
        if (m_##FunctionName) { \
            return m_##FunctionName(args...); \
    } else { \
            /*EXDBG_LOG << "Function " #FunctionName " not found in DLL." << EXDBG_END;*/ \
            return ERR_NOT_SUPPORTED; \
    } \
}

class J2534DllWrapper: public ExJ2534WrapperInterface
{
public:
    J2534DllWrapper();

    std::string getIName();

    bool loadDll(const std::wstring& dllPath);
    void unloadDll();

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
    long PassThruReadVersion(unsigned long DeviceID, char * pFirmwareVersion, char *pDllVersion, char * pApiVersion);
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
    template <typename FunctionType>
    FunctionType loadFunction(const char* functionName, cn_cbor * cb_map = nullptr);
    std::unique_ptr<HMODULE, DllDeleter> m_dllHandle;

private:
    PTOPEN m_PassThruOpen;
    PTCLOSE m_PassThruClose;
    PTCONNECT m_PassThruConnect;
    PTDISCONNECT m_PassThruDisconnect;
    PTREADMSGS m_PassThruReadMsgs;
    PTWRITEMSGS m_PassThruWriteMsgs;
    PTSTARTPERIODICMSG m_PassThruStartPeriodicMsg;
    PTSTOPPERIODICMSG m_PassThruStopPeriodicMsg;
    PTSTARTMSGFILTER m_PassThruStartMsgFilter;
    PTSTOPMSGFILTER m_PassThruStopMsgFilter;
    PTSETPROGRAMMINGVOLTAGE m_PassThruSetProgrammingVoltage;
    PTREADVERSION m_PassThruReadVersion;
    PTGETLASTERROR m_PassThruGetLastError;
    PTIOCTL m_PassThruIoctl;

    PTEXCONFIGWIFI m_PassThruExConfigureWiFi;
    PTEXDEVWATCHDOG m_PassThruExDeviceWatchdog;
    PTEXDLCYPHFLASHDATA m_PassThruExDownloadCypheredFlashData;
    PTEXERASEFLASH m_PassThruExEraseFlash;
    PTEXINITCYPHFLASHDL m_PassThruExInitiateCypheredFlashDownload;
    PTEXREADFLASH m_PassThruExReadFlash;
    PTEXRESETFLASH m_PassThruExResetFlash;
    PTEXRINSELFTEST m_PassThruExRunSelfTest;
    PTEXWRITEFLASH m_PassThruExWriteFlash;
    PTLOADFIRMWARE m_PassThruLoadFirmware;
    PTRECOVERFIRMWARE m_PassThruRecoverFirmware;
    PTREADIPSETUP m_PassThruReadIPSetup;
    PTWRITEIPSETUP m_PassThruWriteIPSetup;
    PTREADPCSETUP m_PassThruReadPCSetup;
    PTGETPOINTER m_PassThruGetPointer;
    PTGETNEXTCARDAQ m_PassThruGetNextCarDAQ;

    PTSCANFORDEVICES m_PassThruScanForDevices;
    PTGETNEXTDEV m_PassThruGetNextDevice;
    PTSELECT m_PassThruSelect;
    PTREADDETAILS m_PassThruReadDetails;

    PTQUEUEMSGS m_PassThruQueueMsgs;

private:
    ExPipeClient *          _p_pipe;
    std::string             p_path;
    uint32_t                p_id;
    uint32_t                p_type;
    uint32_t                p_data_type;

};





#endif // J2534DLLWRAPPER_H
