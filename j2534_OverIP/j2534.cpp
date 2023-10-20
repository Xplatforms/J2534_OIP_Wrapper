#include "j2534.h"
#include "exj2534oipsocks.h"

#include "ExDbg.h"

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include <stdint.h>
#include <string.h>
#include <chrono>
#include <thread>

#include "j2534global.h"

//#pragma comment(linker, "/export:PassThruOpen=PassThruOpen@@YGHPBXPAK@Z")
J2534_EXPORT  _PassThruOpen(const void *pName, unsigned long *pDeviceID)
{
    return J2534Global::obj()->getInterface()->PassThruOpen(pName, pDeviceID);
}

J2534_EXPORT  _PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID,
                               unsigned long Flags, unsigned long Baudrate,
                               unsigned long *pChannelID)
{
    return J2534Global::obj()->getInterface()->PassThruConnect(DeviceID, ProtocolID, Flags, Baudrate, pChannelID);
}

/*
{

  printf("PassThruConnect DeviceID = %08x ProtocolID = %08x\n", DeviceID, ProtocolID);
  printf("PassThruConnect Flags = %08x Baudrate = %08x\n", Flags, Baudrate);



  //return ExJ2534OIPSocks::getInstance()->getJ2534Dev()->m_PassThruConnect(DeviceID, ProtocolID, Flags, Baudrate, pChannelID);


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

  return STATUS_NOERROR;

}*/





J2534_EXPORT  _PassThruReadMsgs(uint32_t ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout)
{
    return J2534Global::obj()->getInterface()->PassThruReadMsgs(ChannelID, pMsg, pNumMsgs, Timeout);
}
/*
{
    printf("PassThruReadMsgs ChannelID %08x *pNumMsgs = %d Timeout = %d\n", ChannelID , *pNumMsgs, Timeout);

    memcpy(pMsg, &_cache_msg, sizeof(PASSTHRU_MSG));
    pMsg->ProtocolID = ISO15765;
    return STATUS_NOERROR;
    auto resp = ExJ2534OIPSocks::getInstance()->getJ2534Dev()->m_PassThruReadMsgs(ChannelID, pMsg, pNumMsgs, Timeout);
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
    return resp;


/*
    auto ex_j2534_socks = ExJ2534OIPSocks::getInstance();
    std::this_thread::sleep_for(std::chrono::microseconds(PASS_THRU_READ_MSGS_SLEEP_MS * 1000));
    //usleep(PASS_THRU_READ_MSGS_SLEEP_MS * 1000);
    WaitForSingleObject(ex_j2534_socks->m_recv_buffer_mutex, INFINITE);
    uint32_t numMsgs = 0;
    // check recv buffer
    if (ex_j2534_socks->m_recv_buffer_size != 0) {
        for (int i = 0; i < ex_j2534_socks->m_recv_buffer_size; ++i) {
            PASSTHRU_MSG *msg = &pMsg[i];
            PASSTHRU_MSG *recv_msg = &ex_j2534_socks->m_recv_buffer[i];
            msg->ProtocolID = recv_msg->ProtocolID;
            msg->RxStatus = recv_msg->RxStatus;
            msg->Timestamp = recv_msg->Timestamp;
            msg->DataSize = recv_msg->DataSize;
            memcpy(msg->Data, recv_msg->Data, msg->DataSize);
            // log
            printf("PassThruReadMsgs: msg->DataSize = %04x\n", msg->DataSize);
            // increment
            numMsgs += 1;
        }
        // set buffer as depleted
        ex_j2534_socks->m_recv_buffer_size = 0;
    }
    ReleaseMutex(ex_j2534_socks->m_recv_buffer_mutex);
    *pNumMsgs = numMsgs;
    return numMsgs == 0 ? ERR_BUFFER_EMPTY : STATUS_NOERROR;
}*/

J2534_EXPORT PassThruQueueMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs)
{
    return J2534Global::obj()->getInterface()->PassThruQueueMsgs(ChannelID, pMsg, pNumMsgs);
}

