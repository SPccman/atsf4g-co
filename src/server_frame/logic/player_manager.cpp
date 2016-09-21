#include <log/log_wrapper.h>
#include <time/time_utility.h>
#include <proto_base.h>

#include "protocol/pbdesc/svr.const.err.pb.h"

#include "player_manager.h"
#include "session_manager.h"

player_manager::player_ptr_t player_manager::make_default_player(const std::string& openid) {
    player_ptr_t ret = create(openid);
    WLOGDEBUG("create player %s with default data", openid.c_str());

    if (!ret) {
        return ret;
    }

    ret->create_init(hello::EN_VERSION_DEFAULT);
    return ret;
}

int player_manager::init() {
    return 0;
}

bool player_manager::remove(player_manager::player_ptr_t u, bool force) {
    if (!u || u->is_removing()) {
        return false;
    }
    u->set_removing(true);

    // 如果不是强制移除则进入缓存队列即可
    // TODO 过期时间配置
    time_t expire_time = 600; //LogicConfig::Instance()->GetCfgLogic().m_iCacheExpireTime;
    if (force || expire_time <= 0) {
        do {
            int res = 0;

            // 先检测登入信息，防止缓存过期
            // 这意味着这个函数必须在Task中使用
            hello::table_login user_lg;
            std::string version;
            res = save(u, &user_lg, &version);
            if (res < 0) {
                break;
            }

            user_lg.set_login_pd(0);
            user_lg.set_logout_time(util::time::time_utility::get_now());
            // TODO RPC save to db
            res = 0;// rpc::db::login::Set(u->GetOpenId().c_str(), user_lg, version);
            if (res < 0) {
                WLOGERROR("player %s try logout load db failed, res: %d version:%s.", u->get_open_id().c_str(), res, version.c_str());
            }

        } while (false);

        // 先执行同步操作
        // 释放用户索引
        WLOGINFO("player %s removed", u->get_open_id().c_str());
        force_erase(u->get_open_id());
    }

    // 释放本地数据, 下线相关Session
    session::ptr_t s = u->get_session();
    if (s) {
        u->set_session(NULL);
        s->set_player(NULL);
        session_manager::me()->remove(s, true);
    }

    u->set_removing(false);
    return true;
}

int player_manager::save(player_ptr_t u, hello::table_login* login_tb, std::string* login_version) {
    if (!u) {
        WLOGERROR("user is null");
        return hello::err::EN_SYS_UNKNOWN;
    }

    // 没初始化的不保存数据  缓存数据不保存
    if (!u->is_inited()) {
        return hello::err::EN_SUCCESS;
    }

    // 尝试保存用户数据
    hello::table_login user_lg;
    if (NULL == login_tb) {
        login_tb = &user_lg;
    }

    std::string version;
    if (NULL == login_version) {
        login_version = &version;
    }

    uint64_t self_bus_id = 0; // TODO read self bus id
    // TODO RPC read from DB
    int res = 0; //rpc::db::login::Get(u->GetOpenId().c_str(), *login_tb, *login_version);
    if (res < 0) {
        WLOGERROR("player %s try load login data failed.", u->get_open_id().c_str());
        return res;
    }

    // 异常的玩家数据记录，自动修复一下
    if (0 == login_tb->login_pd()) {
        WLOGERROR("player %s login pd error(expected: 0x%llx, real: 0x%llx)",
            u->get_open_id().c_str(),
            static_cast<unsigned long long>(self_bus_id),
            static_cast<unsigned long long>(login_tb->login_pd())
        );

        login_tb->set_login_pd(self_bus_id);
        // TODO RPC save to db
        res = 0;//rpc::db::login::Set(u->GetOpenId().c_str(), *login_tb, *login_version);
        if (res < 0) {
            WLOGERROR("user %s try set login data failed.", u->get_open_id().c_str());
            return res;
        }
    }

    if (static_cast<int>(login_tb->login_pd()) != self_bus_id) {
        WLOGERROR("user %s login pd error(expected: 0x%llx, real: 0x%llx)",
            u->get_open_id().c_str(),
            static_cast<unsigned long long>(self_bus_id),
            static_cast<unsigned long long>(login_tb->login_pd())
        );

        // 在其他设备登入的要把这里的Session踢下线
        if (u->get_session()) {
            u->get_session()->send_kickoff(::atframe::gateway::close_reason_t::EN_CRT_KICKOFF);
        }

        return hello::EN_ERR_LOGIN_OTHER_DEVICE;
    }


    // 尝试保存用户数据
    hello::table_user user_gu;
    u->dump(user_gu, true);

    WLOGDEBUG("player %s save curr data version:%s", u->get_open_id().c_str(), u->get_version().c_str());

    // TODO RPC save to DB
    res = 0;//rpc::db::game_user::Set(u->GetOpenId().c_str(), user_gu, u->GetVersion());

    // CAS 序号错误（可能是先超时再返回成功）,重试一次
    // 前面已经确认了当前用户在此处登入并且已经更新了版本号到版本信息
    // TODO RPC save to DB again
    //if (hello::err::EN_DB_OLD_VERSION == res) {
    //    res = rpc::db::game_user::Set(u->get_open_id().c_str(), user_gu, u->get_version());
    //}

    if (res < 0) {
        WLOGERROR("player %s try save db failed. res:%d version:%s", u->get_open_id().c_str(), res, u->get_version().c_str());
    }

    return res;
}

