#include <winsock2.h>
#include <ws2tcpip.h>  // For InetPton
#pragma comment(lib, "Ws2_32.lib")

#include "ExDbg.h"
#include "cbor_utils.h"
#include "timestamp_util.h"
#include "j2534cannelloniclient.h"


#include <windows.h>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <cstring>
#include <chrono>
#include <cctype>

//20000 125кбит  3/11 пин
//20001 500кбит  6/14 пин - Standart CAN Pins on OBD-II
//20002 500кбит 12/13 пин
enum op_codes {DATA, ACK, NACK};

#pragma pack(push, 1)
struct CannelloniDataPacket {
    uint8_t version;
    uint8_t op_code;
    uint8_t seq_no;
    uint16_t count;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct CannelloniCanFrame {
    uint32_t can_id;
    uint8_t length;
    uint8_t data[MAX_CAN_FRAME_DATA_LEN];
};
#pragma pack(pop)

int J2534CannelloniClient::scanIndex = 0;

J2534CannelloniClient::J2534CannelloniClient()
{
    this->_p_pipe = new ExPipeClient("\\\\.\\pipe\\exj2534_overip_pipe");
    this->_p_pipe->connect();

    this->p_path = ExDbg::getProcessPath();
    this->p_id = ExDbg::crc32(ExDbg::getProcessPath());
    this->p_type = 4; // 4 = J2534CannelloniClient
    this->p_data_type = 2; // 2 = cbor
    this->cannelloniVersion = 1;

    this->getDefaults();

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, 0 /* constructor */);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, (uint32_t)this);

    auto ts = get_timestamp();
    EXDBG_LOG << "[J2534CannelloniClient]: timestamp " << ts << EXDBG_END;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        lastError = "WSAStartup failed";
        lastErrorLong = ERR_DEVICE_NOT_CONNECTED;
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, ts, this->p_path, "J2534CannelloniClient WSAStartup failed!", cbor_utils::cbor_to_data(cb_map_root)});
    }
    else
    {
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, ts, this->p_path, "J2534CannelloniClient WSAStartup. Verbunden!", cbor_utils::cbor_to_data(cb_map_root)});
    }

    cn_cbor_free(cb_map_root);
}

J2534CannelloniClient::~J2534CannelloniClient()
{
    if (channelMap.empty()) WSACleanup();
    //WSACleanup();
}

std::string J2534CannelloniClient::getIName()
{
    char pname[1024] = {0};
    sprintf(pname, "J2534CannelloniClient %08x", (unsigned int)this);
    return std::string(pname);
}

long J2534CannelloniClient::getDefaults()
{
    int retc = 0;
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\PassThruSupport.04.04\\XplatformsPassThruOverIP", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        char host[MAX_PATH] = {0};
        DWORD port = 0;
        DWORD version = 0;
        DWORD portSize = sizeof(port);
        DWORD type = 0;

        DWORD hpSize = sizeof(host);
        if (RegQueryValueExA(hKey, "DefaultHost", NULL, NULL, (LPBYTE)host, &hpSize) == ERROR_SUCCESS)
        {
            this->defaultHost.append(host);
            retc++;
        }
        else
        {
            this->defaultHost.append("127.0.0.1");
        }
        if(RegQueryValueExA(hKey,"DefaultPort", NULL, &type, (LPBYTE)&port, &portSize) == ERROR_SUCCESS)
        {
            this->defaultPort = port;
            retc++;
        }
        else
        {
            this->defaultPort = 20000;
        }

        if(RegQueryValueExA(hKey,"CannelloniDataVersion", NULL, &type, (LPBYTE)&version, &portSize) == ERROR_SUCCESS)
        {
            this->cannelloniVersion = version;
            retc++;
        }
        else
        {
            this->cannelloniVersion = 1;
        }

        RegCloseKey(hKey);
    }
    return retc==3?ERROR_SUCCESS:ERROR_ASSERTION_FAILURE;
}