J2534_EXPORT  _PassThruWriteMsgs(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout)
{
    return J2534Global::obj()->getInterface()->PassThruWriteMsgs(ChannelID, pMsg, pNumMsgs, Timeout);
}
/*{
    printf("PassThruWriteMsgs ChannelID %08x *pNumMsgs = %d Timeout = %d\n", ChannelID , *pNumMsgs, Timeout);
    for(int i = 0; i < *pNumMsgs; i++)
    {
        printf("MSG2SEND protocolid %08x rxstatus %08x txflags %08x timestamp %d extra index %08x datasize %d \n [MSG]: ", pMsg[i].ProtocolID, pMsg[i].RxStatus, pMsg[i].TxFlags,
               pMsg[i].Timestamp, pMsg[i].ExtraDataIndex, pMsg[i].DataSize);
        for(int y = 0; y < pMsg[i].DataSize; y++)
        {
            printf("%02x ", pMsg[i].Data[y]);
        }
        printf("\n");
    }

    memcpy(&_cache_msg, &pMsg[0], sizeof(PASSTHRU_MSG));

    //return STATUS_NOERROR;

    return ExJ2534OIPSocks::getInstance()->getJ2534Dev()->m_PassThruWriteMsgs(ChannelID, (void *)pMsg, pNumMsgs, Timeout);

    /*
    return STATUS_NOERROR;

    auto ex_j2534_socks = ExJ2534OIPSocks::getInstance();
    // client not connected yet
    if (ex_j2534_socks->m_client_fd == -1)
    {
        printf("[PassThruWriteMsgs] no clients connected!\n");
        *pNumMsgs = 0x00000000;
        return ERR_FAILED;
    }

    printf("PassThruWriteMsgs *pNumMsgs = %d\n", *pNumMsgs);
    uint32_t numMsgsSent = 0;
    for (int i = 0; i < *pNumMsgs; ++i)
    {
        const PASSTHRU_MSG *msg = &pMsg[i];
        // check for wrong protocol ID
        if (msg->ProtocolID != CAN)
        {
            *pNumMsgs = numMsgsSent;
            return ERR_MSG_PROTOCOL_ID;
        }
        u_long bigEndianMsgDataSize = htonl(msg->DataSize);
        // send length
        if (send(ex_j2534_socks->m_client_fd, (char*)&bigEndianMsgDataSize, sizeof(uint32_t), 0) != sizeof(uint32_t))
        {
            printf("send1 failed errno = %d\n", WSAGetLastError());
            *pNumMsgs = numMsgsSent;
            return ERR_FAILED;
        }
        // send payload
        if (send(ex_j2534_socks->m_client_fd, (char*)msg->Data, msg->DataSize, 0) != msg->DataSize)
        {
            printf("send2 failed errno = %d\n", WSAGetLastError());
            *pNumMsgs = numMsgsSent;
            return ERR_FAILED;
        }
        numMsgsSent += 1;
        // record loopback
        WaitForSingleObject(ex_j2534_socks->m_recv_buffer_mutex, INFINITE);
        PASSTHRU_MSG *loopback_msg = &ex_j2534_socks->m_recv_buffer[ex_j2534_socks->m_recv_buffer_size];
        memset(loopback_msg, 0, sizeof(PASSTHRU_MSG));
        loopback_msg->ProtocolID = msg->ProtocolID;
        loopback_msg->RxStatus = msg->RxStatus | TX_MSG_TYPE;
        loopback_msg->Timestamp = ex_j2534_socks->GetTime();
        loopback_msg->DataSize = msg->DataSize;
        memcpy(loopback_msg->Data, msg->Data, msg->DataSize);
        // log
        printf("PassThruWriteMsgs: loopback_msg->DataSize = %04x\n", msg->DataSize);
        // increment
        ex_j2534_socks->m_recv_buffer_size += 1;
        // release
        ReleaseMutex(ex_j2534_socks->m_recv_buffer_mutex);
        // sleep
        //usleep(PASS_THRU_WRITE_MSGS_SLEEP_MS * 1000);
        std::this_thread::sleep_for(std::chrono::microseconds(PASS_THRU_WRITE_MSGS_SLEEP_MS * 1000));

    }
    *pNumMsgs = numMsgsSent;
    return STATUS_NOERROR;
    */
//}

