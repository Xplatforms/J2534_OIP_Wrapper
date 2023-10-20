#ifndef CBOR_UTILS_H
#define CBOR_UTILS_H

#include <cn-cbor/cn-cbor.h>
#include <vector>
#include "j2534.h"

class cbor_utils
{
public:
    cbor_utils() = default;

    static bool map_put_string(cn_cbor* cb_map /* map */, const char * key, const char * value);
    static bool map_put_bool(cn_cbor* cb_map /* map */, const char * key, bool value);
    static bool map_put_map(cn_cbor* cb_map /* map */, const char * key, cn_cbor* value);
    static bool arr_put_string(cn_cbor* cb_array /* array */, const char * value);

    static bool map_put_int(cn_cbor* cb_map /* map */, uint32_t key, uint32_t value);
    static bool map_put_string(cn_cbor* cb_map /* map */, uint32_t key, const char * value);
    static bool map_put_data(cn_cbor* cb_map /* map */, uint32_t key, const uint8_t* data, uint32_t len);
    static bool map_put_array(cn_cbor* cb_map /* map */, uint32_t key, cn_cbor* value);
    static bool map_put_map(cn_cbor* cb_map /* map */, uint32_t key, cn_cbor* value);
    static bool arr_append_cbor(cn_cbor* cb_array /* array */, cn_cbor* value);

    static bool map_put_PASSTHRU_MSG(cn_cbor* cb_map /* map */, uint32_t key, uint32_t msgs_count, const PASSTHRU_MSG * msgs);

    static std::vector<uint8_t> cbor_to_data(cn_cbor * cb);
};

#endif // CBOR_UTILS_H
