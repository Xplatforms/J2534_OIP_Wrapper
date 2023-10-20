#ifndef EXJ2534DEVICE_H
#define EXJ2534DEVICE_H

#include <windows.h>
#include "j2534_dev_typedefs.h"

class ExJ2534Device
{

public:
    explicit ExJ2534Device();
    ~ExJ2534Device();

    void loadJ2534Dll(const char * dll_path);
    //void ExPDUConstruct(QString optionStr = QLatin1String(), void * tagApi = Q_NULLPTR);
    //D_PDU_VERSION_DATA * PDUGetVersion(UNUM32 hMod);

    //void ExJ2534DeviceGetModuleIds();
    //void ExPDUModuleConnect(UNUM32 hMod);
    //void ExPDUModuleDisconnect(UNUM32 hMod);


private:
    void J2534WrapperInit();

    //T_PDU_ERROR setPduParameterUNUM32Value(UNUM32 hMod, UNUM32 hCLL, CHAR8* ShortName, UNUM32 Value);
    //T_PDU_ERROR getPduParameterUNUM32Value(UNUM32 hMod, UNUM32 hCLL, CHAR8* ShortName, UNUM32 *Value);

public:
    PTOPEN                  m_PassThruOpen;
    PTCLOSE                 m_PassThruClose;
    PTCONNECT               m_PassThruConnect;
    PTDISCONNECT            m_PassThruDisconnect;
    PTREADMSGS              m_PassThruReadMsgs;
    PTWRITEMSGS             m_PassThruWriteMsgs;
    PTSTARTPERIODICMSG      m_PassThruStartPeriodicMsg;
    PTSTOPPERIODICMSG       m_PassThruStopPeriodicMsg;
    PTSTARTMSGFILTER        m_PassThruStartMsgFilter;
    PTSTOPMSGFILTER         m_PassThruStopMsgFilter;
    PTSETPROGRAMMINGVOLTAGE m_PassThruSetProgrammingVoltage;
    PTREADVERSION           m_PassThruReadVersion;
    PTGETLASTERROR          m_PassThruGetLastError;
    PTIOCTL                 m_PassThruIoctl;

private:
    HMODULE                 m_dllHandle;
    char *                  m_dll_path;

};

#endif // EXJ2534DEVICE_H