J2534_EXPORT  _PassThruStartMsgFilter(unsigned long ChannelID, unsigned long FilterType,
                                      const PASSTHRU_MSG *pMaskMsg,
                                      const PASSTHRU_MSG *pPatternMsg,
                                      const PASSTHRU_MSG *pFlowControlMsg,
                                      unsigned long *pMsgID)
{
    return J2534Global::obj()->getInterface()->PassThruStartMsgFilter(ChannelID, FilterType, pMaskMsg, pPatternMsg, pFlowControlMsg, pMsgID);
}
/*{
  // TODO: keep state somewhere?
  printf("PassThruStartMsgFilter\n");
  printf("PassThruStartMsgFilter ChannelID = %08x FilterType = %08x\n", ChannelID, FilterType);
  //printf("PassThruStartMsgFilter ChannelID = %08x FilterType = %08x\n", ChannelID, FilterType);
  if(pMaskMsg != NULL)
  {
      printf("PassThruStartMsgFilter pMaskMsg->ProtocolID = %08x\n", pMaskMsg->ProtocolID);
      if(pMaskMsg->DataSize > 0)
      {
          printf("PassThruStartMsgFilter pMaskMsg->Data =");
          for(int i = 0; i < pMaskMsg->DataSize; i++)
          {
              printf("%02x ", pMaskMsg->Data[i]);
          }
          printf("\n");
      }
  }

  return ExJ2534OIPSocks::getInstance()->getJ2534Dev()->m_PassThruStartMsgFilter(ChannelID, FilterType, (void *)pMaskMsg, (void *)pPatternMsg, (void *)pFlowControlMsg, pMsgID);
  //*pMsgID = 0x00000001;
  //return STATUS_NOERROR;
}*/

J2534_EXPORT  _PassThruStopMsgFilter(uint32_t ChannelID, uint32_t MsgID)
{
    return J2534Global::obj()->getInterface()->PassThruStopMsgFilter(ChannelID, MsgID);
}
/*
{
  // TODO: keep state somewhere?
  printf("PassThruStopMsgFilter channel %d msgid %08x\n", ChannelID, MsgID);

  ExJ2534OIPSocks::getInstance()->getJ2534Dev()->m_PassThruStopMsgFilter(ChannelID, MsgID);
  return STATUS_NOERROR;
}*/


/*
SET_CONFIG
Configures various CarDAQ configuration parameters. The values of multiple parameters may be set with a single function call, by initializing an array of SCONFIG items.

unsigned long status;
SCONFIG CfgItem;
SCONFIG_LIST Input;

CfgItem.Parameter = DATA_RATE;
CfgItem.Value = 10400;

Input.NumOfParams = 1;
Input.ConfigPtr = &CfgItem;

status = PassThruIoctl(ChannelID, SET_CONFIG, &Input, NULL);
*/
J2534_EXPORT  _PassThruIoctl(unsigned long ChannelID, unsigned long IoctlID,
                             const void *pInput, void *pOutput)
{
    return J2534Global::obj()->getInterface()->PassThruIoctl(ChannelID, IoctlID, pInput, pOutput);
}
/*
{


  //PassThruIoctl IoctlID = 00000002
  //PassThruIoctl IoctlID = 00000005


  // TODO: check IoctlID?
  // 3 READ_VBATT?
  // 2 CONFIG_SET?
  printf("PassThruIoctl IoctlID = %08x\n", IoctlID);
  if (IoctlID == READ_VBATT) {
      unsigned int voltageLevel = 14400;
      memcpy(pOutput, &voltageLevel, sizeof(unsigned int));
    //unsigned int *voltageLevel = (unsigned int*)pOutput;
    //*voltageLevel = 14400; // The units will be in milli-volts and will be rounded to the nearest tenth of a volt.
  }


  if(IoctlID == SET_CONFIG)
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

  return ExJ2534OIPSocks::getInstance()->getJ2534Dev()->m_PassThruIoctl(ChannelID, IoctlID, pInput, pOutput);

  //return STATUS_NOERROR;
}*/

