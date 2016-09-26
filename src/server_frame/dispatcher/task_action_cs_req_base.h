//
// Created by owt50 on 2016/9/26.
//

#ifndef _DISPATCHER_TASK_ACTION_CS_REQ_BASE_H
#define _DISPATCHER_TASK_ACTION_CS_REQ_BASE_H

#pragma once

#include "task_action_base.h"

class session;

class task_action_cs_req_base : public task_action_base {
public:
    typedef task_action_base base_type;
    typedef hello::CSMsg msg_type;
    typedef msg_type& msg_ref_type;
    typedef const msg_ref_type msg_cref_type;
    typedef msg_type* msg_ptr_type;

protected:
    using base_type::get_request;

public:
    using base_type::name;
    using base_type::get_task_id;
    using base_type::set_ret_code;
    using base_type::get_ret_code;
    using base_type::set_req_code;
    using base_type::get_req_code;

public:
    task_action_ss_req_base();
    virtual ~task_action_ss_req_base();

    msg_ptr_type get_cs_request() const;

    std::pair<uint64_t, uint64_t> get_gateway_info() const;

    std::shared_ptr<session> get_session() const;

    std::shared_ptr<player> get_player() const;

    msg_ref_type add_rsp_msg();

    std::list<msg_type>& get_rsp_list() { return rsp_msgs_; }
    const std::list<msg_type>& get_rsp_list() const { return rsp_msgs_; }
protected:
    virtual void send_rsp_msg();

private:
    std::string player_openid_;
    mutable std::shared_ptr<player> player_inst_;
    std::list<msg_type> rsp_msgs_;
};


#endif //ATF4G_CO_TASK_ACTION_CS_REQ_BASE_H
