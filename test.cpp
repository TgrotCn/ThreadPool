#include <iostream>
#include "threadpool.h"

/*
example:
需要实现重写的Task接口 以实现
*/ 
using ULL = unsigned long long;

class MyTask : public Task
{

public:
    MyTask(int beg = 0, int end = 0) : beg_(beg), end_(end) { }
    ~MyTask() { }

    Any run() {
        std::cout << "tid: " << std::this_thread::get_id() << " beg!" << std::endl;

        ULL sum = 0;
        for (int i = beg_;i <= end_;++i) {
            sum += i;
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "tid: " << std::this_thread::get_id() << "task end!\n";
        return sum;
    }   
private:
    int beg_, end_;
};


int main() {
    ThreadPool threadpool;
    threadpool.start(4);
    Result res1 = threadpool.submitTask(std::make_shared<MyTask>(1,  100000000));
    ULL sum1 = res1.get().cast_<ULL>();
    std::cout << "res: " << sum1 << "\n";
    std::cout << "main over!\n";
    #if 0
    {
        ThreadPool threadpool;
        PoolMode mode = PoolMode::MODE_CACHED;
        threadpool.setThreadPoolMode(mode);
        threadpool.start(4);

        Result res1 = threadpool.submitTask(std::make_shared<MyTask>(1,  100000000));
        Result res2 = threadpool.submitTask(std::make_shared<MyTask>(100000001,  200000000));
        Result res3 = threadpool.submitTask(std::make_shared<MyTask>(200000001,  300000000));
        threadpool.submitTask(std::make_shared<MyTask>(200000001,  300000000));    
        threadpool.submitTask(std::make_shared<MyTask>(200000001,  300000000));
        threadpool.submitTask(std::make_shared<MyTask>(200000001,  300000000));
        threadpool.submitTask(std::make_shared<MyTask>(200000001,  300000000));

        ULL sum1 = res1.get().cast_<ULL>();
        ULL sum2 = res2.get().cast_<ULL>();
        ULL sum3 = res3.get().cast_<ULL>();
        ULL res = sum1 + sum2 + sum3;
        std::cout << "res: " << res << "\n";
    }
    getchar();
    #endif
    return 0;
}