J2534_EXPORT  _PassThruDisconnect(uint32_t ChannelID)
{
    return J2534Global::obj()->getInterface()->PassThruDisconnect(ChannelID);
}
/*{

    auto ex_j2534_socks = ExJ2534OIPSocks::getInstance();
    printf("PassThruDisconnect\n");
    ex_j2534_socks->getJ2534Dev()->m_PassThruDisconnect(ChannelID);
    //sleep(1);
    std::this_thread::sleep_for(std::chrono::microseconds(1000 * 1000));
    ex_j2534_socks->close_client();
    ex_j2534_socks->reset_buffer();


    auto ex_j2534_socks = ExJ2534OIPSocks::getInstance();
    printf("PassThruDisconnect\n");
    ex_j2534_socks->getJ2534Dev()->m_PassThruDisconnect(ChannelID);

    return STATUS_NOERROR;
}*/

J2534_EXPORT  _PassThruClose(uint32_t DeviceID)
{
    return J2534Global::obj()->getInterface()->PassThruClose(DeviceID);
}
//{
    /*
    auto ex_j2534_socks = ExJ2534OIPSocks::getInstance();
    printf("PassThruClose\n");
    ex_j2534_socks->getJ2534Dev()->m_PassThruClose(DeviceID);
    //sleep(1);
    std::this_thread::sleep_for(std::chrono::microseconds(1000 * 1000));
    ex_j2534_socks->reset_buffer();
    ex_j2534_socks->close_client();
    ex_j2534_socks->close_server();
*/
    /*shutdown(DeviceID, SD_BOTH);
      closesocket(DeviceID);
      WSACleanup();*/

    //auto ex_j2534_socks = ExJ2534OIPSocks::getInstance();
    //printf("PassThruClose\n");
    //ex_j2534_socks->getJ2534Dev()->m_PassThruClose(DeviceID);
    //return STATUS_NOERROR;
//}

J2534_EXPORT  _PassThruStartPeriodicMsg(unsigned long ChannelID,
                                        const PASSTHRU_MSG *pMsg,
                                        unsigned long *pMsgID,
                                        unsigned long TimeInterval)
{
    return J2534Global::obj()->getInterface()->PassThruStartPeriodicMsg(ChannelID, pMsg, pMsgID, TimeInterval);
}
/*
{
  printf("PassThruStartPeriodicMsg ");
  printf("ChannelID %08x  TimeInterval = %d\n", ChannelID , TimeInterval);

  printf("MSG2PERIOD protocolid %08x rxstatus %08x txflags %08x timestamp %d extra index %08x datasize %d \n [MSG]: ", pMsg[0].ProtocolID, pMsg[0].RxStatus, pMsg[0].TxFlags,
          pMsg[0].Timestamp, pMsg[0].ExtraDataIndex, pMsg[0].DataSize);
  for(int y = 0; y < pMsg[0].DataSize; y++)
  {
      printf("%02x ", pMsg[0].Data[y]);
  }
  printf("\n");

  //return STATUS_NOERROR;

  return ExJ2534OIPSocks::getInstance()->getJ2534Dev()->m_PassThruStartPeriodicMsg(ChannelID, (void *)pMsg, pMsgID, TimeInterval);

  //return ERR_NOT_SUPPORTED;
}*/

J2534_EXPORT  _PassThruStopPeriodicMsg(unsigned long ChannelID, unsigned long MsgID)
{
    return J2534Global::obj()->getInterface()->PassThruStopPeriodicMsg(ChannelID, MsgID);
}
/*{
  printf("PassThruStopPeriodicMsg\n");
  return ExJ2534OIPSocks::getInstance()->getJ2534Dev()->m_PassThruStopPeriodicMsg(ChannelID, MsgID);
  //return STATUS_NOERROR;
}*/

J2534_EXPORT  _PassThruSetProgrammingVoltage(uint32_t DeviceID, uint32_t Pin, uint32_t Voltage)
{
    printf("_PassThruSetProgrammingVoltage");
    return J2534Global::obj()->getInterface()->PassThruSetProgrammingVoltage(DeviceID, Pin, Voltage);
}
/*{
  printf("PassThruSetProgrammingVoltage\n");
  EXDBG_LOG << "PassThruSetProgrammingVoltage";

  return ERR_NOT_SUPPORTED;
}*/


