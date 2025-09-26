#include "ExDbg.h"
#include "j2534dllwrapper.h"
#include "cbor_utils.h"
#include "timestamp_util.h"

J2534DllWrapper::J2534DllWrapper()
{

    this->_p_pipe = new ExPipeClient("\\\\.\\pipe\\exj2534_overip_pipe");
    this->_p_pipe->connect();

    this->p_path = ExDbg::getProcessPath();
    this->p_id = ExDbg::crc32(ExDbg::getProcessPath());
    this->p_type = 2; // 2 = J2534DllWrapper
    this->p_data_type = 2; // 2 = cbor

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, 0 /* constructor */);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, (uint32_t)this);

    auto ts = get_timestamp();
    EXDBG_LOG << "[J2534DllWrapper]: timestamp " << ts << EXDBG_END;
    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, ts, this->p_path, "J2534DllWrapper Verbunden!", cbor_utils::cbor_to_data(cb_map_root)});

    cn_cbor_free(cb_map_root);
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

    //EXDBG_LOG << "Loading J2534 Dll from Path: " << dllPathA << EXDBG_END;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);
    cn_cbor* cb_map = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, 1 /* constructor */);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, dllPathA.c_str());

    //cbor_utils::map_put_string(cb_map_root, "dll_path", dllPathA.c_str());

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

        cbor_utils::map_put_map(cb_map_root, ExPipeClient::KEY_Param2, cb_map);
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "wrapped functions", cbor_utils::cbor_to_data(cb_map_root)});

        cn_cbor_free(cb_map);
        cn_cbor_free(cb_map_root);

        return true;
    } else {
        cbor_utils::map_put_map(cb_map_root, ExPipeClient::KEY_Param2, cb_map);
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Can't load DLL ").append(dllPathA).c_str());

        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "loadDll", cbor_utils::cbor_to_data(cb_map_root)});

        cn_cbor_free(cb_map);
        cn_cbor_free(cb_map_root);

        //EXDBG_LOG << "Failed to load library " << dllPathA << "." << EXDBG_END;
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
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruOpen);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, (pName == NULL?"":(const char *)pName));
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2 /* param_2 */, *pDeviceID);

    if (this->m_PassThruOpen)
    {
        retval = this->m_PassThruOpen(pName, pDeviceID);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruOpen", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruClose(unsigned long DeviceID)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruClose);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, DeviceID);

    if (this->m_PassThruClose) {
        retval = this->m_PassThruClose(DeviceID);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);

    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruOpen", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID, unsigned long Flags, unsigned long Baudrate, unsigned long *pChannelID)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruConnect);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, DeviceID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2 /* param_1 */, ProtocolID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3 /* param_1 */, Flags);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param4 /* param_1 */, Baudrate);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param5 /* param_1 */, *pChannelID);

    if (this->m_PassThruConnect) {
        retval = this->m_PassThruConnect(DeviceID, ProtocolID, Flags, Baudrate, pChannelID);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruConnect", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruDisconnect(unsigned long ChannelID)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruDisconnect);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, ChannelID);

    if (this->m_PassThruDisconnect) {
        retval = this->m_PassThruDisconnect(ChannelID);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruConnect", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruReadMsgs);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, ChannelID);
    //cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, 0);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3, *pNumMsgs);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param4, Timeout);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 0);

    if (this->m_PassThruReadMsgs) {
        retval = this->m_PassThruReadMsgs(ChannelID, pMsg, pNumMsgs, Timeout);
        cbor_utils::map_put_PASSTHRU_MSG(cb_map_root, ExPipeClient::KEY_Param2, *pNumMsgs, pMsg);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3, *pNumMsgs);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param4, Timeout);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 1);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, 0);
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruReadMsgs", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruQueueMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruQueueMsgs);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, ChannelID);
    cbor_utils::map_put_PASSTHRU_MSG(cb_map_root, ExPipeClient::KEY_Param2, *pNumMsgs, pMsg);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3 /* param_1 */, *pNumMsgs);

    if (this->m_PassThruQueueMsgs) {
        retval = this->m_PassThruQueueMsgs(ChannelID, pMsg, pNumMsgs);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruQueueMsgs", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruWriteMsgs(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruWriteMsgs);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, ChannelID);
    cbor_utils::map_put_PASSTHRU_MSG(cb_map_root, ExPipeClient::KEY_Param2, *pNumMsgs, pMsg);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3 /* param_1 */, *pNumMsgs);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param4 /* param_1 */, Timeout);

    if (this->m_PassThruWriteMsgs) {
        retval = this->m_PassThruWriteMsgs(ChannelID, (void*)pMsg, pNumMsgs, Timeout);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruWriteMsgs", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruStartPeriodicMsg(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pMsgID, unsigned long TimeInterval)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruStartPeriodicMsg);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, ChannelID);
    cbor_utils::map_put_PASSTHRU_MSG(cb_map_root, ExPipeClient::KEY_Param2, 1, pMsg);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3 /* param_1 */, *pMsgID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param4 /* param_1 */, TimeInterval);

    if (this->m_PassThruStartPeriodicMsg) {
        retval = this->m_PassThruStartPeriodicMsg(ChannelID, (void*)pMsg, pMsgID, TimeInterval);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruStartPeriodicMsg", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruStopPeriodicMsg(unsigned long ChannelID, unsigned long MsgID)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruStopPeriodicMsg);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, ChannelID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, MsgID);

    if (this->m_PassThruStopPeriodicMsg) {
        retval = this->m_PassThruStopPeriodicMsg(ChannelID, MsgID);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruStopPeriodicMsg", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruStartMsgFilter(unsigned long ChannelID, unsigned long FilterType, const PASSTHRU_MSG *pMaskMsg, const PASSTHRU_MSG *pPatternMsg, const PASSTHRU_MSG *pFlowControlMsg, unsigned long *pMsgID)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruStartMsgFilter);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, ChannelID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, FilterType);
    cbor_utils::map_put_PASSTHRU_MSG(cb_map_root, ExPipeClient::KEY_Param3, 1, pMaskMsg);
    cbor_utils::map_put_PASSTHRU_MSG(cb_map_root, ExPipeClient::KEY_Param4, 1, pPatternMsg);
    cbor_utils::map_put_PASSTHRU_MSG(cb_map_root, ExPipeClient::KEY_Param5, 1, pFlowControlMsg);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param6, *pMsgID);

    if (this->m_PassThruStartMsgFilter) {
        retval = this->m_PassThruStartMsgFilter(ChannelID, FilterType, (void*)pMaskMsg, (void*)pPatternMsg, (void*)pFlowControlMsg, pMsgID);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruStartMsgFilter", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruStopMsgFilter(unsigned long ChannelID, unsigned long MsgID)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruStopMsgFilter);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, ChannelID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, MsgID);

    if (this->m_PassThruStopMsgFilter) {
        retval = this->m_PassThruStopMsgFilter(ChannelID, MsgID);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruStopMsgFilter", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruSetProgrammingVoltage(unsigned long DeviceID, unsigned long Pin, unsigned long Voltage)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruSetProgrammingVoltage);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, DeviceID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, Pin);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, Voltage);

    if (this->m_PassThruSetProgrammingVoltage) {
        retval = this->m_PassThruSetProgrammingVoltage(DeviceID, Pin, Voltage);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruSetProgrammingVoltage", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruReadVersion(unsigned long DeviceID, char * pFirmwareVersion, char *pDllVersion, char * pApiVersion)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruReadVersion);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, DeviceID);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param2 /* param_1 */, std::string(pFirmwareVersion).c_str());
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param3 /* param_1 */, std::string(pDllVersion).c_str());
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param4 /* param_1 */, std::string(pApiVersion).c_str());
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 0);


    if (this->m_PassThruReadVersion) {

        retval = this->m_PassThruReadVersion(DeviceID, pFirmwareVersion, pDllVersion, pApiVersion);
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param2 /* param_1 */, std::string(pFirmwareVersion).c_str());
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param3 /* param_1 */, std::string(pDllVersion).c_str());
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param4 /* param_1 */, std::string(pApiVersion).c_str());
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 1);

    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruReadVersion", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruGetLastError(char *pErrorDescription)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruGetLastError);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, std::string(pErrorDescription).c_str());
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 0);

    if (this->m_PassThruGetLastError) {
        retval = this->m_PassThruGetLastError(pErrorDescription);
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, std::string(pErrorDescription).c_str());
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 1);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruGetLastError", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruIoctl(unsigned long ChannelID, unsigned long IoctlID, const void *pInput, void *pOutput)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruIoctl);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1, ChannelID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, IoctlID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3, 0);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param4, 0);


    if (this->m_PassThruIoctl) {
        retval = this->m_PassThruIoctl(ChannelID, IoctlID, pInput, pOutput);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruIoctl", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

///DREW Tech extra functions
///
long J2534DllWrapper::PassThruExConfigureWiFi ( void )
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruExConfigureWiFi);

    if (this->m_PassThruExConfigureWiFi) {
        retval = this->m_PassThruExConfigureWiFi();
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruExConfigureWiFi", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruExDeviceWatchdog ( void )
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruExDeviceWatchdog);

    if (this->m_PassThruExDeviceWatchdog) {
        retval = this->m_PassThruExDeviceWatchdog();
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruExDeviceWatchdog", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruExDownloadCypheredFlashData ( void )
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruExDownloadCypheredFlashData);

    if (this->m_PassThruExDownloadCypheredFlashData) {
        retval = this->m_PassThruExDownloadCypheredFlashData();
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruExDownloadCypheredFlashData", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruExEraseFlash ( void )
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruExDownloadCypheredFlashData);

    if (this->m_PassThruExEraseFlash) {
        retval = this->m_PassThruExEraseFlash();
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruExDownloadCypheredFlashData", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruExInitiateCypheredFlashDownload ( void )
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruExInitiateCypheredFlashDownload);

    if (this->m_PassThruExInitiateCypheredFlashDownload) {
        retval = this->m_PassThruExInitiateCypheredFlashDownload();
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruExInitiateCypheredFlashDownload", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruExReadFlash ( void )
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruExReadFlash);

    if (this->m_PassThruExReadFlash) {
        retval = this->m_PassThruExReadFlash();
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruExReadFlash", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruExResetFlash ( void )
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruExResetFlash);

    if (this->m_PassThruExResetFlash) {
        retval = this->m_PassThruExResetFlash();
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruExResetFlash", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruExRunSelfTest ( void )
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruExRunSelfTest);

    if (this->m_PassThruExRunSelfTest) {
        retval = this->m_PassThruExRunSelfTest();
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruExRunSelfTest", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruExWriteFlash ( void )
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruExWriteFlash);

    if (this->m_PassThruExWriteFlash) {
        retval = this->m_PassThruExWriteFlash();
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruExWriteFlash", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruLoadFirmware(void)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruLoadFirmware);

    if (this->m_PassThruLoadFirmware) {
        retval = this->m_PassThruLoadFirmware();
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruLoadFirmware", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruRecoverFirmware(void)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruRecoverFirmware);

    if (this->m_PassThruRecoverFirmware) {
        retval = this->m_PassThruRecoverFirmware();
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruRecoverFirmware", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruReadIPSetup(unsigned long DeviceID, char *host_name, char *ip_addr, char *subnet_mask, char *gateway, char *dhcp_addr)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruReadIPSetup);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1, DeviceID);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param2, host_name);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param3, ip_addr);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param4, subnet_mask);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param5, gateway);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param6, dhcp_addr);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 0);

    if (this->m_PassThruReadIPSetup) {
        retval = this->m_PassThruReadIPSetup(DeviceID, host_name, ip_addr, subnet_mask, gateway, dhcp_addr);
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param2, host_name);
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param3, ip_addr);
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param4, subnet_mask);
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param5, gateway);
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param6, dhcp_addr);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 1);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);

    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruReadIPSetup", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruWriteIPSetup(unsigned long DeviceID, char *host_name, char *ip_addr, char *subnet_mask, char *gateway, char *dhcp_addr)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruWriteIPSetup);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1, DeviceID);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param2, host_name);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param3, ip_addr);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param4, subnet_mask);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param5, gateway);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param6, dhcp_addr);

    if (this->m_PassThruWriteIPSetup) {
        retval = this->m_PassThruWriteIPSetup(DeviceID, host_name, ip_addr, subnet_mask, gateway, dhcp_addr);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruWriteIPSetup", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruReadPCSetup(char *host_name, char *ip_addr)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruReadPCSetup);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param1, host_name);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param2, ip_addr);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 0);

    if (this->m_PassThruReadPCSetup) {
        retval = this->m_PassThruReadPCSetup(host_name, ip_addr);
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param1, host_name);
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param2, ip_addr);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 1);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruReadPCSetup", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruGetPointer(long vb_pointer)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruGetPointer);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1, vb_pointer);

    if (this->m_PassThruGetPointer) {
        retval = this->m_PassThruGetPointer(vb_pointer);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1, vb_pointer);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 1);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruGetPointer", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruGetNextCarDAQ(char **name, unsigned long *version, char **addr)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruGetNextCarDAQ);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param1, "");
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, *version);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param3, "");
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 0);

    if (this->m_PassThruGetNextCarDAQ) {
        retval = this->m_PassThruGetNextCarDAQ(name, version, addr);

        if(name == NULL)
        {
            cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param1, "");
            cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, *version);
            cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param3, "");
        }
        else
        {
            cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param1, *name);
            cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, *version);
            cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param3, *addr);
        }

        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 1);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruGetNextCarDAQ", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

