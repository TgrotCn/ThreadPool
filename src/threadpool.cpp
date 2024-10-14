

#include "threadpool.h"

/////////////////// ThreadPool相关实现
ThreadPool::ThreadPool()
    : taskNum_(0) 
    , initThreadSize_(0)
    , idelThreadNum_(0)
    , taskNumThresHold_(MAX_TASK_SIZE)
    , threadNumThresHold_(MAX_THREAD_SIZE)
    , isRunning_(false)
    , mode_(PoolMode::MODE_FIXED) { }

ThreadPool::~ThreadPool() { 
    isRunning_ = false;

    //  等待线程池所有线程返回 有两种状态：阻塞 & 正在执行任务 
    std::unique_lock<std::mutex> lock(threadPoolMutex_);
    notEmpty_.notify_all();
    exitCond_.wait(lock, [&]()->bool { return threads_.size() == 0; });
}

void ThreadPool::start(int size) {
    // 记录初始线程个数
    initThreadSize_ = size;
    curThreadNum_ = size;
    
    // 设置线程池运行状态
    isRunning_ = true;

    for (int i = 0;i < initThreadSize_;++i) {
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
        auto threadId = ptr->getId();
        threads_.emplace(threadId, std::move(ptr));
    }

    for (int i = 0;i < initThreadSize_;++i) {
        threads_[i]->start();
        idelThreadNum_++;
    }    
}


void ThreadPool::setThreadPoolMode(PoolMode mode) {
    if (checkRunningState()) return ;
    mode_ = mode;
}

void ThreadPool::setTaskThresHold(int size) {
    if (checkRunningState()) return ;
    taskNumThresHold_ = size;
}

void ThreadPool::setThreadThresHold(int size) {
    if (checkRunningState()) return ;
    if (mode_ == PoolMode::MODE_CACHED) {
        threadNumThresHold_ = size;
    }
}

void ThreadPool::delThread(int threadId){
    // 开始回收线程
    // 记录线程相关数量的值修改
    curThreadNum_--;
    idelThreadNum_--;
    std::cout << "threadId:" << std::this_thread::get_id() << "exit! \n";
    // 把线程对象从线程列表容器中删除
    threads_.erase(threadId);
}


void ThreadPool::threadFunc(int threadId) {
    auto lastTime = std::chrono::high_resolution_clock().now();

    while (isRunning_) {
        std::shared_ptr<Task> task;
        {
            // 获取锁
            std::unique_lock<std::mutex> lock(threadPoolMutex_);
        
            //线程通讯
            // cache模式下， 如果大于60s空闲，将多余的创建线程进行回收。
            // 需要记录当前时间以及上一次处理任务的时间
            // 每一秒判断一次 超时返回还是有任务待执行返回
            // *** 锁 + 双重判断
            while (isRunning_ && tasks_.size() == 0) {
                if (mode_ == PoolMode::MODE_CACHED) {
                    if (std::cv_status::timeout == 
                        notEmpty_.wait_for(lock, std::chrono::seconds(1))) {
                        auto now = std::chrono::high_resolution_clock().now();
                        auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);
                        if (dur.count() >= THREAD_MAX_IDEL_TIME
                            && curThreadNum_ > initThreadSize_) {
                            delThread(threadId);
                            return;
                        }
                    }
                }
                else {
                    notEmpty_.wait(lock);
                }

                // 线程池要结束
                // if (!isRunning_){
                //     delThread(threadId);
                //     exitCond_.notify_all();
                //     return;
                // }


            }
            // 线程池要结束
            if (!isRunning_){
                break;
            }

            // 取出任务
            task = tasks_.back();
            tasks_.pop_back();
            idelThreadNum_--;
            taskNum_--;

            if(tasks_.size() > 0) {
                notEmpty_.notify_all();
            }
            notFull_.notify_all();
        }

        
        if (task) {
            std::cout << "theadId : " << std::this_thread::get_id() << "开始执行任务!\n";
            task->exec();
        }
        idelThreadNum_ ++;
        lastTime = std::chrono::high_resolution_clock().now();
    }

    delThread(threadId);
    exitCond_.notify_all();
}

Result ThreadPool::submitTask(std::shared_ptr<Task> sp) {

    // 获取锁
    std::unique_lock<std::mutex> lock(threadPoolMutex_);


    if (!notFull_.wait_for(lock, std::chrono::seconds(1),
		[&]()->bool { return tasks_.size() < (size_t)MAX_TASK_SIZE; }))
	{
		std::cerr << "task queue is full, submit task fail." << std::endl;
		return Result(sp, false);
	}

    // 提交任务
    tasks_.emplace_back(sp);
    taskNum_++;

    // 通知所有消费者有任务到来
    notEmpty_.notify_all();

    // 需要根据任务的数量来是否创建新的线程
    if (mode_ == PoolMode::MODE_CACHED 
        && taskNum_ > idelThreadNum_
        && curThreadNum_ < threadNumThresHold_) 
    {
        std::cout << "create new thread!\n";
        
        auto ptr = std::make_unique<Thread>(std::bind(&ThreadPool::threadFunc, this, std::placeholders::_1));
        auto threadId = ptr->getId();
        threads_.emplace(threadId, std::move(ptr));
        threads_[threadId]->start();
        curThreadNum_++;
        idelThreadNum_++;
    }
    
    return Result(sp);
}

bool ThreadPool::checkRunningState() const {
    return isRunning_;
}

//////////////////////////// Thread相关实现
int Thread::generateId_ = 0;

Thread::Thread(Func func) 
  : func_(func)
  , threadId_(generateId_++) { 
    // std::cout << "thread " << threadId_ << "create! \n";
}

Thread::~Thread() { }

void Thread::start() {
  std::thread t(func_, threadId_);
  t.detach();
}

int Thread::getId() const {
    return threadId_;
}

//////////////////////////// Task相关实现
Task::Task(): result_(nullptr) { }

Task::~Task() { }


void Task::exec() {

    if (result_)
        result_->setVal(run()); // 调用多态
}

void Task::setResult(Result* result) {
    result_ = result;
}
//////////////////////////// Result相关实现

Result::Result(std::shared_ptr<Task> sp, bool isValid) 
    : taskPtr_(sp)
    , isValid_(isValid) { 

    taskPtr_->setResult(this);
}

Any Result::get() {
    if (!isValid_) { 
        return "";
    }

    semaphore_.wait();
    return std::move(any_);
}  

void Result::setVal(Any any) {
    // 存储task返回值
    this->any_ = std::move(any);
    // 信号量增加
    semaphore_.post();
}