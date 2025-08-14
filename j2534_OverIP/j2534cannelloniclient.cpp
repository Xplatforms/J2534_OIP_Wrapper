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

//20000 125кбит  3/11 пин
//20001 500кбит  6/14 пин - Standart CAN Pins on OBD-II
//20002 500кбит 12/13 пин

#pragma pack(push, 1)
struct CannelloniDataPacket {
    uint8_t version;
    uint8_t op_code;
    uint8_t seq_no;
    uint16_t count;
};

struct CannelloniCanFrame {
    uint32_t can_id;
    uint8_t length;
    uint8_t data[MAX_CAN_FRAME_DATA_LEN];
};
#pragma pack(pop)


J2534CannelloniClient::J2534CannelloniClient()
{
    this->_p_pipe = new ExPipeClient("\\\\.\\pipe\\exj2534_overip_pipe");
    this->_p_pipe->connect();

    this->p_path = ExDbg::getProcessPath();
    this->p_id = ExDbg::crc32(ExDbg::getProcessPath());
    this->p_type = 4; // 4 = J2534CannelloniClient
    this->p_data_type = 2; // 2 = cbor

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
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, ts, this->p_path, "J2534CannelloniClient Verbunden!", cbor_utils::cbor_to_data(cb_map_root)});
    }

    cn_cbor_free(cb_map_root);
}

J2534CannelloniClient::~J2534CannelloniClient()
{
    WSACleanup();
}

std::string J2534CannelloniClient::getIName()
{
    char pname[1024] = {0};
    sprintf(pname, "J2534CannelloniClient %08x", (unsigned int)this);
    return std::string(pname);
}

void J2534CannelloniClient::startReceiveThread(ChannelContext& ctx)
{
    ctx.recvThread = std::thread([this, &ctx]()
                                 {
                                    constexpr size_t maxPacketSize = 512; // big enough for Cannelloni packets
                                    uint8_t buffer[maxPacketSize];

                                     while (!ctx.stopRequested)
                                     {
                                         fd_set readfds;
                                         FD_ZERO(&readfds);
                                         FD_SET(ctx.sockfd, &readfds);

                                         timeval timeout;
                                         timeout.tv_sec = 0;
                                         timeout.tv_usec = 200000; // 200 ms

                                         int ret = select(0, &readfds, nullptr, nullptr, &timeout);
                                         if (ret > 0 && FD_ISSET(ctx.sockfd, &readfds))
                                         {
                                             sockaddr_in fromAddr;
                                             int fromLen = sizeof(fromAddr);
                                             int bytesReceived = recvfrom(ctx.sockfd, reinterpret_cast<char*>(buffer), maxPacketSize, 0,
                                                                          reinterpret_cast<sockaddr*>(&fromAddr), &fromLen);

                                             if (bytesReceived > 0)
                                             {
                                                 std::vector<uint8_t> message(buffer, buffer + bytesReceived);
                                                 {
                                                     std::lock_guard<std::mutex> lock(ctx.msgQueueMutex);
                                                     ctx.msgQueue.push(std::move(message));
                                                 }
                                                 ctx.msgQueueCv.notify_one();
                                             }
                                         }
                                     }
                                 });
}


bool J2534CannelloniClient::popMessage(ChannelContext& ctx, std::vector<uint8_t>& outMsg, int timeoutMs)
{
    std::unique_lock<std::mutex> lock(ctx.msgQueueMutex);

    if (!ctx.msgQueueCv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&ctx]() {
            return !ctx.msgQueue.empty() || ctx.stopRequested;
        }))
    {
        // Timeout reached
        return false;
    }

    if (!ctx.msgQueue.empty())
    {
        outMsg = std::move(ctx.msgQueue.front());
        ctx.msgQueue.pop();
        return true;
    }

    return false; // Stopped without a message
}

void J2534CannelloniClient::pushMessage(ChannelContext& ctx, const std::vector<uint8_t>& msg)
{
    {
        std::lock_guard<std::mutex> lock(ctx.msgQueueMutex);
        ctx.msgQueue.push(msg);
    }
    ctx.msgQueueCv.notify_one();
}

