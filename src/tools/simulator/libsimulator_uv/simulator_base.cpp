//
// Created by owt50 on 2016/10/9.
//

#include <signal.h>

#include <std/foreach.h>
#include <lock/lock_holder.h>
#include <cli/cmd_option_phoenix.h>
#include <time/time_utility.h>

#include "simulator_active.h"

#include "simulator_base.h"

#include <editline/readline.h>

static simulator_base* g_last_simulator = NULL;
static std::list<std::string> g_readline_complete_list;
static int g_readline_signal_prev = 0;
static int g_readline_signal_curr = 0;

simulator_base::cmd_wrapper_t::cmd_wrapper_t(std::shared_ptr<util::cli::cmd_option_ci> o, const std::string& n): owner(o), name(n) {}

// create a child node
simulator_base::cmd_wrapper_t& simulator_base::cmd_wrapper_t::operator[](const std::string& name) {
    if (!owner) {
        return (*this);
    }

    cmd_wrapper_t& c = children[name];
    c.owner = std::dynamic_pointer_cast<util::cli::cmd_option_ci>(owner->bind_child_cmd(name, util::cli::cmd_option_ci::create()));
    c.name = name;
    return c;
}

// bind a cmd handle
simulator_base::cmd_wrapper_t& simulator_base::cmd_wrapper_t::bind(cmd_fn_t fn, const std::string& description) {
    if (owner) {
        owner->bind_cmd(name, fn)->set_help_msg(description.c_str());
    }

    return (*this);
}

simulator_base::simulator_base():is_closing_(false), exec_path_(NULL) {
    uv_loop_init(&loop_);
    uv_async_init(&loop_, &async_cmd_, libuv_on_async_cmd);
    async_cmd_.data = this;

    cmd_mgr_ = util::cli::cmd_option_ci::create();
    args_mgr_ = util::cli::cmd_option::create();
    root_ = std::make_shared<cmd_wrapper_t>(cmd_mgr_, std::string());

    shell_opts_.history_file = ".simulator_history";
    shell_opts_.protocol_log = "protocol.log";
    shell_opts_.no_interactive = false;
    shell_opts_.buffer_.resize(65536);
    g_last_simulator = this;
}

simulator_base::~simulator_base() {
    if (this == g_last_simulator) {
        g_last_simulator = NULL;
    }
}


namespace detail {
    // 绑定的输出函数
    static void help_func(util::cli::callback_param stParams, simulator_base* self) {
        std::cout<< "usage: " << self->get_exec()<< " [options...]"<<  std::endl;
        std::cout<< *self->get_option_manager()<< std::endl;

        // 立即退出
        self->stop();
    }

    struct on_sys_cmd_help {
        simulator_base* owner;
        on_sys_cmd_help(simulator_base* s): owner(s) {}

        void operator()(util::cli::callback_param params) {
            // 指令
            std::cout<< "Usage:"<< std::endl;
            std::cout<< "Commands:"<< std::endl;
            std::cout<< *owner->get_cmd_manager()<< std::endl;
        }
    };

    struct on_sys_cmd_exec {
        simulator_base* owner;
        on_sys_cmd_exec(simulator_base* s): owner(s) {}

        void operator()(util::cli::callback_param params) {
            std::stringstream ss;
            for (size_t i = 0 ; i < params.get_params_number(); ++ i) {
                ss<< params[i]->to_cpp_string()<< " ";
            }

            int res = system(ss.str().c_str());
            if (res != 0) {
                std::cout<< "$? = "<< res<< std::endl;
            }
        }
    };

    struct on_sys_cmd_exit {
        simulator_base* owner;
        on_sys_cmd_exit(simulator_base* s): owner(s) {}

        void operator()(util::cli::callback_param params) {
            owner->stop();

            raise(SIGTERM);
        }
    };

    struct on_sys_cmd_set_player {
        simulator_base* owner;
        on_sys_cmd_set_player(simulator_base* s): owner(s) {}

