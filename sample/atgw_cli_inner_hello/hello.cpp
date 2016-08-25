#include <cstdio>
#include <cstdlib>
#include <string>
#include <algorithm>

#include "uv.h"

#include "common/string_oprs.h"

#include <config/atframe_services_build_feature.h>
#include <proto_base.h>
#include <inner_v1/libatgw_proto_inner.h>

struct client_libuv_data_t {
    uv_tcp_t tcp_sock;
    uv_connect_t tcp_req;
    uv_getaddrinfo_t dns_req;
    uv_write_t write_req;
    uv_timer_t tick_timer;
};

client_libuv_data_t g_client;

struct client_session_data_t {
    uint64_t session_id;
    long long seq;
    std::unique_ptr< ::atframe::gateway::libatgw_proto_inner_v1> proto;
    ::atframe::gateway::proto_base::proto_callbacks_t callbacks;

    bool print_recv;
};

client_session_data_t g_client_sess;

::atframe::gateway::libatgw_proto_inner_v1::crypt_conf_t g_crypt_conf;
std::string g_host;
int g_port;

// ======================== ����Ϊ���紦�����ص� ========================
static int close_sock();

static void libuv_tcp_recv_alloc_fn(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    if (!g_client_sess.proto) {
        uv_read_stop((uv_stream_t*)handle);
        return;
    }


#if _MSC_VER
    size_t len = 0;
    g_client_sess.proto->alloc_recv_buffer(suggested_size, buf->base, len);
    buf->len = static_cast<ULONG>(len);
#else
    size_t len = 0;
    g_client_sess.proto->alloc_recv_buffer(suggested_size, buf->base, len);
    buf->len = len;
#endif

    if (NULL == buf->base && 0 == buf->len) {
        uv_read_stop((uv_stream_t*)handle);
    }
}

static void libuv_tcp_recv_read_fn(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    // if no more data or EAGAIN or break by signal, just ignore
    if (0 == nread || UV_EAGAIN == nread || UV_EAI_AGAIN == nread || UV_EINTR == nread) {
        return;
    }

    // if network error or reset by peer, move session into reconnect queue
    if (nread < 0) {
        // notify to close fd
        close_sock();
        return;
    }

    if (g_client_sess.proto) {
        int errcode = 0;
        g_client_sess.proto->read(static_cast<int>(nread), buf->base, buf->len, errcode);
        if (0 != errcode) {
            fprintf(stderr, "[Read]: failed, res: %d\n", errcode);
            close_sock();
        }
    }
}

static void libuv_tcp_connect_callback(uv_connect_t *req, int status) {
    req->data = NULL;
    if (0 != status) {
        fprintf(stderr, "libuv_tcp_connect_callback callback failed, msg: %s\n", uv_strerror(status));
        uv_stop(req->handle->loop);
        return;
    }

    uv_read_start(req->handle, libuv_tcp_recv_alloc_fn, libuv_tcp_recv_read_fn);

    g_client_sess.proto.reset(new ::atframe::gateway::libatgw_proto_inner_v1());
    g_client_sess.proto->set_recv_buffer_limit(2 * 1024 * 1024, 0);
    g_client_sess.proto->set_send_buffer_limit(2 * 1024 * 1024, 0);

    int ret = g_client_sess.proto->start_session();
    if (0 != ret) {
        fprintf(stderr, "start session failed, res: %d", ret);
        g_client_sess.proto->close(0);
    }
}

static void libuv_dns_callback(uv_getaddrinfo_t* req, int status, struct addrinfo* res) {
    req->data = NULL;
    if (0 != status) {
        fprintf(stderr, "uv_getaddrinfo callback failed, msg: %s\n", uv_strerror(status));
        uv_stop(req->loop);
        return;
    }

    if (NULL != g_client.dns_req.data || NULL != g_client.tcp_sock.data) {
        return;
    }

    uv_tcp_init(req->loop, &g_client.tcp_sock);
    int res_code = uv_tcp_connect(&g_client.tcp_req, &g_client.tcp_sock, res->ai_addr, libuv_tcp_connect_callback);
    if (0 != res_code) {
        fprintf(stderr, "uv_tcp_connect failed, msg: %s\n", uv_strerror(res_code));
        uv_stop(req->loop);
    }
    g_client.dns_req.data = &g_client;
    g_client.tcp_sock.data = &g_client;
}

static int start_connect() {
    if (NULL != g_client.dns_req.data) {
        return 0;
    }

    int ret = uv_getaddrinfo(uv_default_loop(), &g_client.dns_req, libuv_dns_callback, g_host.c_str(), NULL, NULL);
    if (0 != ret) {
        fprintf(stderr, "uv_getaddrinfo failed, msg: %s\n", uv_strerror(ret));
        return ret;
    }

    g_client.dns_req.data = &g_client;
    return 0;
}

static void libuv_close_sock_callback(uv_handle_t* handle) {
    handle->data = NULL;
    printf("close socket finished\n");
}

int close_sock() {
    if (!g_client_sess.proto) {
        if (NULL != g_client.tcp_sock.data) {
            printf("close socket start\n");
            uv_close((uv_handle_t*)&g_client.tcp_sock, libuv_close_sock_callback);
        }

        return 0;
    }

    printf("close protocol start\n");
    return g_client_sess.proto->close(0);
}

// ======================== ����ΪЭ�鴦���ص� ========================
static void proto_inner_callback_on_written_fn(uv_write_t *req, int status) {
    if (g_client_sess.proto) {
        g_client_sess.proto->write_done(status);
    }
}

