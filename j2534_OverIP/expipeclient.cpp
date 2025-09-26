#include "ExPipeClient.h"
#include <cn-cbor/cn-cbor.h>
#include <future>
#include <chrono>

ExPipeClient::ExPipeClient(const std::string& name) : pipe(INVALID_HANDLE_VALUE), name(name)
{

}

ExPipeClient::~ExPipeClient() {
    if (pipe != INVALID_HANDLE_VALUE) {
        CloseHandle(pipe);
    }
}

bool ExPipeClient::connect() {
    const int maxAttempts = 3;
    const std::chrono::seconds sleepDuration(10);

    //auto future = std::async(std::launch::async, [=]() {
        for (int attempt = 0; attempt < maxAttempts; ++attempt) {
            printf("connect to named pipe attempt %d\n", attempt);
            pipe = CreateFileA(
                name.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                0,
                NULL
                );

            if (pipe != INVALID_HANDLE_VALUE) {
                // Successfully connected to the named pipe
                printf("Successfully connected to the named pipe\n");
                return true;
            }

            DWORD error = GetLastError();
            if (error == ERROR_FILE_NOT_FOUND)
            {
                // The named pipe does not exist, so start the server application
                // Read the path of the server application from the registry
                printf("The named pipe does not exist, so start the server application\n");
                HKEY hKey;
                if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\PassThruSupport.04.04\\XplatformsPassThruOverIP", 0, KEY_READ, &hKey) == ERROR_SUCCESS ||
                    RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\PassThruSupport.05.00\\XplatformsPassThruOverIP", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                    char path[MAX_PATH];
                    DWORD pathSize = sizeof(path);
                    if (RegQueryValueExA(hKey, "ConfigApplication", NULL, NULL, (LPBYTE)path, &pathSize) == ERROR_SUCCESS) {
                        // Start the server application
                        printf("Start the server application %s\n", path);
                        STARTUPINFOA si = { sizeof(si) };
                        PROCESS_INFORMATION pi;
                        if (!CreateProcessA(NULL, path, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                            // Failed to start the server application
                            printf("Failed to start the server application. Error: %d\n", GetLastError());
                            return false;
                        } else {
                            // Successfully started the server application
                            printf("Successfully started the server application %s\n", path);
                            CloseHandle(pi.hProcess);
                            CloseHandle(pi.hThread);
                        }
                    }
                    RegCloseKey(hKey);
                }
            } else if (error == ERROR_PIPE_BUSY) {
                // The pipe is busy, all instances are connected
                // Wait for a while and then try again
                printf("The named pipe is busy, waiting for a while and then try again\n");
            } else {
                // Some other error occurred
                printf("ExPipeClient CreateFile Failed! Error: %d\n", (int)error);
                return false;
            }

            // Sleep for a while and then try again
            std::this_thread::sleep_for(sleepDuration);
        }

        // Reached the maximum number of attempts without successfully connecting
        printf("ExPipeClient Failed to connect to named pipe after %d attempts\n", maxAttempts);
        return false;
   // });

    printf("future.get!");
    //return future.get();
}



void ExPipeClient::disconnect() {
    if (pipe != INVALID_HANDLE_VALUE) {
        CloseHandle(pipe);
        pipe = INVALID_HANDLE_VALUE;
    }
}

void ExPipeClient::send(const ExPipeClient::Message& msg)
{
    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb = cn_cbor_map_create(&cn_errback);

    cn_cbor_map_put(cb, cn_cbor_string_create("id", &cn_errback), cn_cbor_int_create(msg.id, &cn_errback), &cn_errback);
    cn_cbor_map_put(cb, cn_cbor_string_create("type", &cn_errback), cn_cbor_int_create(msg.type, &cn_errback), &cn_errback);
    cn_cbor_map_put(cb, cn_cbor_string_create("data_type", &cn_errback), cn_cbor_int_create(msg.data_type, &cn_errback), &cn_errback);
    cn_cbor_map_put(cb, cn_cbor_string_create("timestamp", &cn_errback), cn_cbor_int_create(msg.timestamp, &cn_errback), &cn_errback);
    cn_cbor_map_put(cb, cn_cbor_string_create("path", &cn_errback), cn_cbor_string_create(msg.path.c_str(), &cn_errback), &cn_errback);
    cn_cbor_map_put(cb, cn_cbor_string_create("message", &cn_errback), cn_cbor_string_create(msg.message.c_str(), &cn_errback), &cn_errback);
    cn_cbor_map_put(cb, cn_cbor_string_create("data", &cn_errback), cn_cbor_data_create(reinterpret_cast<const uint8_t*>(msg.data.data()), msg.data.size(), &cn_errback), &cn_errback);

    auto cbor_size = cn_cbor_encoder_write(nullptr, 0, 0, cb);
    if(cbor_size != -1)
    {
        uint8_t * cbor = new uint8_t[cbor_size];
        memset(cbor,0,cbor_size);
        cn_cbor_encoder_write(cbor, 0, cbor_size, cb);

        // Write the message length (as a 4-byte integer) before the message
        DWORD bytesWritten;
        int size = static_cast<int>(cbor_size);
        if (!WriteFile(pipe, &size, sizeof(size), &bytesWritten, nullptr) ||
            bytesWritten != sizeof(size)) {
            // Handle error
            printf("Can't WRITE SIZE TO PIPE!!!!");
            delete[] cbor;
            return;
        }

        // Write the message
        if (!WriteFile(pipe, cbor, cbor_size, &bytesWritten, nullptr) ||
            bytesWritten != cbor_size) {
            // Handle error
            printf("Can't WRITE TO PIPE!!!!");
        }

        delete[] cbor;
    }
}


ExPipeClient::Message ExPipeClient::receive()
{
    DWORD bytesRead;
    DWORD bytesAvailable;

    // Check how many bytes are available to be read
    if (!PeekNamedPipe(pipe, NULL, 0, NULL, &bytesAvailable, NULL)) {
        // Handle error
    }

    // If not enough bytes are available to read the size of the message, return an empty message
    if (bytesAvailable < sizeof(int)) {
        return ExPipeClient::Message{};
    }

    int size;
    if (!ReadFile(pipe, &size, sizeof(size), &bytesRead, nullptr) || bytesRead != sizeof(size))
    {
        // Handle error
    }

    // Check again how many bytes are available to be read
    if (!PeekNamedPipe(pipe, NULL, 0, NULL, &bytesAvailable, NULL))
    {
        // Handle error
    }

    // If not enough bytes are available to read the message, return an empty message
    if (bytesAvailable < size)
    {
        return ExPipeClient::Message{};
    }

    std::vector<char> buffer(size);
    if (!ReadFile(pipe, buffer.data(), size, &bytesRead, nullptr) || bytesRead != size)
    {
        // Handle error
    }

    cn_cbor_errback errp;
    cn_cbor* cb = cn_cbor_decode((const uint8_t*)buffer.data(), bytesRead, &errp);
    ExPipeClient::Message msg;
    msg.path = cn_cbor_mapget_string(cb, "path")->v.str;
    msg.id = cn_cbor_mapget_string(cb, "id")->v.sint;
    msg.type = cn_cbor_mapget_string(cb, "type")->v.sint;
    msg.message = cn_cbor_mapget_string(cb, "message")->v.str;
    cn_cbor* data_cb = cn_cbor_mapget_string(cb, "data");
    msg.data = std::vector<uint8_t>(data_cb->v.str, data_cb->v.str + data_cb->length);

    return msg;
}