        void operator()(util::cli::callback_param params) {
            SIMULATOR_CHECK_PARAMNUM(params, 1)

            simulator_base::player_ptr_t player = owner->get_player_by_id(params[0]->to_cpp_string());
            if (!player) {
                SIMULATOR_ERR_MSG()<< "player "<< params[0]->to_cpp_string()<< " not found"<< std::endl;
                return;
            }

            owner->set_current_player(player);
        }
    };
}

// graceful Exits
static void simulator_setup_signal_func(int signo) {
    g_readline_signal_curr = signo;
}

static int simulator_setup_signal() {
    // block signals
    signal(SIGTERM, simulator_setup_signal_func);
    signal(SIGINT, simulator_setup_signal_func);

#ifndef WIN32
    signal(SIGQUIT, simulator_setup_signal_func);
    signal(SIGHUP, SIG_IGN);  // lost parent process
    signal(SIGPIPE, SIG_IGN); // close stdin, stdout or stderr
    signal(SIGTSTP, SIG_IGN); // close tty
    signal(SIGTTIN, SIG_IGN); // tty input
    signal(SIGTTOU, SIG_IGN); // tty output
#endif

    return 0;
}

int simulator_base::init() {
    // register inner cmd and helper msg
    args_mgr_->bind_cmd("?, -h, --help, help", detail::help_func, this)
        ->set_help_msg("show help message and exit");
    args_mgr_->bind_cmd("--history, --history-file", util::cli::phoenix::assign(shell_opts_.history_file))
        ->set_help_msg("<file path> set command history file");
    args_mgr_->bind_cmd("--protocol, --protocol-log", util::cli::phoenix::assign(shell_opts_.protocol_log))
        ->set_help_msg("<file path> set protocol log file");
    args_mgr_->bind_cmd("-ni, --no-interactive", util::cli::phoenix::set_const(shell_opts_.no_interactive, true))
        ->set_help_msg("disable interactive mode");
    args_mgr_->bind_cmd("-f, --rf, --read-file", util::cli::phoenix::assign<std::string>(shell_opts_.read_file))
        ->set_help_msg("read from file");
    args_mgr_->bind_cmd("-c, --cmd", util::cli::phoenix::push_back(shell_opts_.cmds))
        ->set_help_msg("[cmd ...] add cmd to run");

    reg_req()["!, sh"].bind(detail::on_sys_cmd_exec(this), "<command> [parameters...] execute a external command");
    reg_req()["?, help"].bind(detail::on_sys_cmd_help(this), "show help message");
    reg_req()["exit, quit"].bind(detail::on_sys_cmd_exit(this), "exit");
    reg_req()["set_player"].bind(detail::on_sys_cmd_set_player(this), "<player id> set current player");

    // register all protocol callbacks
    ::proto::detail::simulator_activitor::active_all(this);

    // setup signal
    simulator_setup_signal();
    return 0;
}

int simulator_base::run(int argc, const char* argv[]) {
    util::time::time_utility::update(NULL);
    if (argc > 0) {
        exec_path_ = argv[0];
    }
    args_mgr_->start(argc, argv, false, NULL);
    if (is_closing_) {
        return 0;
    }

    // startup interactive thread
    uv_thread_create(&thd_cmd_, libedit_thd_main, this);

    int ret = uv_run(&loop_, UV_RUN_DEFAULT);
    uv_close((uv_handle_t*) &async_cmd_, NULL);

    uv_thread_join(&thd_cmd_);
    while(UV_EBUSY == uv_loop_close(&loop_)) {
        uv_run(&loop_, UV_RUN_ONCE);
    }

    return ret;
}

