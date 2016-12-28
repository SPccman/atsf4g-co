//
// Created by owt50 on 2016/11/14.
//

#include <protocol/pbdesc/svr.container.pb.h>
#include <protocol/pbdesc/svr.const.err.pb.h>
#include <protocol/pbdesc/com.const.pb.h>

#include <log/log_wrapper.h>

#include "actor_action_base.h"

actor_action_base::actor_action_base(): ret_code_(0), rsp_code_(0), status_(EN_AAS_CREATED) {}
actor_action_base::~actor_action_base() {
    if (EN_AAS_FINISHED != status_) {
        WLOGERROR("actor %s [%p] is created but not run", name(), this);
        set_rsp_code(hello::EN_ERR_TIMEOUT);
        set_ret_code(hello::err::EN_SYS_TIMEOUT);
    }
}

const char* actor_action_base::name() const {
    const char *ret = typeid(*this).name();
    if (NULL == ret) {
        return "RTTI Unavailable: actor_action_base";
    }

    // some compiler will generate number to mark the type
    while (ret && *ret >= '0' && *ret <= '9') {
        ++ret;
    }
    return ret;
}

int32_t actor_action_base::run(msg_type* in) {
    if (get_status() > EN_AAS_CREATED) {
        WLOGERROR("actor %s [%p] already running", name(), this);
        return hello::err::EN_SYS_BUSY;
    }

    if (NULL != in) {
        request_msg_.Swap(in);
    }

    status_ = EN_AAS_RUNNING;
    WLOGDEBUG("actor %s [%p] start to run", name(), this);
    ret_code_ = (*this)(request_msg_);

    // 响应OnSuccess(这时候任务的status还是running)
    int32_t ret = 0;
    if (rsp_code_ < 0) {
        ret = on_failed();
        WLOGINFO("actor %s [%p] finished success but response errorcode, rsp code: %d\n", name(), this, rsp_code_);
    } else {
        ret = on_success();
    }

    send_rsp_msg();
    status_ = EN_AAS_FINISHED;
    return ret;
}

int actor_action_base::on_success() {
    return 0;
}

int actor_action_base::on_failed() {
    return 0;
}

hello::message_container& actor_action_base::get_request() {
    return request_msg_;
}

const hello::message_container& actor_action_base::get_request() const {
    return request_msg_;
}
