//
// Created by owt50 on 2016/10/9.
//

#include <dispatcher/ss_msg_dispatcher.h>

#include <logic/task_action_player_kickoff.h>

#include "handle_ss_msg.h"

#define REG_MSG_HANDLE(dispatcher, ret, act, proto)\
    if (ret < 0) { dispatcher::me()->register_action<act>(proto); } \
    else { ret = dispatcher::me()->register_action<act>(proto); }

int app_handle_ss_msg::init() {
    int ret = 0;

    REG_MSG_HANDLE(ss_msg_dispatcher, ret, task_action_player_kickoff, "mss_player_kickoff_req");

    return ret;
}