int simulator_base::stop() {
    is_closing_ = true;

    for(std::map<std::string, player_ptr_t>::iterator iter = players_.begin(); iter != players_.end(); ++ iter) {
        iter->second->close();
    }
    players_.clear();

    for(std::set<player_ptr_t>::iterator iter = connecting_players_.begin();
        iter != connecting_players_.end(); ++ iter) {
        (*iter)->close();
    }
    connecting_players_.clear();

    uv_stop(&loop_);
    return 0;
}

bool simulator_base::insert_player(player_ptr_t player) {
    util::time::time_utility::update(NULL);
    if (is_closing_) {
        return false;
    }

    if (!player) {
        return false;
    }

    if (this == player->owner_) {
        return true;
    }

    if (NULL != player->owner_) {
        return false;
    }

    if (players_.end() != players_.find(player->get_id())) {
        return false;
    }

    players_[player->get_id()] = player;
    connecting_players_.erase(player);
    player->owner_ = this;
    return true;
}

void simulator_base::remove_player(const std::string& id, bool is_close) {
    util::time::time_utility::update(NULL);
    // will do it in stop function
    if (is_closing_) {
        return;
    }

    std::map<std::string, player_ptr_t>::iterator iter = players_.find(id);
    if (players_.end() == iter) {
        return;
    }

    if (is_close) {
        iter->second->close();
    }

    if (iter->second == cmd_player_) {
        cmd_player_.reset();
    }
    iter->second->owner_ = NULL;
    players_.erase(iter);
}

void simulator_base::remove_player(player_ptr_t player) {
    util::time::time_utility::update(NULL);
    // will do it in stop function
    if (is_closing_ || !player) {
        return;
    }

    remove_player(player->get_id());
    connecting_players_.erase(player);
}

simulator_base::player_ptr_t simulator_base::get_player_by_id(const std::string& id) {
    std::map<std::string, player_ptr_t>::iterator iter = players_.find(id);
    if (players_.end() == iter) {
        return NULL;
    }

    return iter->second;
}

int simulator_base::insert_cmd(player_ptr_t player, const std::string& cmd) {
    // must be thread-safe
    util::lock::lock_holder<util::lock::spin_lock> holder(shell_cmd_manager_.lock);
    shell_cmd_manager_.cmds.push_back(std::pair<player_ptr_t, std::string>(player, cmd));
}

void simulator_base::libuv_on_async_cmd(uv_async_t* handle) {
    simulator_base* self = reinterpret_cast<simulator_base*>(handle->data);
    assert(self);

    while (true) {
        util::time::time_utility::update(NULL);
        std::pair<player_ptr_t, std::string> cmd;
        {
            util::lock::lock_holder<util::lock::spin_lock> holder(self->shell_cmd_manager_.lock);
            if (self->shell_cmd_manager_.cmds.empty()) {
                break;
            }
            cmd = self->shell_cmd_manager_.cmds.front();
            self->shell_cmd_manager_.cmds.pop_front();
        }

        self->exec_cmd(cmd.first, cmd.second);
    }
}


char **simulator_base::libedit_completion(const char* text, int start, int end) {
    if (NULL == g_last_simulator) {
        return rl_completion_matches(text, libedit_complete_cmd_generator);
    }

    // text 记录的是当前单词， rl_line_buffer 记录的是完整行
    // =========================
    std::stringstream ss;
    ss.str(rl_line_buffer);

    std::string cur, prefix, full_cmd;
    cmd_wrapper_t* parent = &g_last_simulator->reg_req();
    simulator_base::cmd_wrapper_t::value_type::iterator iter;
    while(ss >> cur) {
        full_cmd = cur;
        std::transform(cur.begin(), cur.end(), cur.begin(), ::toupper);

        iter = parent->children.find(cur);
        if (iter == g_last_simulator->reg_req().children.end()) {
            break;
        }

        parent = &iter->second;
        prefix += full_cmd + " ";
        cur = "";
    }
    iter = parent->children.begin();

    // 查找不完整词
    if (cur.size() > 0) {
        iter = parent->children.lower_bound(cur);
    }

    while (iter != parent->children.end()) {
        if (cur == iter->first.substr(0, cur.size())) {
            g_readline_complete_list.push_back(iter->first);
        } else {
            break;
        }

        ++ iter;
    }

    // 如果无响应命令，且允许列举文件列表则响应列举目录占位符
    if (g_readline_complete_list.empty()) {
        // 已经找到节点关系的部分不再需要了
        return rl_completion_matches(text + ss.tellp() - full_cmd.size(), rl_filename_completion_function);
    }

    return rl_completion_matches(text, libedit_complete_cmd_generator);
}