/*
Module Short Name      : VendorName='Xplatforms' ModuleName='Xplatforms PassThru Over IP' J2534 Standard Version='4.04'
Module Vendor Name     : Softing Automotive Electronics GmbH
Module HwName          : Xplatforms PassThru Over IP
Module SerialNumber    : 0
Module FW Name         : VeCom
Module FW-Version      : 2.20.42
Module FW-Date         : 18.7.2017 (Calendar week 29)
Module PDU-API SW Name : Softing D-PDU API for VCIs
Module PDU-API Version : 1.20.42
Module PDU-API SW Date : 18.7.2017 (Calendar week 29)

*/
J2534_EXPORT  _PassThruReadVersion(unsigned long DeviceID, char * pFirmwareVersion, char *pDllVersion, char *pApiVersion)
{
    return J2534Global::obj()->getInterface()->PassThruReadVersion(DeviceID, pFirmwareVersion, pDllVersion, pApiVersion);
}
/*
{
  printf("PassThruReadVersion\n");
  EXDBG_LOG << "PassThruReadVersion";

  //ExJ2534OIPSocks::getInstance()->getJ2534Dev()->m_PassThruReadVersion(DeviceID, pApiVersion, pDllVersion, pFirmwareVersion);
  //printf("[PassThruReadVersion] DLLORIG: dev %d api %s dll %s firm %s\n", DeviceID, pApiVersion, pDllVersion, pFirmwareVersion);
  //return STATUS_NOERROR;


  char FirmwareVersion[80] = {0};
  char DllVersion[80] = {0};
  char ApiVersion[80] = {0};

  //OpenPort:
  //api:  1.17.4877
  //dll:  1.01.4341 Jul 10 2014 14:16:35
  //firm: 04.04

  memcpy(ApiVersion, "2.23.9000", 9);
  memcpy(DllVersion, "1.0.001 Feb 28 2023 12:46:00", 28);
  memcpy(FirmwareVersion, "04.04", 5);

  memcpy(pApiVersion, ApiVersion, 80);
  memcpy(pDllVersion, DllVersion, 80);
  memcpy(pFirmwareVersion, FirmwareVersion, 80);

  printf("[PassThruReadVersion] XPL: dev %d api %s dll %s firm %s\n", DeviceID, pApiVersion, pDllVersion, pFirmwareVersion);

  return STATUS_NOERROR;
}*/

J2534_EXPORT  _PassThruGetLastError(char *pErrorDescription)
{
    return J2534Global::obj()->getInterface()->PassThruGetLastError(pErrorDescription);
}

J2534_EXPORT  _PassThruLoadFirmware(void)
{
    return J2534Global::obj()->getInterface()->PassThruLoadFirmware();
}

J2534_EXPORT  _PassThruRecoverFirmware(void)
{
    return J2534Global::obj()->getInterface()->PassThruRecoverFirmware();
}

J2534_EXPORT  _PassThruReadIPSetup(unsigned long DeviceID, char *host_name, char *ip_addr, char *subnet_mask, char *gateway, char *dhcp_addr)
{
    return J2534Global::obj()->getInterface()->PassThruReadIPSetup(DeviceID, host_name, ip_addr, subnet_mask, gateway, dhcp_addr);
}

J2534_EXPORT  _PassThruWriteIPSetup(unsigned long DeviceID, char *host_name, char *ip_addr, char *subnet_mask, char *gateway, char *dhcp_addr)
{
    return J2534Global::obj()->getInterface()->PassThruWriteIPSetup(DeviceID, host_name, ip_addr, subnet_mask, gateway, dhcp_addr);
}

J2534_EXPORT  _PassThruGetNextCarDAQ(char **name, unsigned long *version, char **addr)
{
    return J2534Global::obj()->getInterface()->PassThruGetNextCarDAQ(name, version, addr);
}

J2534_EXPORT  _PassThruReadPCSetup(char *host_name, char *ip_addr)
{
    return J2534Global::obj()->getInterface()->PassThruReadPCSetup(host_name, ip_addr);
}

J2534_EXPORT  _PassThruGetPointer(long vb_pointer)
{
    return J2534Global::obj()->getInterface()->PassThruGetPointer(vb_pointer);
}

