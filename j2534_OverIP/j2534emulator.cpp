#include "ExDbg.h"
#include "j2534_defs.h"
#include "j2534emulator.h"
#include "cbor_utils.h"
#include "timestamp_util.h"

J2534Emulator::J2534Emulator()
{
    this->_p_pipe = new ExPipeClient("\\\\.\\pipe\\exj2534_overip_pipe");
    this->_p_pipe->connect();

    this->p_path = ExDbg::getProcessPath();
    this->p_id = ExDbg::crc32(ExDbg::getProcessPath());
    this->p_type = 3; // 3 = J2534Emulator
    this->p_data_type = 2; // 2 = cbor

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, 0 /* constructor */);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, (uint32_t)this);

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "J2534Emulator Verbunden!", cbor_utils::cbor_to_data(cb_map_root)});

    cn_cbor_free(cb_map_root);
}


std::string J2534Emulator::getIName()
{
    char pname[1024] = {0};
    sprintf(pname, "J2534Emulator %08x", (unsigned int)this);
    return std::string(pname);
}

long J2534Emulator::PassThruOpen(const void * pName, unsigned long * pDeviceID)
{
    *pDeviceID = 777;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruOpen);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, (pName == NULL?"":(const char *)pName));
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2 /* param_2 */, *pDeviceID);

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruOpen", cbor_utils::cbor_to_data(cb_map_root)});

    cn_cbor_free(cb_map_root);

    return STATUS_NOERROR;
}

long J2534Emulator::PassThruClose(unsigned long DeviceID)
{
    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruClose);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, DeviceID);

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruClose", cbor_utils::cbor_to_data(cb_map_root)});

    cn_cbor_free(cb_map_root);

    return STATUS_NOERROR;
}

long J2534Emulator::PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID, unsigned long Flags, unsigned long Baudrate, unsigned long *pChannelID)
{
    switch(ProtocolID)
    {
    //K-LINE
    case ISO9141:
        *pChannelID = 1;
        break;
    //CAN
    case CAN:
        *pChannelID = 2;
        break;
    //ISO-TP
    case ISO15765:
        *pChannelID = 3;
        break;
    }


    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruConnect);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, DeviceID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2 /* param_1 */, ProtocolID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3 /* param_1 */, Flags);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param4 /* param_1 */, Baudrate);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param5 /* param_1 */, *pChannelID);

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruConnect", cbor_utils::cbor_to_data(cb_map_root)});

    cn_cbor_free(cb_map_root);

    return STATUS_NOERROR;
}

long J2534Emulator::PassThruDisconnect(unsigned long ChannelID)
{
    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruDisconnect);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, ChannelID);

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruDisconnect", cbor_utils::cbor_to_data(cb_map_root)});

    cn_cbor_free(cb_map_root);

    return STATUS_NOERROR;
}

long J2534Emulator::PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout)
{
    printf("PassThruReadMsgs ChannelID %08x *pNumMsgs = %d Timeout = %d\n", ChannelID , *pNumMsgs, Timeout);

    // Enable flow control for VGSNAG2 ECU
    PASSTHRU_MSG fcMsg = {0};
    fcMsg.ProtocolID = ISO15765;
    fcMsg.DataSize = 3;
    fcMsg.TxFlags = ISO15765_FRAME_PAD;
    fcMsg.Data[0] = 0x30;
    fcMsg.Data[1] = 0x00;
    fcMsg.Data[2] = 0x01;

    *pNumMsgs = 1;

    memcpy(&pMsg[0], &fcMsg, sizeof(PASSTHRU_MSG));

    //return numMsgs == 0 ? ERR_BUFFER_EMPTY : STATUS_NOERROR;

    /*auto resp = ExJ2534OIPSocks::getInstance()->getJ2534Dev()->m_PassThruReadMsgs(ChannelID, pMsg, pNumMsgs, Timeout);
    if(resp == STATUS_NOERROR)
    {
        for(int i = 0; i < *pNumMsgs; i++)
        {
            printf("[READMSG] protid %08x rxstatus %08x txflags %08x tstamp %d e_index %08x datasize %d \n [MSG]: ",
                   pMsg[i].ProtocolID, pMsg[i].RxStatus, pMsg[i].TxFlags,
                   pMsg[i].Timestamp, pMsg[i].ExtraDataIndex, pMsg[i].DataSize);
            for(int y = 0; y < pMsg[i].DataSize; y++)
            {
                printf("%02x ", pMsg[i].Data[y]);
            }
            printf("\n");
        }
    }
    return resp;*/

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruReadMsgs);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, ChannelID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, 0);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3, *pNumMsgs);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param4, Timeout);

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruReadMsgs", cbor_utils::cbor_to_data(cb_map_root)});

    cn_cbor_free(cb_map_root);

    return STATUS_NOERROR;
}

