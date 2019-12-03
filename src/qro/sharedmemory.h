
#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include <QObject>

class SharedMemory : QObject
{
    Q_OBJECT
public:
    SharedMemory(QObject *parent = nullptr);
    ~SharedMemory();

    void setShmKey(const QString& key);
    QString shmKey() const;

    QString lastError() const;

    size_t size() const;
    void *data();

    bool create(size_t size);
    bool attach(bool writable = false);

    bool isAttached() const;

private:
    void setErrorFromErrno(const QString& hint);
    QString m_shmKey;
    QString m_lastError;

    bool m_attached;
    void *m_data;
    size_t m_dataLen;
};

#endif // SHAREDMEMORY_H
