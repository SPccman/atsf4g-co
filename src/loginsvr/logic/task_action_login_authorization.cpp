//
// Created by 文韬 on 2016/10/6.
//

#include <log/log_wrapper.h>
#include <protocol/pbdesc/svr.const.err.pb.h>
#include <protocol/pbdesc/svr.container.pb.h>

#include <rpc/db/login.h>
#include <rpc/auth/login.h>
#include <config/logic_config.h>
#include <time/time_utility.h>
#include <proto_base.h>

#include <logic/session_manager.h>
#include <data/session.h>

#include "task_action_login_authorization.h"


task_action_login_authorization::task_action_login_authorization(): is_new_player_(false), strategy_type_(hello::EN_VERSION_DEFAULT) {}
task_action_login_authorization::~task_action_login_authorization() {}

int task_action_login_authorization::operator()(hello::message_container& msg) {
    is_new_player_ = false;
    strategy_type_ = hello::EN_VERSION_DEFAULT;

    Session::ptr_t my_sess = GetSession();
    if (!my_sess) {
        WLOGERROR("session not found");
        SetRspCode(moyo_no1::EN_ERR_SYSTEM);
        return moyo_no1::err::EN_SYS_PARAM;
    }
    // 设置登入协议ID
    my_sess->SetLoginTaskId(GetTaskId());

    int res = 0;
    m_iStrategyType = moyo_no1::EN_VERSION_DEFAULT;
    m_strLoginCode.resize(32);
    rpc::auth::login::GenLoginCode(&m_strLoginCode[0], m_strLoginCode.size());

    // 1. 包校验
    msg_ptr_type req = GetRequest();
    if (!req->has_body() || !req->body().has_mcs_login_verify_req()) {
        WLOGERROR("login package error, msg: %s", req->DebugString().c_str());
        SetRspCode(moyo_no1::EN_ERR_INVALID_PARAM);
        return moyo_no1::err::EN_SUCCESS;
    }

    const ::moyo_no1::CSLoginVerifyReq &stMsgBodyRaw = req->body().mcs_login_verify_req();
    ::moyo_no1::CSLoginVerifyReq stMsgBody;
    stMsgBody.CopyFrom(stMsgBodyRaw);

    // 2. 版本号及更新逻辑
    uint32_t plat_id = stMsgBody.platform().platform_id();
    // 调试平台状态，强制重定向平台，并且不验证密码
    if (LogicConfig::Instance()->GetCfgLoginSvr().m_iDebugPlatform > 0) {
        plat_id = LogicConfig::Instance()->GetCfgLoginSvr().m_iDebugPlatform;
    }

    uint32_t channel_id = stMsgBody.platform().channel_id();
    uint32_t system_id = stMsgBody.system_id();
    uint32_t version = stMsgBody.version();

    m_strOpenId = make_openid(stMsgBodyRaw);

    stMsgBody.set_open_id(m_strOpenId);

    do {

        // TODO 测试期间没有版本不检查
        // if (!stMsgBody.has_proto_version()) {
        // break;
        //}

        // 检查账号登入策略
        // res = check_strategy(plat_id, system_id, version, stMsgBody.open_id());
        // if (res < 0)
        //{
        //// 出错返回
        // WLOGERROR("check strategy failed:%d", res);
        // SetRspCode(res);
        // return res;
        //}

        // if (res == moyo_no1::EN_VERSION_REVIEW)
        //{
        //// 审核版本直接下一步
        //// return TaskStep::E_TASK_STEP_NEXT;
        // break;
        //}

        // res = moyo_no1::EN_VERSION_DEFAULT;
        m_iStrategyType = UpdateRuleMgr::Instance()->GetVersionType(plat_id, system_id, version);

        // 检查客户端更新信息 更新不分平台值0
        if (UpdateRuleMgr::Instance()->CheckUpdate(m_stUpCfg, plat_id, channel_id, system_id, version, m_iStrategyType)) {
            SetRspCode(moyo_no1::EN_ERR_LOGIN_VERSION);
            return moyo_no1::EN_ERR_LOGIN_VERSION;
        }

        // 检查协议版本号信息
        // res = check_proto_update(stMsgBody.proto_version());
        // if (res < 0)
        //{
        // SetRspCode(res);
        // return res;
        //}
    } while (false);

    // 3. 平台校验逻辑
    // 调试模式不用验证
    if (LogicConfig::Instance()->GetCfgLoginSvr().m_iDebugPlatform <= 0) {
        verify_fn_t vfn = get_verify_fn(plat_id);
        if (NULL == vfn) {
            // 平台不收支持错误码
            SetRspCode(moyo_no1::EN_ERR_LOGIN_INVALID_PLAT);
            WLOGERROR("user %s report invalid platform %u", stMsgBody.open_id().c_str(), plat_id);
            return moyo_no1::err::EN_SUCCESS;
        }

        // 第三方平台用原始数据
        if (plat_id == moyo_no1::EN_PTI_ACCOUNT) {
            res = (this->*vfn)(stMsgBody);
        } else {
            res = (this->*vfn)(stMsgBodyRaw);
            // 有可能第三方认证会生成新的OpenId
            stMsgBody.set_open_id(m_strOpenId);
        }

        if (res < 0) {
            // 平台校验错误错误码
            SetRspCode(res);
            return moyo_no1::err::EN_SUCCESS;
        }
    }

    // 4. 开放时间限制
    bool pending_check = false;
    if (LogicConfig::Instance()->GetCfgLoginSvr().m_tStartTime > 0 &&
        TimeUtility::getCurrentUnixTimeStamp() < LogicConfig::Instance()->GetCfgLoginSvr().m_tStartTime) {
        pending_check = true;
    }

    if (LogicConfig::Instance()->GetCfgLoginSvr().m_tEndTime > 0 &&
        TimeUtility::getCurrentUnixTimeStamp() > LogicConfig::Instance()->GetCfgLoginSvr().m_tEndTime) {
        pending_check = true;
    }

    if (pending_check) {
        if (m_stWhiteSkipPending.size() != LogicConfig::Instance()->GetCfgLoginSvr().m_stWhiteSkipPending.size()) {
            // 清除缓存
            m_stWhiteSkipPending.clear();
            for (const std::string &openid : LogicConfig::Instance()->GetCfgLoginSvr().m_stWhiteSkipPending) {
                m_stWhiteSkipPending.insert(openid);
            }
        }

        // 白名单放过
        if (m_stWhiteSkipPending.end() == m_stWhiteSkipPending.find(m_strOpenId)) {
            // 维护模式，直接踢下线
            if (LogicConfig::Instance()->GetCfgLogic().m_bIsMaintenanceMode) {
                SetRspCode(moyo_no1::EN_ERR_MAINTENANCE);
            } else {
                SetRspCode(moyo_no1::EN_ERR_LOGIN_SERVER_PENDING);
            }

            return moyo_no1::err::EN_SUCCESS;
        }
    }

    // 5. 获取当前账户登入信息(如果不存在则直接转到 9)
    moyo_no1::table_login tb;
    do {
        res = rpc::db::login::Get(stMsgBody.open_id().c_str(), tb, m_strVersion);
        if (moyo_no1::err::EN_DB_RECORD_NOT_FOUND != res && res < 0) {
            WLOGERROR("call login rpc method failed, msg: %s", stMsgBody.DebugString().c_str());
            SetRspCode(moyo_no1::EN_ERR_UNKNOWN);
            return res;
        }

        if (moyo_no1::err::EN_DB_RECORD_NOT_FOUND == res) {
            break;
        }

        // 6. 是否禁止登入
        if (TimeUtility::getCurrentUnixTimeStamp() < tb.ban_time()) {
            SetRspCode(moyo_no1::EN_ERR_LOGIN_BAN);
            WLOGINFO("user %s try to login but banned", stMsgBody.open_id().c_str());
            m_iBanTime = tb.ban_time();
            return moyo_no1::err::EN_SUCCESS;
        }

        // 优先使用为过期的gamesvr index
        if (tb.has_last_login() && tb.last_login().has_gamesvr_index() &&
            tb.last_login().gamesvr_version() == LogicConfig::Instance()->GetCfgLoginSvr().m_iReloadVersion) {
            if (TimeUtility::getCurrentUnixTimeStamp() > static_cast<time_t>(tb.login_time()) &&
                TimeUtility::getCurrentUnixTimeStamp() - static_cast<time_t>(tb.login_time()) <
                LogicConfig::Instance()->GetCfgLoginSvr().m_tReloginExpireTime) {
                m_iStartIndex = tb.last_login().gamesvr_index();
            } else {
                const std::vector<std::string> &gamesvr_urls = LogicConfig::Instance()->GetCfgLoginSvr().m_stDefaultGamesvrs;
                if (!gamesvr_urls.empty()) {
                    m_iStartIndex = RandomUtility::RandomBetween<uint32_t>(0, gamesvr_urls.size());
                }
                tb.mutable_last_login()->set_gamesvr_index(m_iStartIndex);
            }
        } else {
            const std::vector<std::string> &gamesvr_urls = LogicConfig::Instance()->GetCfgLoginSvr().m_stDefaultGamesvrs;
            if (!gamesvr_urls.empty()) {
                m_iStartIndex = RandomUtility::RandomBetween<uint32_t>(0, gamesvr_urls.size());
            }
            tb.mutable_last_login()->set_gamesvr_index(m_iStartIndex);
            tb.mutable_last_login()->set_gamesvr_version(LogicConfig::Instance()->GetCfgLoginSvr().m_iReloadVersion);
        }

        // 7. 如果在线则尝试踢出
        if (0 != tb.login_pd()) {
            int32_t ret = rpc::game::login::SendKickOff(tb.login_pd(), tb.openid(), moyo_no1::EN_KICKOFF_RELOGIN);
            if (ret) {
                WLOGERROR("user %s send msg to 0x%x fail: %d", tb.openid().c_str(), tb.login_pd(), ret);
                // 超出定时保存的时间间隔的3倍，视为服务器异常断开。直接允许登入
                if (TimeUtility::getCurrentUnixTimeStamp() - static_cast<time_t>(tb.login_time()) <
                    LogicConfig::Instance()->GetCfgLogic().m_iLoginCodeProtect) {
                    SetRspCode(moyo_no1::EN_ERR_LOGIN_ALREADY_ONLINE);
                    return moyo_no1::err::EN_USER_KICKOUT;
                } else {
                    WLOGWARNING("user %s send kickoff failed, but login time timeout, conitnue login.", tb.openid().c_str());
                }
            } else {
                // 8. 验证踢出后的登入pd
                tb.Clear();
                res = rpc::db::login::Get(stMsgBody.open_id().c_str(), tb, m_strVersion);
                if (res < 0) {
                    WLOGERROR("call login rpc method failed, msg: %s", stMsgBody.DebugString().c_str());
                    SetRspCode(moyo_no1::EN_ERR_SYSTEM);
                    return res;
                }
                if (0 != tb.login_pd()) {
                    WLOGERROR("user %s loginout failed.", stMsgBody.open_id().c_str());
                    // 踢下线失败的错误码
                    SetRspCode(moyo_no1::EN_ERR_LOGIN_ALREADY_ONLINE);
                    return moyo_no1::err::EN_USER_KICKOUT;
                }
            }
        }
    } while (false);

    // 9. 创建或更新登入信息（login_code）
    // 新用户则创建
    if (moyo_no1::err::EN_DB_RECORD_NOT_FOUND == res) {
        // TODO 和机器人名字相同
        // auto robot = ConfigArenaIndex::Instance()->getRobotFight(stMsgBody.open_id());
        // if(robot) {
        // WLOGERROR("user %s eq robot_openid failed.", stMsgBody.open_id().c_str());
        //// 账号名非法
        // SetRspCode(moyo_no1::EN_ERR_LOGIN_OPENID);
        // return moyo_no1::err::EN_USER_KICKOUT;
        //}

        // if (std::regex_match(stMsgBody.open_id(), std::regex("__robot__*"))) {
        // WLOGERROR("user %s openid illegal.", stMsgBody.open_id().c_str());
        // return moyo_no1::EN_ERR_LOGIN_OPENID;
        //}

        std::string uuid;
        res = rpc::db::uuid::Generator(moyo_no1::EN_UUID_USER, uuid);
        if (res || !uuid.size()) {
            WLOGERROR("call Generator uuid rpc method failed, openid:%s, uuid:%s, res:%d", stMsgBody.open_id().c_str(), uuid.c_str(), res);
            SetRspCode(moyo_no1::EN_ERR_UNKNOWN);
            return res;
        }

        res = rpc::db::uuid::HSetNXUUIdMapOpenId(moyo_no1::EN_UUID_USER, uuid, stMsgBody.open_id());
        if (res) {
            WLOGERROR("call uuid HSetNXUUIdMapOpenID rpc method failed, openid:%s, uuid:%s, res:%d", stMsgBody.open_id().c_str(),
                      uuid.c_str(), res);
            SetRspCode(moyo_no1::EN_ERR_UNKNOWN);
            return res;
        }

        init_login_data(tb, stMsgBody, uuid, channel_id);

        // 注册日志
        LogMgr::WLOGRegister(stMsgBody.open_id(), uuid, LogicConfig::Instance()->GetSelfPd(), plat_id, channel_id,
                             TimeUtility::getCurrentUnixTimeStamp(), stMsgBody.system_id());
    }

    // 登入信息
    {
        // 登入码
        tb.set_login_code(m_strLoginCode);
        tb.set_login_code_expired(LogicConfig::Instance()->GetCfgLogic().m_iLoginCodeValidSec + TimeUtility::getCurrentUnixTimeStamp());

        // 平台信息更新
        ::moyo_no1::platform_information *plat_dst = tb.mutable_platform();
        const ::moyo_no1::DPlatformData &plat_src = stMsgBody.platform();

        plat_dst->set_platform_id(static_cast<moyo_no1::EnPlatformTypeID>(plat_id));
        if (plat_src.has_access()) {
            plat_dst->set_access(plat_src.access());
        }

        plat_dst->set_version_type(m_iStrategyType);

        // 苹果GameCenter更新
        if (stMsgBody.has_apple_gamecenter() && !stMsgBody.apple_gamecenter().player_id().empty()) {
            plat_dst->mutable_apple_gamecenter()->CopyFrom(stMsgBody.apple_gamecenter());
        }
    }

    // 保存登入信息
    res = rpc::db::login::Set(stMsgBody.open_id().c_str(), tb, m_strVersion);
    if (res < 0) {
        WLOGERROR("save login data for %s failed, msg:\n%s", stMsgBody.open_id().c_str(), tb.DebugString().c_str());
        SetRspCode(moyo_no1::EN_ERR_SYSTEM);
        return res;
    }

    m_iRegisterTime = tb.register_time();

    // 10.登入成功
    return moyo_no1::err::EN_SUCCESS;
}

