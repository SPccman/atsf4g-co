//
// Created by owt50 on 2016/10/9.
//

#include <iostream>
#include <cstring>
#include <assert.h>

#include <cli/shell_font.h>

#include "simulator_base.h"
#include "simulator_player_impl.h"

simulator_player_impl::simulator_player_impl(): is_closing_(false), port_(0), owner_(NULL) {
    memset(&network_, 0, sizeof(network_));
}
simulator_player_impl::~simulator_player_impl() {
    assert(NULL == owner_);
    util::cli::shell_stream ss(std::cerr);
    ss()<< util::cli::shell_font_style::SHELL_FONT_COLOR_GREEN
        << "player "<< id_ << " destroyed"<< std::endl;
}

bool simulator_player_impl::set_id(const std::string& id) {
    if (id_ == id) {
        return true;
    }

    if (NULL == owner_) {
        id_ = id;
        return true;
    }

    std::string old_id = id;
    old_id.swap(id_);
    if(false == owner_->insert_player(watcher_.lock())) {
        id_.swap(old_id);
        util::cli::shell_stream ss(std::cerr);
        ss()<< util::cli::shell_font_style::SHELL_FONT_COLOR_RED
            << util::cli::shell_font_style::SHELL_FONT_SPEC_BOLD
            << "insert player "<<id << " failed"<< std::endl;

        return false;
    }

    if (!old_id.empty()) {
        owner_->remove_player(old_id, false);
    }

    return true;
}


// ================== receive ===================
void simulator_player_impl::libuv_on_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    simulator_player_impl* self = reinterpret_cast<simulator_player_impl*>(handle->data);
    assert(self);
    self->on_alloc(suggested_size, buf);
}

void simulator_player_impl::libuv_on_read_data(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    simulator_player_impl* self = reinterpret_cast<simulator_player_impl*>(stream->data);
    assert(self);
    self->on_read_data(nread, buf);
}

// ================= connect ==================
void simulator_player_impl::libuv_on_connected(uv_connect_t *req, int status) {
    ptr_t* self_sptr = reinterpret_cast<ptr_t*>(req->data);
    assert(self_sptr);
    ptr_t self = *self_sptr;
    req->data = NULL;

    if (0 != status) {
        util::cli::shell_stream ss(std::cerr);
        ss()<< util::cli::shell_font_style::SHELL_FONT_COLOR_RED
            << "libuv_on_connected callback failed, msg: "<< uv_strerror(status)<< std::endl;
        fprintf(stderr, "libuv_tcp_connect_callback callback failed, msg: %s\n", uv_strerror(status));

        self->on_connected(req, status);
        self->network_.tcp_sock.data = self_sptr;
        self->on_close();
        uv_close((uv_handle_t *)&self->network_.tcp_sock, libuv_on_closed);
        return;
    }

    req->handle->data = self.get();
    uv_read_start(req->handle, libuv_on_alloc, libuv_on_read_data);
    int res = self->on_connected(req, status);
    if (res < 0) {
        util::cli::shell_stream ss(std::cerr);
        ss()<< util::cli::shell_font_style::SHELL_FONT_COLOR_RED
            << "player on_connected callback failed, ret: "<< res<< std::endl;

        uv_read_stop(req->handle);
        self->network_.tcp_sock.data = self_sptr;
        self->on_close();
        uv_close((uv_handle_t *)&self->network_.tcp_sock, libuv_on_closed);
    } else {
        delete self_sptr;
    }
}