///v05.00
long J2534DllWrapper::PassThruScanForDevices(unsigned long *pDeviceCount)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruScanForDevices);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1, *pDeviceCount);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 0);

    if (this->m_PassThruScanForDevices) {
        retval = this->m_PassThruScanForDevices(pDeviceCount);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1, *pDeviceCount);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 1);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruScanForDevices", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruGetNextDevice(SDEVICE *psDevice)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);
    SDEVICE _tmp_sdev = {{0}};
    memset(&_tmp_sdev, 0, sizeof(SDEVICE));

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruGetNextDevice);
    cbor_utils::map_put_data(cb_map_root, ExPipeClient::KEY_Param1, (const uint8_t *)&_tmp_sdev, sizeof(SDEVICE));
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 0);

    if (this->m_PassThruGetNextDevice) {
        retval = this->m_PassThruGetNextDevice(psDevice);
        if(psDevice == NULL)cbor_utils::map_put_data(cb_map_root, ExPipeClient::KEY_Param1, (const uint8_t *)&_tmp_sdev, sizeof(SDEVICE));
        else cbor_utils::map_put_data(cb_map_root, ExPipeClient::KEY_Param1, (const uint8_t *)psDevice, sizeof(SDEVICE));

        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 1);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruGetNextDevice", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruSelect(SCHANNELSET *ChannelSetPtr, unsigned long SelectType, unsigned long Timeout)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruSelect);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1, 0);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, SelectType);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3, Timeout);

    if (this->m_PassThruSelect) {
        retval = this->m_PassThruSelect(ChannelSetPtr, SelectType, Timeout);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruSelect", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534DllWrapper::PassThruReadDetails(unsigned long* pName)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruSelect);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1, *pName);

    if (this->m_PassThruReadDetails) {
        retval = this->m_PassThruReadDetails(pName);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1, *pName);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 1);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruSelect", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

