//
// Created by owent on 2016/9/23.
//

#ifndef _CONFIG_EXCEL_CONFIG_MANAGER_H
#define _CONFIG_EXCEL_CONFIG_MANAGER_H

#pragma once

#include <stdint.h>
#include <cstddef>
#include <string>
#include <std/functional.h>
#include <list>

#include <design_pattern/singleton.h>

namespace excel {
    class config_set_base;

    class config_manager : public util::design_pattern::singleton<config_manager> {
    public:
        typedef std::function<bool(std::string&, const std::string&)> read_buffer_func_t;

    protected:
        config_manager();
        ~config_manager();

    public:
        int init(read_buffer_func_t read_data_fn);

        bool load_file_data(std::string& write_to, const std::string& file_path);

        int reloadAll();

        void add_config_set(config_set_base* config_set);
    private:
        read_buffer_func_t read_file_handle_;
        std::list<config_set_base*> config_set_list_;
    };

}
#endif //ATF4G_CO_CONFIG_MANAGER_H