int task_action_login_authorization::on_success() {
    hello::CSMsg &msg = add_rsp_msg();

    ::hello::SCLoginAuthRsp *rsp_body = msg.mutable_body()->mutable_msc_login_auth_rsp();
    rsp_body->set_login_code(login_data_.login_code());
    rsp_body->set_open_id(final_openid_); // 最终使用的OpenID
    rsp_body->set_version_type(strategy_type_);
    rsp_body->set_is_new_player(is_new_player_);
    rsp_body->set_zone_id(logic_config::me()->get_cfg_logic().zone_id);

    std::shared_ptr<session> my_sess = get_session();

    // 登入过程中掉线了，直接退出
    if (!my_sess) {
        WLOGERROR("session not found");
        return hello::err::EN_SUCCESS;
    }

    // 完成登入流程，不再处于登入状态
    my_sess->set_login_task_id(0);

    // 如果是版本过低则要下发更新信息
    if (hello::EN_UPDATE_NONE != update_info_.result()) {
        rsp_body->mutable_update_info()->Swap(&update_info_);
    }

    // TODO 临时的登入服务器，以后走平台下发策略
    const std::vector<std::string> &gamesvr_urls = logic_config::me()->get_cfg_svr_login().gamesvr_list;
    if (!gamesvr_urls.empty()) {
        for (size_t i = 0; i < gamesvr_urls.size(); ++i) {
            rsp_body->add_login_address(gamesvr_urls[(login_data_.last_login().gamesvr_index() + i) % gamesvr_urls.size()]);
        }
    }

    // 先发送数据，再通知踢下线
    send_rsp_msg();

    // 登入成功，不需要再在LoginSvr上操作了
    session_manager::me()->remove(my_sess, ::atframe::gateway::close_reason_t::EN_CRT_EOF);
    return get_ret_code();
}