void player_manager::force_erase(const std::string& openid) {
    player_index_t::iterator iter = all_players_.find(openid);
    if (iter != all_players_.end()) {
        iter->second->on_remove();
        all_players_.erase(iter);
    }
}

player_manager::player_cache_t* player_manager::set_offline_cache(player_ptr_t user, bool is_save) {
    if (!user) {
        WLOGERROR("user can not be null");
        return NULL;
    }

    // TODO get configure from file
    time_t cache_expire_time = 600;
    cache_expire_list_.push_back(player_cache_t());
    player_cache_t& new_cache = cache_expire_list_.back();
    new_cache.failed_times = 0;
    new_cache.operation_sequence = ++ user->schedule_data_.cache_sequence;
    new_cache.player_inst = user;
    new_cache.expire_time = util::time::time_utility::get_now() + cache_expire_time;
    new_cache.save = is_save;

    return &new_cache;
}

player_manager::player_ptr_t player_manager::load(const std::string &openid, bool force) {
    player_ptr_t user = find(openid);
    if(force || !user) {
        hello::table_user tbu;
        std::string version;

        // 这个函数必须在task中运行
        // 尝试从数据库读数据
        // TODO RPC get from DB
        int res = 0;//rpc::db::game_user::GetAll(openid.c_str(), tbu, version);
        if(res) {
            WLOGERROR("load game user data for %s failed, error code:%d", openid.c_str(), res);
            return nullptr;
        }

        user = find(openid);
        if(!user) {
            user = create(openid);
        }

        if(user && !user->is_inited() && user->get_version() != version && !version.empty()) {
            user->set_version(version);

            user->init_from_table_data(tbu);

            // 只是load的数据不要保存，不然出现版本错误。
            set_offline_cache(user, false);
        }
    }

    return user;
}

size_t player_manager::size() const {
    return all_players_.size();
}

player_manager::player_ptr_t player_manager::create(const std::string& openid) {
    if(find(openid)) {
        WLOGERROR("player %s already exists, can not create again", openid.c_str());
        return player_ptr_t();
    }

    // TODO online user number limit
    player_ptr_t ret = std::make_shared<player>();
    if(!ret) {
        WLOGERROR("malloc player %s failed", openid.c_str());
        return ret;
    }
    ret->init(openid);
    all_players_[openid] = ret;

    return ret;
}