//nc -u -w 1 172.16.90.1 20000 < <(printf '\x02\x00\x00\x00\x01\x80\x07\xE0\x00\x08\x01\x02\x03\x04\x05\x06\x07\x08')
void J2534CannelloniClient::startReceiveThread(ChannelContext& ctx)
{
    ctx.recvThread = std::thread([this, &ctx](){
        constexpr size_t maxPacketSize = 1500;  // UDP MTU
        uint8_t buffer[maxPacketSize] = {0};

        while (!ctx.stopRequested.load())
        {
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(ctx.sockfd, &readfds);
            timeval timeout{0, 1000};  // 100ms poll

            int ret = select(static_cast<int>(ctx.sockfd + 1), &readfds, nullptr, nullptr, &timeout);
            if (ret > 0 && FD_ISSET(ctx.sockfd, &readfds))
            {
                //sockaddr_in fromAddr;
                int bytesReceived = recv(ctx.sockfd, reinterpret_cast<char*>(buffer), maxPacketSize, 0);
                if (bytesReceived > 0)
                {
                    printf("GOT UDP MSG. LEN %d\n", bytesReceived);
                    if (bytesReceived < static_cast<int>(sizeof(CannelloniDataPacket)))
                    {
                        printf("PACKET to Small \n");
                        break;
                    }

                    printf("sizeof(CannelloniDataPacket) %d\n", sizeof(CannelloniDataPacket));
                    printf("sizeof(CannelloniCanFrame) %d\n", sizeof(CannelloniCanFrame));

                    for(int x = 0; x < bytesReceived; x++ )
                    {
                        printf("%02x ", buffer[x]);
                    }
                    printf("\n");

                    CannelloniDataPacket tmp{};
                    memcpy(&tmp, buffer, sizeof(CannelloniDataPacket));
                    //tmp.count = ntohs(tmp.count);
                    const CannelloniDataPacket* pkt = &tmp;//reinterpret_cast<const CannelloniDataPacket*>(buffer);
                    if (pkt->version != this->cannelloniVersion)
                    {
                        printf("NOT CANNELLONI PACKET \n");
                        break;
                    }
                    else
                    {
                        printf("GOT CannelloniDataPacket ver %d op_code %d seq_no %d count %d\n ", pkt->version, pkt->op_code, pkt->seq_no, pkt->count );
                    }

                    size_t frameOffset = sizeof(CannelloniDataPacket);
                    size_t framesSize = bytesReceived - frameOffset;
                    if (framesSize < static_cast<size_t>(pkt->count) * sizeof(CannelloniCanFrame))
                    {
                        printf("Packet size too small for CannelloniCanFrame. Continue. ");
                        break;
                    }

                    const CannelloniCanFrame* frames = reinterpret_cast<const CannelloniCanFrame*>(buffer + frameOffset);
                    {
                        printf("GOT Cannelloni frames %d\n", pkt->count);

                        std::lock_guard<std::mutex> lock(ctx.msgQueueMutex);
                        for (uint16_t i = 0; i < pkt->count; ++i)
                        {
                            const auto& f = frames[i];
                            PASSTHRU_MSG msg{0};

                            msg.RxStatus = 0;  // No errors
                            msg.ProtocolID = CAN;
                            msg.DataSize = 4 + f.length;
                            if (msg.DataSize > sizeof(msg.Data))
                            {
                                printf("FRAME truncated\n");
                                continue;  // Truncate
                            }

                            uint32_t canId = f.can_id;
                            // J2534 CAN format: Byte 0 = flags (e.g., 0x08 extended), ID in 0-3
                            msg.Data[0] = static_cast<uint8_t>(canId & 0xFF);  // Assume 11-bit for now; add flags if extended
                            std::copy_n(reinterpret_cast<uint8_t*>(&canId) + 1, 3, msg.Data + 1);
                            std::copy_n(f.data, f.length, msg.Data + 4);

                            printf("[GOT FRAME] protid %08x rxstatus %08x txflags %08x tstamp %d e_index %08x datasize %d \n [MSG]: ",
                                   msg.ProtocolID,msg.RxStatus, msg.TxFlags,
                                   msg.Timestamp, msg.ExtraDataIndex, msg.DataSize);
                            for(int y = 0; y < msg.DataSize; y++)
                            {
                                printf("%02x ", msg.Data[y]);
                            }
                            printf("\n");

                            ctx.msgQueue.push(msg);
                        }
                    }
                    ctx.msgQueueCv.notify_one();
                }
            }
        }
    });
}

bool J2534CannelloniClient::popParsedMsg(ChannelContext& ctx, PASSTHRU_MSG& outMsg)
{
    std::unique_lock<std::mutex> lock(ctx.msgQueueMutex);
    if (!ctx.msgQueue.empty())
    {
        outMsg = std::move(ctx.msgQueue.front());
        ctx.msgQueue.pop();
        return true;
    }
    else
    {
        printf("[popParsedMsg] msgQueue empty...\n ");
    }
    return false;
}

long J2534CannelloniClient::PassThruOpen(const void* pName, unsigned long* pDeviceID)
{
    std::lock_guard<std::mutex> lock(globalMutex);

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    if(pName != nullptr)printf("[PassThruOpen] %s\n", pName);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName, ExPipeClient::KEY_PassThruOpen);
    //cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param1, host);
    //cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, *pDeviceID);

    if (pDeviceID == nullptr)
    {
        lastError = "pDeviceID is null";
        lastErrorLong = J2534Err::ERR_NULL_PARAMETER;

        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param1, "");
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, 0);
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" -> ").append(lastError).c_str());
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruOpen", cbor_utils::cbor_to_data(cb_map_root)});
        cn_cbor_free(cb_map_root);

        return lastErrorLong;
    }

    unsigned long port = this->defaultPort;
    const char* host = "127.0.0.1";
    if (pName != nullptr)
    {
        if(isValidHostString(static_cast<const char*>(pName)))host = static_cast<const char*>(pName);
        else host = this->defaultHost.data();
        std::string tmpPort((const char *)pName);
        if(tmpPort.contains("20000"))port = 20000;
        else if(tmpPort.contains("20001"))port = 20001;
        else if(tmpPort.contains("20002"))port = 20002;
    }

    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param1, host);
    //cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, *pDeviceID);

    static constexpr int numPorts = 3;
    static constexpr unsigned long basePort = 20000;

    if (channelMap.size() >= static_cast<size_t>(numPorts))
    {
        lastError = "All virtual devices are already opened";
        lastErrorLong = J2534Err::ERR_DEVICE_IN_USE;

        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param1, "");
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, 0);
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" -> ").append(lastError).c_str());
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruOpen", cbor_utils::cbor_to_data(cb_map_root)});
        cn_cbor_free(cb_map_root);

        return lastErrorLong;
    }

    auto candidate = channelMap.find(port);
    if(candidate != channelMap.end())
    {
        printf("ChannelMap already contains port %d\n", port);
        lastError = "No free ports";
        lastErrorLong = J2534Err::ERR_DEVICE_IN_USE;

        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param1, host);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, port);
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" -> ").append(lastError).c_str());
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruOpen", cbor_utils::cbor_to_data(cb_map_root)});
        cn_cbor_free(cb_map_root);

        return lastErrorLong;
    }

    // Create UDP socket
    SOCKET sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd == INVALID_SOCKET)
    {
        lastError = "Socket creation failed: " + std::to_string(WSAGetLastError());
        lastErrorLong = J2534Err::ERR_DEVICE_NOT_CONNECTED;

        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param1, host);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, port);
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" -> ").append(lastError).c_str());
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruOpen", cbor_utils::cbor_to_data(cb_map_root)});
        cn_cbor_free(cb_map_root);

        return lastErrorLong;
    }

    ChannelContext ctx;
    ctx.sockfd = sockfd;

    ctx.targetAddr.sin_family = AF_INET;
    ctx.targetAddr.sin_port = htons(static_cast<uint16_t>(port));

    if (InetPtonA(AF_INET, host, &ctx.targetAddr.sin_addr) != 1)
    {
        closesocket(sockfd);
        lastError = "Invalid host IP: " + std::string(host);
        lastErrorLong = J2534Err::ERR_INVALID_DEVICE_ID;

        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param1, host);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, port);
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" -> ").append(lastError).c_str());
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruOpen", cbor_utils::cbor_to_data(cb_map_root)});
        cn_cbor_free(cb_map_root);

        return lastErrorLong;
    }

    // Connect the socket to remote for default peer
    if (connect(sockfd, reinterpret_cast<sockaddr*>(&ctx.targetAddr), sizeof(ctx.targetAddr)) == SOCKET_ERROR)
    {
        lastError = "Connect failed: " + std::to_string(WSAGetLastError());
        lastErrorLong = J2534Err::ERR_DEVICE_NOT_CONNECTED;

        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" -> ").append(lastError).c_str());
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruOpen", cbor_utils::cbor_to_data(cb_map_root)});
        cn_cbor_free(cb_map_root);
        closesocket(sockfd);
        return lastErrorLong;
    }

    // Send initial init packet (empty Cannelloni DATA packet) so ESP32 can capture client address
    CannelloniDataPacket initPkt;
    initPkt.version = this->cannelloniVersion;
    initPkt.op_code = 0;  // DATA = 0
    initPkt.seq_no = 0;
    initPkt.count = 0;
    int sent = send(sockfd, reinterpret_cast<const char*>(&initPkt), sizeof(initPkt), 0);
    if (sent != sizeof(initPkt))
    {
        printf("Warning: Failed to send init packet: %d\n", WSAGetLastError());
    }

    channelMap[port] = std::move(ctx);
    ChannelContext& refCtx = channelMap[port];
    startReceiveThread(refCtx);  // Now parses into PASSTHRU_MSG

    *pDeviceID = port;
    lastErrorLong = J2534Err::STATUS_NOERROR;

    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param1, host);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, *pDeviceID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, lastErrorLong);
    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruOpen", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);

    return lastErrorLong;
}

