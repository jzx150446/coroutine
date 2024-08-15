#include "scheduler.h"
//#include <functional>
static bool debug = false;

namespace sylar
{
static thread_local Scheduler *t_scheduler = nullptr;
Scheduler* Scheduler::GetThis()
{
    return t_scheduler;
}

void Scheduler::SetThis()
{
    t_scheduler = this;
}

Scheduler::Scheduler(size_t threads,bool use_caller ,const std::string& name ):
m_useCaller(use_caller),m_name(name)
{
    assert(threads >0 &&Scheduler::GetThis == nullptr);
    SetThis();
    
    Thread::SetName(name);
    //使用主线程为工作线程
    if(use_caller){
        threads--;
        //创建主协程
        Fiber::GetThis();

        //创建调度协程
        m_schedulerFiber.reset(new Fiber(std::bind(&Scheduler::run,this),0,false));  //该协程退出后将直接返回主协程
        Fiber::SetSchedulerFiber(m_schedulerFiber.get());

        m_rootThread = Thread::getThreadId();
        m_threadIds.push_back(m_rootThread);
    }
    m_threadCount = threads;
    if(debug) std::cout<<"Scheduler::Scheduler() success\n";
}
Scheduler::~Scheduler()
{
    assert(stopping()==true);
    if(GetThis() == this){
        t_scheduler == nullptr;
    }
    if(debug) std::cout<<"Scheduler::~Scheduler() success\n";
}

void Scheduler::start()
{
    std::lock_guard<std::mutex>lock(m_mutex);
    if(m_stopping){
        std::cerr<<"Scheduler is stopping"<<std::endl;
        return;
    }
    assert(m_threads.empty());
    m_threads.resize(m_threadCount);
    for(size_t i = 0; i<m_threadCount;++i)
    {
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run,this),m_name +'_'+std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());

    }
    if(debug) std::cout<<"Scheduler::start() success\n";
}

void Scheduler::run()
{
    int thread_id = Thread::getThreadId();
    if(debug) std::cout << "Schedule::run() starts in thread: " << thread_id << std::endl;

    SetThis();

    //运行在新创建的线程->需要创建主协程
    if(thread_id != m_rootThread){
        Fiber::GetThis();
    }
    std::shared_ptr<Fiber> idle_fiber = std::make_shared<Fiber>(std::bind(&Scheduler::idle,this));
    ScheduleTask task;
    while(true)
    {
        task.reset();
        bool tickle_me = false;
        {
            std::lock_guard<std::mutex>lock(m_mutex);
            auto it = m_tasks.begin();
            //遍历任务队列
            while(it !=m_tasks.end()){
                if(it->thread != -1 && it->thread!=thread_id){
                    it++;
                    tickle_me = true;
                    continue;
                }
                //取出任务
                assert(it->fiber || it->cb);
                task = *it;
                m_tasks.erase(it);
                m_activeThreadCount++;
                break;
            }
            tickle_me = tickle_me || it!=m_tasks.end();
        }
        if(tickle_me){
            tickle();
        }
        //执行任务
        if(task.fiber){
            {
                std::lock_guard<std::mutex>lock(m_mutex);
                if(task.fiber->getState() != Fiber::TERM){
                    task.fiber->resume();
                }
            }
            m_activeThreadCount--;
            task.reset();
        }
        else if(task.cb)
        {
            std::shared_ptr<Fiber>cb_fiber = std::make_shared<Fiber>(task.cb);
            {
                std::lock_guard<std::mutex>lock(m_mutex);
                cb_fiber->resume();
            }
            m_activeThreadCount--;
            task.reset();
        }else{ //无任务->执行空闲协程
            //系统关闭->idle协程从死循环中跳出并结束->此时的idle协程状态为TREM->再出进入将跳出循环并退出run
            if(idle_fiber->getState()==Fiber::TERM)
            {
                if(debug) std::cout << "Schedule::run() ends in thread: " << thread_id << std::endl;
                break;
            }
            m_idleThreadCount++;
            idle_fiber->resume();
            m_idleThreadCount--;
        }

    }
}

void Scheduler::stop()
{
    if(debug) std::cout << "Schedule::stop() starts in thread: " << Thread::getThreadId() << std::endl;
    if(stopping()){
        return ;
    }
    m_stopping = true;

    if(m_useCaller){
        assert(GetThis() == this);
    }else{
        assert(GetThis() != this);
    }
    for(size_t i = 0;i<m_threadCount;++i){
        tickle();
    }
    if(m_schedulerFiber)
    {
        tickle();
    }
    if(m_schedulerFiber){
        m_schedulerFiber->resume();
        if(debug) std::cout << "m_schedulerFiber ends in thread:" << Thread::getThreadId() << std::endl;
    }
    std::vector<std::shared_ptr<Thread>>thrs;
    {
        std::lock_guard<std::mutex>lock(m_mutex);
        thrs.swap(m_threads);
    }
    for(auto &i:thrs){
        i->join();
    }


}
void Scheduler::tickle()
{
}

void Scheduler::idle()
{
	while(!stopping())
	{
		if(debug) std::cout << "Scheduler::idle(), sleeping in thread: " << Thread::getThreadId() << std::endl;	
		sleep(1);	
		Fiber::GetThis()->yield();
	}
}

bool Scheduler::stopping() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stopping && m_tasks.empty() && m_activeThreadCount == 0;
}


}