char* simulator_base::libedit_complete_cmd_generator(const char* text, int state) {
    // 将会被多次调用，直到返回NULL为止
    // 所有的生成的指令必须以malloc分配内存并返回，每次一条,readline内部会负责释放

    // 所有候选命令列举完毕
    if (g_readline_complete_list.empty())
        return NULL;

    std::list<std::string>::iterator iter = g_readline_complete_list.begin();

    char* ret = reinterpret_cast<char*>(::malloc((*iter).size() + 1));
    if (ret == NULL) {
        std::cerr<< __FILE__<< ":"<< __LINE__<< " => "<< "malloc memory for readline cmds failed."<< std::endl;
        return ret;
    }

    strncpy(ret, (*iter).c_str(), (*iter).size() + 1);
    g_readline_complete_list.pop_front();

    return ret;
}

void simulator_base::libedit_thd_main(void* arg) {
    simulator_base* self = reinterpret_cast<simulator_base*>(arg);
    assert(self);

    if (!self->shell_opts_.cmds.empty()) {
        owent_foreach(std::string& cmd, self->shell_opts_.cmds) {
            self->insert_cmd(self->cmd_player_, cmd);
        }
    }

    if(!self->shell_opts_.read_file.empty()) {
        std::fstream fin;
        fin.open(self->shell_opts_.read_file.c_str(), std::ios::in);
        std::string cmd;
        while(std::getline(fin, cmd)) {
            self->insert_cmd(self->cmd_player_, cmd);
        }
    }

    if (self->shell_opts_.no_interactive) {
        return;
    }

    // init
    rl_attempted_completion_function = libedit_completion;
    if (!self->shell_opts_.history_file.empty()) {
        using_history();
        read_history(self->shell_opts_.history_file.c_str());
    }

    // readline loop
    std::string prompt = "~>";
    char *cmd_c = NULL;
    bool is_continue = true;
    while(is_continue && NULL != g_last_simulator && !g_last_simulator->is_closing()) {
        cmd_c = readline(prompt.c_str());

        // signal
        if (0 != g_readline_signal_curr) {
            if (SIGTERM == g_readline_signal_curr ||
#ifndef WIN32
                SIGQUIT == g_readline_signal_curr ||
#endif
                (SIGINT == g_readline_signal_curr && SIGINT == g_readline_signal_prev)) {
                g_last_simulator->insert_cmd(g_last_simulator->get_current_player(), "quit");
                is_continue = false;
            } else if (SIGINT == g_readline_signal_curr) {
                // libedit do not provide rl_cleanup_after_signal
            }

            if (NULL != cmd_c) {
                ::free(cmd_c);
            }
            g_readline_signal_prev = g_readline_signal_curr;
            g_readline_signal_curr = 0;
            continue;
        }

        // skip empty line
        if (NULL != cmd_c && *cmd_c != '\0') {
            g_last_simulator->insert_cmd(g_last_simulator->get_current_player(), cmd_c);

            add_history(cmd_c);
            if (!self->shell_opts_.history_file.empty()) {
                write_history(self->shell_opts_.history_file.c_str());
            }
        }

        if (cmd_c != NULL) {
            ::free(cmd_c);
        }

        // update promote
        if (self->get_current_player()) {
            prompt = self->get_current_player()->get_id() + ">";
        }
    }
}