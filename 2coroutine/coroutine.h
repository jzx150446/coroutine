#ifndef COROUTINE_H_
#define COROUTINE_H_


#include<iostream>
#include<memory>
#include<ucontext.h>
#include<atomic>
#include<functional>
#include<cassert>


class Fiber:public std::enable_shared_from_this<Fiber>
{
public:
    typedef std::shared_ptr<Fiber> ptr;
    enum State{
        READY,
        RUNNING,
        TERM,
    };
private:
    Fiber();
public:
    Fiber(std::function<void()>cb,size_t stacksize = 0, bool run_in_scheduler = true);

    ~Fiber();


    void reset(std::function<void()>cb);

    void resume();

    void yield();

    uint64_t getID() const{return m_id;}

    State getState() const{return m_state;}

public:
    static void SetThis(Fiber *f);

    static std::shared_ptr<Fiber>GetThis();

    static uint64_t TotalFibers();

    static void MainFunc();

    static uint64_t GetFiberId();

private:
    uint64_t m_id = 0;
    uint32_t m_stacksize =0;

    State m_state = READY;

    ucontext_t m_ctx;
    void *m_stack = nullptr;
    std::function<void()>m_cb;

    bool in_run_scheduler;
};


#endif