void simulator_player_impl::libuv_on_dns_callback(uv_getaddrinfo_t *req, int status, struct addrinfo *res) {
    ptr_t* self_sptr = reinterpret_cast<ptr_t*>(req->data);
    assert(self_sptr);
    ptr_t self = *self_sptr;
    req->data = NULL;

    do {
        if (0 != status) {
            util::cli::shell_stream ss(std::cerr);
            ss()<< util::cli::shell_font_style::SHELL_FONT_COLOR_RED
                << "uv_getaddrinfo callback failed, "<< uv_strerror(status)<< std::endl;
            break;
        }

        if (NULL != self->network_.tcp_sock.data) {
            break;
        }

        sockaddr_storage real_addr;
        uv_tcp_init(req->loop, &self->network_.tcp_sock);
        self->network_.tcp_sock.data = self.get();

        if (AF_INET == res->ai_family) {
            sockaddr_in *res_c = (struct sockaddr_in *)(res->ai_addr);
            char ip[17] = {0};
            uv_ip4_name(res_c, ip, sizeof(ip));
            uv_ip4_addr(ip, self->port_, (struct sockaddr_in *)&real_addr);
        } else if (AF_INET6 == res->ai_family) {
            sockaddr_in6 *res_c = (struct sockaddr_in6 *)(res->ai_addr);
            char ip[40] = {0};
            uv_ip6_name(res_c, ip, sizeof(ip));
            uv_ip6_addr(ip, self->port_, (struct sockaddr_in6 *)&real_addr);
        } else {
            util::cli::shell_stream ss(std::cerr);
            ss()<< util::cli::shell_font_style::SHELL_FONT_COLOR_RED
                << "uv_tcp_connect failed, ai_family not supported: "<< res->ai_family<< std::endl;
            break;
        }

        self->network_.connect_req.data = self_sptr;
        int res_code = uv_tcp_connect(&self->network_.connect_req, &self->network_.tcp_sock, (struct sockaddr *)&real_addr, libuv_on_connected);
        if (0 != res_code) {
            util::cli::shell_stream ss(std::cerr);
            ss()<< util::cli::shell_font_style::SHELL_FONT_COLOR_RED
                << "uv_tcp_connect failed, "<<  uv_strerror(res_code)<< std::endl;

            self->network_.tcp_sock.data = self_sptr;
            self->network_.connect_req.data = NULL;
            self->on_close();
            uv_close((uv_handle_t *)&self->network_.tcp_sock, libuv_on_closed);
            break;
        }
    } while (false);

    if (self->network_.connect_req.data != self_sptr && self->network_.tcp_sock.data != self_sptr) {
        delete self_sptr;
    }

    // free addrinfo
    if (NULL != res) {
        uv_freeaddrinfo(res);
    }
}

void simulator_player_impl::on_close() {}
void simulator_player_impl::on_closed() {}

int simulator_player_impl::connect(const std::string& host, int port) {
    if (is_closing_ || NULL == owner_) {
        return -1;
    }

    if (network_.dns_req.data != NULL) {
        return 0;
    }

    port_ = port;
    network_.dns_req.data = new ptr_t(watcher_.lock());
    assert(network_.dns_req.data);

    int ret = uv_getaddrinfo(owner_->get_loop(), &network_.dns_req, libuv_on_dns_callback, host.c_str(), NULL, NULL);
    if (0 != ret) {
        util::cli::shell_stream ss(std::cerr);
        ss()<< util::cli::shell_font_style::SHELL_FONT_COLOR_RED
            << "player connect to "<< host<< ":"<< port<< " failed, "<< uv_strerror(ret)<< std::endl;

        delete (ptr_t*)network_.dns_req.data;
        network_.dns_req.data = NULL;
        return -1;
    }

    return ret;
}

// ================= write data =================
static void simulator_player_impl::libuv_on_written_data(uv_write_t *req, int status) {
    ptr_t* self_sptr = reinterpret_cast<ptr_t*>(req->data);
    assert(self_sptr);
    req->data = NULL;
    ptr_t self = (*self_sptr);
    (*self_sptr)->network_.write_req.data = NULL;
    (*self_sptr)->network_write_holder_.reset();

    self->on_written_data(req, status);
}

