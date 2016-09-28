//
// Created by owt50 on 2016/9/27.
//

#include <log/log_wrapper.h>

#include "cs_msg_dispatcher.h"

#include <protocol/pbdesc/svr.container.pb.h>
#include <protocol/pbdesc/svr.const.err.pb.h>


const const char* cs_msg_dispatcher::name() const UTIL_CONFIG_OVERRIDE {
    return "cs_msg_dispatcher";
}

int32_t cs_msg_dispatcher::init() UTIL_CONFIG_OVERRIDE {
    return 0;
}

int32_t cs_msg_dispatcher::unpack_msg(msg_ptr_t msg_container, const void* msg_buf, size_t msg_size) UTIL_CONFIG_OVERRIDE {
    if (NULL == msg_container) {
        WLOGERROR("parameter error");
        return hello::err::EN_SYS_PARAM;
    }

    if(false == msg_container->mutable_csmsg()->ParseFromArray(msg_buf, static_cast<int>(msg_size))) {
        WLOGERROR("unpack msg failed\n%s", msg_container->mutable_csmsg()->InitializationErrorString().c_str());
        return hello::err::EN_SYS_UNPACK;
    }

    return hello::err::EN_SUCCESS;
}

uint64_t cs_msg_dispatcher::pick_msg_task(const msg_ptr_t msg_container) UTIL_CONFIG_OVERRIDE {
    // cs msg not allow resume task
    return 0;
}

const std::string& cs_msg_dispatcher::pick_msg_name(const msg_ptr_t msg_container) UTIL_CONFIG_OVERRIDE {
    if (NULL == msg_container) {
        return get_empty_string();
    }

    if(false == msg_container->has_csmsg()) {
        return get_empty_string();
    }

    if(false == msg_container->csmsg().has_body()) {
        return get_empty_string();
    }

    std::vector<const google::protobuf::FieldDescriptor*> output;
    msg_container->csmsg().body().GetReflection()->ListFields(msg_container->csmsg().body(), &output);
    if (output.empty()) {
        return get_empty_string();
    }

    if (output.size() > 1) {
        WLOGERROR("there is more than one body");
        for(size_t i = 0; i < output.size(); ++ i) {
            WLOGERROR("body[%d]=%s", static_cast<int>(i), output[i]->name().c_str());
        }
    }

    return output[0]->name();
}

cs_msg_dispatcher::msg_type_t cs_msg_dispatcher::pick_msg_type_id(const msg_ptr_t msg_container) UTIL_CONFIG_OVERRIDE {
    if (NULL == msg_container) {
        return 0;
    }

    if(false == msg_container->has_csmsg()) {
        return 0;
    }

    if(false == msg_container->csmsg().has_body()) {
        return 0;
    }

    std::vector<const google::protobuf::FieldDescriptor*> output;
    msg_container->csmsg().body().GetReflection()->ListFields(msg_container->csmsg().body(), &output);
    if (output.empty()) {
        return 0;
    }

    if (output.size() > 1) {
        WLOGERROR("there is more than one body");
        for(size_t i = 0; i < output.size(); ++ i) {
            WLOGERROR("body[%d]=%s", static_cast<int>(i), output[i]->name().c_str());
        }
    }

    return static_cast<msg_type_t>(output[0]->number());
}

cs_msg_dispatcher::msg_type_t cs_msg_dispatcher::msg_name_to_type_id(const std::string& msg_name) UTIL_CONFIG_OVERRIDE {
    hello::CSMsgBody empty_body;
    const google::protobuf::FieldDescriptor* desc = empty_body.GetDescriptor()->FindFieldByName(msg_name);
    if (NULL == desc) {
        return 0;
    }

    return static_cast<msg_type_t>(desc->number());
}