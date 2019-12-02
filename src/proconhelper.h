#pragma once

#include <vector>
#include <string>
#include <opencv2/core.hpp>

static const int N_OF_DATAFRAMES = 2000;
static const int N_OF_RUNS = 4;

typedef struct
{
    size_t id;
    time_t timestamp;
    cv::Mat frame;
} MyDataFrame;

cv::Mat process_data_instant(const MyDataFrame &data);
cv::Mat process_data_fast(const MyDataFrame &data);
cv::Mat process_data_slow(const MyDataFrame &data);

MyDataFrame transform_data_fast(const MyDataFrame &data, size_t id);

MyDataFrame create_data_200Hz (size_t index);

void display_frame(const cv::Mat& frame, const std::string& winName = "TestWindow");

void run_timed(const std::string &name, std::function<double()> func, int n_times);

int mydata_frame_to_memfd(const MyDataFrame &data);
cv::Mat memfd_to_cvmat(int fd);
