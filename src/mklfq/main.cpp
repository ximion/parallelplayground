
#include <iostream>
#include <thread>
#include <atomic>
#include <pthread.h>

#include "barrier.h"
#include "proconhelper.h"
#include "mklfqstream.h"

void producer_fast(const std::string& threadName, Barrier *barrier, MKLFQStream<MyDataFrame> *stream)
{
    pthread_setname_np(pthread_self(), threadName.c_str());

    barrier->wait();
    for (size_t i = 1; i <= N_OF_DATAFRAMES; ++i) {
        auto data = create_data_200Hz(i);
        stream->push(data);
    }
    stream->terminate();
}

void consumer_fast(const std::string& threadName, Barrier *barrier, MKLFQStream<MyDataFrame> *stream)
{
    pthread_setname_np(pthread_self(), threadName.c_str());
    size_t lastId = 0;

    auto sub = stream->subscribe(threadName);
    barrier->wait();
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

void consumer_slow(const std::string& threadName, Barrier *barrier, MKLFQStream<MyDataFrame> *stream)
{
    pthread_setname_np(pthread_self(), threadName.c_str());
    size_t lastId = 0;

    auto sub = stream->subscribe(threadName);
    barrier->wait();
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

void consumer_instant(const std::string& threadName, Barrier *barrier, MKLFQStream<MyDataFrame> *stream)
{
    pthread_setname_np(pthread_self(), threadName.c_str());

    auto sub = stream->subscribe(threadName);
    barrier->wait();
    while (true) {
        auto data = sub->next();
        if (!data.has_value())
            break; // subscription has been terminated
        process_data_instant(data.value());
    }
}

void transformer_fast(const std::string& threadName, Barrier *barrier, MKLFQStream<MyDataFrame> *recvStream, MKLFQStream<MyDataFrame> *prodStream)
{
    pthread_setname_np(pthread_self(), threadName.c_str());
    size_t count = 1;

    auto sub = recvStream->subscribe(threadName);
    barrier->wait();
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

double run_6threads()
{
    Barrier barrier(6);

    std::vector<std::thread> threads;
    std::shared_ptr<MKLFQStream<MyDataFrame>> prodStream(new MKLFQStream<MyDataFrame>());
    std::shared_ptr<MKLFQStream<MyDataFrame>> transStream(new MKLFQStream<MyDataFrame>());

    threads.push_back(std::thread(producer_fast, "producer", &barrier, prodStream.get()));
    threads.push_back(std::thread(consumer_fast, "consumer_fast", &barrier, prodStream.get()));
    threads.push_back(std::thread(consumer_slow, "consumer_slow", &barrier, prodStream.get()));
    threads.push_back(std::thread(consumer_instant, "consumer_instant", &barrier, prodStream.get()));

    threads.push_back(std::thread(transformer_fast, "transformer", &barrier, prodStream.get(), transStream.get()));
    threads.push_back(std::thread(consumer_fast, "consumer_tfo", &barrier, transStream.get()));

    for(auto& t: threads)
        t.join();

    return barrier.timeElapsed();
}

double run_overcapacity()
{
    std::vector<std::thread> threads;
    const auto threadCount = std::thread::hardware_concurrency() * 2 + 2;
    Barrier barrier(threadCount);

    std::shared_ptr<MKLFQStream<MyDataFrame>> prodStream(new MKLFQStream<MyDataFrame>());
    std::shared_ptr<MKLFQStream<MyDataFrame>> transStream(new MKLFQStream<MyDataFrame>());

    threads.push_back(std::thread(producer_fast, "producer", &barrier, prodStream.get()));
    threads.push_back(std::thread(consumer_fast, "consumer_fast", &barrier, prodStream.get()));
    threads.push_back(std::thread(consumer_instant, "consumer_instant", &barrier, prodStream.get()));

    threads.push_back(std::thread(transformer_fast, "transformer", &barrier, prodStream.get(), transStream.get()));

    for (uint i = 0; i < threadCount - 4; ++i) {
        // we connect half of the regular consumers to the producer, the rest goes to the transformer
        if ((i % 2) == 0)
            threads.push_back(std::thread(consumer_fast, std::string("consumer_raw_") + std::to_string(i), &barrier, prodStream.get()));
        else
            threads.push_back(std::thread(consumer_fast, std::string("consumer_tf_") + std::to_string(i), &barrier, transStream.get()));
    }

    std::cout << "Running " << threads.size() << " threads." << std::endl;

    for(auto& t: threads)
        t.join();

    return barrier.timeElapsed();
}

int main()
{
    run_timed("MKLFQStream-6threads", run_6threads, N_OF_RUNS);

    std::cout << std::endl;
    run_timed("MKLFQStream-overcapacity", run_overcapacity, N_OF_RUNS);

    return 0;
}
