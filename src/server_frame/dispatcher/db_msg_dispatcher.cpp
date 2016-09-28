//
// Created by owt50 on 2016/9/27.
//

#include <log/log_wrapper.h>

#include "db_msg_dispatcher.h"

#include <protocol/pbdesc/svr.container.pb.h>
#include <protocol/pbdesc/svr.const.err.pb.h>

const const char* db_msg_dispatcher::name() const UTIL_CONFIG_OVERRIDE {
    return "db_msg_dispatcher";
}

int32_t db_msg_dispatcher::init() UTIL_CONFIG_OVERRIDE;

int db_msg_dispatcher::tick() UTIL_CONFIG_OVERRIDE;

int32_t db_msg_dispatcher::unpack_msg(msg_ptr_t msg_container, const void* msg_buf, size_t msg_size) UTIL_CONFIG_OVERRIDE;

uint64_t db_msg_dispatcher::pick_msg_task(const msg_ptr_t msg_container) UTIL_CONFIG_OVERRIDE {
    if (NULL == msg_container || !msg_container->has_src_server()) {
        return 0;
    }

    return msg_container->src_server().dst_task_id();
}

const std::string& db_msg_dispatcher::pick_msg_name(const msg_ptr_t msg_container) UTIL_CONFIG_OVERRIDE {
    return get_empty_string();
}

db_msg_dispatcher::msg_type_t db_msg_dispatcher::pick_msg_type_id(const msg_ptr_t msg_container) UTIL_CONFIG_OVERRIDE {
    return 0;
}

db_msg_dispatcher::msg_type_t db_msg_dispatcher::msg_name_to_type_id(const std::string& msg_name) UTIL_CONFIG_OVERRIDE {
    return 0;
}