#include <log/log_wrapper.h>
#include <proto_base.h>
#include <time/time_utility.h>

#include "session_manager.h"
#include "player_manager.h"
#include "data/player.h"

int session_manager::init() {
    return 0;
}

int session_manager::proc() {
    // TODO 写入时间可配,实时在线统计
    // time_t cur_time = util::time::time_utility::get_now();
    // cur_time = cur_time - cur_time % util::time::time_utility::MINITE_SECONDS;
    // static time_t wlog_last_time = cur_time;
    // if (cur_time > wlog_last_time) {
    //     wlog_last_time = cur_time;
    //     // send online stats
    // }

    return 0;
}

const session_manager::sess_ptr_t session_manager::find(const session::key_t& key) const {
    session_index_t::const_iterator iter = all_sessions_.find(key);
    if (all_sessions_.end() == iter) {
        return sess_ptr_t();
    }

    return iter->second;
}

session_manager::sess_ptr_t session_manager::find(const session::key_t& key) {
    session_index_t::iterator iter = all_sessions_.find(key);
    if (all_sessions_.end() == iter) {
        return sess_ptr_t();
    }

    return iter->second;
}

session_manager::sess_ptr_t session_manager::create(const session::key_t& key) {
    if (find(key)) {
        WLOGERROR("session registered, failed, bus id: 0x%llx, session id: 0x%llx\n",
            static_cast<unsigned long long>(key.bus_id),
            static_cast<unsigned long long>(key.session_id)
        );

        return sess_ptr_t();
    }

    sess_ptr_t sess = all_sessions_[key] = std::make_shared<session>();
    if(!sess) {
        WLOGERROR("malloc failed");
        return sess;
    }

    sess->set_key(key);
    return sess;
}

void session_manager::remove(const session::key_t& key, bool kickoff) {
    remove(find(key), kickoff);
}

void session_manager::remove(sess_ptr_t sess, bool kickoff) {
    if(!sess) {
        return;
    }

    if (kickoff) {
        sess->send_kickoff(::atframe::gateway::close_reason_t::EN_CRT_KICKOFF);
    }

    session::key_t key = sess->get_key();
    WLOGINFO("session (0x%llx:0x%llx) removed",
       static_cast<unsigned long long>(key.bus_id),
       static_cast<unsigned long long>(key.session_id)
    );

    all_sessions_.erase(key);

    // 移除绑定的player
    player::ptr_t u = sess->get_player();
    if (u) {
        sess->set_player(NULL);
        u->set_session(NULL);

        // TODO 统计日志
        // u->GetLogMgr().WLOGLogout();

        // 如果是踢下线，则需要强制保存并移除GameUser对象
        player_manager::me()->remove(u, kickoff);
    }
}

void session_manager::remove_all() {
    for(session_index_t::iterator iter = all_sessions_.begin(); iter != all_sessions_.end(); ++ iter) {
        if(iter->second) {
            iter->second->send_kickoff(::atframe::gateway::close_reason_t::EN_CRT_SERVER_CLOSED);
        }
    }

    all_sessions_.clear();
}

size_t session_manager::size() const {
    return all_sessions_.size();
}