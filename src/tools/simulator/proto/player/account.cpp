//
// Created by owt50 on 2016/10/11.
//

#include <cli/cmd_option.h>

#include <utility/client_simulator.h>
#include <simulator_active.h>

namespace proto {
    namespace player {
        void on_cmd_login_auth(util::cli::callback_param);
        void on_rsp_login_auth(client_simulator::player_ptr_t player, client_simulator::msg_t& msg);

        void on_cmd_login(util::cli::callback_param);
        void on_rsp_login(client_simulator::player_ptr_t player, client_simulator::msg_t& msg);

        void on_cmd_ping(util::cli::callback_param);
        void on_rsp_pong(client_simulator::player_ptr_t player, client_simulator::msg_t& msg);

        void on_cmd_get_info(util::cli::callback_param);
        void on_rsp_get_info(client_simulator::player_ptr_t player, client_simulator::msg_t& msg);
    }
}

SIMULATOR_ACTIVE(player_account, base) {
    client_simulator::cast(base)->reg_req()["Player"]["Login"].bind(proto::player::on_cmd_login_auth, "<openid> [system id=0] [platform=1] [access=''] [use gamesvr=0] login into loginsvr");
    client_simulator::cast(base)->reg_rsp("msc_login_auth_rsp", proto::player::on_rsp_login_auth);

    client_simulator::cast(base)->reg_req()["Player"]["LoginGame"].bind(proto::player::on_cmd_login, "login into gamesvr");
    client_simulator::cast(base)->reg_rsp("msc_login_rsp", proto::player::on_rsp_login);

    client_simulator::cast(base)->reg_req()["Player"]["Ping"].bind(proto::player::on_cmd_ping, "send ping package");
    client_simulator::cast(base)->reg_rsp("msc_pong_rsp", proto::player::on_rsp_pong);

    client_simulator::cast(base)->reg_req()["Player"]["GetInfo"].bind(proto::player::on_cmd_get_info, "[segments...] get player data");
    client_simulator::cast(base)->reg_rsp("msc_player_getinfo_rsp", proto::player::on_rsp_get_info);
    // special auto complete for GetInfo
}