long J2534CannelloniClient::PassThruOpen(const void *pName, unsigned long *pDeviceID)
{
    std::lock_guard<std::mutex> lock(globalMutex);

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruOpen);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, (pName == NULL?"":(const char *)pName));
    //cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2 /* param_2 */, *pDeviceID);

    if (pDeviceID == nullptr)
    {
        lastError = "pDeviceID is null";

        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(lastError).c_str());
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruOpen", cbor_utils::cbor_to_data(cb_map_root)});
        cn_cbor_free(cb_map_root);
        return J2534Err::ERR_NULL_PARAMETER;
    }

    static const int numPorts = 3;
    static const unsigned long basePort = 20000;

    const char *host = "127.0.0.1";
    if (pName != nullptr)
    {
        const char* const* pHostStr = reinterpret_cast<const char* const*>(pName);
        if (pHostStr && *pHostStr && strlen(*pHostStr) > 0)
            host = *pHostStr;
    }

    if (channelMap.size() >= numPorts)
    {
        lastError = "All virtual devices are already opened";
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(lastError).c_str());
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruOpen", cbor_utils::cbor_to_data(cb_map_root)});
        cn_cbor_free(cb_map_root);
        return J2534Err::ERR_DEVICE_IN_USE;
    }

    for (int i = 0; i < numPorts; i++)
    {
        unsigned long port = basePort + i;
        if (channelMap.find(port) == channelMap.end())
        {
            ChannelContext ctx;
            ctx.sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            if (ctx.sockfd < 0)
            {
                lastError = "Socket creation failed";
                cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(lastError).c_str());
                this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruOpen", cbor_utils::cbor_to_data(cb_map_root)});
                cn_cbor_free(cb_map_root);
                return J2534Err::ERR_DEVICE_NOT_CONNECTED;
            }

            memset(&ctx.targetAddr, 0, sizeof(ctx.targetAddr));
            ctx.targetAddr.sin_family = AF_INET;
            ctx.targetAddr.sin_port = htons(static_cast<uint16_t>(port));
            if(InetPtonA(AF_INET, host, &ctx.targetAddr.sin_addr) != 1)
            {
                closesocket(ctx.sockfd);
                lastError = "Invalid IP address";
                cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(lastError).c_str());
                this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruOpen", cbor_utils::cbor_to_data(cb_map_root)});
                cn_cbor_free(cb_map_root);
                return J2534Err::ERR_INVALID_DEVICE_ID;
            }

            channelMap[port] = std::move(ctx);
            ChannelContext& refCtx = channelMap[port];
            startReceiveThread(refCtx);

            *pDeviceID = port;            
        }
    }

    printf("pDeviceID %d\n", *pDeviceID );

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2 /* param_2 */, *pDeviceID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, J2534Err::STATUS_NOERROR);
    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruOpen", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);


    return J2534Err::STATUS_NOERROR;

    /*
    lastError = "No free virtual device ports available";
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(lastError).c_str());
    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruOpen", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return J2534Err::ERR_DEVICE_IN_USE;
    */
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
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(lastError).c_str());
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
    if (ctx.recvThread.joinable())
        ctx.recvThread.join();

    closesocket(ctx.sockfd);

    channelMap.erase(it);


    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, J2534Err::STATUS_NOERROR);
    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruClose", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID, unsigned long Flags, unsigned long Baudrate, unsigned long *pChannelID)
{
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


    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruConnect);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, DeviceID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2 /* param_1 */, ProtocolID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3 /* param_1 */, Flags);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param4 /* param_1 */, Baudrate);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param5 /* param_1 */, *pChannelID);

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruConnect", cbor_utils::cbor_to_data(cb_map_root)});

    cn_cbor_free(cb_map_root);

    return STATUS_NOERROR;
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


