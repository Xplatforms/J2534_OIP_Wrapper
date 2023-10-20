#include "exj2534device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


ExJ2534Device::ExJ2534Device()
{
    this->m_dllHandle = NULL;
    this->m_dll_path = NULL;
}

ExJ2534Device::~ExJ2534Device()
{

}

void ExJ2534Device::loadJ2534Dll(const char * dll_path)
{
    auto s = strlen(dll_path);
    this->m_dll_path = new char[s+1];
    memset(this->m_dll_path,0,s+1);
    memcpy(this->m_dll_path, dll_path, s);

    printf("[ExJ2534Device] J2534 DLL Selected: %s\n", this->m_dll_path);
    this->J2534WrapperInit();
}

void ExJ2534Device::J2534WrapperInit()
{
    if(this->m_dll_path == NULL)
    {
        printf("[ExJ2534Device] DLL Not Selected!\n");
        return;
    }

    //auto wpath = QDir::toNativeSeparators(this->m_dll_path).toStdWString();
    this->m_dllHandle = LoadLibraryExA(this->m_dll_path, 0, LOAD_WITH_ALTERED_SEARCH_PATH);

    if(this->m_dllHandle == NULL)
    {
        printf("[ExJ2534Device] Can't load D-PDU Dll -> %s\n", this->m_dll_path);
        return;
    }

    this->m_PassThruOpen                    =(PTOPEN)                       GetProcAddress(this->m_dllHandle,"PassThruOpen");
    this->m_PassThruClose                   =(PTCLOSE)                      GetProcAddress(this->m_dllHandle,"PassThruClose");
    this->m_PassThruConnect                 =(PTCONNECT)                    GetProcAddress(this->m_dllHandle,"PassThruConnect");
    this->m_PassThruDisconnect              =(PTDISCONNECT)                 GetProcAddress(this->m_dllHandle,"PassThruDisconnect");
    this->m_PassThruReadMsgs                =(PTREADMSGS)                   GetProcAddress(this->m_dllHandle,"PassThruReadMsgs");
    this->m_PassThruWriteMsgs               =(PTWRITEMSGS)                  GetProcAddress(this->m_dllHandle,"PassThruWriteMsgs");
    this->m_PassThruStartPeriodicMsg        =(PTSTARTPERIODICMSG)           GetProcAddress(this->m_dllHandle,"PassThruStartPeriodicMsg");
    this->m_PassThruStopPeriodicMsg         =(PTSTOPPERIODICMSG)            GetProcAddress(this->m_dllHandle,"PassThruStopPeriodicMsg");
    this->m_PassThruStartMsgFilter          =(PTSTARTMSGFILTER)             GetProcAddress(this->m_dllHandle,"PassThruStartMsgFilter");
    this->m_PassThruStopMsgFilter           =(PTSTOPMSGFILTER)              GetProcAddress(this->m_dllHandle,"PassThruStopMsgFilter");
    this->m_PassThruSetProgrammingVoltage   =(PTSETPROGRAMMINGVOLTAGE)      GetProcAddress(this->m_dllHandle,"PassThruSetProgrammingVoltage");
    this->m_PassThruReadVersion             =(PTREADVERSION)                GetProcAddress(this->m_dllHandle,"PassThruReadVersion");
    this->m_PassThruGetLastError            =(PTGETLASTERROR)               GetProcAddress(this->m_dllHandle,"PassThruGetLastError");
    this->m_PassThruIoctl                   =(PTIOCTL)                      GetProcAddress(this->m_dllHandle,"PassThruIoctl");
}
