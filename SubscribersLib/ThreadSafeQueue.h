#include <queue>
#include <mutex>
#include <condition_variable>

#pragma once

template <typename T>
class ThreadSafeQueue {
private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable cond_;
    bool aborted_ = false;

public:
    ThreadSafeQueue() = default;

    void Push(T value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(value));
        }
        cond_.notify_one();
    }

    bool Pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        cond_.wait(lock, [this] { 
            return !queue_.empty() || aborted_; 
        });

        if (aborted_ || queue_.empty()) return false;

        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    void Abort() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            aborted_ = true;
        }
        cond_.notify_all(); 
    }
};