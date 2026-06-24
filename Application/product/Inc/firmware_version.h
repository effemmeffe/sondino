#ifndef FIRMWARE_VERSION_H
#define FIRMWARE_VERSION_H

#include <stdint.h>

#include "firmware_version_gen.h"

typedef struct
{
    const char* product_name;
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    const char* prerelease;
    const char* version_string;
    const char* version_string_full;
    const char* version_tag;
    uint32_t version_num;
    const char* git_sha_short;
    const char* git_describe;
    const char* git_branch;
    uint8_t git_dirty;
} firmware_version_info_t;

const firmware_version_info_t* firmware_version_get(void);

const char* firmware_version_string(void);
const char* firmware_version_string_full(void);
const char* firmware_version_tag(void);
uint32_t firmware_version_num(void);

#endif
