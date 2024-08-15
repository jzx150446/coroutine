#include "coroutine.h"
#include<iostream>


static bool debug = false;
namespace sylar{
/*
typedef struct ucontext_t 
{
	// 当前上下⽂结束后，下⼀个激活的上下⽂对象的指针，由makecontext()设置
	struct ucontext_t *uc_link;
	
	// 当前上下⽂的信号屏蔽掩码
	sigset_t uc_sigmask;
	
	// 当前上下⽂使⽤的栈空间，由makecontext()设置
	stack_t uc_stack;
	
	// 平台相关的上下⽂具体内容，包含寄存器的值
	mcontext_t uc_mcontext;
	
	...
} ucontext_t;
*/
//正在运行的协程
static thread_local Fiber *t_fiber = nullptr;
//主协程
static thread_local std::shared_ptr<Fiber> t_thread_fiber = nullptr;
//调度协程
static thread_local Fiber *t_scheduler_fiber = nullptr;
//协程计数器
static thread_local int s_fiber_count = 0;
//协程id
static thread_local uint64_t s_fiber_id = 0;


void Fiber::SetThis(Fiber *f)
{
    t_fiber = f;
}
//首先运行该函数创建主协程
std::shared_ptr<Fiber>Fiber::GetThis()
{
    if(t_fiber){
        return t_fiber->shared_from_this();
    }

    std::shared_ptr<Fiber>main_fiber(new Fiber());
   
    t_thread_fiber = main_fiber;
    t_scheduler_fiber = main_fiber.get(); // 除非主动设置 主协程默认为调度协程 
    assert(t_fiber==main_fiber.get());
    return t_fiber->shared_from_this();
}   

void Fiber::SetSchedulerFiber(Fiber* f){
    t_scheduler_fiber = f;
}

uint64_t Fiber::GetFiberId()
{
    if(t_fiber){
        return t_fiber->getID();
    }
    return (uint64_t)-1;
}
Fiber::Fiber()
{
    SetThis(this);
    m_state = RUNNING;


    if(getcontext(&m_ctx)){
        std::cerr<<"Fiber() error\n";
        exit(0);
    }
    s_fiber_count++;
    m_id = s_fiber_id++;
    if(debug) std::cout<<"Fiber() main id =  "<<m_id<<std::endl;
}
Fiber::Fiber(std::function<void()>cb,size_t stacksize , bool run_in_scheduler )
:m_cb(cb),m_in_run_scheduler(run_in_scheduler)
{
    m_state = READY;
    m_stacksize = stacksize?stacksize:128000;
    m_stack= malloc(m_stacksize);

    if(getcontext(&m_ctx)){
        std::cerr<<"Fiber(std::function<void()>cb,size_t stacksize , bool run_in_scheduler ) fail\n";
        pthread_exit(nullptr);
    }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_size = m_stacksize;
    m_ctx.uc_stack.ss_sp = m_stack;

    makecontext(&m_ctx,&Fiber::MainFunc,0);
    m_id = s_fiber_id++;
    s_fiber_count++;
    if(debug) std::cout<<"Fiber() id =  "<<m_id<<std::endl;
}

Fiber::~Fiber()
{   
    s_fiber_count--;
    if(m_stack){
        free(m_stack);
    }
     if(debug) std::cout<<"~Fiber() id =  "<<m_id<<std::endl;
}

void Fiber::reset(std::function<void()>cb)
{
    assert(m_stack!=nullptr);
    assert(m_state ==TERM);

    m_cb = cb;
    m_state = READY;

    if(getcontext(&m_ctx))
    {
        std::cerr<<"reset() fail\n";
        pthread_exit(nullptr);
    }

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    makecontext(&m_ctx,&Fiber::MainFunc,0);

}

void Fiber::resume()
{
    assert(m_state == READY);
    //SetThis(this);
    m_state = RUNNING;
    if(m_in_run_scheduler)
    {
        SetThis(this);
        if (swapcontext(&(t_scheduler_fiber->m_ctx), &m_ctx))
        {
            std::cerr << "resume() to t_scheduler_fiber failed\n";
			pthread_exit(NULL);
        }
    }else{
        SetThis(this);
        if (swapcontext(&(t_thread_fiber->m_ctx), &m_ctx))
        {
            std::cerr << "resume() to t_thread_fiber failed\n";
			pthread_exit(NULL);
        }
    }
}

void Fiber::yield()
{
    assert(m_state == RUNNING || m_state == TERM );
    //SetThis(t_thread_fiber.get());
    if(m_state!=TERM)
    {
        m_state =RUNNING;
    }
    if (m_in_run_scheduler)
    {
        SetThis(t_scheduler_fiber);
        if (swapcontext(&m_ctx, &(t_scheduler_fiber->m_ctx)))
        {
            std::cerr << "resume() to t_scheduler_fiber failed\n";
			pthread_exit(NULL);
        }
    }else{
        SetThis(t_thread_fiber.get());
        if (swapcontext(&m_ctx, &(t_thread_fiber->m_ctx)))
        {
            std::cerr << "resume() to t_thread_fiber failed\n";
			pthread_exit(NULL);
        }
    }
}

void Fiber::MainFunc()
{
    std::shared_ptr<Fiber>curr = GetThis();
    assert(curr!=nullptr);

    curr->m_cb();

    curr->m_cb = nullptr;
    curr->m_state = TERM;
    auto raw_ptr = curr.get();
    curr.reset();
    raw_ptr->yield();
}
}
