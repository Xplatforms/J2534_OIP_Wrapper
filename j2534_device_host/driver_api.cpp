#include "driver_api.h"

namespace {

template <typename T>
void resolve(HMODULE module, T &function, const char *name)
{
    function = reinterpret_cast<T>(GetProcAddress(module, name));
}

} // namespace

bool DriverApi::load(const std::wstring &path, std::string *error)
{
    unload();

    m_module = LoadLibraryW(path.c_str());
    if (!m_module) {
        if (error)
            *error = "Can't load the J2534 driver (Windows error " + std::to_string(::GetLastError()) + ")";
        return false;
    }

    resolve(m_module, Open, "PassThruOpen");
    resolve(m_module, Close, "PassThruClose");
    resolve(m_module, Connect, "PassThruConnect");
    resolve(m_module, Disconnect, "PassThruDisconnect");
    resolve(m_module, ReadMsgs, "PassThruReadMsgs");
    resolve(m_module, WriteMsgs, "PassThruWriteMsgs");
    resolve(m_module, StartPeriodicMsg, "PassThruStartPeriodicMsg");
    resolve(m_module, StopPeriodicMsg, "PassThruStopPeriodicMsg");
    resolve(m_module, StartMsgFilter, "PassThruStartMsgFilter");
    resolve(m_module, StopMsgFilter, "PassThruStopMsgFilter");
    resolve(m_module, SetProgrammingVoltage, "PassThruSetProgrammingVoltage");
    resolve(m_module, ReadVersion, "PassThruReadVersion");
    resolve(m_module, GetLastError, "PassThruGetLastError");
    resolve(m_module, Ioctl, "PassThruIoctl");

    if (!Open || !Close || !Connect || !Disconnect || !ReadMsgs || !WriteMsgs) {
        if (error)
            *error = "The DLL is not a J2534 04.04 driver (PassThruOpen/Connect/ReadMsgs/WriteMsgs missing)";
        unload();
        return false;
    }
    return true;
}

void DriverApi::unload()
{
    if (m_module) {
        FreeLibrary(m_module);
        m_module = nullptr;
    }
    Open = nullptr; Close = nullptr; Connect = nullptr; Disconnect = nullptr;
    ReadMsgs = nullptr; WriteMsgs = nullptr; StartPeriodicMsg = nullptr; StopPeriodicMsg = nullptr;
    StartMsgFilter = nullptr; StopMsgFilter = nullptr; SetProgrammingVoltage = nullptr;
    ReadVersion = nullptr; GetLastError = nullptr; Ioctl = nullptr;
}