long J2534CannelloniClient::PassThruClose(unsigned long DeviceID)
{
    std::lock_guard<std::mutex> lock(globalMutex);

    long retval = STATUS_NOERROR;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruClose);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, DeviceID);

    auto it = channelMap.find(DeviceID);
    if (it == channelMap.end())
    {
        lastError = "DeviceID not found";
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" -> ").append(lastError).c_str());
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruClose", cbor_utils::cbor_to_data(cb_map_root)});
        cn_cbor_free(cb_map_root);
        return ERR_INVALID_DEVICE_ID;
    }

    ChannelContext& ctx = it->second;

    ctx.stopRequested = true;

    // Wake up select() if needed by sending dummy data to self
    sockaddr_in dummyAddr = ctx.targetAddr;
    dummyAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sendto(ctx.sockfd, "", 1, 0, (sockaddr*)&dummyAddr, sizeof(dummyAddr));

    ctx.stopRequested = true;   
    if (ctx.recvThread.joinable())ctx.recvThread.join();

    closesocket(ctx.sockfd);

    channelMap.erase(it);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, J2534Err::STATUS_NOERROR);
    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruClose", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID, unsigned long Flags, unsigned long Baudrate, unsigned long* pChannelID) {
    std::lock_guard<std::mutex> lock(globalMutex);

    printf("[PassThruConnect] devID: %d ProtID: %d Flags: %d Baud: %d\n", DeviceID, ProtocolID, Flags, Baudrate);

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName, ExPipeClient::KEY_PassThruConnect);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1, DeviceID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, ProtocolID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3, Flags);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param4, Baudrate);

    auto it = channelMap.find(DeviceID);
    if (it == channelMap.end())
    {
        lastError = "Invalid DeviceID";
        lastErrorLong = J2534Err::ERR_INVALID_DEVICE_ID;
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param5, DeviceID);
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" -> ").append(lastError).c_str());
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruConnect", cbor_utils::cbor_to_data(cb_map_root)});
        cn_cbor_free(cb_map_root);
        return lastErrorLong;
    }

    // Only CAN and CAN_PS supported
    /*if (ProtocolID != CAN && ProtocolID != CAN_PS)
    {
        lastError = "Unsupported protocol";
        lastErrorLong = J2534Err::ERR_INVALID_PROTOCOL_ID;
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param5, DeviceID);
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" -> ").append(lastError).c_str());
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruConnect", cbor_utils::cbor_to_data(cb_map_root)});
        cn_cbor_free(cb_map_root);
        return lastErrorLong;
    }*/

    *pChannelID = DeviceID;  // Single channel == device
    lastErrorLong = J2534Err::STATUS_NOERROR;


    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param5, *pChannelID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, lastErrorLong);
    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruConnect", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);

    return lastErrorLong;
}

