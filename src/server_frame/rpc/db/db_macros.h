//
// Created by owt50 on 2016/9/28.
//

#ifndef _RPC_DB_DB_MACROS_H
#define _RPC_DB_DB_MACROS_H

#pragma once

#include <common/string_oprs.h>
#include <config/logic_config.h>

#define DBGETGLOBALKEY_FMT(table_name) "g:" #table_name ":%s"

#define DBGETGLOBALKEY_ARG(id) id

#define DBGETTABLEKEY_FMT(table_name) "z-%u:" #table_name ":%s"

// get zone id
#define DBGETTABLEKEY_ARG(openid) logic_config::me()->get_cfg_logic().zone_id, openid

#define DBGETZONETABLEKEY_ARG(zone, openid) zone, openid

#define DBUSERCMD(table_name, cmd, openid, format, args...) #cmd " " DBGETTABLEKEY_FMT(table_name) " " format, \
    DBGETTABLEKEY_ARG(openid), ##args

#define DBZONEUSERCMD(zone, table_name, cmd, openid, format, args...) #cmd " " DBGETTABLEKEY_FMT(table_name) " " format, \
    DBGETZONETABLEKEY_ARG(zone, openid), ##args

#define DBUSERKEYWITHVAR(table_name, keyvar, keylen, v) \
    { \
        int __snprintf_writen_length = UTIL_STRFUNC_SNPRINTF(keyvar, static_cast<int>(keylen), DBGETTABLEKEY_FMT(table_name), DBGETTABLEKEY_ARG(v)); \
        if (__snprintf_writen_length < 0) { \
            keyvar[sizeof(keyvar) - 1] = '\0'; \
            keylen = 0; \
        } else { \
            keylen = static_cast<size_t>(__snprintf_writen_length); \
            keyvar[__snprintf_writen_length] = '\0'; \
        } \
    }

#define DBZONEUSERKEYWITHVAR(zone, table_name, keyvar, keylen, v) \
    { \
        int __snprintf_writen_length = UTIL_STRFUNC_SNPRINTF(keyvar, static_cast<int>(keylen), DBGETTABLEKEY_FMT(table_name), DBGETZONETABLEKEY_ARG(zone, v)); \
        if (__snprintf_writen_length < 0) { \
            keyvar[sizeof(keyvar) - 1] = '\0'; \
            keylen = 0; \
        } else { \
            keylen = static_cast<size_t>(__snprintf_writen_length); \
            keyvar[__snprintf_writen_length] = '\0'; \
        } \
    }

#define DBUSERKEY(table_name, keyvar, keylen, v) \
    char keyvar[256]; \
    size_t keylen = sizeof(keyvar) - 1; \
    DBUSERKEYWITHVAR(table_name, keyvar, keylen, v)

#define DBZONEUSERKEY(zone, table_name, keyvar, keylen, v) \
    char keyvar[256]; \
    size_t keylen = sizeof(keyvar) - 1; \
    DBZONEUSERKEYWITHVAR(zone, table_name, keyvar, keylen, v)

#define DBGLOBALCMD(table_name, cmd, id, format, args...) #cmd " " DBGETGLOBALKEY_FMT(table_name) " " format, \
    DBGETGLOBALKEY_ARG(id), ##args

#define DBGLOBALKEY(table_name, keyvar, keylen, v)  \
    char keyvar[256]; \
    size_t keylen = 0; \
    { \
        int __snprintf_writen_length = UTIL_STRFUNC_SNPRINTF(keyvar, static_cast<int>(sizeof(keyvar)) - 1, DBGETGLOBALKEY_FMT(table_name), DBGETGLOBALKEY_ARG(v)); \
        if (__snprintf_writen_length < 0) { \
            keyvar[sizeof(keyvar) - 1] = keyvar[0] = '\0'; \
        } else { \
            keylen = static_cast<size_t>(__snprintf_writen_length); \
            keyvar[__snprintf_writen_length] = '\0'; \
        } \
    }


// ################# macros for tables #################
#define DBLOGINCMD(cmd, openid, format, args...) DBUSERCMD(login, cmd, openid, format, ##args)
#define DBLOGINKEY(keyvar, keylen, v) DBUSERKEY(login, keyvar, keylen, v)

#define DBPLAYERCMD(cmd, openid, format, args...) DBUSERCMD(player, cmd, openid, format, ##args)
#define DBPLAYERKEY(keyvar, keylen, v) DBUSERKEY(player, keyvar, keylen, v)


namespace rpc {
    namespace db {

        /**
         * allocate a buffer in specify buffer block and align address to type Ty
         * @note it's useful in allocate args when using redis to store data and using reflect to pack message.
         *       because object's address must be align to type size on x86 or ARM architecture, such as size_t, uint32_t, uint64_t and etc.
         * @param buf_addr input available buffer block, output left available address
         * @param buf_len input available buffer length, output left available length
         * @return allocated buffer address, NULL if failed
         */
        template<typename Ty>
        Ty* align_alloc(void*& buf_addr, size_t& buf_len) {
            out = NULL;
            if (NULL == buf_addr) {
                return NULL;
            }

            uintptr_t in_addr = (uintptr_t)buf_addr;
            uintptr_t padding_sz = (size_t)(in_addr % sizeof(Ty));
            if (0 != padding_sz) {
                padding_sz = sizeof(Ty) - padding_sz;
                in_addr += padding_sz;
            }

            // buffer not enough
            if (buf_len < sizeof(Ty) + padding_sz) {
                return NULL;
            }

            buf_len -= sizeof(Ty) + padding_sz;
            buf_addr = (void*)(in_addr + sizeof(Ty));
            return (Ty*)(in_addr);
        }
    }
}

#endif //_RPC_DB_DB_MACROS_H
