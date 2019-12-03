
#pragma once

#include <QObject>
#include <QQueue>
#include <QTimer>

#include "rep_interface_source.h"
#include "../../proconhelper.h"
#include "../sharedmemory.h"

class SimpleWorker : public SimpleWorkerSource
{
    Q_OBJECT
public:
    SimpleWorker(QObject *parent = nullptr);
    ~SimpleWorker() override;

    bool processFrame(int style, uint id, long timestamp, const QString &shmKey) override;
    bool ready() const override;

public slots:
    void processAndSendData();

private:
    QTimer *m_timer;
    QQueue<std::pair<int, MyDataFrame>> m_queue;

    std::unique_ptr<SharedMemory> m_shmSend;
    std::unique_ptr<SharedMemory> m_shmRecv;
    std::string m_winName;
};
