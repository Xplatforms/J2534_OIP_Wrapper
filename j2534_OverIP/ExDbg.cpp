#include "ExDbg.h"


ExDbg::ExDbg()
{
    //log_file_name_ = "C:\\Dev\\log.txt";
    this->_client_path = getProcessPath();
    this->_crc32_id = (long)crc32(this->_client_path);

    _pipe_client = new ExPipeClient("\\\\.\\pipe\\exj2534_overip_pipe");
    _pipe_client->connect();

}
