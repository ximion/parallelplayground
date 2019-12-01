
#include <iostream>
#include <thread>
#include <atomic>
#include <pthread.h>

#include "proconhelper.h"
#include <lfqstream.h>

static std::atomic_bool producer_running = false;

void producer_fast(const std::string& threadName, LFQStream<MyDataFrame> *stream)
{
    pthread_setname_np(pthread_self(), threadName.c_str());

    while (!producer_running) {}

    for (size_t i = 1; i <= N_OF_DATAFRAMES; ++i) {
        auto data = create_data_200Hz(i);
        stream->push(data);
    }
    stream->terminate();
}

void consumer_fast(const std::string& threadName, LFQStream<MyDataFrame> *stream)
{
    pthread_setname_np(pthread_self(), threadName.c_str());
    size_t lastId = 0;

    auto sub = stream->subscribe(threadName);
    while (true) {
        auto data = sub->next();
        if (!data.has_value())
            break; // subscription has been terminated
        auto result = process_data_fast(data.value());

        //display_frame(result, sub->name());

        if (data->id != (lastId + 1))
            std::cout << "Value dropped (fast consumer) [" << data->id << "]" << std::endl;
        lastId = data->id;
    }
    if (lastId != N_OF_DATAFRAMES)
        std::cout << "Fast consumer received only " << lastId << " data elements out of " << N_OF_DATAFRAMES << std::endl;
}

void consumer_slow(const std::string& threadName, LFQStream<MyDataFrame> *stream)
{
    pthread_setname_np(pthread_self(), threadName.c_str());
    size_t lastId = 0;

    auto sub = stream->subscribe(threadName);
    while (true) {
        auto data = sub->next();
        if (!data.has_value())
            break; // subscription has been terminated
        process_data_slow(data.value());

        if (data->id != (lastId + 1))
            std::cout << "Value dropped (slow consumer) [" << data->id << "]" << std::endl;
        lastId = data->id;
    }

    if (lastId != N_OF_DATAFRAMES)
        std::cout << "Slow consumer received only " << lastId << " data elements out of " << N_OF_DATAFRAMES << std::endl;
}

void consumer_instant(const std::string& threadName, LFQStream<MyDataFrame> *stream)
{
    pthread_setname_np(pthread_self(), threadName.c_str());

    auto sub = stream->subscribe(threadName);
    while (true) {
        auto data = sub->next();
        if (!data.has_value())
            break; // subscription has been terminated
        process_data_instant(data.value());
    }
}

void transformer_fast(const std::string& threadName, LFQStream<MyDataFrame> *recvStream, LFQStream<MyDataFrame> *prodStream)
{
    pthread_setname_np(pthread_self(), threadName.c_str());
    size_t count = 1;

    auto sub = recvStream->subscribe(threadName);
    while (true) {
        auto data = sub->next();
        if (!data.has_value())
            break; // subscription has been terminated
        auto newData = transform_data_fast(data.value(), count);
        //display_frame(newData.frame, sub->name());

        prodStream->push(newData);
        count++;
    }
    prodStream->terminate();
}

void run_6threads()
{
    std::vector<std::thread> threads;
    std::shared_ptr<LFQStream<MyDataFrame>> prodStream(new LFQStream<MyDataFrame>());
    std::shared_ptr<LFQStream<MyDataFrame>> transStream(new LFQStream<MyDataFrame>());

    producer_running = false;
    threads.push_back(std::thread(producer_fast, "producer", prodStream.get()));
    threads.push_back(std::thread(consumer_fast, "consumer_fast", prodStream.get()));
    threads.push_back(std::thread(consumer_slow, "consumer_slow", prodStream.get()));
    threads.push_back(std::thread(consumer_instant, "consumer_instant", prodStream.get()));

    threads.push_back(std::thread(transformer_fast, "transformer", prodStream.get(), transStream.get()));
    threads.push_back(std::thread(consumer_fast, "consumer_tfo", transStream.get()));

    // launch production of elements, now that all threads
    // have been set up.
    producer_running = true;

    for(auto& t: threads)
        t.join();
}

void run_overcapacity()
{
    std::vector<std::thread> threads;
    const auto threadCount = std::thread::hardware_concurrency() * 2 + 2;

    std::shared_ptr<LFQStream<MyDataFrame>> prodStream(new LFQStream<MyDataFrame>());
    std::shared_ptr<LFQStream<MyDataFrame>> transStream(new LFQStream<MyDataFrame>());

    producer_running = false;
    threads.push_back(std::thread(producer_fast, "producer", prodStream.get()));
    threads.push_back(std::thread(consumer_fast, "consumer_fast", prodStream.get()));
    threads.push_back(std::thread(consumer_instant, "consumer_instant", prodStream.get()));

    threads.push_back(std::thread(transformer_fast, "transformer", prodStream.get(), transStream.get()));

    for (uint i = 0; i < threadCount - 4; ++i) {
        // we connect half of the regular consumers to the producer, the rest goes to the transformer
        if ((i % 2) == 0)
            threads.push_back(std::thread(consumer_fast, std::string("consumer_raw_") + std::to_string(i), prodStream.get()));
        else
            threads.push_back(std::thread(consumer_fast, std::string("consumer_tf_") + std::to_string(i), prodStream.get()));
    }

    std::cout << "Running " << threads.size() << " threads." << std::endl;

    // launch production of elements, now that all threads
    // have been set up.
    producer_running = true;

    for(auto& t: threads)
        t.join();
}

int main()
{
    run_timed("LFQStream-6threads", run_6threads, N_OF_RUNS);

    std::cout << std::endl;
    run_timed("LFQStream-overcapacity", run_overcapacity, N_OF_RUNS);

    return 0;
}
