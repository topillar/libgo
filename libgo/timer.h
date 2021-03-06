#pragma once
#include <map>
#include <functional>
#include <chrono>
#include <memory>
#include <vector>
#include <list>
#include "spinlock.h"

namespace co
{

class CoTimer;
typedef std::shared_ptr<CoTimer> CoTimerPtr;

typedef std::chrono::time_point<std::chrono::system_clock> SystemTimePoint;
typedef std::chrono::time_point<std::chrono::steady_clock> SteadyTimePoint;

class CoTimer
{
public:
    typedef std::function<void()> fn_t;
    typedef std::multimap<SystemTimePoint, CoTimerPtr>::iterator SystemToken;
    typedef std::multimap<SteadyTimePoint, CoTimerPtr>::iterator SteadyToken;

    uint64_t GetId();
    void operator()();

private:
    explicit CoTimer(fn_t const& fn);
    bool Cancel();
    bool BlockCancel();

    enum class e_token_state
    {
        none,
        system,
        steady
    };

private:
    uint64_t id_;
    static std::atomic<uint64_t> s_id;
    fn_t fn_;
    bool active_;
    LFLock fn_lock_;
    SystemToken system_token_;
    SteadyToken steady_token_;
    e_token_state token_state_;

    friend class CoTimerMgr;
};
typedef std::shared_ptr<CoTimer> CoTimerPtr;
typedef CoTimerPtr TimerId;

// 定时器管理
class CoTimerMgr
{
public:
    typedef std::multimap<SystemTimePoint, CoTimerPtr> SystemDeadLines;
    typedef std::multimap<SteadyTimePoint, CoTimerPtr> SteadyDeadLines;

    CoTimerMgr();

    CoTimerPtr ExpireAt(SystemTimePoint const& time_point, CoTimer::fn_t const& fn);

    CoTimerPtr ExpireAt(SteadyTimePoint const& time_point, CoTimer::fn_t const& fn);

    template <typename Duration>
    CoTimerPtr ExpireAt(Duration const& duration, CoTimer::fn_t const& fn)
    {
        return ExpireAt(SteadyNow() + duration, fn);
    }

    bool Cancel(CoTimerPtr co_timer_ptr);
    bool BlockCancel(CoTimerPtr co_timer_ptr);

    // @returns: 下一个触发的timer时间(单位: milliseconds)
    long long GetExpired(std::list<CoTimerPtr> &result, uint32_t n = 1);

    std::size_t Size();

private:
    static SystemTimePoint SystemNow();
    static SteadyTimePoint SteadyNow();

    void __Cancel(CoTimerPtr co_timer_ptr);

    long long GetNextTriggerTime();

    void SetNextTriggerTime(SystemTimePoint const& sys_tp);
    void SetNextTriggerTime(SteadyTimePoint const& sdy_tp);

private:
    SystemDeadLines system_deadlines_;
    SteadyDeadLines steady_deadlines_;
    LFLock lock_;

    // 下一个timer触发的时间
    //  单位: milliseconds
    // 这个值由GetExpired时成功lock的线程来设置, 未lock成功的线程也允许读取.
    std::atomic<long long> system_next_trigger_time_;
    std::atomic<long long> steady_next_trigger_time_;
};

} //namespace co