long J2534CannelloniClient::PassThruDisconnect(unsigned long ChannelID)
{
    long retval = STATUS_NOERROR;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruDisconnect);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, ChannelID);


    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);


    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruConnect", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout)
{
    std::lock_guard<std::mutex> lock(globalMutex);

    long retval = J2534Err::STATUS_NOERROR;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruReadMsgs);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, ChannelID);
    //cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, 0);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3, *pNumMsgs);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param4, Timeout);
    //cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 0);

    auto it = channelMap.find(ChannelID);
    if (it == channelMap.end())
    {
        lastError = "Invalid ChannelID";
        lastErrorLong = J2534Err::ERR_INVALID_CHANNEL_ID;

        printf("[PassThruReadMsgs] Invalid ChannelID %d\n", ChannelID);

        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, 0);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 0);
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" -> ").append(lastError).c_str());
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruReadMsgs", cbor_utils::cbor_to_data(cb_map_root)});
        cn_cbor_free(cb_map_root);

        return lastErrorLong;
    }

    ChannelContext& ctx = it->second;
    unsigned long readCount = 0;
    auto startTime = std::chrono::steady_clock::now();

    //if Timeout nr set, just read msgs and return
    if(Timeout <= 0)
    {
        while (readCount < *pNumMsgs)
        {
            PASSTHRU_MSG msg;
            if (popParsedMsg(ctx, msg))
            {
                if (pMsg) pMsg[readCount] = msg;  // Copy
                ++readCount;
            }
            else
            {
                break;
            }
        }
    }
    else
    {
        while (readCount < *pNumMsgs)
        {
            PASSTHRU_MSG msg;
            auto timeoutLeft = Timeout - std::chrono::duration_cast<std::chrono::milliseconds>(
                                             std::chrono::steady_clock::now() - startTime).count();
            if (timeoutLeft <= 0)
            {
                printf("[PassThruReadMsgs] timeout %d \n", Timeout);
                break;
            }

            if (popParsedMsg(ctx, msg))
            {
                if (pMsg) pMsg[readCount] = msg;  // Copy
                ++readCount;
            }
            else
            {
                //if (ctx.stopRequested.load()) break;
                break;
            }
        }
    }

    *pNumMsgs = readCount;
    lastErrorLong = (readCount > 0 || *pNumMsgs == 0) ? J2534Err::STATUS_NOERROR : J2534Err::ERR_BUFFER_EMPTY;

    cbor_utils::map_put_PASSTHRU_MSG(cb_map_root, ExPipeClient::KEY_Param2, *pNumMsgs, pMsg);
    //cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3, *pNumMsgs);
    //cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param4, Timeout);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 1);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruReadMsgs", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);

    return lastErrorLong;
}

long J2534CannelloniClient::PassThruQueueMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruQueueMsgs);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, ChannelID);
    cbor_utils::map_put_PASSTHRU_MSG(cb_map_root, ExPipeClient::KEY_Param2, *pNumMsgs, pMsg);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3 /* param_1 */, *pNumMsgs);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruQueueMsgs", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruWriteMsgs(unsigned long ChannelID, const PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout) {
    std::lock_guard<std::mutex> lock(globalMutex);

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruWriteMsgs);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, ChannelID);
    //cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, 0);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3, *pNumMsgs);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param4, Timeout);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 0);

    auto it = channelMap.find(ChannelID);
    if (it == channelMap.end())
    {
        lastError = "Invalid ChannelID";
        lastErrorLong = J2534Err::ERR_INVALID_CHANNEL_ID;

        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, 0);
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" -> ").append(lastError).c_str());
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruWriteMsgs", cbor_utils::cbor_to_data(cb_map_root)});
        cn_cbor_free(cb_map_root);
        return lastErrorLong;
    }

    auto& ctx = it->second;
    unsigned long sentMsgs = 0;

    for (unsigned long i = 0; i < *pNumMsgs; ++i)
    {
        const auto& srcMsg = pMsg[i];

        printf("[WRITEMSG] protid %08x rxstatus %08x txflags %08x tstamp %d e_index %08x datasize %d \n [MSG]: ",
               srcMsg.ProtocolID,srcMsg.RxStatus, srcMsg.TxFlags,
               srcMsg.Timestamp, srcMsg.ExtraDataIndex, srcMsg.DataSize);
        for(int y = 0; y < srcMsg.DataSize; y++)
        {
            printf("%02x ", srcMsg.Data[y]);
        }
        printf("\n");

        if ((srcMsg.ProtocolID != CAN && srcMsg.ProtocolID != CAN_PS) || srcMsg.DataSize < 4 || srcMsg.DataSize > 4 + MAX_CAN_FRAME_DATA_LEN)
        {
            lastError = "Invalid CAN msg format";
            printf("[PassThruWriteMsgs] %s\n", lastError.data());
            lastErrorLong = J2534Err::ERR_BUFFER_OVERFLOW;
            cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, 0);
            cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" -> ").append(lastError).c_str());
            this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruWriteMsgs", cbor_utils::cbor_to_data(cb_map_root)});
            cn_cbor_free(cb_map_root);
            break; // Skip invalid messages
        }

        // Build frame
        CannelloniCanFrame frame{};
        uint32_t canId;
        std::copy_n(srcMsg.Data, 4, reinterpret_cast<uint8_t*>(&canId));  // Extract ID
        // Check for extended frame flag
        if (srcMsg.TxFlags & CAN_29BIT_ID)
        {
            canId |= CAN_EFF_FLAG;  // Set extended frame flag
        }
        frame.can_id = canId;  // Convert to network byte order
        frame.length = static_cast<uint8_t>(srcMsg.DataSize - 4);
        std::copy_n(srcMsg.Data + 4, frame.length, frame.data);

        // Build packet (single frame)
        CannelloniDataPacket pkt{this->cannelloniVersion, op_codes::DATA, ctx.seqNo.fetch_add(1), 1};  // version=1, op=DATA, seq, count=1 (network order)

        // Calculate exact buffer size
        size_t packetSize = sizeof(CannelloniDataPacket) + sizeof(frame.can_id) + sizeof(frame.length) + frame.length;
        std::vector<uint8_t> buffer(packetSize);

        // Copy packet header
        std::copy_n(reinterpret_cast<uint8_t*>(&pkt), sizeof(CannelloniDataPacket), buffer.begin());
        // Copy frame header and data
        size_t offset = sizeof(pkt);
        std::copy_n(reinterpret_cast<uint8_t*>(&frame.can_id), sizeof(frame.can_id), buffer.begin() + offset);
        offset += sizeof(frame.can_id);
        buffer[offset] = frame.length;
        offset += sizeof(frame.length);
        std::copy_n(frame.data, frame.length, buffer.begin() + offset);

        // Send packet
        int sent = sendto(ctx.sockfd, reinterpret_cast<const char*>(buffer.data()), buffer.size(), 0,
                          reinterpret_cast<sockaddr*>(&ctx.targetAddr), sizeof(ctx.targetAddr));
        if (sent == static_cast<int>(buffer.size()))
        {
            ++sentMsgs;
        }
        else
        {
            lastError = "Send failed: " + std::to_string(WSAGetLastError());
            lastErrorLong = J2534Err::ERR_FAILED;
            cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" -> ").append(lastError).c_str());
            this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruWriteMsgs", cbor_utils::cbor_to_data(cb_map_root)});
            cn_cbor_free(cb_map_root);
            return lastErrorLong;
        }
    }

    *pNumMsgs = sentMsgs;
    lastErrorLong = (sentMsgs == *pNumMsgs) ? J2534Err::STATUS_NOERROR : J2534Err::ERR_FAILED;

    cbor_utils::map_put_PASSTHRU_MSG(cb_map_root, ExPipeClient::KEY_Param2, *pNumMsgs, pMsg);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3, *pNumMsgs);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param4, Timeout);

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruWriteMsgs", cbor_utils::cbor_to_data(cb_map_root)});

    cn_cbor_free(cb_map_root);

    return lastErrorLong;
}

