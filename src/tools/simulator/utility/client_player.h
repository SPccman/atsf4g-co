//
// Created by owt50 on 2016/10/11.
//

#ifndef ATFRAMEWORK_LIBSIMULATOR_UTILITY_CLIENT_PLAYER_H
#define ATFRAMEWORK_LIBSIMULATOR_UTILITY_CLIENT_PLAYER_H

#pragma once

#include <simulator_player_impl.h>

class client_player : public simulator_player_impl {
public:
    virtual ~client_player();

    virtual void on_connected(uv_connect_t *req, int status) = 0;
    virtual void on_alloc(size_t suggested_size, uv_buf_t* buf) = 0;
    virtual void on_read_data(ssize_t nread, const uv_buf_t *buf) = 0;
    virtual void on_read_message(const void* buffer, size_t sz) = 0;
    virtual void on_written_data(uv_write_t *req, int status) = 0;
    virtual int on_write_message(void *buffer, uint64_t sz) = 0;

    virtual void on_close();
    virtual void on_closed();
};


#endif //ATFRAMEWORK_LIBSIMULATOR_UTILITY_CLIENT_PLAYER_H
