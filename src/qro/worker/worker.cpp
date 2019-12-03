
#include "worker.h"

#include <unistd.h>
#include <sys/mman.h>

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <../cvmatshm.h>

SimpleWorker::SimpleWorker(QObject *parent)
    : SimpleWorkerSource(parent),
      m_shmSend(new SharedMemory),
      m_shmRecv(new SharedMemory)
{
    m_timer = new QTimer(this); // Initialize timer
    QObject::connect(m_timer, &QTimer::timeout, this, &SimpleWorker::processAndSendData);
    m_timer->start(0);

    m_winName = QCoreApplication::arguments().at(1).toStdString();
}

SimpleWorker::~SimpleWorker()
{

}

bool SimpleWorker::processFrame(int style, uint id, long timestamp, const QString &shmKey)
{
    MyDataFrame data;
    m_shmRecv->setShmKey(shmKey);

    data.frame = shm_to_cvmat(m_shmRecv);
    data.id = id;
    data.timestamp = timestamp;
    m_queue.enqueue(std::make_pair(style, data));

    return true;
}

bool SimpleWorker::ready() const
{
    return true;
}

void SimpleWorker::processAndSendData()
{
    if (m_queue.isEmpty())
        return;
    auto pair = m_queue.dequeue();
    auto data = pair.second;

    if (pair.first == 0)
        data.frame = process_data_instant(data);
    else if (pair.first == 1)
        data.frame = process_data_fast(data);
    else if (pair.first == 2)
        data.frame = process_data_slow(data);
    else if (pair.first == 3)
        data = transform_data_fast(data, data.id);

    cvmat_to_shm(m_shmSend, data.frame);

    //cv::imshow(m_winName, data.frame);
    //cv::waitKey(1);

    emit frameProcessed(data.id, data.timestamp, m_shmSend->shmKey());
}