long J2534CannelloniClient::PassThruStartPeriodicMsg(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pMsgID, unsigned long TimeInterval)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruStartPeriodicMsg);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, ChannelID);
    cbor_utils::map_put_PASSTHRU_MSG(cb_map_root, ExPipeClient::KEY_Param2, 1, pMsg);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3 /* param_1 */, *pMsgID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param4 /* param_1 */, TimeInterval);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruStartPeriodicMsg", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruStopPeriodicMsg(unsigned long ChannelID, unsigned long MsgID)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruStopPeriodicMsg);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, ChannelID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, MsgID);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruStopPeriodicMsg", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruStartMsgFilter(unsigned long ChannelID, unsigned long FilterType, const PASSTHRU_MSG *pMaskMsg, const PASSTHRU_MSG *pPatternMsg, const PASSTHRU_MSG *pFlowControlMsg, unsigned long *pMsgID)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruStartMsgFilter);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, ChannelID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, FilterType);
    cbor_utils::map_put_PASSTHRU_MSG(cb_map_root, ExPipeClient::KEY_Param3, 1, pMaskMsg);
    cbor_utils::map_put_PASSTHRU_MSG(cb_map_root, ExPipeClient::KEY_Param4, 1, pPatternMsg);
    cbor_utils::map_put_PASSTHRU_MSG(cb_map_root, ExPipeClient::KEY_Param5, 1, pFlowControlMsg);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param6, *pMsgID);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruStartMsgFilter", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruStopMsgFilter(unsigned long ChannelID, unsigned long MsgID)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruStopMsgFilter);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, ChannelID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, MsgID);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruStopMsgFilter", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruSetProgrammingVoltage(unsigned long DeviceID, unsigned long Pin, unsigned long Voltage)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruSetProgrammingVoltage);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, DeviceID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, Pin);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, Voltage);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruSetProgrammingVoltage", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruReadVersion(unsigned long DeviceID, char *pFirmwareVersion, char *pDllVersion, char *pApiVersion)
{
    long retval = STATUS_NOERROR;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruReadVersion);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, DeviceID);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param2 /* param_1 */, std::string(pFirmwareVersion).c_str());
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param3 /* param_1 */, std::string(pDllVersion).c_str());
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param4 /* param_1 */, std::string(pApiVersion).c_str());
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 0);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);

    char FirmwareVersion[80] = {0};
    char DllVersion[80] = {0};
    char ApiVersion[80] = {0};

    //OpenPort:
    //api:  1.17.4877
    //dll:  1.01.4341 Jul 10 2014 14:16:35
    //firm: 04.04

    memcpy(ApiVersion, "04.04", 5);
    memcpy(DllVersion, "1.0.001 Feb 28 2023 12:46:00", 28);
    memcpy(FirmwareVersion, "04.04", 5);

    memcpy(pApiVersion, ApiVersion, 80);
    memcpy(pDllVersion, DllVersion, 80);
    memcpy(pFirmwareVersion, FirmwareVersion, 80);

    printf("[PassThruReadVersion] DeviceID %d firmw %s dllver %s api %s\n", pFirmwareVersion, pDllVersion, pApiVersion );

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruReadVersion", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

// In j2534cannelloniclient.cpp (implement/update PassThruGetLastError)
long J2534CannelloniClient::PassThruGetLastError(char* pErrorDescription)
{
    std::lock_guard<std::mutex> lock(globalMutex);  // Thread-safe access to lastError/lastErrorLong

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName, ExPipeClient::KEY_PassThruGetLastError);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param1, lastError.data());  // Log current error

    if (pErrorDescription == nullptr) {
        lastError = "pErrorDescription is null";
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" -> ").append(lastError).c_str());
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruGetLastError", cbor_utils::cbor_to_data(cb_map_root)});
        cn_cbor_free(cb_map_root);
        return J2534Err::ERR_NULL_PARAMETER;
    }

    // Fill buffer with lastError string (up to 80 chars, null-terminated)
    std::strncpy(pErrorDescription, lastError.c_str(), 79);
    pErrorDescription[79] = '\0';

    // Return lastErrorLong (0 for no error, else J2534 error code)
    long retval = (lastErrorLong == 0) ? J2534Err::STATUS_NOERROR : lastErrorLong;

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, lastError.data());
    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruGetLastError", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);

    return retval;
}

