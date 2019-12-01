
#pragma once

#include <QObject>
#include <QQueue>
#include <QTimer>

#include "rep_interface_source.h"
#include "../../proconhelper.h"

class SimpleWorker : public SimpleWorkerSource
{
    Q_OBJECT
public:
    SimpleWorker(QObject *parent = nullptr);
    ~SimpleWorker() override;

    bool processFrame(int style, const QString &shmKey) override;
    bool ready() const override;

public slots:
    void processAndSendData();

private:
    QTimer *m_timer;
    QQueue<cv::Mat> m_queue;

    std::unique_ptr<QSharedMemory> m_shmSend;
    std::unique_ptr<QSharedMemory> m_shmRecv;
};
