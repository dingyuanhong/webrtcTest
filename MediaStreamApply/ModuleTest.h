#pragma once

#define NOMINMAX
#pragma once
#include "class_api.h"

//������Ƶ�ɼ�
void CLASS_API TestVideoCapture();

//������Ƶ����
void CLASS_API TestAudioDevice();

//������Ƶ�ɼ�
void CLASS_API TestAudioCapture();

//������Ƶ�ػ�����
void CLASS_API TestAudioLoopCapture();

//������Ƶ�ػ�����
void CLASS_API TestAudioLoopBack();

//��ʼ���߳�
void CLASS_API InitThread();
void CLASS_API UninitThread();