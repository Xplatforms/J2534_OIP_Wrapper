#include "exj2534oipsocks.h"

#include <stdint.h>
#include <winsock2.h>
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <chrono>
#include <thread>


ExJ2534OIPSocks * ExJ2534OIPSocks::_global_ExJ2534OIPSocks_singletone = NULL;

ExJ2534OIPSocks * ExJ2534OIPSocks::getInstance()
{
    if(ExJ2534OIPSocks::_global_ExJ2534OIPSocks_singletone == NULL)
    {
        ExJ2534OIPSocks::_global_ExJ2534OIPSocks_singletone = new ExJ2534OIPSocks();
    }
    return ExJ2534OIPSocks::_global_ExJ2534OIPSocks_singletone;
}


ExJ2534OIPSocks::ExJ2534OIPSocks()
{
    this->m_J2534_dev = new ExJ2534Device();
    memset(this->m_recv_buffer, 0, sizeof(PASSTHRU_MSG)*100000);
    this->m_recv_buffer_mutex = NULL;
    this->m_recv_buffer_size = 0;
    this->m_client_fd = -1;
    this->m_server_fd = -1;
    this->m_recv_buffer_mutex = CreateMutex(NULL, FALSE, NULL);
}

ExJ2534OIPSocks::~ExJ2534OIPSocks()
{

}

void ExJ2534OIPSocks::init_server()
{
    WSADATA wsa;
    struct sockaddr_in server_address;
    // wsa startup
    WSAStartup(MAKEWORD(2,2), &wsa);
    // server socket
    this->m_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    // server address
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    // bind
    char yes = 1;
    setsockopt(this->m_server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    bind(this->m_server_fd, (struct sockaddr*)&server_address, sizeof(server_address));
    // listen
    listen(this->m_server_fd, SOMAXCONN);
    printf("listening...\n");
}

void ExJ2534OIPSocks::init_threads()
{
  HANDLE accept_handle = CreateThread(NULL, 0, &ExJ2534OIPSocks::accept_thread, NULL, 0, NULL);
  if (!accept_handle)
  {
    printf("failed to create accept thread\n");
  }
  HANDLE recv_handle = CreateThread(NULL, 0, &ExJ2534OIPSocks::recv_thread, NULL, 0, NULL);
  if (!recv_handle)
  {
    printf("failed to create accept thread\n");
  }
}


void ExJ2534OIPSocks::close_client()
{
  if (this->m_client_fd != -1)
  {
    printf("closing client\n");
    shutdown(this->m_client_fd, SD_BOTH);
    closesocket(this->m_client_fd);
    this->m_client_fd = -1;
  }
}

void ExJ2534OIPSocks::close_server()
{
  if (this->m_server_fd != -1) {
    printf("closing server\n");
    shutdown(this->m_server_fd, SD_BOTH);
    closesocket(this->m_server_fd);
    this->m_server_fd = -1;
  }
}

void ExJ2534OIPSocks::reset_buffer()
{
  WaitForSingleObject(this->m_recv_buffer_mutex, INFINITE);
  this->m_recv_buffer_size = 0;
  ReleaseMutex(this->m_recv_buffer_mutex);
}

unsigned long ExJ2534OIPSocks::GetTime()
{
  FILETIME t;
  GetSystemTimeAsFileTime(&t);
  return t.dwLowDateTime;
}


DWORD WINAPI ExJ2534OIPSocks::accept_thread(void* data)
{
    auto ex_j2534_sock = ExJ2534OIPSocks::getInstance();
    for (;;)
    {
        // client address
        struct sockaddr_in client_address;
        int client_address_size;
        client_address_size = sizeof(struct sockaddr_in);
        memset(&client_address, 0, client_address_size);
        // accept client
        //printf("waiting to accept...\n");
        ex_j2534_sock->m_client_fd = accept(ex_j2534_sock->m_client_fd, (struct sockaddr*)&client_address, &client_address_size);

        if(ex_j2534_sock->m_client_fd == -1)
        {
            //printf("failed to accept\n");
            continue;
        }

        printf("accepted...\n");
        // set sockopt
        char yes = 1;
        setsockopt(ex_j2534_sock->m_client_fd, SOL_SOCKET, TCP_NODELAY, &yes, sizeof(yes));
        // reset buffer
        ex_j2534_sock->reset_buffer();
    }
    return 0;
}

DWORD WINAPI ExJ2534OIPSocks::recv_thread(void* data)
{
    auto ex_j2534_sock = ExJ2534OIPSocks::getInstance();
  char buffer[8192];
  for (;;) {
    // sleep
    //usleep(RECV_THREAD_SLEEP_MS * 1000);
    std::this_thread::sleep_for(std::chrono::microseconds(RECV_THREAD_SLEEP_MS * 1000));
    // client not connected yet;
    if (ex_j2534_sock->m_client_fd == -1)
    {
      continue;
    }
    // receive length
    uint32_t networkMsgDataSize;
    if (recv(ex_j2534_sock->m_client_fd, (char*)&networkMsgDataSize, sizeof(uint32_t), MSG_WAITALL) != sizeof(uint32_t)) {
      printf("recv1 failed errno = %d\n", WSAGetLastError());
      continue;
    }
    uint32_t hostMsgDataSize = ntohl(networkMsgDataSize);
    // receive payload
    if (recv(ex_j2534_sock->m_client_fd, buffer, hostMsgDataSize, MSG_WAITALL) != hostMsgDataSize) {
      printf("recv2 failed errno = %d\n", WSAGetLastError());
      continue;
    }
    // skip ping frames
    if (buffer[0] == 0x00 &&
        buffer[1] == 0x00 &&
        buffer[2] == 0x00 &&
        buffer[3] == 0x00) {
      printf("got ping frame\n");
      continue;
    }
    // push to buffer
    WaitForSingleObject(ex_j2534_sock->m_recv_buffer_mutex, INFINITE);
    PASSTHRU_MSG *buffer_msg = &ex_j2534_sock->m_recv_buffer[ex_j2534_sock->m_recv_buffer_size];
    memset(buffer_msg, 0, sizeof(PASSTHRU_MSG));
    buffer_msg->ProtocolID = CAN;
    buffer_msg->Timestamp = ex_j2534_sock->GetTime();
    buffer_msg->DataSize = hostMsgDataSize;
    memcpy(buffer_msg->Data, buffer, hostMsgDataSize);
    // log
    printf("recv_thread: buffer_msg->DataSize = %04x\n", buffer_msg->DataSize);
    // increment
    ex_j2534_sock->m_recv_buffer_size += 1;
    // release
    ReleaseMutex(ex_j2534_sock->m_recv_buffer_mutex);
  }
}

