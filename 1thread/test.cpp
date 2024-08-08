#include "thread.h"
#include<iostream>
#include<vector>
#include<unistd.h>
#include<memory>


using namespace sylar;


void func()
{
    std::cout<<"id: "<<Thread::getThreadId()<<", name:"<<Thread::GetName()<<std::endl;
    std::cout<<",this id:"<<Thread::GetThis()->getId()<<",this name:"<<Thread::GetThis()->getName()<<std::endl;
    sleep(60);
    
}
int main()
{
    std::vector<std::shared_ptr<Thread>>thrs;

    for(int i = 0; i<5;i++){
        std::shared_ptr<Thread>ptr = std::make_shared<Thread>(&func,"thread_"+std::to_string(i));
        thrs.push_back(ptr);
    }

    for(int i = 0 ;i<5;i++){
        thrs[i]->join();
    }
    return 0;
}