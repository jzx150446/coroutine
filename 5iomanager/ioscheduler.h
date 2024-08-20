#ifndef SYLAR_IOSCHEDULER_H_
#define SYLAR_IOSCHEDULER_H_

#include "scheduler.h"
#include "timer.h"

namespace sylar{

    //work frow
    //1 register one event-> 2wait for it to ready ->3schedule the callback ->4unregister the event ->5 run the callback
class IOManager : public Scheduler,public TimerManager
{
public:
    /** *
    * @brief IO事件，继承自epoll对事件的定义
    * @details 这里只关心socket fd的读和写事件，其他epoll事件会归类到这两类事件中
    */
   enum Event{
    //无事件
    NONE = 0x0,
    READ = 0x1,
    WRITE = 0x4
   };
private:
    struct FdContext 
    {
        struct EventContext 
        {
            // scheduler
            Scheduler *scheduler = nullptr;
            // callback fiber
            std::shared_ptr<Fiber> fiber;
            // callback function
            std::function<void()> cb;
        };

        // read event context
        EventContext read; 
        // write event context
        EventContext write;
        int fd = 0;
        // events registered
        Event events = NONE;
        std::mutex mutex;

        EventContext& getEventContext(Event event);
        void resetEventContext(EventContext &ctx);
        void triggerEvent(Event event);        
    };
    
public:
    IOManager(size_t threads = 1,bool use_caller = true,const std::string &name = "IOmanager");
    ~IOManager();

    //add event at one time
    int addEvent(int fd, Event event,std::function<void()>cb= nullptr);
    //delete event;
    bool delEvent(int fd,Event event);
    //delete the event and trigger the callback
    bool cancelEvent(int fd,Event event);
    // delete all events and trigger its callback
    bool cancelAll(int fd);

    static IOManager* GetThis();
protected:

    void tickle() override;
    bool stopping() override;
    void idle() override;

    void onTimerInsertedAtFront() override;
    void contextResize(size_t size);
private:
    //epoll 文件句柄
    int m_epfd = 0;
    //pipe管道句柄
    int m_tickleFds[2];
    //当前等待执行的IO事件数量
    std::atomic<size_t>m_pendingEventCount = {0};
    
    std::shared_timed_mutex m_mutex;
    //socket事件上下文容器
    std::vector<FdContext*>m_fdContexts;



};
}


#endif