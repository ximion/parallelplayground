
#include "worker.h"

#include <unistd.h>
#include <sys/mman.h>

#include <opencv2/imgproc.hpp>
#include <../cvmatshm.h>

SimpleWorker::SimpleWorker(QObject *parent)
    : SimpleWorkerSource(parent),
      m_shmSend(new QSharedMemory),
      m_shmRecv(new QSharedMemory)
{
    m_shmSend->setKey(QUuid::createUuid().toString(QUuid::Id128));
    m_timer = new QTimer(this); // Initialize timer
    QObject::connect(m_timer, &QTimer::timeout, this, &SimpleWorker::processAndSendData);
    m_timer->start(0);
    qDebug() << "Worker started";
}

SimpleWorker::~SimpleWorker()
{

}

bool SimpleWorker::processFrame(int style, const QString &shmKey)
{
    m_shmRecv->setKey(shmKey);

    auto frame = shm_to_cvmat(m_shmRecv);
    m_queue.enqueue(frame);

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
    auto frame = m_queue.dequeue();

    cv::blur(frame, frame, cv::Size(5, 5));

    cvmat_to_shm(m_shmSend, frame);
    emit frameProcessed(m_shmSend->key());
}
