
#include "cvmatshm.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <numeric>
#include <math.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <QUuid>

static void error(const QString& msg)
{
    perror(qPrintable(msg));
    exit(EXIT_FAILURE);
}

void cvmat_to_shm(std::unique_ptr<QSharedMemory> &shm, const cv::Mat &frame)
{
    int mat_type = frame.type();
    int mat_channels = frame.channels();
    ssize_t memsize = sizeof(int) * 6;
    if (frame.isContinuous())
        memsize += frame.dataend - frame.datastart;
    else
        memsize += CV_ELEM_SIZE(mat_type) * frame.cols * frame.rows;

    if (shm->size() == 0) {
        // this is a fresh shared-memory object, so create it
        if (!shm->create(memsize))
            error(shm->errorString());
    } else {
        if (shm->size() != memsize) {
            // the memory segment doesn't have the right size, let's create a new one!
            shm.reset(new QSharedMemory);
            shm->setKey(QUuid::createUuid().toString(QUuid::Id128));
            if (!shm->create(memsize))
                error(shm->errorString());
        }
    }

    shm->lock();
    auto shm_data = static_cast<char*>(shm->data());

    // write header
    size_t pos = 0;
    memcpy(shm_data + pos, &mat_type, sizeof(int));
    pos += sizeof(int);
    memcpy(shm_data + pos, &mat_channels, sizeof(int));
    pos += sizeof(int);
    memcpy(shm_data + pos, &frame.rows, sizeof(int));
    pos += sizeof(int);
    memcpy(shm_data + pos, &frame.cols, sizeof(int));
    pos += sizeof(int);

    // write image data
    if (frame.isContinuous()) {
        memcpy(shm_data + pos, frame.ptr<char>(0), (frame.dataend - frame.datastart));
    }
    else {
        size_t rowsz = static_cast<size_t>(CV_ELEM_SIZE(mat_type) * frame.cols);
        for (int r = 0; r < frame.rows; ++r) {
            memcpy(shm_data + pos, frame.ptr<char>(r), rowsz);
        }
    }

    shm->unlock();
}

cv::Mat shm_to_cvmat(std::unique_ptr<QSharedMemory> &shm)
{
    if (!shm->isAttached() && !shm->attach(QSharedMemory::ReadOnly))
        error(shm->errorString());

    shm->lock();
    auto shm_data = static_cast<const char*>(shm->constData());
    size_t pos = 0;

    // read header
    int rows, cols, mat_type, mat_channels;
    memcpy(&mat_type, shm_data + pos, sizeof(int));
    pos += sizeof(int);
    memcpy(&mat_channels, shm_data + pos, sizeof(int));
    pos += sizeof(int);
    memcpy(&rows, shm_data + pos, sizeof(int));
    pos += sizeof(int);
    memcpy(&cols, shm_data + pos, sizeof(int));
    pos += sizeof(int);

    // read data
    cv::Mat mat(rows, cols, mat_type);
    memcpy(mat.data, shm_data + pos, shm->size() - pos);
    shm->unlock();

    return mat;
}