int task_action_login_authorization::on_failed() {
    std::shared_ptr<session> s = get_session();
    // 登入过程中掉线了，直接退出
    if (!s) {
        WLOGERROR("session not found");
        return hello::err::EN_SUCCESS;
    }

    hello::CSMsg &msg = add_rsp_msg();
    hello::SCLoginAuthRsp *rsp_body = msg.mutable_body()->mutable_msc_login_auth_rsp();
    rsp_body->set_login_code("");
    rsp_body->set_open_id(final_openid_);
    rsp_body->set_ban_time(login_data_.ban_time());
    rsp_body->set_version_type(strategy_type_);
    rsp_body->set_zone_id(logic_config::me()->get_cfg_logic().zone_id);

    // 如果是版本过低则要下发更新信息
    if (hello::EN_UPDATE_NONE != update_info_.result()) {
        rsp_body->mutable_update_info()->Swap(&update_info_);
    }

    if (hello::EN_ERR_LOGIN_SERVER_PENDING == get_rsp_code() || hello::EN_ERR_MAINTENANCE == get_rsp_code()) {
        rsp_body->set_start_time(logic_config::me()->get_cfg_logic().server_open_time);
    } else {
        WLOGERROR("session [0x%llx, 0x%llx] login failed",
                  static_cast<unsigned long long>(get_gateway_info().first),
                  static_cast<unsigned long long>(get_gateway_info().second)
        );
    }

    send_rsp_msg();

    // 无情地踢下线
    session_manager::me()->remove(s, ::atframe::gateway::close_reason_t::EN_CRT_KICKOFF);
    return get_ret_code();
}

