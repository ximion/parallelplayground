
#include <iostream>
#include <thread>
#include <atomic>
#include <pthread.h>
#include <opencv2/highgui/highgui.hpp>

#include <QRemoteObjectNode>
#include <QSharedPointer>

#include <unistd.h>

#include "proconhelper.h"
#include "qrostream.h"
#include "cvmatshm.h"

#include "../barrier.h"

#include "workerconnector.h"

static const int CONSUMER_TYPE_INSTANT = 0;
static const int CONSUMER_TYPE_FAST = 1;
static const int CONSUMER_TYPE_SLOW = 2;
static const int CONSUMER_TYPE_TRANSFORM = 3;

void producer_fast(const std::string& threadName, Barrier *barrier, QROStream<MyDataFrame> *stream)
{
    pthread_setname_np(pthread_self(), threadName.c_str());

    barrier->wait();
    for (size_t i = 1; i <= N_OF_DATAFRAMES; ++i) {
        auto data = create_data_200Hz(i);
        stream->push(data);
    }
    stream->terminate();
}

void consumer_generic(const std::string& threadName, Barrier *barrier, QROStream<MyDataFrame> *stream, int ctype)
{
    pthread_setname_np(pthread_self(), threadName.c_str());
    int lastId = 0;
    auto sub = stream->subscribe(threadName);

    QEventLoop loop;
    QSharedPointer<SimpleWorkerReplica> ptr;
    QRemoteObjectNode repNode;
    ptr.reset(repNode.acquire<SimpleWorkerReplica>());
    WorkerConnector wc(ptr);
    wc.connectAndRun(nullptr);

    cv::VideoWriter video;
    auto writeVideo = false;
    //if (threadName.rfind("consumer_tf", 0) == 0) {
    //    writeVideo = true;
    //    video = cv::VideoWriter("/tmp/ptestout_cons.avi", cv::VideoWriter::fourcc('M','J','P','G'), 60, cv::Size(800, 600));
    //}

    barrier->wait();
    while (true) {
        auto data = sub->next();
        if (!data.has_value())
            break; // subscription has been terminated

        loop.processEvents();
        wc.sendProcessFrame(ctype, data.value());

        if (writeVideo) {
            auto lf = wc.lastFrame();
            if (!lf.empty())
                video.write(lf);
        }

        if (data->id != (lastId + 1))
            std::cout << threadName << " value dropped [" << data->id << "]" << std::endl;
        lastId = data->id;
    }
    if (lastId != N_OF_DATAFRAMES)
        std::cout << threadName << " consumer received only " << lastId << " data elements out of " << N_OF_DATAFRAMES << std::endl;
    if (writeVideo)
        video.release();
}

void transformer_fast(const std::string& threadName, Barrier *barrier, QROStream<MyDataFrame> *recvStream, QROStream<MyDataFrame> *prodStream)
{
    pthread_setname_np(pthread_self(), threadName.c_str());
    int lastId = 0;
    auto sub = recvStream->subscribe(threadName);

    QEventLoop loop;
    QSharedPointer<SimpleWorkerReplica> ptr;
    QRemoteObjectNode repNode;
    ptr.reset(repNode.acquire<SimpleWorkerReplica>());
    WorkerConnector wc(ptr);
    wc.connectAndRun(prodStream);

    barrier->wait();
    while (true) {
        auto data = sub->next();
        if (!data.has_value())
            break; // subscription has been terminated

        loop.processEvents();
        wc.sendProcessFrame(CONSUMER_TYPE_TRANSFORM, data.value());

        if (data->id != (lastId + 1))
            std::cout << "Value dropped [" << data->id << "]" << std::endl;
        lastId = data->id;
    }
    prodStream->terminate();
    if (lastId != N_OF_DATAFRAMES)
        std::cout << "Transformer received only " << lastId << " data elements out of " << N_OF_DATAFRAMES << std::endl;
}

double run_6threads()
{
    Barrier barrier(6);
    std::vector<std::thread> threads;
    std::shared_ptr<QROStream<MyDataFrame>> prodStream(new QROStream<MyDataFrame>());
    std::shared_ptr<QROStream<MyDataFrame>> transStream(new QROStream<MyDataFrame>());

    threads.push_back(std::thread(producer_fast, "producer", &barrier, prodStream.get()));
    threads.push_back(std::thread(consumer_generic, "consumer_fast", &barrier, prodStream.get(), CONSUMER_TYPE_FAST));
    threads.push_back(std::thread(consumer_generic, "consumer_slow", &barrier, prodStream.get(), CONSUMER_TYPE_SLOW));
    threads.push_back(std::thread(consumer_generic, "consumer_instant", &barrier, prodStream.get(), CONSUMER_TYPE_INSTANT));

    threads.push_back(std::thread(transformer_fast, "transformer", &barrier, prodStream.get(), transStream.get()));
    threads.push_back(std::thread(consumer_generic, "consumer_tfo", &barrier, transStream.get(), CONSUMER_TYPE_FAST));

    for(auto& t: threads)
        t.join();

    return barrier.timeElapsed();
}

double run_overcapacity()
{
    std::vector<std::thread> threads;
    const auto threadCount = std::thread::hardware_concurrency() * 2 + 2;
    Barrier barrier(threadCount);

    std::shared_ptr<QROStream<MyDataFrame>> prodStream(new QROStream<MyDataFrame>());
    std::shared_ptr<QROStream<MyDataFrame>> transStream(new QROStream<MyDataFrame>());

    threads.push_back(std::thread(producer_fast, "producer", &barrier, prodStream.get()));
    threads.push_back(std::thread(consumer_generic, "consumer_fast", &barrier, prodStream.get(), CONSUMER_TYPE_FAST));
    threads.push_back(std::thread(consumer_generic, "consumer_instant", &barrier, prodStream.get(), CONSUMER_TYPE_INSTANT));

    threads.push_back(std::thread(transformer_fast, "transformer", &barrier, prodStream.get(), transStream.get()));

    for (uint i = 0; i < threadCount - 4; ++i) {
        // we connect half of the regular consumers to the producer, the rest goes to the transformer
        if ((i % 2) == 0)
            threads.push_back(std::thread(consumer_generic, std::string("consumer_raw_") + std::to_string(i), &barrier, prodStream.get(), CONSUMER_TYPE_FAST));
        else
            threads.push_back(std::thread(consumer_generic, std::string("consumer_tf_") + std::to_string(i), &barrier, transStream.get(), CONSUMER_TYPE_FAST));
    }

    std::cout << "Running " << threads.size() << " threads." << std::endl;

    for(auto& t: threads)
        t.join();

    return barrier.timeElapsed();
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    run_timed("QROStream-6threads", run_6threads, N_OF_RUNS);

    std::cout << std::endl;
    run_timed("QROStream-overcapacity", run_overcapacity, N_OF_RUNS);

    return 0;
}
