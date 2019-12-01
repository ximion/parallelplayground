
#include "workerconnector.h"

#include <thread>
#include <QUuid>

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "cvmatshm.h"

//static cv::VideoWriter video("/tmp/outcpp.avi", cv::VideoWriter::fourcc('M','J','P','G'), 60, cv::Size(800, 600));

WorkerConnector::WorkerConnector(QSharedPointer<SimpleWorkerReplica> ptr)
    : QObject(nullptr),
      m_reptr(ptr),
      m_proc(new QProcess(this)),
      m_shmSend(new QSharedMemory),
      m_shmRecv(new QSharedMemory)
{
    m_shmSend->setKey(QUuid::createUuid().toString(QUuid::Id128));
    connect(ptr.data(), SIGNAL(frameProcessed(const QString&)), this, SLOT(receiveProcessedFrame(const QString&)));
    m_proc->setProcessChannelMode(QProcess::ForwardedChannels);
}

WorkerConnector::~WorkerConnector()
{
    if (m_proc->state() == QProcess::Running) {
        m_proc->terminate();
        m_proc->waitForFinished(10000);
        m_proc->kill();
    }

    //video.release();
}

bool WorkerConnector::connectAndRun()
{
    const auto address = QStringLiteral("local:%1").arg(QUuid::createUuid().toString(QUuid::Id128));
    m_reptr->node()->connectToNode(QUrl(address));

    const auto workerExe = QStringLiteral("%1/worker/qroworker").arg(QCoreApplication::applicationDirPath());
    m_proc->start(workerExe, QStringList() << address);
    if (!m_proc->waitForStarted())
        return false;
    return m_reptr->waitForSource(10000);
}

void WorkerConnector::sendProcessFrame(const MyDataFrame &data)
{
    cvmat_to_shm(m_shmSend, data.frame);

    if (!m_reptr.data()->processFrame(0, m_shmSend->key()).waitForFinished(10000))
        qDebug() << "Frame processing failed!";
}

void WorkerConnector::receiveProcessedFrame(const QString &id)
{
    m_shmRecv->setKey(id);
    auto frame = shm_to_cvmat(m_shmRecv);
    //video.write(frame);
}
