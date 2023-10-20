#include "ExDbg.h"
#include "j2534dllwrapper.h"
#include "cbor_utils.h"

J2534DllWrapper::J2534DllWrapper()
{
    this->_pipe_client = new ExPipeClient("\\\\.\\pipe\\exj2534_overip_pipe");
    if(!this->_pipe_client->connect())
    {
        printf("Failed to connect to named pipe!");
    }
}

std::string J2534DllWrapper::getIName()
{
    char pname[1024] = {0};
    sprintf(pname, "J2534DllWrapper %08x", (unsigned int)this);
    return std::string(pname);
}


bool J2534DllWrapper::loadDll(const std::wstring& dllPath)
{
    std::vector<char> buf(dllPath.size());
    std::use_facet<std::ctype<wchar_t>>(std::locale{}).narrow(dllPath.data(), dllPath.data() + dllPath.size(), '?', buf.data());
    auto dllPathA = std::string(buf.data(), buf.size());

    EXDBG_LOG << "Loading J2534 Dll from Path: " << dllPathA << EXDBG_END;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);
    cn_cbor* cb_map = cn_cbor_map_create(&cn_errback);
    cbor_utils::map_put_string(cb_map_root, "dll_path", dllPathA.c_str());

    HMODULE hModule = LoadLibraryW(dllPath.c_str());
    if (hModule)
    {
        m_dllHandle.reset(hModule);

        m_PassThruOpen = loadFunction<PTOPEN>("PassThruOpen", cb_map);
        m_PassThruClose = loadFunction<PTCLOSE>("PassThruClose", cb_map);
        m_PassThruConnect = loadFunction<PTCONNECT>("PassThruConnect", cb_map);
        m_PassThruDisconnect = loadFunction<PTDISCONNECT>("PassThruDisconnect", cb_map);
        m_PassThruReadMsgs = loadFunction<PTREADMSGS>("PassThruReadMsgs", cb_map);
        m_PassThruWriteMsgs = loadFunction<PTWRITEMSGS>("PassThruWriteMsgs", cb_map);
        m_PassThruStartPeriodicMsg = loadFunction<PTSTARTPERIODICMSG>("PassThruStartPeriodicMsg", cb_map);
        m_PassThruStopPeriodicMsg = loadFunction<PTSTOPPERIODICMSG>("PassThruStopPeriodicMsg", cb_map);
        m_PassThruStartMsgFilter = loadFunction<PTSTARTMSGFILTER>("PassThruStartMsgFilter", cb_map);
        m_PassThruStopMsgFilter = loadFunction<PTSTOPMSGFILTER>("PassThruStopMsgFilter", cb_map);
        m_PassThruSetProgrammingVoltage = loadFunction<PTSETPROGRAMMINGVOLTAGE>("PassThruSetProgrammingVoltage", cb_map);
        m_PassThruReadVersion = loadFunction<PTREADVERSION>("PassThruReadVersion", cb_map);
        m_PassThruGetLastError = loadFunction<PTGETLASTERROR>("PassThruGetLastError", cb_map);
        m_PassThruIoctl = loadFunction<PTIOCTL>("PassThruIoctl", cb_map);

        // Drew Tech specific function calls
        m_PassThruLoadFirmware = loadFunction<PTLOADFIRMWARE>("PassThruLoadFirmware", cb_map);
        m_PassThruRecoverFirmware = loadFunction<PTRECOVERFIRMWARE>("PassThruRecoverFirmware", cb_map);
        m_PassThruReadIPSetup = loadFunction<PTREADIPSETUP>("PassThruReadIPSetup", cb_map);
        m_PassThruWriteIPSetup = loadFunction<PTWRITEIPSETUP>("PassThruWriteIPSetup", cb_map);
        m_PassThruReadPCSetup = loadFunction<PTREADPCSETUP>("PassThruReadPCSetup", cb_map);
        m_PassThruGetPointer = loadFunction<PTGETPOINTER>("PassThruGetPointer", cb_map);
        m_PassThruGetNextCarDAQ = loadFunction<PTGETNEXTCARDAQ>("PassThruGetNextCarDAQ", cb_map);

        m_PassThruExConfigureWiFi = loadFunction<PTEXCONFIGWIFI>("PassThruExConfigureWiFi", cb_map);
        m_PassThruExDeviceWatchdog = loadFunction<PTEXDEVWATCHDOG>("PassThruExDeviceWatchdog", cb_map);
        m_PassThruExDownloadCypheredFlashData = loadFunction<PTEXDLCYPHFLASHDATA>("PassThruExDownloadCypheredFlashData", cb_map);
        m_PassThruExEraseFlash = loadFunction<PTEXERASEFLASH>("PassThruExEraseFlash", cb_map);
        m_PassThruExInitiateCypheredFlashDownload = loadFunction<PTEXINITCYPHFLASHDL>("PassThruExInitiateCypheredFlashDownload", cb_map);
        m_PassThruExReadFlash = loadFunction<PTEXREADFLASH>("PassThruExReadFlash", cb_map);
        m_PassThruExResetFlash = loadFunction<PTEXRESETFLASH>("PassThruExResetFlash", cb_map);
        m_PassThruExRunSelfTest = loadFunction<PTEXRINSELFTEST>("PassThruExRunSelfTest", cb_map);
        m_PassThruExWriteFlash = loadFunction<PTEXWRITEFLASH>("PassThruExWriteFlash", cb_map);
        m_PassThruQueueMsgs = loadFunction<PTQUEUEMSGS>("PassThruQueueMsgs", cb_map);

        ///v05.00
        m_PassThruScanForDevices = loadFunction<PTSCANFORDEVICES>("PassThruScanForDevices", cb_map);
        m_PassThruGetNextDevice = loadFunction<PTGETNEXTDEV>("PassThruGetNextDevice", cb_map);
        m_PassThruSelect = loadFunction<PTSELECT>("PassThruSelect", cb_map);

        cbor_utils::map_put_map(cb_map_root, "functions", cb_map);
        ExPipeClient::Message msg = {(int)ExDbg::crc32(ExDbg::getProcessPath()), 1, 1, 0, ExDbg::getProcessPath(),
                                     "wrapped functions",
                                     cbor_utils::cbor_to_data(cb_map_root)};
        this->_pipe_client->send(msg);

        return true;
    } else {
        cbor_utils::map_put_string(cb_map_root, "error", std::string("Can't load DLL ").append(dllPathA).c_str());
        ExPipeClient::Message msg = {(int)ExDbg::crc32(ExDbg::getProcessPath()), 1, 1, 0, ExDbg::getProcessPath(),
                                     "wrapped functions",
                                     cbor_utils::cbor_to_data(cb_map_root)};
        this->_pipe_client->send(msg);

        EXDBG_LOG << "Failed to load library " << dllPathA << "." << EXDBG_END;
        return false;
    }
}