int simulator_player_impl::write_message(void *buffer, uint64_t sz) {
    if (is_closing_) {
        util::cli::shell_stream ss(std::cerr);
        ss()<< util::cli::shell_font_style::SHELL_FONT_COLOR_RED
            << "closed or closing player "<<id_ << " can not send any data"<< std::endl;
        return -1;
    }

    if (NULL == network_.tcp_sock.data) {
        util::cli::shell_stream ss(std::cerr);
        ss()<< util::cli::shell_font_style::SHELL_FONT_COLOR_RED
            << "player "<<id_ << " socket not available"<< std::endl;
        return -1;
    }

    if (network_write_holder_) {
        util::cli::shell_stream ss(std::cerr);
        ss()<< util::cli::shell_font_style::SHELL_FONT_COLOR_RED
            << "player "<<id_ << " write data failed, another writhing is running."<< std::endl;
        return -1;
    }

    network_write_holder_ = watcher_.lock();
    network_.write_req.data = &network_write_holder_;
    uv_buf_t bufs[1] = {uv_buf_init(reinterpret_cast<char *>(buffer), static_cast<unsigned int>(sz))};
    int ret = uv_write(&network_.write_req, (uv_stream_t*)&network_.tcp_sock, bufs, 1, libuv_on_written_data);
    if (0 != ret) {
        network_.write_req.data = NULL;
        network_write_holder_.reset();

        util::cli::shell_stream ss(std::cerr);
        ss()<< util::cli::shell_font_style::SHELL_FONT_COLOR_RED
            << "player "<<id_ << " write data failed,"<< uv_strerror(ret)<< std::endl;
    }

    return ret;
}

int simulator_player_impl::read_message(const void *buffer, uint64_t sz) {
    if (is_closing_) {
        util::cli::shell_stream ss(std::cout);
        ss()<< util::cli::shell_font_style::SHELL_FONT_COLOR_YELLOW
            << "closed or closing player "<<id_ << " do not deal with any message any more"<< std::endl;
        return -1;
    }

    if (NULL == owner_) {
        util::cli::shell_stream ss(std::cout);
        ss()<< util::cli::shell_font_style::SHELL_FONT_COLOR_YELLOW
            << "player "<<id_ << " without simulator manager can not deal with any message"<< std::endl;
        return -1;
    }

    return owner_->dispatch_message(watcher_.lock(), buffer, sz);
}

// =================== close =======================
void simulator_player_impl::libuv_on_closed(uv_handle_t *handle) {
    ptr_t* self_sptr = reinterpret_cast<ptr_t*>(handle->data);
    assert(self_sptr);
    handle->data = NULL;

    (*self_sptr)->network_.tcp_sock.data = NULL;
    (*self_sptr)->on_closed();
    delete self_sptr;
}

int simulator_player_impl::close() {
    if (is_closing_) {
        return 0;
    }
    is_closing_ = true;

    on_close();

    // owner not available any more
    ptr_t self = watcher_.lock();
    assert(self.get());
    if (NULL != owner_) {
        owner_->remove_player(self);
        owner_ = NULL;
    }

    // close fd
    if (NULL != network_.tcp_sock.data) {
        network_.tcp_sock.data = new ptr_t(self);
        uv_close((uv_handle_t*)&network_.tcp_sock, libuv_on_closed);
    } else {
        on_closed();
    }

    return 0;
}

// ======================== insert cmd ======================
// this function must be thread-safe
int simulator_player_impl::insert_cmd(const std::string &cmd) {
    if (is_closing_) {
        util::cli::shell_stream ss(std::cerr);
        ss()<< util::cli::shell_font_style::SHELL_FONT_COLOR_RED
            << util::cli::shell_font_style::SHELL_FONT_SPEC_BOLD
            << "insert cmd into closed or closing player "<<id_ << " failed"<< std::endl;
        return -1;
    }

    if (NULL == owner_) {
        util::cli::shell_stream ss(std::cerr);
        ss()<< util::cli::shell_font_style::SHELL_FONT_COLOR_RED
            << util::cli::shell_font_style::SHELL_FONT_SPEC_BOLD
            << "insert cmd into player "<<id_ << " without simulator manager failed"<< std::endl;
        return -1;
    }

    return owner_->insert_cmd(watcher_.lock(), cmd);
}