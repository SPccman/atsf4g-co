//
// Created by owt50 on 2016/9/23.
//

#include <log/log_wrapper.h>
#include "config_manager.h"
#include "config_set.h"

namespace excel {
    config_manager::config_manager() {}
    config_manager::~config_manager() {}

    int config_manager::init(read_buffer_func_t read_data_fn) {
        read_file_handle_ = read_data_fn;

        // 过滤掉excel里的空数据
        //config_player_init_items::Instance()->add_filter([](const config_player_init_items::value_type &v) { return v->id() > 0; });

        //config_player_init_items::Instance()->init("init_items_cfg",
        //                                        [](config_player_init_items::value_type v) { return config_player_init_items::key_type(v->id()); });
        return 0;
    }

    bool config_manager::load_file_data(std::string& write_to, const std::string& file_path) {
        if (!read_file_handle_) {
            WLOGERROR("invalid file data loader.");
            return false;
        }

        return read_file_handle_(write_to, file_path);
    }

    int config_manager::reload_all() {
        int ret = 0;
        for (config_set_base *cs : config_set_list_) {
            bool res = cs->reload();
            ret += res ? 1 : 0;
        }

        // index update

        return ret;
    }

    void config_manager::add_config_set(config_set_base* cs) {
        if (NULL == cs) {
            WLOGERROR("add null config set is not allowed.");
            return;
        }

        return config_set_list_.push_back(cs);
    }
}