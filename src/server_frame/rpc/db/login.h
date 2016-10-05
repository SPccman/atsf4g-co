//
// Created by owt50 on 2016/9/28.
//

#ifndef _RPC_DB_LOGIN_H
#define _RPC_DB_LOGIN_H

#pragma once

#include <stdint.h>
#include <cstddef>
#include <string>
#include <vector>

#include <protocol/pbdesc/svr.container.pb.h>

namespace rpc {
    namespace db {
        namespace login {

            /**
             * @brief 获取登入表的rpc操作
             * @param openid 登入用户的openid
             * @param rsp 返回的登入信息
             * @return 0或错误码
             */
            int get(const char* openid, hello::table_login& rsp, std::string &version);

            /**
             * @brief 获取登入表的rpc操作
             * @param zone_id 大区id
             * @param openid 登入用户的openid
             * @param rsp 返回的登入信息
             * @return 0或错误码
             */
            int get(uint32_t zone_id, const char* openid, hello::table_login& rsp, std::string &version);

            /**
             * @brief 设置登入表的rpc操作
             * @param openid 登入用户的openid
             * @param store 要保持的数据
             * @return 0或错误码
             */
            int set(const char* openid, hello::table_login& store, std::string &version);
        }
    }
}

#endif //ATF4G_CO_LOGIN_H