long J2534Emulator::PassThruQueueMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs)
{
    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    for(int i = 0; i < *pNumMsgs; i++)
    {
        printf("[READMSG] protid %08x rxstatus %08x txflags %08x tstamp %d e_index %08x datasize %d \n [MSG]: ",
               pMsg[i].ProtocolID, pMsg[i].RxStatus, pMsg[i].TxFlags,
               pMsg[i].Timestamp, pMsg[i].ExtraDataIndex, pMsg[i].DataSize);
        for(int y = 0; y < pMsg[i].DataSize; y++)
        {
            printf("%02x ", pMsg[i].Data[y]);
        }
        printf("\n");
    }

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruQueueMsgs);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, ChannelID);
    cbor_utils::map_put_PASSTHRU_MSG(cb_map_root, ExPipeClient::KEY_Param2, *pNumMsgs, pMsg);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3, *pNumMsgs);

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruQueueMsgs", cbor_utils::cbor_to_data(cb_map_root)});

    cn_cbor_free(cb_map_root);

    return STATUS_NOERROR;
}

long J2534Emulator::PassThruWriteMsgs(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout)
{
    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    for(int i = 0; i < *pNumMsgs; i++)
    {
        printf("[READMSG] protid %08x rxstatus %08x txflags %08x tstamp %d e_index %08x datasize %d \n [MSG]: ",
               pMsg[i].ProtocolID, pMsg[i].RxStatus, pMsg[i].TxFlags,
               pMsg[i].Timestamp, pMsg[i].ExtraDataIndex, pMsg[i].DataSize);
        for(int y = 0; y < pMsg[i].DataSize; y++)
        {
            printf("%02x ", pMsg[i].Data[y]);
        }
        printf("\n");
    }

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruWriteMsgs);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, ChannelID);
    cbor_utils::map_put_PASSTHRU_MSG(cb_map_root, ExPipeClient::KEY_Param2, *pNumMsgs, pMsg);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3, *pNumMsgs);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param4, Timeout);

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruWriteMsgs", cbor_utils::cbor_to_data(cb_map_root)});

    cn_cbor_free(cb_map_root);

    return STATUS_NOERROR;
}

long J2534Emulator::PassThruStartPeriodicMsg(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pMsgID, unsigned long TimeInterval)
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruStartPeriodicMsg called!"});
    return STATUS_NOERROR;
}

long J2534Emulator::PassThruStopPeriodicMsg(unsigned long ChannelID, unsigned long MsgID)
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruStopPeriodicMsg called!"});
    return STATUS_NOERROR;
}

long J2534Emulator::PassThruStartMsgFilter(unsigned long ChannelID, unsigned long FilterType, const PASSTHRU_MSG *pMaskMsg, const PASSTHRU_MSG *pPatternMsg, const PASSTHRU_MSG *pFlowControlMsg, unsigned long *pMsgID)
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruStartMsgFilter called!"});
    return STATUS_NOERROR;
}

