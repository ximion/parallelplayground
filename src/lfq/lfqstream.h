
#pragma once

#include <boost/circular_buffer.hpp>
#include <thread>
#include <atomic>
#include <mutex>
#include <iostream>
#include <queue>
#include <condition_variable>

#include "ProducerConsumerQueue.h"

template<typename T>
class LFQStream;

template<typename T>
class LFQStreamSubscription
{
    friend LFQStream<T>;
public:
    LFQStreamSubscription(const std::string& name)
        : m_queue(folly::ProducerConsumerQueue<T>(4096))
    {
        m_name = name;
        m_terminated = false;
    }

    std::optional<T> next()
    {
        if (m_queue.isEmpty()) {
            std::unique_lock<std::mutex> lock(m_mtxNodata);
            while (m_queue.isEmpty() && !m_terminated) {
                m_waitrd.wait(lock);
            }
        }
        if (m_queue.isEmpty() && m_terminated) {
            return std::nullopt;
        }

        T data;
        m_queue.read(data);

        return data;
    }

    auto name() const
    {
        return m_name;
    }

private:
    folly::ProducerConsumerQueue<T> m_queue;
    std::mutex m_mtxNodata;
    std::condition_variable m_waitrd;
    std::atomic_bool m_terminated;

    std::string m_name;

    void push(const T &data)
    {
        m_queue.write(data);
        m_waitrd.notify_all();
    }

    void terminate()
    {
        m_terminated = true;
        m_waitrd.notify_all();
    }
};


template<typename T>
class LFQStream
{
public:
    LFQStream()
        : m_allowSubscribe(true)
    {
        m_ownerId = std::this_thread::get_id();
    }

    ~LFQStream()
    {
        terminate();
    }

    std::shared_ptr<LFQStreamSubscription<T>> subscribe(const std::string& name)
    {
        //if (!m_allowSubscribe)
        //    return nullptr;
        std::lock_guard<std::mutex> lock(m_mutex);
        std::shared_ptr<LFQStreamSubscription<T>> sub(new LFQStreamSubscription<T> (name));
        m_subs.push_back(sub);
        return sub;
    }

    void push(const T &data)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_allowSubscribe = false;
        for(auto& sub: m_subs)
            sub->push(data);
    }

    void terminate()
    {
        std::for_each(m_subs.begin(), m_subs.end(), [](std::shared_ptr<LFQStreamSubscription<T>> sub){ sub->terminate(); });
        m_subs.clear();
        m_allowSubscribe = true;
    }

private:
    std::mutex m_mutex;
    std::thread::id m_ownerId;
    std::atomic_bool m_allowSubscribe;
    std::vector<std::shared_ptr<LFQStreamSubscription<T>>> m_subs;
};