long J2534CannelloniClient::PassThruIoctl(unsigned long ChannelID, unsigned long IoctlID, const void *pInput, void *pOutput)
{
    long retval = STATUS_NOERROR;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruIoctl);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1, ChannelID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, IoctlID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3, 0);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param4, 0);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);

    unsigned long data_rate = 500000;
    if(ChannelID == 20000)data_rate = 125000;

    // TODO: check IoctlID?
    // 3 READ_VBATT?
    // 2 CONFIG_SET?
    printf("PassThruIoctl Channel or Device %d IoctlID = %08x\n", ChannelID, IoctlID);

    if (IoctlID == READ_VBATT)
    {
        //unsigned int voltageLevel = 14400;
        //memcpy(pOutput, &voltageLevel, sizeof(unsigned int));

        unsigned int *voltageLevel = (unsigned int*)pOutput;
        *voltageLevel = 12800; // The units will be in milli-volts and will be rounded to the nearest tenth of a volt.
    }
    else if(IoctlID == READ_PROG_VOLTAGE)
    {
        printf("[READ_PROG_VOLTAGE] \n");
        unsigned int *voltageLevel = (unsigned int*)pOutput;
        *voltageLevel = 5200; // The units will be in milli-volts and will be rounded to the nearest tenth of a volt.
    }
    else if(IoctlID == READ_PIN_VOLTAGE)
    {
        printf("[READ_PIN_VOLTAGE] \n");
        unsigned int *voltageLevel = (unsigned int*)pOutput;

        if(pInput == NULL)
        {
            printf("READ_PIN_VOLTAGE with no Input? \n");
            *voltageLevel = 0;
        }
        else
        {
            unsigned int *pin = (unsigned int*)pInput;
            printf("[READ_PIN_VOLTAGE] PIN Number: %d \n", *pin);
            *voltageLevel = 5000;
        }
    }
    else if(IoctlID == SET_CONFIG)
    {
        printf("[SET_CONFIG] \n");
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

        printf("[GET_DEVICE_INFO] SPARAM_LIST NumOfParameters %d\n", param_list->NumOfParameters);
        for(int i = 0; i < param_list->NumOfParameters; i++)
        {
            printf("SParamPtr %d paramter %08x value %08x supported %08x \n", i, param_list->SParamPtr[i].Parameter, param_list->SParamPtr[i].Value, param_list->SParamPtr[i].Supported);
            switch(param_list->SParamPtr[i].Parameter)
            {
            case SERIAL_NUMBER:
                printf("[GET_DEVICE_INFO][SERIAL_NUMBER] \n");
                param_list->SParamPtr[i].Supported = 1;
                param_list->SParamPtr[i].Value = 987654321;
                break;
            case CAN_SUPPORTED:
                printf("[GET_DEVICE_INFO][CAN_SUPPORTED] \n");
                param_list->SParamPtr[i].Supported = 1;
                break;
            case SHORT_TO_GND_J1962:
                printf("[GET_DEVICE_INFO][SHORT_TO_GND_J1962] %08x\n", param_list->SParamPtr[i].Value);
                if(param_list->SParamPtr[i].Value == 0)param_list->SParamPtr[i].Supported = 1;
                break;
            case PGM_VOLTAGE_J1962:
                printf("[GET_DEVICE_INFO][PGM_VOLTAGE_J1962] %08x\n", param_list->SParamPtr[i].Value);
                param_list->SParamPtr[i].Supported = 1;
                break;
            }
        }
    }
    else if(IoctlID == GET_CONFIG)
    {
        printf("[GET_CONFIG] \n");
        if(pInput == NULL)
        {
            printf("GET_CONFIG with no Input? \n");
        }
        else
        {
            const SCONFIG_LIST * input = reinterpret_cast<const SCONFIG_LIST *>(pInput);
            printf("GET_CONFIG NumOfParams %d\n", input->NumOfParams);
            for(int i = 0; i < input->NumOfParams; i++)
            {
                printf("ConfigPtr %d paramter %08x value %08x \n", i, input->ConfigPtr[i].Parameter, input->ConfigPtr[i].Value);
                switch(input->ConfigPtr[i].Parameter)
                {
                case ConfigParamId::DATA_RATE:
                    printf("[GET_CONFIG][DATA_RATE] \n");

                    input->ConfigPtr[i].Value = data_rate;
                    break;
                case ConfigParamId::DATA_BITS:
                    printf("[GET_CONFIG][DATA_BITS] \n");
                    break;
                case ConfigParamId::LOOPBACK:
                    printf("[GET_CONFIG][LOOPBACK] \n");
                    break;
                case ConfigParamId::BIT_SAMPLE_POINT:
                    printf("[GET_CONFIG][BIT_SAMPLE_POINT] \n");
                    break;
                case ConfigParamId::SYNC_JUMP_WIDTH:
                    printf("[GET_CONFIG][SYNC_JUMP_WIDTH] \n");
                    break;
                }
            }
        }
    }


    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruIoctl", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}




