#ifndef DRIVER_API_H
#define DRIVER_API_H

#include <windows.h>
#include <string>

// J2534 04.04 entry points (same as j2534_dev_typedefs.h, without pulling in
// the PassThru DLL's own export declarations).
typedef long (CALLBACK *PTOPEN)(const void *, unsigned long *);
typedef long (CALLBACK *PTCLOSE)(unsigned long);
typedef long (CALLBACK *PTCONNECT)(unsigned long, unsigned long, unsigned long, unsigned long, unsigned long *);
typedef long (CALLBACK *PTDISCONNECT)(unsigned long);
typedef long (CALLBACK *PTREADMSGS)(unsigned long, void *, unsigned long *, unsigned long);
typedef long (CALLBACK *PTWRITEMSGS)(unsigned long, const void *, unsigned long *, unsigned long);
typedef long (CALLBACK *PTSTARTPERIODICMSG)(unsigned long, void *, unsigned long *, unsigned long);
typedef long (CALLBACK *PTSTOPPERIODICMSG)(unsigned long, unsigned long);
typedef long (CALLBACK *PTSTARTMSGFILTER)(unsigned long, unsigned long, void *, void *, void *, unsigned long *);
typedef long (CALLBACK *PTSTOPMSGFILTER)(unsigned long, unsigned long);
typedef long (CALLBACK *PTSETPROGRAMMINGVOLTAGE)(unsigned long, unsigned long, unsigned long);
typedef long (CALLBACK *PTREADVERSION)(unsigned long, char *, char *, char *);
typedef long (CALLBACK *PTGETLASTERROR)(char *);
typedef long (CALLBACK *PTIOCTL)(unsigned long, unsigned long, const void *, void *);

// The J2534 04.04 functions of a loaded driver DLL. Missing exports stay null.
struct DriverApi
{
    PTOPEN Open = nullptr;
    PTCLOSE Close = nullptr;
    PTCONNECT Connect = nullptr;
    PTDISCONNECT Disconnect = nullptr;
    PTREADMSGS ReadMsgs = nullptr;
    PTWRITEMSGS WriteMsgs = nullptr;
    PTSTARTPERIODICMSG StartPeriodicMsg = nullptr;
    PTSTOPPERIODICMSG StopPeriodicMsg = nullptr;
    PTSTARTMSGFILTER StartMsgFilter = nullptr;
    PTSTOPMSGFILTER StopMsgFilter = nullptr;
    PTSETPROGRAMMINGVOLTAGE SetProgrammingVoltage = nullptr;
    PTREADVERSION ReadVersion = nullptr;
    PTGETLASTERROR GetLastError = nullptr;
    PTIOCTL Ioctl = nullptr;

    // Loads the driver and resolves its exports. Returns false and sets
    // `error` if the DLL can't be loaded or lacks the core functions.
    bool load(const std::wstring &path, std::string *error);
    void unload();

    ~DriverApi() { unload(); }

private:
    HMODULE m_module = nullptr;
};

#endif // DRIVER_API_H
