#ifndef J2534_H
#define J2534_H

#include "j2534_OverIP_global.h"
#include "PassThruStruct.h"
#include "PassThruSpecial.h"


#define PASS_THRU_WRITE_MSGS_SLEEP_MS 0
#define PASS_THRU_READ_MSGS_SLEEP_MS 0
#define RECV_THREAD_SLEEP_MS 0
#define PORT 30000

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#pragma pack(push, 1)  // Ensure no padding

typedef struct {
  char DeviceName[80];
  unsigned long DeviceAvailable;
  unsigned long DeviceDLLFWStatus;
  unsigned long DeviceConnectMedia;
  unsigned long DeviceConnectSpeed;
  unsigned long DeviceSignalQuality;
  unsigned long DeviceSignalStrength;
} SDEVICE;

typedef struct {
  unsigned long ChannelCount;     /* number of ChannelList elements */
  unsigned long ChannelThreshold; /* minimum number of channels that must have messages */
  unsigned long *ChannelList;     /* pointer to an array of Channel IDs to be monitored */
} SCHANNELSET;

typedef struct _SPARAM
{
  uint32_t Parameter;
  uint32_t Value;
  uint32_t Supported;
}SPARAM;

typedef struct _SPARAM_LIST
{
  unsigned long NumOfParameters;
  SPARAM * SParamPtr;
}SPARAM_LIST;

#pragma pack(pop)


J2534_EXPORT  _PassThruOpen(const void *pName, unsigned long *pDeviceID);
J2534_EXPORT  _PassThruClose(unsigned long DeviceID);
J2534_EXPORT  _PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID, unsigned long Flags, unsigned long Baudrate, unsigned long *pChannelID);
J2534_EXPORT  _PassThruDisconnect(unsigned long ChannelID);
J2534_EXPORT  _PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout);
J2534_EXPORT  _PassThruWriteMsgs(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout);
J2534_EXPORT  _PassThruStartPeriodicMsg(unsigned long ChannelID,const PASSTHRU_MSG *pMsg, unsigned long *pMsgID, unsigned long TimeInterval);
J2534_EXPORT  _PassThruStopPeriodicMsg(unsigned long ChannelID, unsigned long MsgID);
J2534_EXPORT  _PassThruStartMsgFilter(unsigned long ChannelID, unsigned long FilterType, const PASSTHRU_MSG *pMaskMsg, const PASSTHRU_MSG *pPatternMsg, const PASSTHRU_MSG *pFlowControlMsg, unsigned long *pMsgID);
J2534_EXPORT  _PassThruStopMsgFilter(unsigned long ChannelID, unsigned long MsgID);
J2534_EXPORT  _PassThruSetProgrammingVoltage(unsigned long DeviceID, unsigned long Pin, unsigned long Voltage);
J2534_EXPORT  _PassThruReadVersion(unsigned long DeviceID, char * pFirmwareVersion, char *pDllVersion, char *pApiVersion);
J2534_EXPORT  _PassThruGetLastError(char *pErrorDescription);
J2534_EXPORT  _PassThruIoctl(unsigned long ChannelID, unsigned long IoctlID, const void *pInput, void *pOutput);

///DREW Tech extra functions
///
J2534_EXPORT  _PassThruExConfigureWiFi ( void );
J2534_EXPORT  _PassThruExDeviceWatchdog ( void );
J2534_EXPORT  _PassThruExDownloadCypheredFlashData ( void );
J2534_EXPORT  _PassThruExEraseFlash ( void );
J2534_EXPORT  _PassThruExInitiateCypheredFlashDownload ( void );
J2534_EXPORT  _PassThruExReadFlash ( void );
J2534_EXPORT  _PassThruExResetFlash ( void );
J2534_EXPORT  _PassThruExRunSelfTest ( void );
J2534_EXPORT  _PassThruExWriteFlash ( void );

J2534_EXPORT  _PassThruLoadFirmware(void);
J2534_EXPORT  _PassThruRecoverFirmware(void);
J2534_EXPORT  _PassThruReadIPSetup(unsigned long DeviceID, char *host_name, char *ip_addr, char *subnet_mask, char *gateway, char *dhcp_addr);
J2534_EXPORT  _PassThruWriteIPSetup(unsigned long DeviceID, char *host_name, char *ip_addr, char *subnet_mask, char *gateway, char *dhcp_addr);
J2534_EXPORT  _PassThruReadPCSetup(char *host_name, char *ip_addr);
J2534_EXPORT  _PassThruGetPointer(long vb_pointer);
J2534_EXPORT  _PassThruGetNextCarDAQ(char **name, unsigned long *version, char **addr);

///v05.00
J2534_EXPORT  _PassThruScanForDevices(unsigned long *pDeviceCount);
J2534_EXPORT  _PassThruGetNextDevice(SDEVICE *psDevice);

J2534_EXPORT  _PassThruSelect(SCHANNELSET *ChannelSetPtr, unsigned long SelectType, unsigned long Timeout);
J2534_EXPORT  _PassThruReadDetails(unsigned long* pName);

J2534_EXPORT _PassThruQueueMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs);

#endif // J2534_H