long J2534Emulator::PassThruStopMsgFilter(unsigned long ChannelID, unsigned long MsgID)
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruStopMsgFilter called!"});
    return STATUS_NOERROR;
}

long J2534Emulator::PassThruSetProgrammingVoltage(unsigned long DeviceID, unsigned long Pin, unsigned long Voltage)
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruSetProgrammingVoltage called!"});
    return STATUS_NOERROR;
}

long J2534Emulator::PassThruReadVersion(unsigned long DeviceID, char *pFirmwareVersion, char *pDllVersion, char *pApiVersion)
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruReadVersion called!"});

    char FirmwareVersion[80] = {0};
    char DllVersion[80] = {0};
    char ApiVersion[80] = {0};

    //OpenPort:
    //api:  1.17.4877
    //dll:  1.01.4341 Jul 10 2014 14:16:35
    //firm: 04.04

    memcpy(ApiVersion, "04.04", 5);
    memcpy(DllVersion, "1.0.001 Feb 28 2023 12:46:00", 28);
    memcpy(FirmwareVersion, "9", 1);

    memcpy(pApiVersion, ApiVersion, 80);
    memcpy(pDllVersion, DllVersion, 80);
    memcpy(pFirmwareVersion, FirmwareVersion, 80);

    return STATUS_NOERROR;
}

long J2534Emulator::PassThruGetLastError(char *pErrorDescription)
{
    if(pErrorDescription == NULL)
    {
        pErrorDescription = new char[2048];
    }
    sprintf(pErrorDescription, "%s", "NOT SUPPORTED");

    std::string message = __FUNCTION__;

    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, message});

    return STATUS_NOERROR;
}

long J2534Emulator::PassThruIoctl(unsigned long ChannelID, unsigned long IoctlID, const void *pInput, void *pOutput)
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruIoctl called!"});

    // TODO: check IoctlID?
    // 3 READ_VBATT?
    // 2 CONFIG_SET?
    printf("PassThruIoctl Channel or Device %d IoctlID = %08x\n", ChannelID, IoctlID);
    if (IoctlID == READ_VBATT)
    {
        //unsigned int voltageLevel = 14400;
        //memcpy(pOutput, &voltageLevel, sizeof(unsigned int));

        unsigned int *voltageLevel = (unsigned int*)pOutput;
        *voltageLevel = 14400; // The units will be in milli-volts and will be rounded to the nearest tenth of a volt.
    }
    else if(IoctlID == SET_CONFIG)
    {
        if(pInput == NULL)
        {
            printf("SET_CONFIG with no Input? \n");
        }
        else
        {
            const SCONFIG_LIST * input = reinterpret_cast<const SCONFIG_LIST *>(pInput);
            printf("SCONFIG_LIST NumOfParams %d\n", input->NumOfParams);
            for(int i = 0; i < input->NumOfParams; i++)
            {
                printf("ConfigPtr %d paramter %08x value %08x \n", i, input->ConfigPtr[i].Parameter, input->ConfigPtr[i].Value);
            }
        }
    }
    else if(IoctlID == GET_DEVICE_INFO)
    {
        auto param_list = (SPARAM_LIST*)pOutput;
        switch(param_list->SParamPtr[0].Parameter)
        {
            case SERIAL_NUMBER:
                param_list->SParamPtr[0].Supported = 1;
                param_list->SParamPtr[0].Value = 987654321;
            break;
        }


    }

    return STATUS_NOERROR;
}

///DREW Tech extra functions
///
long J2534Emulator::PassThruExConfigureWiFi ( void )
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruExConfigureWiFi called!"});
    return STATUS_NOERROR;
}

long J2534Emulator::PassThruExDeviceWatchdog ( void )
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruExDeviceWatchdog called!"});
    return STATUS_NOERROR;
}

long J2534Emulator::PassThruExDownloadCypheredFlashData ( void )
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruExDownloadCypheredFlashData called!"});
    return STATUS_NOERROR;
}

