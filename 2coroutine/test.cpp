#include "coroutine.h"
#include <vector>


using namespace sylar;
class scheduler{
public:
    void schedule(std::shared_ptr<Fiber>task){
        m_tasks.push_back(task);
    }

    void run()
    {
        std::cout<<"number: "<<m_tasks.size()<<std::endl;
        std::shared_ptr<Fiber>task;
        auto it = m_tasks.begin();
        while(it != m_tasks.end())
        {
            task = *it;
            task->resume();
            it++;
        }
        m_tasks.clear();
    }
private:
    std::vector<std::shared_ptr<Fiber>>m_tasks;
};

void test_(int i){
    std::cout<<"hello world "<<i<<std::endl;
}

int main()
{
    Fiber::GetThis();
    scheduler sc;

    for(auto i = 0; i<20;++i)
    {
        std::shared_ptr<Fiber>fiber = std::make_shared<Fiber>(std::bind(test_,i),0,false);
        sc.schedule(fiber);
    }
    sc.run();
    return 0;
}