long J2534CannelloniClient::PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout)
{
    long retval = J2534Err::STATUS_NOERROR;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruReadMsgs);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, ChannelID);
    //cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, 0);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3, *pNumMsgs);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param4, Timeout);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 0);

    if (pMsg == nullptr || pNumMsgs == nullptr)
    {
        lastError = "Null pointer for pMsg or pNumMsgs";
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, 0);
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(lastError).c_str());
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruReadMsgs", cbor_utils::cbor_to_data(cb_map_root)});
        cn_cbor_free(cb_map_root);
        return J2534Err::ERR_NULL_PARAMETER;
    }

    std::lock_guard<std::mutex> lock(globalMutex);
    auto it = channelMap.find(ChannelID);
    if (it == channelMap.end())
    {
        lastError = "Invalid ChannelID";
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, 0);
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(lastError).c_str());
        this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruReadMsgs", cbor_utils::cbor_to_data(cb_map_root)});
        cn_cbor_free(cb_map_root);
        return J2534Err::ERR_INVALID_CHANNEL_ID;
    }

    ChannelContext& ctx = it->second;

    std::unique_lock<std::mutex> queueLock(ctx.msgQueueMutex);
    if (ctx.msgQueue.empty())
    {
        // Wait for data or timeout
        if (!ctx.msgQueueCv.wait_for(queueLock, std::chrono::milliseconds(Timeout), [&ctx]{ return !ctx.msgQueue.empty(); }))
        {
            *pNumMsgs = 0;

            cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, 0);
            cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(lastError).c_str());
            this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruReadMsgs", cbor_utils::cbor_to_data(cb_map_root)});
            cn_cbor_free(cb_map_root);
            return J2534Err::ERR_TIMEOUT;
        }
    }

    unsigned long maxMsgs = *pNumMsgs;
    unsigned long msgsRead = 0;

    while (msgsRead < maxMsgs && !ctx.msgQueue.empty())
    {
        const std::vector<uint8_t>& packet = ctx.msgQueue.front();

        if (packet.size() < sizeof(CannelloniDataPacket))
        {
            lastError = "Cannelloni packet too short";
            cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, 0);
            cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(lastError).c_str());
            this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruReadMsgs", cbor_utils::cbor_to_data(cb_map_root)});
            cn_cbor_free(cb_map_root);
            return J2534Err::ERR_INVALID_MSG;
        }

        // Parse Cannelloni header
        const CannelloniDataPacket* header = reinterpret_cast<const CannelloniDataPacket*>(packet.data());
        size_t offset = sizeof(CannelloniDataPacket);

        // 'count' field is number of CAN frames inside, network byte order?
        uint16_t canFrameCount = ntohs(header->count); // assuming network order

        // Validate packet length
        size_t expectedLength = sizeof(CannelloniDataPacket) + canFrameCount * sizeof(CannelloniCanFrame);
        if (packet.size() < expectedLength)
        {
            lastError = "Cannelloni packet length mismatch";
            cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, 0);
            cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(lastError).c_str());
            this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruReadMsgs", cbor_utils::cbor_to_data(cb_map_root)});
            cn_cbor_free(cb_map_root);
            return J2534Err::ERR_INVALID_MSG;
        }

        // Extract CAN frames one by one
        for (uint16_t i = 0; i < canFrameCount && msgsRead < maxMsgs; i++)
        {
            const CannelloniCanFrame* frame = reinterpret_cast<const CannelloniCanFrame*>(packet.data() + offset);

            // Fill PASSTHRU_MSG from CannelloniCanFrame
            pMsg[msgsRead].ProtocolID = ISO15765; // Use appropriate protocol id if known
            pMsg[msgsRead].RxStatus = 0;   // Fill if needed, e.g. CAN flags
            pMsg[msgsRead].TxFlags = ISO15765_FRAME_PAD;    // Fill if transmit flags needed
            pMsg[msgsRead].Timestamp = 0;  // Could be filled if you keep track of timestamp elsewhere

            // DataSize is length, but limit to MAX_CAN_FRAME_DATA_LEN to be safe
            pMsg[msgsRead].DataSize = (frame->length > MAX_CAN_FRAME_DATA_LEN) ? MAX_CAN_FRAME_DATA_LEN : frame->length;
            pMsg[msgsRead].ExtraDataIndex = 0; // no extra data in this example

            // Copy CAN frame data payload
            memcpy(pMsg[msgsRead].Data, frame->data, pMsg[msgsRead].DataSize);

            ++msgsRead;
            offset += sizeof(CannelloniCanFrame);
        }

        ctx.msgQueue.pop();
    }
    *pNumMsgs = msgsRead;

    cbor_utils::map_put_PASSTHRU_MSG(cb_map_root, ExPipeClient::KEY_Param2, *pNumMsgs, pMsg);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3, *pNumMsgs);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param4, Timeout);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 1);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruReadMsgs", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);

    return J2534Err::STATUS_NOERROR;
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

