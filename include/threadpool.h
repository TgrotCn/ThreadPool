#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <functional>
#include <thread>
#include <vector>
#include <memory>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <unordered_map>

const int MAX_TASK_SIZE = 1024;
const int MAX_THREAD_SIZE = 200;
const int THREAD_MAX_IDEL_TIME = 10;
enum class PoolMode{
    MODE_FIXED,
    MODE_CACHED,
};


class Any {
public:
  Any() = default;
  ~Any() = default;

  Any(const Any&) = delete;
  Any& operator=(const Any&) = delete;
  Any(Any&&) = default;
  Any& operator=(Any&&) = default;


  template<typename T> 
  Any(T data) : base_(std::make_unique<Derive<T>>(data)) { }
  
  template<typename T>
  T cast_() {
      // 从base_找到指向的Derive对象，从里面取出data成员变量
      // 基类指针 -> 派生类指针 RTTI
      Derive<T>* pd =  dynamic_cast<Derive<T>*>(base_.get());
      if (pd == nullptr) {
          throw "type is unmatch!";
      }
      return pd->data_;
  }
private:
    class Base{
    public:
      virtual ~Base() = default;
    };


    template<typename T>
    class Derive : public Base{
        public:
          Derive(T data): data_(data) { }
        private:
          T data_;
          friend class Any;
    };

private:
    // 定义
    std::unique_ptr<Base> base_;
};

class Semaphore {
public:
    Semaphore(int limit = 0) : resLimit_(limit) {}
    ~Semaphore() = default;

    void wait(){
        std::unique_lock<std::mutex> lock(innerMtx_);
        cv.wait(lock, [&]()->bool { return resLimit_ > 0; });
        resLimit_--;
    }

    void post() {
        std::unique_lock<std::mutex> lock(innerMtx_);
        resLimit_++;
        cv.notify_all();
    }

private:
    int resLimit_;
    std::mutex innerMtx_;
    std::condition_variable cv;

};

class Task;

// 实现接受提交到线程池的task任务完成后的返回值类型Result
class Result {
public:
    Result(std::shared_ptr<Task> sp, bool isValid = true);
    ~Result() = default;

    // 问题一: setVal方法， 获取任务执行完的返回值的;
    void setVal(Any Any) ;
    // 问题二: get方法，用户调用这个方法获取task返回值;
    Any get();

private:
    Any any_; // 任务的返回值
    Semaphore semaphore_; // 调用get方法用于线程阻塞的信号量
    std::shared_ptr<Task> taskPtr_;

    std::atomic_bool isValid_;
};

class Thread{
public: 
    Thread();
    ~Thread();
    using Func = std::function<void(int)>;
    Thread(Func func) ;

    void start();
    int getId() const;
private:
    Func func_;
    static int generateId_;
    int threadId_;
};




class Task
{
public:
    Task();
    virtual ~Task();
    virtual Any run() = 0;

    void exec();
    void setResult(Result* result);
private:
    Result* result_;

};



class ThreadPool {
public:
    ThreadPool();
    ~ThreadPool();

    void start(int size = std::thread::hardware_concurrency());

    Result submitTask(std::shared_ptr<Task> sp);

    void setThreadPoolMode(PoolMode mode);
    void setTaskThresHold(int size);
    void setThreadThresHold(int size);

private:
    bool checkRunningState() const;
    void threadFunc(int threadId);
    void delThread(int threadId);

private:
    // 线程相关
    std::unordered_map<int, std::unique_ptr<Thread>> threads_;
    int initThreadSize_; // 初始线程数量
    int threadNumThresHold_; // 线程数量上线阈值
    std::atomic_int idelThreadNum_; // 空闲线程数量
    std::atomic_int curThreadNum_; // 线程池当前总的线程数量

    // 任务相关
    std::vector<std::shared_ptr<Task>> tasks_;
    int taskNumThresHold_; // 任务上线阈值
    std::atomic_int taskNum_; // 任务数量

    //线程通讯
    std::mutex threadPoolMutex_;
    std::condition_variable notFull_;
    std::condition_variable notEmpty_;
    std::condition_variable exitCond_; // 等待线程资源回收 析构线程池

    PoolMode mode_;
    std::atomic_bool isRunning_;

};


#endif