///DREW Tech extra functions
///
long J2534CannelloniClient::PassThruExConfigureWiFi ( void )
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruExConfigureWiFi);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruExConfigureWiFi", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruExDeviceWatchdog ( void )
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruExDeviceWatchdog);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruExDeviceWatchdog", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruExDownloadCypheredFlashData ( void )
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruExDownloadCypheredFlashData);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruExDownloadCypheredFlashData", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruExEraseFlash ( void )
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruExDownloadCypheredFlashData);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruExDownloadCypheredFlashData", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruExInitiateCypheredFlashDownload ( void )
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruExInitiateCypheredFlashDownload);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruExInitiateCypheredFlashDownload", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruExReadFlash ( void )
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruExReadFlash);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruExReadFlash", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruExResetFlash ( void )
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruExResetFlash);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruExResetFlash", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruExRunSelfTest ( void )
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruExRunSelfTest);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruExRunSelfTest", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruExWriteFlash ( void )
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruExWriteFlash);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruExWriteFlash", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruLoadFirmware(void)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruLoadFirmware);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruLoadFirmware", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruRecoverFirmware(void)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruRecoverFirmware);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruRecoverFirmware", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruReadIPSetup(unsigned long DeviceID, char *host_name, char *ip_addr, char *subnet_mask, char *gateway, char *dhcp_addr)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruReadIPSetup);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1, DeviceID);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param2, host_name);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param3, ip_addr);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param4, subnet_mask);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param5, gateway);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param6, dhcp_addr);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 0);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruReadIPSetup", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruWriteIPSetup(unsigned long DeviceID, char *host_name, char *ip_addr, char *subnet_mask, char *gateway, char *dhcp_addr)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruWriteIPSetup);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1, DeviceID);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param2, host_name);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param3, ip_addr);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param4, subnet_mask);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param5, gateway);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param6, dhcp_addr);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruWriteIPSetup", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruReadPCSetup(char *host_name, char *ip_addr)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruReadPCSetup);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param1, host_name);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param2, ip_addr);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 0);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruReadPCSetup", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruGetPointer(long vb_pointer)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruGetPointer);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1, vb_pointer);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());


    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruGetPointer", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruGetNextCarDAQ(char **name, unsigned long *version, char **addr)
{
    std::lock_guard<std::mutex> lock(globalMutex);

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName, ExPipeClient::KEY_PassThruGetNextCarDAQ);

    // Parameter logging (simplified; expand if needed)
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param1, "N/A");  // No direct param
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, (version ? *version : 0UL));
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param3, "N/A");

    if (name == nullptr || version == nullptr || addr == nullptr)
    {
        lastError = "Null parameter(s)";
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" -> ").append(lastError).c_str());
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruGetNextCarDAQ", cbor_utils::cbor_to_data(cb_map_root)});
        cn_cbor_free(cb_map_root);

        return J2534Err::ERR_NULLPARAMETER;
    }

    static constexpr const char* DEFAULT_HOST = "127.0.0.1";


    if (scanIndex >= static_cast<int>(VIRTUAL_DEVICE_COUNT))
    {
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, ERR_NO_MORE_DEVICES);
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruGetNextCarDAQ", cbor_utils::cbor_to_data(cb_map_root)});
        cn_cbor_free(cb_map_root);
        return ERR_NO_MORE_DEVICES;
    }

    // Allocate strings with LocalAlloc (caller frees with LocalFree; standard for J2534 extensions)
    // Name: Descriptive string
    unsigned long port = getPortFromIndex(scanIndex);
    std::array<char, 80> deviceName{0};  // Modern fixed buffer
    std::snprintf(deviceName.data(), deviceName.size(), "Cannelloni UDP Port %lu (CAN)", port);

    std::string deviceNameStr;
    deviceNameStr.append(deviceName.data());

    size_t nameLen = deviceNameStr.length() + 1;
    *name = static_cast<char*>(LocalAlloc(LMEM_FIXED, nameLen));
    if (*name == nullptr)
    {
        lastError = "LocalAlloc failed for name";
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" -> ").append(lastError).c_str());
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruGetNextCarDAQ", cbor_utils::cbor_to_data(cb_map_root)});
        cn_cbor_free(cb_map_root);
        return J2534Err::ERR_NULLPARAMETER;
    }
    //std::strcpy_s(*name, nameLen, deviceName.c_str());
    std::strncpy(*name, deviceNameStr.c_str(), nameLen - 1);
    (*name)[nameLen - 1] = '\0';

    // Version: Unsigned long, e.g., 100 for 1.00
    *version = 100UL;  // Or tie to DLL version; could be 10404 for API 04.04

    // Addr: IP address string
    std::string deviceAddr = DEFAULT_HOST;  // Default; could scan channelMap for open host if needed
    size_t addrLen = deviceAddr.length() + 1;
    *addr = static_cast<char*>(LocalAlloc(LMEM_FIXED, addrLen));
    if (*addr == nullptr)
    {
        LocalFree(*name);  // Cleanup on failure
        *name = nullptr;
        lastError = "LocalAlloc failed for addr";
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" -> ").append(lastError).c_str());
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruGetNextCarDAQ", cbor_utils::cbor_to_data(cb_map_root)});
        cn_cbor_free(cb_map_root);
        return J2534Err::ERR_NULLPARAMETER;
    }
    //std::strcpy_s(*addr, addrLen, deviceAddr.c_str());
    std::strncpy(*addr, deviceAddr.c_str(), addrLen - 1);
    (*addr)[addrLen - 1] = '\0';

    ++scanIndex;

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, J2534Err::STATUS_NOERROR);
    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruGetNextCarDAQ", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);

    return J2534Err::STATUS_NOERROR;
}

