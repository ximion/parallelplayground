
#pragma once

#include <QObject>
#include <QSharedPointer>
#include <QSharedMemory>
#include <memory>

#include "../proconhelper.h"

#include "rep_interface_replica.h"

class WorkerConnector : public QObject
{
    Q_OBJECT
public:
    WorkerConnector(QSharedPointer<SimpleWorkerReplica> ptr);
    ~WorkerConnector() override;

    bool connectAndRun();

    void sendProcessFrame(const MyDataFrame &data);

public slots:
    void receiveProcessedFrame(const QString &id);

private:
    QSharedPointer<SimpleWorkerReplica> m_reptr;
    QProcess *m_proc;
    std::unique_ptr<QSharedMemory> m_shmSend;
    std::unique_ptr<QSharedMemory> m_shmRecv;
 };