void J2534DllWrapper::unloadDll() {
    m_dllHandle.reset();
}

template <typename FunctionType>
FunctionType J2534DllWrapper::loadFunction(const char* functionName, cn_cbor * cb_map) {
    auto proc = reinterpret_cast<FunctionType>(GetProcAddress(reinterpret_cast<HMODULE>(m_dllHandle.get()), functionName));
    if (!proc) {
        if(cb_map != nullptr)cbor_utils::map_put_bool(cb_map, functionName, false);
        //EXDBG_LOG << "Failed to load function " << functionName << "." << EXDBG_END;
        return nullptr;
    }
    if(cb_map != nullptr)cbor_utils::map_put_bool(cb_map, functionName, true);
    return proc;
}

long J2534DllWrapper::PassThruOpen(const void *pName, unsigned long *pDeviceID)
{
    if (this->m_PassThruOpen) {
        return this->m_PassThruOpen(pName, pDeviceID);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruClose(unsigned long DeviceID)
{
    if (this->m_PassThruClose) {
        return this->m_PassThruClose(DeviceID);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID, unsigned long Flags, unsigned long Baudrate, unsigned long *pChannelID)
{
    if (this->m_PassThruConnect) {
        return this->m_PassThruConnect(DeviceID, ProtocolID, Flags, Baudrate, pChannelID);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruDisconnect(unsigned long ChannelID)
{
    if (this->m_PassThruDisconnect) {
        return this->m_PassThruDisconnect(ChannelID);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout)
{
    if (this->m_PassThruReadMsgs) {
        return this->m_PassThruReadMsgs(ChannelID, pMsg, pNumMsgs, Timeout);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruQueueMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs)
{
    if (this->m_PassThruQueueMsgs) {
        return this->m_PassThruQueueMsgs(ChannelID, pMsg, pNumMsgs);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruWriteMsgs(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout)
{
    if (this->m_PassThruWriteMsgs) {
        return this->m_PassThruWriteMsgs(ChannelID, (void*)pMsg, pNumMsgs, Timeout);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruStartPeriodicMsg(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pMsgID, unsigned long TimeInterval)
{
    if (this->m_PassThruStartPeriodicMsg) {
        return this->m_PassThruStartPeriodicMsg(ChannelID, (void*)pMsg, pMsgID, TimeInterval);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruStopPeriodicMsg(unsigned long ChannelID, unsigned long MsgID)
{
    if (this->m_PassThruStopPeriodicMsg) {
        return this->m_PassThruStopPeriodicMsg(ChannelID, MsgID);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruStartMsgFilter(unsigned long ChannelID, unsigned long FilterType, const PASSTHRU_MSG *pMaskMsg, const PASSTHRU_MSG *pPatternMsg, const PASSTHRU_MSG *pFlowControlMsg, unsigned long *pMsgID)
{
    if (this->m_PassThruStartMsgFilter) {
        return this->m_PassThruStartMsgFilter(ChannelID, FilterType, (void*)pMaskMsg, (void*)pPatternMsg, (void*)pFlowControlMsg, pMsgID);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruStopMsgFilter(unsigned long ChannelID, unsigned long MsgID)
{
    if (this->m_PassThruStopMsgFilter) {
        return this->m_PassThruStopMsgFilter(ChannelID, MsgID);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruSetProgrammingVoltage(unsigned long DeviceID, unsigned long Pin, unsigned long Voltage)
{
    if (this->m_PassThruSetProgrammingVoltage) {
        return this->m_PassThruSetProgrammingVoltage(DeviceID, Pin, Voltage);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruReadVersion(unsigned long DeviceID, char * pFirmwareVersion, char *pDllVersion, char * pApiVersion)
{
    if (this->m_PassThruReadVersion) {
        return this->m_PassThruReadVersion(DeviceID, pFirmwareVersion, pDllVersion, pApiVersion);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruGetLastError(char *pErrorDescription)
{
    if (this->m_PassThruGetLastError) {
        return this->m_PassThruGetLastError(pErrorDescription);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruIoctl(unsigned long ChannelID, unsigned long IoctlID, const void *pInput, void *pOutput)
{
    if (this->m_PassThruIoctl) {
        return this->m_PassThruIoctl(ChannelID, IoctlID, pInput, pOutput);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

///DREW Tech extra functions
///
long J2534DllWrapper::PassThruExConfigureWiFi ( void )
{
    if (this->m_PassThruExConfigureWiFi) {
        return this->m_PassThruExConfigureWiFi();
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruExDeviceWatchdog ( void )
{
    if (this->m_PassThruExDeviceWatchdog) {
        return this->m_PassThruExDeviceWatchdog();
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruExDownloadCypheredFlashData ( void )
{
    if (this->m_PassThruExDownloadCypheredFlashData) {
        return this->m_PassThruExDownloadCypheredFlashData();
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruExEraseFlash ( void )
{
    if (this->m_PassThruExEraseFlash) {
        return this->m_PassThruExEraseFlash();
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruExInitiateCypheredFlashDownload ( void )
{
    if (this->m_PassThruExInitiateCypheredFlashDownload) {
        return this->m_PassThruExInitiateCypheredFlashDownload();
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruExReadFlash ( void )
{
    if (this->m_PassThruExReadFlash) {
        return this->m_PassThruExReadFlash();
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruExResetFlash ( void )
{
    if (this->m_PassThruExResetFlash) {
        return this->m_PassThruExResetFlash();
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruExRunSelfTest ( void )
{
    if (this->m_PassThruExRunSelfTest) {
        return this->m_PassThruExRunSelfTest();
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruExWriteFlash ( void )
{
    if (this->m_PassThruExWriteFlash) {
        return this->m_PassThruExWriteFlash();
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruLoadFirmware(void)
{
    if (this->m_PassThruLoadFirmware) {
        return this->m_PassThruLoadFirmware();
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruRecoverFirmware(void)
{
    if (this->m_PassThruRecoverFirmware) {
        return this->m_PassThruRecoverFirmware();
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruReadIPSetup(unsigned long DeviceID, char *host_name, char *ip_addr, char *subnet_mask, char *gateway, char *dhcp_addr)
{
    if (this->m_PassThruReadIPSetup) {
        return this->m_PassThruReadIPSetup(DeviceID, host_name, ip_addr, subnet_mask, gateway, dhcp_addr);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruWriteIPSetup(unsigned long DeviceID, char *host_name, char *ip_addr, char *subnet_mask, char *gateway, char *dhcp_addr)
{
    if (this->m_PassThruWriteIPSetup) {
        return this->m_PassThruWriteIPSetup(DeviceID, host_name, ip_addr, subnet_mask, gateway, dhcp_addr);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruReadPCSetup(char *host_name, char *ip_addr)
{
    if (this->m_PassThruReadPCSetup) {
        return this->m_PassThruReadPCSetup(host_name, ip_addr);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruGetPointer(long vb_pointer)
{
    if (this->m_PassThruGetPointer) {
        return this->m_PassThruGetPointer(vb_pointer);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruGetNextCarDAQ(char **name, unsigned long *version, char **addr)
{
    if (this->m_PassThruGetNextCarDAQ) {
        return this->m_PassThruGetNextCarDAQ(name, version, addr);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

///v05.00
long J2534DllWrapper::PassThruScanForDevices(unsigned long *pDeviceCount)
{
    if (this->m_PassThruScanForDevices) {
        return this->m_PassThruScanForDevices(pDeviceCount);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruGetNextDevice(SDEVICE *psDevice)
{
    if (this->m_PassThruGetNextDevice) {
        return this->m_PassThruGetNextDevice(psDevice);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruSelect(SCHANNELSET *ChannelSetPtr, unsigned long SelectType, unsigned long Timeout)
{
    if (this->m_PassThruSelect) {
        return this->m_PassThruSelect(ChannelSetPtr, SelectType, Timeout);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

long J2534DllWrapper::PassThruReadDetails(unsigned long* pName)
{
    if (this->m_PassThruReadDetails) {
        return this->m_PassThruReadDetails(pName);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        return ERR_NOT_SUPPORTED;
    }
}