long J2534CannelloniClient::PassThruWriteMsgs(unsigned long ChannelID, const PASSTHRU_MSG *pMsg, unsigned long *pNumMsgs, unsigned long Timeout)
{
    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    for(int i = 0; i < *pNumMsgs; i++)
    {
        printf("[WRITEMSG] protid %08x rxstatus %08x txflags %08x tstamp %d e_index %08x datasize %d \n [MSG]: ",
               pMsg[i].ProtocolID, pMsg[i].RxStatus, pMsg[i].TxFlags,
               pMsg[i].Timestamp, pMsg[i].ExtraDataIndex, pMsg[i].DataSize);
        for(int y = 0; y < pMsg[i].DataSize; y++)
        {
            printf("%02x ", pMsg[i].Data[y]);
        }
        printf("\n");
    }

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruWriteMsgs);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, ChannelID);
    cbor_utils::map_put_PASSTHRU_MSG(cb_map_root, ExPipeClient::KEY_Param2, *pNumMsgs, pMsg);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3, *pNumMsgs);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param4, Timeout);

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruWriteMsgs", cbor_utils::cbor_to_data(cb_map_root)});

    cn_cbor_free(cb_map_root);

    return STATUS_NOERROR;
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
    memcpy(FirmwareVersion, "9", 1);

    memcpy(pApiVersion, ApiVersion, 80);
    memcpy(pDllVersion, DllVersion, 80);
    memcpy(pFirmwareVersion, FirmwareVersion, 80);


    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruReadVersion", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruGetLastError(char *pErrorDescription)
{
    long retval = STATUS_NOERROR;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruGetLastError);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param1 /* param_1 */, std::string(pErrorDescription).c_str());
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 0);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    //cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());

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

    // TODO: check IoctlID?
    // 3 READ_VBATT?
    // 2 CONFIG_SET?
    printf("PassThruIoctl Channel or Device %d IoctlID = %08x\n", ChannelID, IoctlID);
    if (IoctlID == READ_VBATT)
    {
        //unsigned int voltageLevel = 14400;
        //memcpy(pOutput, &voltageLevel, sizeof(unsigned int));

        unsigned int *voltageLevel = (unsigned int*)pOutput;
        *voltageLevel = 14400; // The units will be in milli-volts and will be rounded to the nearest tenth of a volt.
    }
    else if(IoctlID == SET_CONFIG)
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
    else if(IoctlID == GET_DEVICE_INFO)
    {
        auto param_list = (SPARAM_LIST*)pOutput;
        switch(param_list->SParamPtr[0].Parameter)
        {
        case SERIAL_NUMBER:
            param_list->SParamPtr[0].Supported = 1;
            param_list->SParamPtr[0].Value = 987654321;
            break;
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
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruGetNextCarDAQ);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param1, "");
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, *version);
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Param3, "");
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 0);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());


    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruGetNextCarDAQ", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

///v05.00
long J2534CannelloniClient::PassThruScanForDevices(unsigned long *pDeviceCount)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruScanForDevices);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1, *pDeviceCount);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 0);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruScanForDevices", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruGetNextDevice(SDEVICE *psDevice)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);
    SDEVICE _tmp_sdev = {{0}};
    memset(&_tmp_sdev, 0, sizeof(SDEVICE));

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruGetNextDevice);
    cbor_utils::map_put_data(cb_map_root, ExPipeClient::KEY_Param1, (const uint8_t *)&_tmp_sdev, sizeof(SDEVICE));
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 0);

    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());

    this->_p_pipe->send({this->p_id, this->p_type, this->p_data_type, get_timestamp(), this->p_path, "PassThruGetNextDevice", cbor_utils::cbor_to_data(cb_map_root)});
    cn_cbor_free(cb_map_root);
    return retval;
}

long J2534CannelloniClient::PassThruSelect(SCHANNELSET *ChannelSetPtr, unsigned long SelectType, unsigned long Timeout)
{
    long retval = ERR_NOT_SUPPORTED;

    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);

    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName /* function */, ExPipeClient::KEY_PassThruSelect);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1, 0);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, SelectType);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3, Timeout);


    //EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
    cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not implemented").c_str());


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


