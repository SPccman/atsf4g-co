
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


struct app_handle_on_msg {
    app_handle_on_msg() {}

    int operator()(atapp::app &app, const atapp::app::msg_t &msg, const void *buffer, size_t len) {
        if (NULL == msg.body.forward || 0 == msg.head.src_bus_id) {
            WLOGERROR("receive a message from unknown source");
            return app.get_bus_node()->send_data(msg.head.src_bus_id, msg.head.type, buffer, len);
        }

        switch (msg.head.type) {
        case ::atframe::component::service_type::EN_ATST_GATEWAY: {
            break;
        }

        default:
            WLOGERROR("receive a message of invalid type:%d", msg.head.type);
            break;
        }

        return 0;
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

int main(int argc, char *argv[]) {
    atapp::app app;

    // setup message handle
    app.set_evt_on_recv_msg(app_handle_on_msg(&gws));
    app.set_evt_on_send_fail(app_handle_on_send_fail);
    app.set_evt_on_app_connected(app_handle_on_connected);
    app.set_evt_on_app_disconnected(app_handle_on_disconnected);

    // run
    return app.run(uv_default_loop(), argc, (const char **)argv, NULL);
}