static int proto_inner_callback_on_write(::atframe::gateway::proto_base *proto, void *buffer, size_t sz, bool *is_done) {
    if (!g_client_sess.proto || NULL == buffer) {
        if (NULL != is_done) {
            *is_done = true;
        }
        return ::atframe::gateway::error_code_t::EN_ECT_PARAM;
    }

    if (0 == g_client_sess.session_id) {
        if (NULL != is_done) {
            *is_done = true;
        }
        return -1;
    }

    uv_buf_t bufs[1] = { uv_buf_init(reinterpret_cast<char *>(buffer), static_cast<unsigned int>(sz)) };
    int ret = uv_write(&g_client.write_req, (uv_stream_t*)&g_client.tcp_sock, bufs, 1, proto_inner_callback_on_written_fn);
    if (0 != ret) {
        fprintf(stderr, "send data to proto 0x%llx failed, msg: %s\n", g_client_sess.session_id, uv_strerror(ret));
    }

    if (NULL != is_done) {
        // if not writting, notify write finished
        *is_done = (0 != ret);
    }

    return ret;
}

static int proto_inner_callback_on_message(::atframe::gateway::proto_base *proto, const void *buffer, size_t sz) {
    if (g_client_sess.print_recv && NULL != buffer && sz > 0) {
        printf("[recv message]: %s\n", reinterpret_cast<const char*>(buffer));
    }
    return 0;
}

// useless
static int proto_inner_callback_on_new_session(::atframe::gateway::proto_base *proto, uint64_t &sess_id) {
    printf("create session 0x%llx\n", sess_id);
    return 0;
}

// useless
static int proto_inner_callback_on_reconnect(::atframe::gateway::proto_base *proto, uint64_t sess_id) {
    printf("reconnect session 0x%llx\n", sess_id);
    return 0;
}

static int proto_inner_callback_on_close(::atframe::gateway::proto_base *proto, int reason) {
    if (NULL == g_client.tcp_sock.data) {
        return 0;
    }

    printf("close socket start\n");
    uv_close((uv_handle_t*)&g_client.tcp_sock, libuv_close_sock_callback);
    return 0;
}

static int proto_inner_callback_on_error(::atframe::gateway::proto_base *, const char *filename, int line, int errcode, const char *errmsg) {
    fprintf(stderr, "[Error][%s:%d]: error code: %d, msg: %s\n", filename, line, errcode, errmsg);
    return 0;
}

static void libuv_tick_timer_callback(uv_timer_t* handle) {
    if (!g_client_sess.proto) {
        return;
    }

    if (0 == g_client_sess.session_id) {
        return;
    }

    if (g_client_sess.proto->check_flag(::atframe::gateway::proto_base::flag_t::EN_PFT_CLOSING)) {
        return;
    }

    if (!g_client_sess.proto->check_flag(::atframe::gateway::proto_base::flag_t::EN_PFT_HANDSHAKE_DONE)) {
        return;
    }

    char msg[64] = {0};
    UTIL_STRFUNC_SNPRINTF(msg, sizeof(msg), "hello 0x%llx, %lld", g_client_sess.session_id, (++g_client_sess.seq));
    int ret = g_client_sess.proto->send_post(msg, strlen(msg));
    printf("[Tick]: send %s, res: %d\n", msg, ret);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <ip> <port> [dhparam] [mode]\n\tmode can only be tick\n", argv[0]);
        return -1;
    }

    // init
    g_client_sess.callbacks.write_fn = proto_inner_callback_on_write;
    g_client_sess.callbacks.message_fn = proto_inner_callback_on_message;
    g_client_sess.callbacks.new_session_fn = proto_inner_callback_on_new_session;
    g_client_sess.callbacks.reconnect_fn = proto_inner_callback_on_reconnect;
    g_client_sess.callbacks.close_fn = proto_inner_callback_on_close;
    g_client_sess.callbacks.on_error_fn = proto_inner_callback_on_error;

    g_client_sess.session_id = 0;
    g_client_sess.seq = 0;
    g_client_sess.print_recv = false;

    g_crypt_conf.default_key = "default";
    g_crypt_conf.update_interval = 600;
    g_crypt_conf.type = ::atframe::gw::inner::v1::crypt_type_t_EN_ET_NONE;
    g_crypt_conf.switch_secret_type = ::atframe::gw::inner::v1::switch_secret_t_EN_SST_DIRECT;
    g_crypt_conf.keybits = 128;
    g_crypt_conf.rsa_sign_type = ::atframe::gw::inner::v1::rsa_sign_t_EN_RST_PKCS1;
    g_crypt_conf.hash_id = ::atframe::gw::inner::v1::hash_id_t_EN_HIT_MD5;

    std::string mode = "tick";
    g_host = argv[1];
    g_port = static_cast<int>(strtol(argv[2], NULL, 10));
    memset(&g_client, 0, sizeof(g_client));
    if (argc > 3) {
        g_crypt_conf.type = ::atframe::gw::inner::v1::crypt_type_t_EN_ET_XXTEA;
        g_crypt_conf.switch_secret_type = ::atframe::gw::inner::v1::switch_secret_t_EN_SST_DH;
        g_crypt_conf.dh_param = argv[3];
    }

    if (argc > 4) {
        mode = argv[4];
        std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);
    }

    // crypt data
    int ret = ::atframe::gateway::libatgw_proto_inner_v1::global_reload(g_crypt_conf);
    if (0 != ret) {
        fprintf(stderr, "reload crypt info failed, res: %d", ret);
        return ret;
    }

    if ("tick" == mode) {
        uv_timer_init(uv_default_loop(), &g_client.tick_timer);
        uv_timer_start(&g_client.tick_timer, libuv_tick_timer_callback, 2000, 2000);
        g_client_sess.print_recv = true;
    } else {
        fprintf(stderr, "unsupport mode %s\n", mode.c_str());
        return -1;
    }

    ret = start_connect();
    if (ret < 0) {
        return ret;
    }

    return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}