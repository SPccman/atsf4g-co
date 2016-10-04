//
// Created by owt50 on 2016/9/23.
//

#include <stringstream>
#include <common/string_oprs.h>
#include <std/foreach.h>

#include "logic_config.h"

logic_config::logic_config(): bus_id_(0){}
logic_config::~logic_config() {}


int logic_config::init(uint64_t bus_id) {
    bus_id_ = bus_id;
    return 0;
}

int logic_config::reload(util::config::ini_loader& cfg_set) {
    const util::config::ini_value::node_type& children = cfg_set.get_root_node().get_children();
    if (children.find("logic") != children.end()) {
        _load_logic(cfg_set);
    }

    if (children.find("db") != children.end()) {
        _load_db(cfg_set);
    }

    if (children.find("gamesvr") != children.end()) {
        _load_gamesvr(cfg_set);
    }

    if (children.find("loginsvr") != children.end()) {
        _load_loginsvr(cfg_set);
    }

    return 0;
}

uint64_t logic_config::get_self_bus_id() const {
    return bus_id_;
}

void logic_config::_load_logic(util::config::ini_loader& loader);

void logic_config::_load_db(util::config::ini_loader& loader);
void logic_config::_load_db_hosts(std::vector<LC_DBCONN>& out, const char* group_name, util::config::ini_loader& loader) {
    std::stringstream ss;
    ss<< "db."<< group_name<< ".host";
    std::string path = ss.str();
    std::vector<std::string> urls;
    loader.dump_to(path, urls);

    owent_foreach(std::string& url, urls) {
        LC_DBCONN db_conn;
        db_conn.url = url;
        std::string::size_type fn = db_conn.url.find_last_of(":");
        if(std::string::npos == fn) {
            db_conn.host = url;
            db_conn.port = 6379;
            out.push_back(db_conn);
        } else {
            db_conn.host = url.substr(0, fn);

            // check if it's IP:port-port mode
            std::string::size_type minu_pos = url.find('-', fn + 1);
            if (std::string::npos == minu_pos) {
                // IP:port
                util::string::str2int(db_conn.port, url.substr(fn + 1).c_str());
                out.push_back(db_conn);
            } else {
                // IP:begin_port-end_port
                uint16_t begin_port = 0, end_port = 0;
                util::string::str2int(begin_port, &url[fn + 1]);
                util::string::str2int(end_port, &url[minu_pos + 1]);

                for (db_conn.port = begin_port; db_conn.port < end_port; ++ db_conn.port) {
                    ss.clear();
                    ss<< db_conn.host<< ":"<< db_conn.port;
                    db_conn.url = ss.str();

                    out.push_back(db_conn);
                }
            }
        }
    }
}

void logic_config::_load_loginsvr(util::config::ini_loader& loader);
void logic_config::_load_gamesvr(util::config::ini_loader& loader);
