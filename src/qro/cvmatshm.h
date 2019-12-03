#pragma once

#include <vector>
#include <string>
#include <opencv2/core.hpp>
#include "sharedmemory.h"
#include "../proconhelper.h"

void cvmat_to_shm(std::unique_ptr<SharedMemory> &shm, const cv::Mat &frame);
cv::Mat shm_to_cvmat(std::unique_ptr<SharedMemory> &shm);
