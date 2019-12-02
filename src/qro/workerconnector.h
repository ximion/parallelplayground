
#pragma once

#include <QObject>
#include <QSharedPointer>
#include <QSharedMemory>
#include <memory>

#include "../proconhelper.h"
#include "qrostream.h"

#include "rep_interface_replica.h"

class WorkerConnector : public QObject
{
    Q_OBJECT
public:
    WorkerConnector(QSharedPointer<SimpleWorkerReplica> ptr);
    ~WorkerConnector() override;

    bool connectAndRun(QROStream<MyDataFrame> *prodStream);

    void sendProcessFrame(int style, const MyDataFrame &data);

    cv::Mat lastFrame() const;

public slots:
    void receiveProcessedFrame(uint id, long timestamp, const QString &shmKey);

private:
    QSharedPointer<SimpleWorkerReplica> m_reptr;
    QProcess *m_proc;
    std::unique_ptr<QSharedMemory> m_shmSend;
    std::unique_ptr<QSharedMemory> m_shmRecv;
    QROStream<MyDataFrame> *m_prodStream;
    cv::Mat m_lastFrame;
};
