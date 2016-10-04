
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>


#include <atframe/atapp.h>
#include <time/time_utility.h>

#include <config/atframe_service_types.h>
#include <libatgw_server_protocol.h>
#include <config/extern_service_types.h>
#include <dispatcher/cs_msg_dispatcher.h>
#include <dispatcher/ss_msg_dispatcher.h>
#include <config/logic_config.h>


#ifdef _MSC_VER

#define INIT_CALL(MOD_NAME, ...) { \
    int res = MOD_NAME::me()->init(__VA_ARGS__);\
    if (res < 0) {\
        WLOGERROR("initialize %s failed, res: %d", #MOD_NAME, res); \
        return res; \
    }\
}

#define RELOAD_CALL(RET_VAR, MOD_NAME, ...) { \
    int res = MOD_NAME::me()->reload(__VA_ARGS__);\
    if (res < 0) {\
        WLOGERROR("reload %s failed, res: %d", #MOD_NAME, res); \
        RET_VAR = res; \
    }\
}

#else
#define INIT_CALL(MOD_NAME, args...) { \
    int res = MOD_NAME::me()->init(args);\
    if (res < 0) {\
        WLOGERROR("initialize %s failed, res: %d", #MOD_NAME, res); \
        return res; \
    }\
}

#define RELOAD_CALL(RET_VAR, MOD_NAME, args...) { \
    int res = MOD_NAME::me()->reload(args);\
    if (res < 0) {\
        WLOGERROR("reload %s failed, res: %d", #MOD_NAME, res); \
        RET_VAR = res; \
    }\
}

#endif

struct app_handle_on_msg {
    app_handle_on_msg() {}

    int operator()(atapp::app &app, const atapp::app::msg_t &msg, const void *buffer, size_t len) {
        if (NULL == msg.body.forward || 0 == msg.head.src_bus_id) {
            WLOGERROR("receive a message from unknown source");
            return app.get_bus_node()->send_data(msg.head.src_bus_id, msg.head.type, buffer, len);
        }

        int ret = 0;
        switch (msg.head.type) {
        case ::atframe::component::service_type::EN_ATST_GATEWAY: {
            ret = cs_msg_dispatcher::me()->dispatch(msg, buffer, len);
            break;
        }

        case ::atframe::component::ext_service_type::EN_ATST_SS_MSG: {
            ret = ss_msg_dispatcher::me()->dispatch(msg, buffer, len);
            break;
        }

        default: {
            WLOGERROR("receive a message of invalid type:%d", msg.head.type);
            break;
        }
        }

        return ret;
    }
};

static int app_handle_on_send_fail(atapp::app &app, atapp::app::app_id_t src_pd, atapp::app::app_id_t dst_pd, const atbus::protocol::msg &m) {
    WLOGERROR("send data from 0x%llx to 0x%llx failed", static_cast<unsigned long long>(src_pd), static_cast<unsigned long long>(dst_pd));
    return 0;
}

static int app_handle_on_connected(atapp::app &app, atbus::endpoint &ep, int status) {
    WLOGINFO("app 0x%llx connected, status: %d", static_cast<unsigned long long>(ep.get_id()), status);
    return 0;
}

static int app_handle_on_disconnected(atapp::app &app, atbus::endpoint &ep, int status) {
    WLOGINFO("app 0x%llx disconnected, status: %d", static_cast<unsigned long long>(ep.get_id()), status);
    return 0;
}


class main_service_module : public atapp::module_impl {
public:
    virtual int init() {
        WLOGINFO("============ server initialize ============");
        INIT_CALL(logic_config, get_app()->get_id());
        return 0;
    };

    virtual int reload() {
        WLOGINFO("============ server reload ============");
        int ret = 0;
        util::config::ini_loader &cfg = get_app()->get_configure();

        RELOAD_CALL(ret, logic_config, get_app()->get_configure());

        return ret;
    }

    virtual int stop() {
        WLOGINFO("============ server stop ============");
        return 0;
    }

    virtual int timeout() {
        WLOGINFO("============ server timeout ============");
        return 0;
    }

    virtual const char *name() const { return "main_service_module"; }

    virtual int tick() {
        return 0;
    }
};

int main(int argc, char *argv[]) {
    atapp::app app;

    app.add_module(std::make_shared<main_service_module>());

    // setup message handle
    app.set_evt_on_recv_msg(app_handle_on_msg());
    app.set_evt_on_send_fail(app_handle_on_send_fail);
    app.set_evt_on_app_connected(app_handle_on_connected);
    app.set_evt_on_app_disconnected(app_handle_on_disconnected);

    // run
    return app.run(uv_default_loop(), argc, (const char **)argv, NULL);
}