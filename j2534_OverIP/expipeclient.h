#ifndef EXPIPECLIENT_H
#define EXPIPECLIENT_H

#include <windows.h>
#include <string>
#include <vector>
#include <cn-cbor/cn-cbor.h>

class ExPipeClient {
public:
    enum {KEY_FuncName = 0, KEY_Param1, KEY_Param2, KEY_Param3, KEY_Param4, KEY_Param5, KEY_Param6, KEY_Param7, KEY_RX_TX, KEY_RETVAL_LONG, KEY_Error};

    enum {KEY_PassThruOpen = 20, KEY_PassThruClose, KEY_PassThruSelect, KEY_PassThruConnect, KEY_PassThruDisconnect, KEY_PassThruGetLastError, KEY_PassThruIoctl,
           KEY_PassThruReadMsgs, KEY_PassThruReadVersion, KEY_PassThruSetProgrammingVoltage, KEY_PassThruStartMsgFilter, KEY_PassThruStartPeriodicMsg, KEY_PassThruStopMsgFilter,
           KEY_PassThruStopPeriodicMsg, KEY_PassThruScanForDevices, KEY_PassThruGetNextDevice, KEY_PassThruWriteMsgs, KEY_PassThruExConfigureWiFi, KEY_PassThruExDeviceWatchdog,
           KEY_PassThruExDownloadCypheredFlashData, KEY_PassThruExEraseFlash, KEY_PassThruExInitiateCypheredFlashDownload, KEY_PassThruExReadFlash, KEY_PassThruExResetFlash,
           KEY_PassThruExRunSelfTest, KEY_PassThruExWriteFlash, KEY_PassThruLoadFirmware, KEY_PassThruRecoverFirmware, KEY_PassThruReadIPSetup, KEY_PassThruWriteIPSetup,
           KEY_PassThruReadPCSetup, KEY_PassThruGetPointer, KEY_PassThruGetNextCarDAQ, KEY_PassThruReadDetails, KEY_PassThruQueueMsgs};

public:
    struct Message {
        uint32_t id;                // crc32 sum from 'path' parameter
        uint32_t type;              // type of the message
        uint32_t data_type;         // type of data. 0 = string, 1 = cbor, 2 = cbor j2534 functions struct
        int64_t timestamp;
        std::string path;           // name of the client
        std::string message;
        std::vector<uint8_t> data;  // unknown custom data
    };

    explicit ExPipeClient(const std::string& name);
    ~ExPipeClient();

    // Deleted copy constructor and assignment operator
    ExPipeClient(const ExPipeClient&) = delete;
    ExPipeClient& operator=(const ExPipeClient&) = delete;

    bool connect();
    void disconnect();
    void send(const Message& msg);
    Message receive();

private:
    HANDLE pipe;
    std::string name;
};

#endif // EXPIPECLIENT_H
