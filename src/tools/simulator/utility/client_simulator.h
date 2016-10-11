//
// Created by owt50 on 2016/10/11.
//

#ifndef ATFRAMEWORK_LIBSIMULATOR_UTILITY_CLIENT_SIMULATOR_H
#define ATFRAMEWORK_LIBSIMULATOR_UTILITY_CLIENT_SIMULATOR_H

#pragma once

#include <protocol/pbdesc/com.protocol.pb.h>

#include <simulator_base.h>

#include "client_player.h"

class client_simulator : public simulator_msg_base<client_player, hello::CSMsg> {
public:
    typedef client_simulator self_type;
    typedef simulator_msg_base<client_player, hello::CSMsg> base_type;
    typedef typename base_type::player_t player_t;
    typedef typename base_type::player_ptr_t player_ptr_t;
    typedef typename base_type::msg_t msg_t;

public:
    virtual ~client_simulator();

    virtual uint32_t pick_message_id(const msg_t& msg) const;
    virtual std::string pick_message_name(const msg_t& msg) const;
    virtual const std::string& dump_message(const msg_t& msg);

    virtual int pack_message(const msg_t& msg, void* buffer, size_t& sz) const = 0;
    virtual int unpack_message(msg_t& msg, const void* buffer, size_t sz) const = 0;

    static client_simulator* cast(simulator_base* b);
};


#endif //ATFRAMEWORK_LIBSIMULATOR_UTILITY_CLIENT_SIMULATOR_H
