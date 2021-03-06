/*
 * Copyright 2019 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _XF_GAUSSIAN_FILTER_CONFIG_H_
#define _XF_GAUSSIAN_FILTER_CONFIG_H_

#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_gaussian_filter.hpp"
#include "imgproc/xf_duplicateimage.hpp"
#include "imgproc/xf_delay.hpp"
#include "core/xf_arithm.hpp"
#include "xf_config_params.h"

typedef unsigned short int uint16_t;

#define WIDTH 128
#define HEIGHT 128

#define MAXDELAY 15360

#if FILTER_SIZE_3
#define FILTER_WIDTH 3
#define FILTER 3
#elif FILTER_SIZE_5
#define FILTER_WIDTH 5
#define FILTER 5
#elif FILTER_SIZE_7
#define FILTER_WIDTH 7
#define FILTER 7
#endif

#if NO
#define NPC1 XF_NPPC1
#endif
#if RO
#define NPC1 XF_NPPC8
#endif

void gaussian_diff_accel(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                         xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgin1,
                         xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgin2,
                         xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgin3,
                         xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgin4,
                         xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgin5,
                         xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput,
                         float sigma);

#endif //_XF_GAUSSIAN_FILTER_CONFIG_H_
