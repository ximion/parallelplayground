
#pragma once

#include <thread>
#include <atomic>
#include <mutex>
#include <iostream>
#include <queue>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

template<typename T>
class EPLFQStream;

template<typename T>
class EPLFQStreamSubscription
{
    friend EPLFQStream<T>;
public:
    EPLFQStreamSubscription(const std::string& name)
    {
        m_name = name;
        m_terminated = false;
        m_mtxNodata.try_lock();

        struct epoll_event event;
        int ret = -1;

        m_efd = epoll_create1(EPOLL_CLOEXEC);
        m_efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if (m_efd == -1) {
            std::cout << "eventfd create: " <<std::strerror(errno) << std::endl;
            exit(9);
        }

        event.data.fd = m_efd;
        event.events = EPOLLIN | EPOLLET;
        ret = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_efd, &event);
        if (ret != 0) {
            std::cout << "epoll_ctl" << std::endl;
            exit(9);
        }
    }

    ~EPLFQStreamSubscription()
    {
        close(m_efd);
    }

    std::optional<T> next()
    {
        struct epoll_event *events;
        events = (epoll_event *) calloc(1, sizeof(struct epoll_event));
        if (events == NULL) {
                std::cout << "calloc epoll events" << std::endl;
                exit(9);
        }

        auto nfds = epoll_wait(m_epfd, events, 1, -1);
        for (int i = 0; i < nfds; i++) {
            if (events[i].events & EPOLLIN) {
                uint64_t v;
                //log_debug("[consumer-%d] got event from fd-%d",
                //        c->rank, events[i].data.fd);
                auto ret = read(events[i].data.fd, &v, sizeof(v));
                if (ret < 0) {
                    std::cout << "failed to read eventfd" << std::endl;
                    continue;
                }
            }
        }


        if (m_terminated) {
            if (!m_mtxNodata.try_lock())
                return std::nullopt;
        } else {
            m_mtxNodata.lock(); // lock if no data is available
        }
        std::lock_guard<std::mutex> lock(m_mutex); // lock buffer write
        if (m_buffer.empty()) {
            if (m_terminated) {
                m_mtxNodata.try_lock();
                return std::nullopt;
            }
            std::cerr << "UNEXPECTED EMPTY BUFFER" << std::endl;
            exit(9);
        }

        auto data = m_buffer.front();
        m_buffer.pop();

        // stay locked if no data is available in the buffer, otherwise lock again
        if (!m_buffer.empty())
            m_mtxNodata.unlock();

        return data;
    }

    auto bufferLength()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_buffer.size();
    }

    auto name() const
    {
        return m_name;
    }

private:
    std::queue<T> m_buffer;
    std::mutex m_mtxNodata;
    std::mutex m_mutex;
    std::atomic_bool m_terminated;

    int m_epfd;
    int m_efd;

    std::string m_name;

    void push(const T &data)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_buffer.push(data);
        m_mtxNodata.unlock();

        uint64_t val = 1;
        auto ret = write(m_efd, &val, sizeof(uint64_t));
        if (ret != 8) {
            std::cout << "failed to write eventfd" << std::endl;
            exit(9);
        }
    }

    void terminate()
    {
        m_terminated = true;
        m_mtxNodata.unlock();
    }
};


template<typename T>
class EPLFQStream
{
public:
    EPLFQStream()
        : m_streamTerminated(false)
    {
        m_ownerId = std::this_thread::get_id();
    }

    ~EPLFQStream()
    {
        terminate();
    }

    std::shared_ptr<EPLFQStreamSubscription<T>> subscribe(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::shared_ptr<EPLFQStreamSubscription<T>> sub(new EPLFQStreamSubscription<T> (name));
        m_subs.push_back(sub);
        return sub;
    }

    void push(const T &data)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::for_each(m_subs.begin(), m_subs.end(), [&](std::shared_ptr<EPLFQStreamSubscription<T>> sub){ sub->push(data); });
    }

    void terminate()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::for_each(m_subs.begin(), m_subs.end(), [](std::shared_ptr<EPLFQStreamSubscription<T>> sub){ sub->terminate(); });
        m_streamTerminated = true;
        m_subs.clear();
    }

private:
    std::thread::id m_ownerId;
    std::mutex m_mutex;
    std::vector<std::shared_ptr<EPLFQStreamSubscription<T>>> m_subs;
    std::atomic_bool m_streamTerminated;
};
