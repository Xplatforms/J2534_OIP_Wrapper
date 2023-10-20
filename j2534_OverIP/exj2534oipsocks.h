#ifndef EXJ2534OIPSOCKS_H
#define EXJ2534OIPSOCKS_H

#include "j2534_OverIP_global.h"
#include "j2534_defs.h"
#include <windows.h>
#include "j2534.h"
#include "exj2534device.h"



class ExJ2534OIPSocks
{

public:

    static ExJ2534OIPSocks * getInstance();

    ExJ2534Device * getJ2534Dev(){return this->m_J2534_dev;}

    void init_server();
    void init_threads();


    void close_client();
    void close_server();
    void reset_buffer();
    unsigned long GetTime();

    static DWORD WINAPI accept_thread(void* data);
    static DWORD WINAPI recv_thread(void* data);


public:
    PASSTHRU_MSG m_recv_buffer[100000];
    HANDLE m_recv_buffer_mutex;
    int m_recv_buffer_size;
    int m_client_fd;
    int m_server_fd;


private:
    explicit ExJ2534OIPSocks();
    ~ExJ2534OIPSocks();
    static ExJ2534OIPSocks *        _global_ExJ2534OIPSocks_singletone;

    ExJ2534Device *                 m_J2534_dev;
};


#endif // EXJ2534OIPSOCKS_H
