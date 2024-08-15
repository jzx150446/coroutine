#include "scheduler.h"

using namespace sylar;
static unsigned int test_number;

std::mutex m_mux;
void task()
{
    {
        std::lock_guard<std::mutex>lock(m_mux);
        std::cout<<"task"<<test_number++<<" is under processing in thread: "<<Thread::getThreadId()<<std::endl;
    }
    sleep(1);
}

int main(int argc,char const * argv[])
{
    {
        //std::lock_guard<std::mutex>lock(m_mux);
        std::shared_ptr<Scheduler> scheduler = std::make_shared<Scheduler>(3,true,"scheduler_1");
        scheduler->start();
        sleep(2);

        std::cout << "\nbegin post\n\n"; 
        for(int i = 0; i<5;i++){
            std::shared_ptr<Fiber>fiber = std::make_shared<Fiber>(task);
            scheduler->scheduleLock(fiber);
        }

        sleep(6);

		std::cout << "\npost again\n\n"; 
		for(int i=0;i<15;i++)
		{
			std::shared_ptr<Fiber> fiber = std::make_shared<Fiber>(task);
			scheduler->scheduleLock(fiber);
		}		

		sleep(3);
		// scheduler如果有设置将加入工作处理
		scheduler->stop();

    }
    return 0;
}