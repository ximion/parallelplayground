#include "proconhelper.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <numeric>
#include <math.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

cv::Mat process_data_instant(const MyDataFrame &data)
{
    return data.frame;
}

cv::Mat process_data_fast(const MyDataFrame &data)
{
    auto result = data.frame.clone();

    //cv::GaussianBlur(result, result, cv::Size(11, 11), 0, 0);
    //cv::medianBlur(result, result, 5);
    //cv::GaussianBlur(result, result, cv::Size(3, 3), 0, 0);
    cv::blur(result, result, cv::Size(5, 5));

    return result;
}

cv::Mat process_data_slow(const MyDataFrame &data)
{
    for (uint i = 0; i < 4; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return process_data_fast(data);
}

MyDataFrame transform_data_fast(const MyDataFrame &data, size_t id)
{
    MyDataFrame newData;

    newData.frame = process_data_fast(data);
    cv::putText(newData.frame,
                std::string("E ") + std::to_string(id),
                cv::Point(24, 320),
                cv::FONT_HERSHEY_COMPLEX,
                1.5,
                cv::Scalar(140, 140, 255));
    newData.id = id;

    return newData;
}

MyDataFrame create_data_200Hz(size_t index)
{
    MyDataFrame data;
    data.id = index;

    cv::Mat frame(cv::Size(800, 600), CV_8UC3);
    frame.setTo(cv::Scalar(67, 42, 30));
    cv::putText(frame,
                std::string("Frame ") + std::to_string(index),
                cv::Point(24, 240),
                cv::FONT_HERSHEY_COMPLEX,
                1.5,
                cv::Scalar(255,255,255));
    data.frame = frame;

    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    return data;
}

void display_frame(const cv::Mat &frame, const std::string &winName)
{
    cv::imshow(winName, frame);
    cv::waitKey(1);
}

void run_timed(const std::string &name, std::function<void ()> func, int n_times)
{
    std::vector<double> timings;

    std::cout << "Running " << name << " with " << n_times << " trials using " << N_OF_DATAFRAMES << " produced elements each." << std::endl;

    for (int i = 1; i <= n_times; i++) {
        std::cout << "Trial " << i << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = finish - start;
        timings.push_back(elapsed.count() * 1000);
    }

    double sum = std::accumulate(timings.begin(), timings.end(), 0.0);
    double mean = sum / timings.size();

    std::vector<double> diff(timings.size());
    std::transform(timings.begin(), timings.end(), diff.begin(), [mean](double x) { return x - mean; });
    double sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
    double stdev = std::sqrt(sq_sum / timings.size());

    std::cout << "Mean time for " << name << ": " << mean << "msec (" << timings.size() << " runs)" << " Standard deviation: " << stdev << std::endl;
}

static void error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

static void error(const std::string msg)
{
    error(msg.c_str());
}

int mydata_frame_to_memfd(const MyDataFrame &data)
{
    int fd, ret;

    const auto frame = data.frame;

    fd = memfd_create("frame", MFD_ALLOW_SEALING);
    if (fd == -1)
        error("memfd_create()");

    int mat_type = frame.type();
    int mat_channels = frame.channels();
    ssize_t memsize = sizeof(int) * 6;
    if (frame.isContinuous())
        memsize += frame.dataend - frame.datastart;
    else
        memsize += CV_ELEM_SIZE(mat_type) * frame.cols * frame.rows;

    ret = ftruncate(fd, memsize);
    if (ret == -1)
        error("ftruncate()");

    // seal shrinking
    ret = fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK);
    if (ret == -1)
        error("fcntl(F_SEAL_SHRINK)");

    // write header
    write(fd, &mat_type, sizeof(int));
    write(fd, &mat_channels, sizeof(int));
    write(fd, &frame.rows, sizeof(int));
    write(fd, &frame.cols, sizeof(int));

    std::cout << "Matrix with: " << frame.cols << "x" << frame.rows << " type: " << mat_type << std::endl;

    // write image data
    if (frame.isContinuous()) {
        write(fd, frame.ptr<char>(0), (frame.dataend - frame.datastart));
    }
    else {
        size_t rowsz = static_cast<size_t>(CV_ELEM_SIZE(mat_type) * frame.cols);
        for (int r = 0; r < frame.rows; ++r) {
            write(fd, frame.ptr<char>(r), rowsz);
        }
    }

    // seal writes
    ret = fcntl(fd, F_ADD_SEALS, F_SEAL_WRITE);
    if (ret == -1)
        error("fcntl(F_SEAL_WRITE)");

    // seal setting more seals
    ret = fcntl(fd, F_ADD_SEALS, F_SEAL_SEAL);
    if (ret == -1)
        error("fcntl(F_SEAL_SEAL)");

    return fd;
}

cv::Mat memfd_to_cvmat(int fd)
{
    // read header
    int rows, cols, mat_type, mat_channels;
    if (read(fd, &mat_type, sizeof(int)) < 0) error(std::strerror(errno));
    if (read(fd, &mat_channels, sizeof(int)) < 0) error(std::strerror(errno));
    read(fd, &rows, sizeof(int));
    read(fd, &cols, sizeof(int));

    // read data
    std::cout << "Matrix with: " << cols << "x" << rows << " type: " << mat_type << std::endl;
    cv::Mat mat(rows, cols, mat_type);
    read(fd, mat.data, CV_ELEM_SIZE(mat_type) * rows * cols);

    return mat;
}
