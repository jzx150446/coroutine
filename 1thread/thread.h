#ifndef THREAD_H_
#define THREAD_H

#include<mutex>
#include<functional>
#include<condition_variable>

namespace sylar{

class Semaphore
{
private:
    std::mutex mtx;
    std::condition_variable cv;
    int count;

public:
    explicit Semaphore(int _count = 0):count(_count){}
    //p
    void wait()
    {
        std::unique_lock<std::mutex>lock(mtx);
        while(count == 0){
            cv.wait(lock);
        }
        count--;
    }
    //v
    void signal()
    {
        std::unique_lock<std::mutex>lock(mtx);
        count++;
        cv.notify_one();
    }
    //~Semaphore();
};

//线程
class Thread
{
private:
    pid_t m_id = -1;
    pthread_t m_thread = 0;
    std::function<void()>m_cb;
    std::string m_name;
    Semaphore m_semaphore;      
public:
    Thread(std::function<void()>cb,const std::string& name);
    ~Thread();

    pid_t getId() const{return m_id;}
    const std::string &getName() const{return m_name;}
    void join();
public:
    static pid_t getThreadId();
    static Thread* GetThis();
    static const std::string& GetName();
    static void SetName(const std::string& name);
private:
    static void *run(void *arg);

};






}














#endif