#include <Windows.h>
#include "ExDbg.h"
#include "j2534.h"
#include "exj2534oipsocks.h"
#include "j2534dllwrapper.h"
#include "j2534simplelogger.h"
#include "j2534emulator.h"
#include "j2534cannelloniclient.h"
#include "j2534global.h"

void init_console()
{
  AllocConsole();
  freopen("CONIN$", "r", stdin);
  freopen("CONOUT$", "w", stdout);
  freopen("CONOUT$", "w", stderr);
}


/*
cd C:\Program Files (x86)\Softing\Diagnostic Tool Set 8\8.14\bin\
C:\Program Files (x86)\Softing\D-PDU API\11.26.072\VeCom\vecomw32fwj25proc.exe /s"j2534-tcp" /p9804 /l"C:\j2534-tcp\j2534-tcp.dll" /v2 /k0 /t"C:\ProgramData\Softing\D-PDU API\11.26.072\d-pduapi.ini"
*/

std::string getClassNameFromRegistry()
{
  HKEY hKey;
  char className[MAX_PATH];
  DWORD classNameSize = sizeof(className);

  if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\PassThruSupport.04.04\\XplatformsPassThruOverIP", 0, KEY_READ, &hKey) == ERROR_SUCCESS ||
      RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\PassThruSupport.05.00\\XplatformsPassThruOverIP", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
      if (RegQueryValueExA(hKey, "ExSelectedInterface", NULL, NULL, (LPBYTE)className, &classNameSize) == ERROR_SUCCESS) {
          RegCloseKey(hKey);
          return std::string(className);
      }
      RegCloseKey(hKey);
  }

  return "";
}

std::wstring getWrappedDllFromRegistry()
{
    HKEY hKey;
    wchar_t className[MAX_PATH];
    DWORD classNameSize = sizeof(className);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\PassThruSupport.04.04\\XplatformsPassThruOverIP", 0, KEY_READ, &hKey) == ERROR_SUCCESS ||
        RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\PassThruSupport.05.00\\XplatformsPassThruOverIP", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hKey, L"WrappedDll", NULL, NULL, (LPBYTE)className, &classNameSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return std::wstring(className);
        }
        RegCloseKey(hKey);
    }

    return L"";
}


void ProcessIdToName(DWORD processId)
{
    HANDLE handle = OpenProcess(
        PROCESS_QUERY_LIMITED_INFORMATION,
        FALSE,
        processId /* This is the PID, you can find one from windows task manager */
    );
    if (handle)
    {
        DWORD buffSize = 1024;
        CHAR buffer[1024];
        if (QueryFullProcessImageNameA(handle, 0, buffer, &buffSize))
        {
            printf("ProcessIdToName %s\n", buffer);
        }
        else
        {
            printf("Error GetModuleBaseNameA : %lu", GetLastError());
        }
        CloseHandle(handle);
    }
    else
    {
        printf("Error OpenProcess : %lu", GetLastError());
    }
}

BOOL WINAPI DllMain(HINSTANCE hInstDll, DWORD dwReason, LPVOID lpvReserved) {
  switch (dwReason) {
    case DLL_PROCESS_ATTACH:
    {
      init_console();

      char lpFilename[52600] = {0};
      GetModuleFileNameA((HMODULE)hInstDll, (LPSTR)&lpFilename, 52600 );
      {
          printf("LoadModule: %08x %s\n",lpvReserved, lpFilename);
      }

      uint32_t pid = GetCurrentProcessId();
      memset(lpFilename, 0, 52600);
      ProcessIdToName(pid);

      EXDBG_LOG << lpFilename << EXDBG_END;

      auto factory = J2534Global::obj()->factory();

      //auto simple_loger = [](){return new ExJ2534SimpleLogger();};
      factory->registerGenerator("ExJ2534SimpleLogger", [](){return new ExJ2534SimpleLogger();});
      factory->registerGenerator("J2534DllWrapper", [] (){ return new J2534DllWrapper(); } );
      factory->registerGenerator("J2534Emulator", [] (){ return new J2534Emulator(); });
      factory->registerGenerator("J2534Cannelloni", [] (){ return new J2534CannelloniClient(); });


      auto className = getClassNameFromRegistry();
      if(!className.empty())
      {
          printf("SELETED INTERFACE %s\n", className.c_str());
          auto j2534 = factory->create(className);
          if(className.compare("J2534DllWrapper") == 0)reinterpret_cast<J2534DllWrapper*>(j2534)->loadDll(getWrappedDllFromRegistry().c_str());
          if(j2534 != nullptr)J2534Global::obj()->setInterface(j2534);
      }
      else
      {
          printf("DEFAULT INTERFACE NOT SET! USING SimpleLogger!");
      }

      /*
      auto dll_wrapper = new J2534DllWrapper();
      dll_wrapper->loadDll(L"C:\\WINDOWS\\SysWOW64\\op20pt32.dll");
      */
      /*
      auto ex_j2534_socks = ExJ2534OIPSocks::getInstance();
      ex_j2534_socks->init_server();
      ex_j2534_socks->init_threads();

      ex_j2534_socks->getJ2534Dev()->loadJ2534Dll("C:\\WINDOWS\\SysWOW64\\op20pt32.dll");
      */
      printf("DLL_PROCESS_ATTACH!\n");
      printf("LOADED INTERFACE %s\n", J2534Global::obj()->getInterface()->getIName().c_str() );
      break;
    }
    case DLL_THREAD_ATTACH:
    {
      printf("DLL_THREAD_ATTACH\n");
      printf("Current Intrface -> %s\n", J2534Global::obj()->getInterface()->getIName().c_str() );


      break;
    }
    case DLL_PROCESS_DETACH: {
      //ExJ2534OIPSocks::getInstance()->close_client();      
      printf("DLL_PROCESS_DETACH\n");
      break;
    }
    case DLL_THREAD_DETACH: {
      printf("DLL_THREAD_DETACH\n");
      break;
    }
  }
  return TRUE;
}
