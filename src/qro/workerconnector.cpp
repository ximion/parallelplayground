
#include "workerconnector.h"

#include <thread>
#include <QUuid>

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "cvmatshm.h"

WorkerConnector::WorkerConnector(QSharedPointer<SimpleWorkerReplica> ptr)
    : QObject(nullptr),
      m_reptr(ptr),
      m_proc(new QProcess(this)),
      m_shmSend(new SharedMemory),
      m_shmRecv(new SharedMemory),
      m_prodStream(nullptr)
{
    connect(ptr.data(), &SimpleWorkerReplica::frameProcessed, this, &WorkerConnector::receiveProcessedFrame);
    //m_proc->setProcessChannelMode(QProcess::ForwardedChannels);
}

WorkerConnector::~WorkerConnector()
{
    if (m_proc->state() == QProcess::Running) {
        m_proc->terminate();
        m_proc->waitForFinished(10000);
        m_proc->kill();
    }
}

bool WorkerConnector::connectAndRun(QROStream<MyDataFrame> *prodStream)
{
    m_prodStream = prodStream;
    const auto address = QStringLiteral("local:%1").arg(QUuid::createUuid().toString(QUuid::Id128));
    m_reptr->node()->connectToNode(QUrl(address));

    char threadName[40];
    pthread_getname_np(pthread_self(), &threadName[0], sizeof(threadName));

    const auto workerExe = QStringLiteral("%1/worker/qroworker").arg(QCoreApplication::applicationDirPath());
    m_proc->start(workerExe, QStringList() << address << QString::fromUtf8(threadName));
    if (!m_proc->waitForStarted())
        return false;
    return m_reptr->waitForSource(10000);
}

void WorkerConnector::sendProcessFrame(int style, const MyDataFrame &data)
{
    cvmat_to_shm(m_shmSend, data.frame);

    if (!m_reptr.data()->processFrame(style, data.id, data.timestamp, m_shmSend->shmKey()).waitForFinished(10000))
        qDebug() << "Frame processing failed!";
}

cv::Mat WorkerConnector::lastFrame() const
{
    return m_lastFrame;
}

void WorkerConnector::receiveProcessedFrame(uint id, long timestamp, const QString &shmKey)
{
    m_shmRecv->setShmKey(shmKey);
    MyDataFrame data;
    data.id = id;
    data.timestamp = timestamp;
    data.frame = shm_to_cvmat(m_shmRecv);

    if (m_prodStream != nullptr)
        m_prodStream->push(data);
    m_lastFrame = data.frame;
}
