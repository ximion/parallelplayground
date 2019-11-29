#include "proconhelper.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <numeric>
#include <math.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

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

    for (int i = 1; i <= N_OF_RUNS; i++) {
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