/*

std::unordered_map<unsigned long, ChannelContext> channels;
std::atomic<unsigned long> nextChannelId{1};
std::string lastError;



void receiveThread(ChannelContext& ctx) {
    char buffer[1500];
    while (ctx.running) {
        sockaddr_in from;
        int fromLen = sizeof(from);
        int len = recvfrom(ctx.socket, buffer, sizeof(buffer), 0, (sockaddr*)&from, &fromLen);
        if (len <= 0) continue;

        if ((size_t)len < sizeof(CannelloniDataPacket)) continue;

        CannelloniDataPacket* pkt = (CannelloniDataPacket*)buffer;
        CannelloniCanFrame* frames = (CannelloniCanFrame*)(buffer + sizeof(CannelloniDataPacket));

        for (int i = 0; i < pkt->count && i * sizeof(CannelloniCanFrame) + sizeof(CannelloniDataPacket) <= (size_t)len; ++i) {
            CannelloniCanFrame& f = frames[i];

            PASSTHRU_MSG msg = {};
            msg.ProtocolID = PROTOCOL_CAN;
            msg.DataSize = f.length + 4;
            memcpy(msg.Data, &f.can_id, 4);
            memcpy(msg.Data + 4, f.data, f.length);

            std::lock_guard<std::mutex> lock(ctx.queueMutex);
            ctx.recvQueue.push(msg);
        }
    }
}



long PassThruOpen(const void* pName, unsigned long* pDeviceID) {
    static std::atomic<unsigned long> nextDeviceId{1};
    *pDeviceID = nextDeviceId++;
    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        lastError = "WSAStartup failed";
        return ERR_FAILED;
    }
    return STATUS_NOERROR;
}

long PassThruClose(unsigned long DeviceID) {
    for (auto& [id, ctx] : channels) {
        if (ctx.running) {
            ctx.running = false;
            closesocket(ctx.socket);
            if (ctx.recvThread.joinable()) ctx.recvThread.join();
        }
    }
    channels.clear();
    WSACleanup();
    return STATUS_NOERROR;
}

long PassThruConnect(unsigned long DeviceID, unsigned long ProtocolID, unsigned long Flags, unsigned long Baudrate, unsigned long* pChannelID) {
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        lastError = "Socket creation failed";
        return ERR_FAILED;
    }

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("192.168.4.1");
    if (Baudrate == 125000)
        addr.sin_port = htons(CANNELLONI_PORT_125K);
    else if (Baudrate == 500000 && Flags == 0)
        addr.sin_port = htons(CANNELLONI_PORT_500K_6_14);
    else
        addr.sin_port = htons(CANNELLONI_PORT_500K_12_13);

    unsigned long chId = nextChannelId++;
    ChannelContext ctx;
    ctx.socket = sock;
    ctx.target = addr;
    ctx.running = true;
    ctx.recvThread = std::thread(receiveThread, std::ref(ctx));

    channels[chId] = std::move(ctx);
    *pChannelID = chId;
    return STATUS_NOERROR;
}

long PassThruDisconnect(unsigned long ChannelID) {
    auto it = channels.find(ChannelID);
    if (it == channels.end()) return ERR_INVALID_CHANNEL_ID;
    it->second.running = false;
    closesocket(it->second.socket);
    if (it->second.recvThread.joinable()) it->second.recvThread.join();
    channels.erase(it);
    return STATUS_NOERROR;
}

long PassThruWriteMsgs(unsigned long ChannelID, const PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout) {
    auto it = channels.find(ChannelID);
    if (it == channels.end()) return ERR_INVALID_CHANNEL_ID;
    const ChannelContext& ctx = it->second;

    for (unsigned i = 0; i < *pNumMsgs; ++i) {
        CannelloniDataPacket pkt = {0x01, 0x01, 0x00, 1};
        CannelloniCanFrame frame = {};
        memcpy(&frame.can_id, pMsg[i].Data, 4);
        frame.length = (uint8_t)(pMsg[i].DataSize - 4);
        memcpy(frame.data, pMsg[i].Data + 4, frame.length);

        char buffer[32] = {};
        memcpy(buffer, &pkt, sizeof(pkt));
        memcpy(buffer + sizeof(pkt), &frame, sizeof(frame));

        sendto(ctx.socket, buffer, sizeof(pkt) + sizeof(frame), 0, (sockaddr*)&ctx.target, sizeof(ctx.target));
    }

    return STATUS_NOERROR;
}

long PassThruReadMsgs(unsigned long ChannelID, PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout) {
    auto it = channels.find(ChannelID);
    if (it == channels.end()) return ERR_INVALID_CHANNEL_ID;

    unsigned count = 0;
    auto& q = it->second.recvQueue;
    auto& mtx = it->second.queueMutex;

    auto start = std::chrono::steady_clock::now();
    while (count < *pNumMsgs) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (!q.empty()) {
                pMsg[count++] = q.front();
                q.pop();
                continue;
            }
        }
        if (Timeout == 0) break;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count() > Timeout) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    *pNumMsgs = count;
    return STATUS_NOERROR;
}

long PassThruReadVersion(unsigned long DeviceID, char* pApiVersion, char* pDllVersion, char* pFirmwareVersion) {
    strcpy_s(pApiVersion, 32, "04.04");
    strcpy_s(pDllVersion, 32, "CANelloniWrapper 1.0");
    strcpy_s(pFirmwareVersion, 32, "ESP32 CANelloni");
    return STATUS_NOERROR;
}

long PassThruGetLastError(char* pErrorDescription) {
    strncpy_s(pErrorDescription, 80, lastError.c_str(), _TRUNCATE);
    return STATUS_NOERROR;
}

// Not implemented
long PassThruStartPeriodicMsg(...) { return ERR_FAILED; }
long PassThruStopPeriodicMsg(...) { return ERR_FAILED; }
long PassThruStartMsgFilter(...) { return ERR_FAILED; }
long PassThruStopMsgFilter(...) { return ERR_FAILED; }
long PassThruSetProgrammingVoltage(...) { return ERR_FAILED; }
long PassThruIoctl(...) { return ERR_FAILED; }


*/