///v05.00
// In j2534cannelloniclient.cpp
long J2534CannelloniClient::PassThruScanForDevices(unsigned long* pDeviceCount)
{
    std::lock_guard<std::mutex> lock(globalMutex);

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName, ExPipeClient::KEY_PassThruScanForDevices);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1, (pDeviceCount ? *pDeviceCount : 0));

    if (pDeviceCount == nullptr) {
        lastError = "pDeviceCount is null";
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" -> ").append(lastError).c_str());
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruScanForDevices", cbor_utils::cbor_to_data(cb_map_root)});
        cn_cbor_free(cb_map_root);
        return J2534Err::ERR_NULL_PARAMETER;
    }

    *pDeviceCount = VIRTUAL_DEVICE_COUNT;
    scanIndex = 0;  // Reset enumeration

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, J2534Err::STATUS_NOERROR);
    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruScanForDevices", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);

    return J2534Err::STATUS_NOERROR;
}

long J2534CannelloniClient::PassThruGetNextDevice(SDEVICE* psDevice) {
    std::lock_guard<std::mutex> lock(globalMutex);

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName, ExPipeClient::KEY_PassThruGetNextDevice);

    if (psDevice == nullptr) {
        lastError = "psDevice is null";
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" -> ").append(lastError).c_str());
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruGetNextDevice", cbor_utils::cbor_to_data(cb_map_root)});
        cn_cbor_free(cb_map_root);
        return J2534Err::ERR_NULL_PARAMETER;
    }

    if (scanIndex >= static_cast<int>(VIRTUAL_DEVICE_COUNT)) {
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, ERR_NO_MORE_DEVICES);
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruGetNextDevice", cbor_utils::cbor_to_data(cb_map_root)});
        cn_cbor_free(cb_map_root);
        return ERR_NO_MORE_DEVICES;
    }

    // Fill SDEVICE for this virtual device
    unsigned long port = getPortFromIndex(scanIndex);
    bool isOpen = (channelMap.find(port) != channelMap.end());
    std::array<char, 80> name{};  // Modern fixed buffer
    std::snprintf(name.data(), name.size(), "Cannelloni UDP Port %lu (CAN)", port);
    std::strncpy(psDevice->DeviceName, name.data(), sizeof(psDevice->DeviceName) - 1);
    psDevice->DeviceName[sizeof(psDevice->DeviceName) - 1] = '\0';

    psDevice->DeviceAvailable = isOpen ? 0UL : 1UL;  // 1 = free to open
    psDevice->DeviceDLLFWStatus = 0UL;  // Compatible
    psDevice->DeviceConnectMedia = 3UL;  // Ethernet
    psDevice->DeviceConnectSpeed = 1000000000UL;  // 1 Gbps
    psDevice->DeviceSignalQuality = 100UL;  // Perfect (virtual)
    psDevice->DeviceSignalStrength = 100UL;

    ++scanIndex;

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, J2534Err::STATUS_NOERROR);
    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruGetNextDevice", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);

    return J2534Err::STATUS_NOERROR;
}

long J2534CannelloniClient::PassThruSelect(SCHANNELSET* ChannelSetPtr, unsigned long SelectType, unsigned long Timeout) {
    std::lock_guard<std::mutex> lock(globalMutex);  // Note: For polling, release/reacquire per channel if contention

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName, ExPipeClient::KEY_PassThruSelect);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1, reinterpret_cast<unsigned long>(ChannelSetPtr));
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, SelectType);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3, Timeout);

    if (ChannelSetPtr == nullptr) {
        lastError = "ChannelSetPtr is null";
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" -> ").append(lastError).c_str());
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruSelect", cbor_utils::cbor_to_data(cb_map_root)});
        cn_cbor_free(cb_map_root);
        return J2534Err::ERR_NULL_PARAMETER;
    }

    if (ChannelSetPtr->ChannelCount == 0 || ChannelSetPtr->ChannelList == nullptr) {
        lastError = "Invalid ChannelSet (empty list)";
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" -> ").append(lastError).c_str());
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruSelect", cbor_utils::cbor_to_data(cb_map_root)});
        cn_cbor_free(cb_map_root);
        return J2534Err::ERR_INVALID_CHANNEL_ID;
    }

    // Ignore SelectType or validate (e.g., if !=0 return ERR_NOT_SUPPORTED)
    auto startTime = std::chrono::steady_clock::now();
    unsigned long readyCount = 0;
    bool done = false;

    while (!done) {
        readyCount = 0;
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();

        if (elapsed >= static_cast<long long>(Timeout)) {
            done = true;  // Timeout
        } else {
            // Poll each channel
            for (unsigned long i = 0; i < ChannelSetPtr->ChannelCount; ++i) {
                unsigned long chanId = ChannelSetPtr->ChannelList[i];
                auto it = channelMap.find(chanId);
                if (it != channelMap.end()) {
                    std::lock_guard<std::mutex> chanLock(it->second.msgQueueMutex);
                    if (!it->second.msgQueue.empty()) {
                        ++readyCount;
                    }
                }
            }

            if (readyCount >= ChannelSetPtr->ChannelThreshold) {
                done = true;  // Condition met
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Poll interval
            }
        }
    }

    // Return readyCount (even on timeout, per spec)
    long retval = (readyCount > 0) ? static_cast<long>(readyCount) : J2534Err::ERR_TIMEOUT;

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruSelect", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);

    return retval;
}

long J2534CannelloniClient::PassThruReadDetails(unsigned long* pName)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruSelect);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1, *pName);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruSelect", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

bool J2534CannelloniClient::isValidHostString(const char* str) const
{
    if (!str) return false;

    // Basic checks: Non-empty, null-terminated, reasonable length (e.g., IP <=15 chars)
    size_t len = strlen(str);
    if (len == 0 || len > 15) return false;

    // Printable ASCII (no control chars/garbage)
    for (size_t i = 0; i < len; ++i) {
        if (!isprint(static_cast<unsigned char>(str[i]))) return false;
    }

    // Optional: Loose IP validation (dots, digits) – skip for now, or use InetPton later
    if (InetPtonA(AF_INET, str, nullptr) != 1) return false;  // Strict IP only

    return true;
}
