#pragma once

#define NOMINMAX
#pragma once
#include "class_api.h"

//测试视频采集
void CLASS_API TestVideoCapture();

//测试音频播放
void CLASS_API TestAudioDevice();

//测试音频采集
void CLASS_API TestAudioCapture();

//测试音频回环播放
void CLASS_API TestAudioLoopCapture();

//测试音频回环播放
void CLASS_API TestAudioLoopBack();

//初始化线程
void CLASS_API InitThread();
void CLASS_API UninitThread();