J2534_EXPORT  _PassThruExConfigureWiFi ( void )
{
    return J2534Global::obj()->getInterface()->PassThruExConfigureWiFi();
}
J2534_EXPORT  _PassThruExDeviceWatchdog ( void )
{
    return J2534Global::obj()->getInterface()->PassThruExDeviceWatchdog();
}

J2534_EXPORT  _PassThruExDownloadCypheredFlashData ( void )
{
    return J2534Global::obj()->getInterface()->PassThruExDownloadCypheredFlashData();
}

J2534_EXPORT  _PassThruExEraseFlash ( void )
{
    return J2534Global::obj()->getInterface()->PassThruExEraseFlash();
}

J2534_EXPORT  _PassThruExInitiateCypheredFlashDownload ( void )
{
    return J2534Global::obj()->getInterface()->PassThruExInitiateCypheredFlashDownload();
}

J2534_EXPORT  _PassThruExReadFlash ( void )
{
    return J2534Global::obj()->getInterface()->PassThruExReadFlash();
}

J2534_EXPORT  _PassThruExResetFlash ( void )
{
    return J2534Global::obj()->getInterface()->PassThruExResetFlash();
}

J2534_EXPORT  _PassThruExRunSelfTest ( void )
{
    return J2534Global::obj()->getInterface()->PassThruExRunSelfTest();
}

J2534_EXPORT  _PassThruExWriteFlash ( void )
{
    return J2534Global::obj()->getInterface()->PassThruExWriteFlash();
}

J2534_EXPORT  _PassThruScanForDevices(unsigned long *pDeviceCount)
{
    return J2534Global::obj()->getInterface()->PassThruScanForDevices(pDeviceCount);
}
/*{
    printf ( "PassThruScanForDevices\n" );
    EXDBG_LOG << "PassThruScanForDevices";

    if(pDeviceCount == NULL) {
        return ERR_NULL_PARAMETER;
      }
    //_called = false;
    *pDeviceCount = 1;
    //__asm int 3
    return STATUS_NOERROR;
}*/



J2534_EXPORT  _PassThruGetNextDevice(SDEVICE *psDevice)
{
    return J2534Global::obj()->getInterface()->PassThruGetNextDevice(psDevice);
}
/*{
    printf ( "PassThruGetNextDevice\n" );
    EXDBG_LOG << "PassThruGetNextDevice";


    //strcpy(j2534_device_list[i].DeviceName, sThingName);
    //j2534_device_list[i].DeviceAvailable = DEVICE_AVAILABLE;
    //j2534_device_list[i].DeviceDLLFWStatus = DEVICE_DLL_FW_COMPATIBLE;
    //j2534_device_list[i].DeviceConnectMedia = DEVICE_CONN_WIRELESS;
    //j2534_device_list[i].DeviceConnectSpeed = 100000;
    //j2534_device_list[i].DeviceSignalQuality = 100;
    //j2534_device_list[i].DeviceSignalStrength = 100;


    char DevName[80] = {0};

    sprintf(DevName,"%s", "VI");

    SDEVICE sim_dev;
    memcpy(sim_dev.DeviceName, DevName, 80);
    sim_dev.DeviceAvailable = DEVICE_AVAILABLE;
    sim_dev.DeviceDLLFWStatus = DEVICE_DLL_OR_FW_NOT_COMPATIBLE;
    sim_dev.DeviceConnectMedia = DEVICE_CONN_WIRELESS;
    sim_dev.DeviceConnectSpeed = 500000;
    sim_dev.DeviceSignalQuality = 100;
    sim_dev.DeviceSignalStrength = 100;

    memcpy(psDevice, &sim_dev, sizeof(SDEVICE));    
    _called++;


    //if(_called)return ERR_EXCEEDED_LIMIT;
    //_called = true;

    //__asm int 3
    return STATUS_NOERROR;
}*/

J2534_EXPORT _PassThruSelect(SCHANNELSET *ChannelSetPtr, unsigned long SelectType, unsigned long Timeout)
{
    return J2534Global::obj()->getInterface()->PassThruSelect(ChannelSetPtr, SelectType, Timeout);
}

J2534_EXPORT _PassThruReadDetails(unsigned long* pName)
{
    return J2534Global::obj()->getInterface()->PassThruReadDetails(pName);
}


