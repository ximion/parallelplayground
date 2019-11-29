
#include <iostream>
#include <thread>
#include <atomic>
#include <pthread.h>

#include "proconhelper.h"
#include <lockstream.h>

static std::atomic_bool producer_running = false;

void producer_fast(LockStream<MyDataFrame> *stream)
{
    pthread_setname_np(pthread_self(), "producer");

    while (!producer_running) {}

    for (size_t i = 1; i <= N_OF_DATAFRAMES; ++i) {
        auto data = create_data_200Hz(i);
        stream->push(data);
    }
    stream->terminate();
}

void consumer_fast(LockStream<MyDataFrame> *stream)
{
    pthread_setname_np(pthread_self(), "consumer_fast");
    size_t lastId = 0;

    auto sub = stream->subscribe("FastConsumer");
    while (true) {
        auto data = sub->next();
        if (!data.has_value())
            break; // subscription has been terminated
        auto result = process_data_fast(data.value());

        //display_frame(result);

        if (data->id != (lastId + 1))
            std::cout << "Value dropped (fast consumer) [" << data->id << "]" << " BufferLen: " << sub->bufferLength() << std::endl;
        lastId = data->id;
    }
    if (lastId != N_OF_DATAFRAMES)
        std::cout << "Fast consumer received only " << lastId << " data elements out of " << N_OF_DATAFRAMES << std::endl;
}

void consumer_slow(LockStream<MyDataFrame> *stream)
{
    pthread_setname_np(pthread_self(), "consumer_slow");
    size_t lastId = 0;

    auto sub = stream->subscribe("SlowConsumer");
    while (true) {
        auto data = sub->next();
        if (!data.has_value())
            break; // subscription has been terminated
        process_data_slow(data.value());

        if (data->id != (lastId + 1))
            std::cout << "Value dropped (slow consumer) [" << data->id << "]" << " BufferLen: " << sub->bufferLength() << std::endl;
        lastId = data->id;
    }

    if (lastId != N_OF_DATAFRAMES)
        std::cout << "Slow consumer received only " << lastId << " data elements out of " << N_OF_DATAFRAMES << std::endl;
}

void consumer_instant(LockStream<MyDataFrame> *stream)
{
    pthread_setname_np(pthread_self(), "consumer_instant");

    auto sub = stream->subscribe("InstantConsumer");
    while (true) {
        auto data = sub->next();
        if (!data.has_value())
            break; // subscription has been terminated
        process_data_instant(data.value());
    }
}

void run_4threads()
{
    std::vector<std::thread> threads;
    auto stream = new LockStream<MyDataFrame>();

    producer_running = false;
    threads.push_back(std::thread(producer_fast, stream));
    threads.push_back(std::thread(consumer_fast, stream));
    threads.push_back(std::thread(consumer_slow, stream));
    threads.push_back(std::thread(consumer_instant, stream));

    // launch production of elements, now that all threads
    // have been set up.
    producer_running = true;

    for(auto& t: threads)
        t.join();
    delete stream;
}

void run_overcapacity()
{
    std::vector<std::thread> threads;
    const auto threadCount = std::thread::hardware_concurrency() * 2;

    auto stream = new LockStream<MyDataFrame>();

    producer_running = false;
    threads.push_back(std::thread(producer_fast, stream));
    threads.push_back(std::thread(consumer_fast, stream));
    threads.push_back(std::thread(consumer_slow, stream));
    threads.push_back(std::thread(consumer_instant, stream));

    for (uint i = 0; i < threadCount - 4; ++i)
        threads.push_back(std::thread(consumer_fast, stream));

    std::cout << "Running " << threads.size() << " threads." << std::endl;

    // launch production of elements, now that all threads
    // have been set up.
    producer_running = true;

    for(auto& t: threads)
        t.join();
    delete stream;
}

int main()
{
    run_timed("LockStream-4threads", run_4threads, N_OF_RUNS);

    std::cout << std::endl;
    run_timed("LockStream-overcapacity", run_overcapacity, N_OF_RUNS);

    return 0;
}