int32_t task_action_login_authorization::check_proto_update(uint32_t ver_no) {
    // TODO check if client version is available
    return hello::err::EN_SUCCESS;
}

task_action_login_authorization::auth_fn_t task_action_login_authorization::get_verify_fn(uint32_t plat_id) {
    static auth_fn_t all_auth_fns[hello::EnPlatformTypeID_ARRAYSIZE];

    if (NULL != all_auth_fns[hello::EN_PTI_ACCOUNT]) {
        return all_auth_fns[plat_id % hello::EnPlatformTypeID_ARRAYSIZE];
    }

    all_auth_fns[hello::EN_PTI_ACCOUNT] = &task_action_login_authorization::verify_plat_account;
    return all_auth_fns[plat_id % hello::EnPlatformTypeID_ARRAYSIZE];
}

void task_action_login_authorization::init_login_data(hello::table_login& tb, const ::hello::CSLoginAuthReq& req, const std::string &uuid, uint32_t channel_id) {
    tb.set_openid(req.open_id());

    tb.set_login_pd(0);
    tb.set_login_time(0);
    tb.set_register_time(util::time::time_utility::get_now());

    tb.set_ban_time(0);

    tb.mutable_platform()->mutable_profile()->set_open_id(req.open_id());
    tb.mutable_platform()->mutable_profile()->set_uuid(uuid);
    tb.mutable_platform()->set_channel_id(channel_id);

    version_.assign("0");
    is_new_player_ = true;
}

std::string task_action_login_authorization::make_openid(const hello::CSLoginAuthReq &req) {
    return rpc::auth::login::make_open_id(req.platform().platform_id(), req.platform().channel_id(), req.open_id());
}

int task_action_login_authorization::verify_plat_account(const ::hello::CSLoginAuthReq& req) {
    hello::table_login tb;
    std::string version;
    int res = rpc::db::login::get(req.open_id().c_str(), tb, version);
    if (hello::err::EN_DB_RECORD_NOT_FOUND != res && res < 0) {
        WLOGERROR("call login rpc method failed, msg: %s", req.DebugString().c_str());
        return hello::EN_ERR_SYSTEM;
    }

    if (hello::err::EN_DB_RECORD_NOT_FOUND == res) {
        return hello::EN_SUCCESS;
    }

    // 校验密码
    if (!req.has_platform()) {
        // 参数错误
        return hello::EN_ERR_INVALID_PARAM;
    }

    if (req.platform().access() != tb.platform().access()) {
        // 平台校验不通过错误码
        return hello::EN_ERR_LOGIN_VERIFY;
    }

    return hello::EN_SUCCESS;
}