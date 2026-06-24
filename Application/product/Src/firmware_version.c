#include "firmware_version.h"

static const firmware_version_info_t s_firmware_version = {
    .product_name = FW_PRODUCT_NAME,
    .major = (uint8_t) FW_VERSION_MAJOR,
    .minor = (uint8_t) FW_VERSION_MINOR,
    .patch = (uint8_t) FW_VERSION_PATCH,
    .prerelease = FW_VERSION_PRERELEASE,
    .version_string = FW_VERSION_STRING,
    .version_string_full = FW_VERSION_STRING_FULL,
    .version_tag = FW_VERSION_TAG,
    .version_num = FW_VERSION_NUM,
    .git_sha_short = FW_GIT_SHA_SHORT,
    .git_describe = FW_GIT_DESCRIBE,
    .git_branch = FW_GIT_BRANCH,
    .git_dirty = (uint8_t) FW_GIT_DIRTY,
};

const firmware_version_info_t* firmware_version_get(void)
{
    return &s_firmware_version;
}

const char* firmware_version_string(void)
{
    return s_firmware_version.version_string;
}

const char* firmware_version_string_full(void)
{
    return s_firmware_version.version_string_full;
}

const char* firmware_version_tag(void)
{
    return s_firmware_version.version_tag;
}

uint32_t firmware_version_num(void)
{
    return s_firmware_version.version_num;
}