const player_manager::player_ptr_t player_manager::find(const std::string& openid) const {
    player_index_t::const_iterator iter = all_players_.find(openid);
    if(all_players_.end() == iter) {
        return player_ptr_t();
    }

    return iter->second;
}

player_manager::player_ptr_t player_manager::find(const std::string& openid) {
    player_index_t::iterator iter = all_players_.find(openid);
    if(all_players_.end() == iter) {
        return player_ptr_t();
    }

    return iter->second;
}

int player_manager::proc() {
    int ret = 0;

    // 自动保存
    do {
        if (auto_save_list_.empty()) {
            break;
        }

        player_ptr_t user = auto_save_list_.front().lock();

        // 如果已下线并且用户缓存失效则跳过
        if (!user) {
            auto_save_list_.pop_front();
            continue;
        }

        // 没有初始化完成的直接移除
        if (!user->is_inited()) {
            force_erase(user->get_open_id());
            auto_save_list_.pop_front();
            continue;
        }

        // 如果没有开启自动保存则跳过
        if (0 == user->schedule_data_.save_pending_time) {
            auto_save_list_.pop_front();
            continue;
        }

        // 如果最近自动保存用户的保存时间大于当前时间，则没有用户需要保存数据
        if (util::time::time_utility::get_now() <= user->schedule_data_.save_pending_time) {
            break;
        }

        // TODO 启动用户数据保存Task
        // TaskManager::id_t task_id = 0;
        // int res = TaskManager::Instance()->CreateTask<TaskActionAutoSaveGameUser>(task_id);
        // if ( moyo_no1::err::EN_SUCCESS != res || 0 == task_id) {
        //     WLOGERROR("create task to auto save failed.");
        //     break;
        // }

        // moyo_no1::message_container msg;
        // TaskManager::Instance()->StartTask(task_id, msg);

        ++ ret;
        break;
    } while(true);


    // 缓存失效定时器
    do {
        if (cache_expire_list_.empty()) {
            break;
        }

        player_cache_t& cache = cache_expire_list_.front();
        player_ptr_t user = cache.player_inst.lock();

        // 如果已下线并且用户缓存失效则跳过
        if (!user) {
            cache_expire_list_.pop_front();
            continue;
        }

        // 不需要保存则跳过
        if (false == cache.save) {
            cache_expire_list_.pop_front();
            continue;
        }

        // 如果操作序列失效则跳过
        if (false == user->check_logout_cache(cache.operation_sequence)) {
            cache_expire_list_.pop_front();
            continue;
        }

        // 如果没到时间，后面的全没到时间
        if (util::time::time_utility::get_now() <= cache.expire_time) {
            break;
        }

        // TODO 启动用户登出Task
        //TaskManager::id_t task_id = 0;
        //int res = TaskManager::Instance()->CreateTask<TaskActionUserCacheExpired>(task_id);
        //if ( moyo_no1::err::EN_SUCCESS != res || 0 == task_id) {
        //    WLOGERROR("create task to expire cache failed.");
        //    break;
        //}

        //moyo_no1::message_container msg;
        //TaskManager::Instance()->StartTask(task_id, msg);

        ++ ret;
        break;
    } while(true);

    return ret;
}

void player_manager::update_auto_save(player_ptr_t& user) {
    if (!user) {
        WLOGERROR("this function can not be call with user=null");
        return;
    }

    // 没有设置定时保存限制则跳过
    time_t auto_save_interval = 600;// TODO read from configutr
    if (auto_save_interval <= 0) {
        return;
    }

    time_t now_tm = util::time::time_utility::get_now();
    time_t update_tm = auto_save_interval + now_tm;

    // 没有设置定时保存则跳过
    if (update_tm <= now_tm) {
        return;
    }

    // 如果有未完成的定时保存任务则不需要再设一个
    if(0 != user->schedule_data_.save_pending_time && now_tm <= user->schedule_data_.save_pending_time) {
        return;
    }

    user->schedule_data_.save_pending_time = update_tm;
    auto_save_list_.push_back(std::weak_ptr<player>(user));
}