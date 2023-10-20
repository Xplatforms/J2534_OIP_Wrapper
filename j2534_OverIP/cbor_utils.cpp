#include "cbor_utils.h"
#include "expipeclient.h"

bool cbor_utils::map_put_int(cn_cbor* cb_map /* map */, uint32_t key, uint32_t value)
{
    cn_cbor_errback cn_errback = {0};
    cn_cbor_map_put(cb_map, cn_cbor_int_create(key, &cn_errback), cn_cbor_int_create(value, &cn_errback), &cn_errback);
    return cn_errback.err == CN_CBOR_NO_ERROR;
}

bool cbor_utils::map_put_string(cn_cbor* cb_map /* map */, uint32_t key, const char * value)
{
    cn_cbor_errback cn_errback = {0};
    cn_cbor_map_put(cb_map, cn_cbor_int_create(key, &cn_errback), cn_cbor_string_create(value, &cn_errback), &cn_errback);
    return cn_errback.err == CN_CBOR_NO_ERROR;
}

bool cbor_utils::map_put_data(cn_cbor* cb_map /* map */, uint32_t key, const uint8_t* data, uint32_t len)
{
    cn_cbor_errback cn_errback = {0};
    cn_cbor_map_put(cb_map, cn_cbor_int_create(key, &cn_errback), cn_cbor_data_create(data, len, &cn_errback), &cn_errback);
    return cn_errback.err == CN_CBOR_NO_ERROR;
}

bool cbor_utils::map_put_array(cn_cbor* cb_map /* map */, uint32_t key, cn_cbor* value)
{
    cn_cbor_errback cn_errback = {0};
    cn_cbor_map_put(cb_map, cn_cbor_int_create(key, &cn_errback), value, &cn_errback);
    return cn_errback.err == CN_CBOR_NO_ERROR;
}

bool cbor_utils::arr_append_cbor(cn_cbor* cb_array /* array */, cn_cbor* value)
{
    cn_cbor_errback cn_errback = {0};
    cn_cbor_array_append(cb_array, value, &cn_errback);
    return cn_errback.err == CN_CBOR_NO_ERROR;
}

bool cbor_utils::map_put_PASSTHRU_MSG(cn_cbor* cb_map /* map */, uint32_t key, uint32_t msgs_count, const PASSTHRU_MSG * msgs)
{
    cn_cbor_errback cn_errback = {0};
    cn_cbor* cb_map_root = cn_cbor_map_create(&cn_errback);
    cn_cbor* cb_array = cn_cbor_array_create(&cn_errback);

    // zero param in map is array length
    cn_cbor_map_put(cb_map_root, cn_cbor_int_create(0, &cn_errback), cn_cbor_int_create(msgs_count, &cn_errback), &cn_errback);

    for(int i = 0; i < msgs_count; i++)
    {
        /*
          uint32_t ProtocolID;
          uint32_t RxStatus;
          uint32_t TxFlags;
          uint32_t Timestamp;
          uint32_t DataSize;
          uint32_t ExtraDataIndex;
          unsigned char Data[4128];
        */
        cn_cbor* cb_map_elem = cn_cbor_map_create(&cn_errback);
        cbor_utils::map_put_int(cb_map_elem, ExPipeClient::KEY_Param1, msgs[i].ProtocolID);
        cbor_utils::map_put_int(cb_map_elem, ExPipeClient::KEY_Param2, msgs[i].RxStatus);
        cbor_utils::map_put_int(cb_map_elem, ExPipeClient::KEY_Param3, msgs[i].TxFlags);
        cbor_utils::map_put_int(cb_map_elem, ExPipeClient::KEY_Param4, msgs[i].Timestamp);
        cbor_utils::map_put_int(cb_map_elem, ExPipeClient::KEY_Param5, msgs[i].DataSize);
        cbor_utils::map_put_int(cb_map_elem, ExPipeClient::KEY_Param6, msgs[i].ExtraDataIndex);
        cbor_utils::map_put_data(cb_map_elem, ExPipeClient::KEY_Param7, msgs[i].Data, msgs[i].DataSize);
        cbor_utils::arr_append_cbor(cb_array, cb_map_elem);
    }

    // first param is the array itself
    cbor_utils::map_put_array(cb_map_root, 1, cb_array);

    //put this map with array into root map
    cn_cbor_map_put(cb_map, cn_cbor_int_create(key, &cn_errback), cb_map_root, &cn_errback);

    return cn_errback.err == CN_CBOR_NO_ERROR;
}

bool cbor_utils::map_put_string(cn_cbor* cb_map, const char * key, const char * value)
{
    cn_cbor_errback cn_errback = {0};
    cn_cbor_map_put(cb_map, cn_cbor_string_create(key, &cn_errback), cn_cbor_string_create(value, &cn_errback), &cn_errback);
    return cn_errback.err == CN_CBOR_NO_ERROR;
}

bool cbor_utils::map_put_bool(cn_cbor* cb_map /* map */, const char * key, bool value)
{
    cn_cbor_errback cn_errback = {0};
    cn_cbor_map_put(cb_map, cn_cbor_string_create(key, &cn_errback), cn_cbor_bool_create(value, &cn_errback), &cn_errback);
    return cn_errback.err == CN_CBOR_NO_ERROR;
}

bool cbor_utils::map_put_map(cn_cbor* cb_map /* map */, const char * key, cn_cbor* value)
{
    cn_cbor_errback cn_errback = {0};
    cn_cbor_map_put(cb_map, cn_cbor_string_create(key, &cn_errback), value, &cn_errback);
    return cn_errback.err == CN_CBOR_NO_ERROR;
}

bool cbor_utils::map_put_map(cn_cbor* cb_map /* map */, uint32_t key, cn_cbor* value)
{
    cn_cbor_errback cn_errback = {0};
    cn_cbor_map_put(cb_map, cn_cbor_int_create(key, &cn_errback), value, &cn_errback);
    return cn_errback.err == CN_CBOR_NO_ERROR;
}

bool cbor_utils::arr_put_string(cn_cbor* cb_array, const char * value)
{
    cn_cbor_errback cn_errback = {0};
    cn_cbor_array_append(cb_array, cn_cbor_string_create(value, &cn_errback), &cn_errback);
    return cn_errback.err == CN_CBOR_NO_ERROR;
}

std::vector<uint8_t> cbor_utils::cbor_to_data(cn_cbor * cb)
{
    std::vector<uint8_t> cbor_data;
    auto cbor_size = cn_cbor_encoder_write(nullptr, 0, 0, cb);
    if(cbor_size != -1)
    {
        cbor_data.resize(cbor_size);
        cn_cbor_encoder_write(cbor_data.data(), 0, cbor_size, cb);
    }

    return cbor_data;
}
