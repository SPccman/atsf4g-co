//
// Created by owt50 on 2016/10/9.
//

#include <dispatcher/ss_msg_dispatcher.h>

#include "handle_ss_msg.h"

#define REG_MSG_HANDLE(dispatcher, ret, act, proto)\
    if (ret < 0) { dispatcher::me()->register_action<act>(proto); } \
    else { ret = dispatcher::me()->register_action<act>(proto); }

int app_handle_ss_msg::init() {
    int ret = 0;
    return ret;
}