long J2534Emulator::PassThruExEraseFlash ( void )
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruExEraseFlash called!"});
    return STATUS_NOERROR;
}

long J2534Emulator::PassThruExInitiateCypheredFlashDownload ( void )
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruExInitiateCypheredFlashDownload called!"});
    return STATUS_NOERROR;
}

long J2534Emulator::PassThruExReadFlash ( void )
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruExReadFlash called!"});
    return STATUS_NOERROR;
}

long J2534Emulator::PassThruExResetFlash ( void )
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruExResetFlash called!"});
    return STATUS_NOERROR;
}

long J2534Emulator::PassThruExRunSelfTest ( void )
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruExRunSelfTest called!"});
    return STATUS_NOERROR;
}

long J2534Emulator::PassThruExWriteFlash ( void )
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruExWriteFlash called!"});
    return STATUS_NOERROR;
}

long J2534Emulator::PassThruLoadFirmware(void)
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruLoadFirmware called!"});
    return STATUS_NOERROR;
}

long J2534Emulator::PassThruRecoverFirmware(void)
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruRecoverFirmware called!"});
    return STATUS_NOERROR;
}

long J2534Emulator::PassThruReadIPSetup(unsigned long DeviceID, char *host_name, char *ip_addr, char *subnet_mask, char *gateway, char *dhcp_addr)
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruReadIPSetup called!"});
    return STATUS_NOERROR;
}

long J2534Emulator::PassThruWriteIPSetup(unsigned long DeviceID, char *host_name, char *ip_addr, char *subnet_mask, char *gateway, char *dhcp_addr)
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruWriteIPSetup called!"});
    return STATUS_NOERROR;
}

long J2534Emulator::PassThruReadPCSetup(char *host_name, char *ip_addr)
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruReadPCSetup called!"});
    return STATUS_NOERROR;
}

long J2534Emulator::PassThruGetPointer(long vb_pointer)
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruGetPointer called!"});
    return STATUS_NOERROR;
}

int _dev_counter = 0;
long J2534Emulator::PassThruGetNextCarDAQ(char **name, unsigned long *version, char **addr)
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruGetNextCarDAQ called!"});

    if(name == NULL || version == NULL || addr == NULL)
    {
        this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "PassThruGetNextCarDAQ NULLPARAM"});
        return STATUS_NOERROR;
    }

    if(_dev_counter == 0)
    {
        char * pName = (char*)malloc(16);
        memset(pName, 0, 16);
        sprintf(pName, "%s", "XDev1");

        char * pAddr = (char*)malloc(128);
        memset(pAddr, 0, 128);
        sprintf(pAddr, "%d.%d.%d.%d", 192, 168, 155, 100);

        unsigned long ver = 0xFFFFFF;
        unsigned long *pVersion = (unsigned long*)malloc(sizeof(unsigned long));
        memcpy(pVersion, &ver, sizeof(unsigned long));

        *name = pName;
        version = pVersion;
        *addr = pAddr;

        _dev_counter++;
    }
    else
    {
        *name = nullptr;
    }

    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "PassThruGetNextCarDAQ"});

    return STATUS_NOERROR;
}

///v05.00
long J2534Emulator::PassThruScanForDevices(unsigned long *pDeviceCount)
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruScanForDevices called!"});
    return STATUS_NOERROR;
}

long J2534Emulator::PassThruGetNextDevice(SDEVICE *psDevice)
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruGetNextDevice called!"});
    return STATUS_NOERROR;
}

long J2534Emulator::PassThruSelect(SCHANNELSET *ChannelSetPtr, unsigned long SelectType, unsigned long Timeout)
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruSelect called!"});
    return STATUS_NOERROR;
}

long J2534Emulator::PassThruReadDetails(unsigned long* pName)
{
    this->_p_pipe->send({this->p_id, this->p_type, 0, get_timestamp(), this->p_path, "J2534Emulator PassThruReadDetails called!"});
    return STATUS_NOERROR;
}
