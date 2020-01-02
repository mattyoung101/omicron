#pragma once
#ifndef SKIP_INCLUDES
#include <vector>
#include <memory>
#include <iosfwd>
#include <iostream>
#include <opencv2/core/cuda_stream_accessor.hpp>
#endif

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/core/cuda.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/cudafilters.hpp"
#include "opencv2/cudaimgproc.hpp"
#include "opencv2/cudaarithm.hpp"

// Source for this: https://github.com/opencv/opencv/issues/6295#issuecomment-321727869

void inRange_gpu(cv::cuda::GpuMat &src, cv::Scalar &lowerb, cv::Scalar &upperb,
                 cv::cuda::GpuMat &dst, cv::cuda::Stream &stream);