//
// Created by 文韬 on 2016/10/6.
//

#include <log/log_wrapper.h>

#include "task_action_auto_save_players.h"


task_action_auto_save_players::task_action_auto_save_players(): success_count_(0), failed_count_(0) {}

task_action_auto_save_players::~task_action_auto_save_players() {}

int task_action_auto_save_players::operator()(hello::message_container& msg) {
    success_count_ = failed_count_ = 0;

    return hello::err::EN_SUCCESS;
}

int task_action_auto_save_players::on_success() {
    WLOGINFO("auto save task done.(success save: %d, failed save: %d)", success_count_, failed_count_);

    if (0 == success_count_ && 0 == failed_count_) {
        WLOGWARNING("there is no need to start a auto save task when no user need save.");
    }
    return get_ret_code();
}

int task_action_auto_save_players::on_failed() {
    WLOGERROR("auto save task failed.(success save: %d, failed save: %d) ret: %d", success_count_, failed_count_, get_ret_code());
    return get_ret_code();
}

int task_action_auto_save_players::on_timeout() {
    WLOGWARNING("auto save task timeout, we will continue on next round.");
}