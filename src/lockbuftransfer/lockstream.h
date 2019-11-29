
#pragma once

#include <boost/circular_buffer.hpp>
#include <thread>
#include <atomic>
#include <mutex>
#include <iostream>
#include <queue>

template<typename T>
class LockStream;

template<typename T>
class LockStreamSubscription
{
    friend LockStream<T>;
public:
    LockStreamSubscription(const std::string& name)
    {
        m_name = name;
        m_terminated = false;
        m_mtxNodata.try_lock();
        //m_buffer = boost::circular_buffer<T> (24);
    }

    std::optional<T> next()
    {
        if (m_terminated) {
            if (!m_mtxNodata.try_lock())
                return std::nullopt;
        } else {
            m_mtxNodata.lock(); // lock if no data is available
        }

        std::lock_guard<std::mutex> lock(m_mutex); // lock buffer write
        assert(!m_buffer.empty());

        auto data = m_buffer.front();
        m_buffer.pop();
        if (!m_buffer.empty())
            m_mtxNodata.unlock();

        return data;
    }

    auto bufferLength()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_buffer.size();
    }

private:
    std::queue<T> m_buffer;
    std::mutex m_mtxNodata;
    std::mutex m_mutex;
    std::atomic_bool m_terminated;

    std::string m_name;

    void push(const T &data)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_buffer.push(data);
        m_mtxNodata.unlock();
    }

    void terminate()
    {
        m_terminated = true;
        m_mtxNodata.unlock();
    }
};


template<typename T>
class LockStream
{
public:
    LockStream()
        : m_streamTerminated(false)
    {
        m_ownerId = std::this_thread::get_id();
    }

    ~LockStream()
    {
        terminate();
    }

    std::shared_ptr<LockStreamSubscription<T>> subscribe(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::shared_ptr<LockStreamSubscription<T>> sub(new LockStreamSubscription<T> (name));
        m_subs.push_back(sub);
        return sub;
    }

    void push(const T &data)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::for_each(m_subs.begin(), m_subs.end(), [&](std::shared_ptr<LockStreamSubscription<T>> sub){ sub->push(data); });
    }

    void terminate()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::for_each(m_subs.begin(), m_subs.end(), [](std::shared_ptr<LockStreamSubscription<T>> sub){ sub->terminate(); });
        m_streamTerminated = true;
        m_subs.clear();
    }

private:
    std::thread::id m_ownerId;
    std::mutex m_mutex;
    std::vector<std::shared_ptr<LockStreamSubscription<T>>> m_subs;
    std::atomic_bool m_streamTerminated;
};
