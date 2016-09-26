//
// Created by owt50 on 2016/9/26.
//

#ifndef _DISPATCHER_TASK_MANAGER_H
#define _DISPATCHER_TASK_MANAGER_H

#pragma once

#include <design_pattern/singleton.h>

namespace hello {
    class message_container;
}

class task_manager : public ::util::design_pattern::singleton<task_manager> {
public:
    struct task_macro_coroutine {
        typedef copp::detail::coroutine_context_base coroutine_t;
        typedef copp::allocator::stack_allocator_posix stack_allocator_t; // TODO 有需要可以换成通过内存池创建栈空间
        typedef copp::detail::coroutine_context_container<coroutine_t, stack_allocator_t> coroutine_container_t;
    };

    typedef cotask::task<task_macro_coroutine> task_t; // TODO 以后有需要可以换成通过内存池创建协程任务
    typedef typename task_t::id_t id_t;
    typedef typename task_t::action_ptr_t action_ptr_t;
    typedef std::function<int(task_manager::id_t &)> action_creator_t;

    template<typename TAction>
    struct action_maker_t {
        int operator()(task_manager::id_t &task_id) {
            return task_manager::me()->create_task<TAction>(task_id);
        };
    };

protected:
    task_manager();
    ~task_manager();

public:
    int init();

    /**
     * 获取栈大小
     */
    size_t get_stack_size() const;

    /**
     * @brief 创建任务
     * @param task_id 协程任务的ID
     * @param msg 相关消息体
     * @return 0或错误码
     */
    template <typename TAction, typename... TParams>
    int create_task(id_t &task_id, TParams &&... args) {
        std::shared_ptr<task_t> res = task_t::create_with<TAction>(get_stack_size(), std::forward<TParams>(args)...);
        if (!res) {
            task_id = 0;
            WLOGERROR("create task failed. current task number=%u",
                      static_cast<uint32_t>(native_mgr_->get_task_size()));
            return moyo_no1::err::EN_SYS_MALLOC;
        }

        task_id = res->get_id();
        return add_task(res, 0);
    }

    /**
     * @brief 创建任务并指定超时时间
     * @param task_id 协程任务的ID
     * @param timeout 超时时间
     * @param msg 相关消息体
     * @return 0或错误码
     */
    template <typename TAction, typename... TParams>
    int create_task_with_timeout(id_t &task_id, time_t timeout, TParams &&... args) {
        std::shared_ptr<task_t> res = task_t::create_with<TAction>(get_stack_size(), std::forward<TParams>(args)...);
        if (!res) {
            task_id = 0;
            WLOGERROR("create task failed. current task number=%u",
                      static_cast<uint32_t>(native_mgr_->get_task_size()));
            return moyo_no1::err::EN_SYS_MALLOC;
        }

        task_id = res->get_id();
        return add_task(res, timeout);
    }

    /**
     * @brief 创建任务
     * @param action 协程任务的执行体
     * @param task_id 协程任务的ID
     * @return 0或错误码
     */
    template <typename TAction>
    action_creator_t make_task_creator() {
        return action_maker_t<TAction>();
    }

    /**
     * @brief 开始任务
     * @param task_id 协程任务的ID
     * @param msg 相关消息体
     * @return 0或错误码
     */
    int start_task(id_t task_id, hello::message_container &msg);

    /**
     * @brief 恢复任务
     * @param task_id 协程任务的ID
     * @param msg 相关消息体
     * @return 0或错误码
     */
    int resume_task(id_t task_id, hello::message_container &msg);

    /**
     * @brief tick，可能会触发任务过期
     */
    int tick(time_t sec, int nsec);

private:
    /**
     * @brief 创建任务
     * @param task 协程任务
     * @param timeout 超时时间
     * @return 0或错误码
     */
    int add_task(const std::shared_ptr<task_t> &task, time_t timeout = 0);

private:
    typedef cotask::task_manager<id_t> mgr_t;
    typedef typename mgr_t::ptr_t mgr_ptr_t;
    mgr_ptr_t native_mgr_;
};


#endif //ATF4G_CO_TASK_MANAGER_H
