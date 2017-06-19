#include "stdafx.h"
#include "AudioDenoise.h"
#include <stdint.h>

#pragma comment(lib,"libspeex.lib")
#pragma comment(lib,"libspeexdsp.lib")

#define DENOISE_DB (-90)

AudioDenoise::AudioDenoise()
	:ST(NULL), SampleRate(0), FrameSize(0)
{
}

AudioDenoise::~AudioDenoise()
{
	Uninit();
}

int AudioDenoise::Init(int frameSize,int sampleRate)
{
	ST = speex_preprocess_state_init(frameSize, sampleRate);
	int i = 1;
	speex_preprocess_ctl(ST, SPEEX_PREPROCESS_SET_DENOISE, &i); //降噪开关
	int noiseSuppress = DENOISE_DB;
	speex_preprocess_ctl(ST, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &noiseSuppress); //设置噪音抑制点
	i = 0;
	speex_preprocess_ctl(ST, SPEEX_PREPROCESS_SET_AGC, &i);//增益开关
	i = 8000;
	speex_preprocess_ctl(ST, SPEEX_PREPROCESS_SET_AGC_LEVEL, &i);//设置增益的dB 
	i = 0;
	speex_preprocess_ctl(ST, SPEEX_PREPROCESS_SET_DEREVERB, &i);//混音
	float f = .0;
	speex_preprocess_ctl(ST, SPEEX_PREPROCESS_SET_DEREVERB_DECAY, &f);
	f = .0;
	speex_preprocess_ctl(ST, SPEEX_PREPROCESS_SET_DEREVERB_LEVEL, &f);

	//int vad = 1, vadProbStart = 80, vadProbContinue = 65;
	//speex_preprocess_ctl(ST, SPEEX_PREPROCESS_SET_VAD, &vad); //静音检测  
	////Set probability required for the VAD to go from silence to voice  
	//speex_preprocess_ctl(ST, SPEEX_PREPROCESS_SET_PROB_START, &vadProbStart); 
	////Set probability required for the VAD to stay i
	//speex_preprocess_ctl(ST, SPEEX_PREPROCESS_SET_PROB_CONTINUE, &vadProbContinue); 

	SampleRate = sampleRate;
	FrameSize = frameSize;

	return 0;
}

void AudioDenoise::Uninit()
{
	if (ST != NULL)
	{
		speex_preprocess_state_destroy(ST);
		ST = NULL;
	}
}

int AudioDenoise::Process(char *in,int size,int sampleRate, int sampleByte)
{
	CheckPreprocessState(size/ sampleByte,sampleRate);

	if (ST == NULL)
	{
		return -1;
	}

	int vad = speex_preprocess_run(ST, (spx_int16_t*)in);

	return vad;
}

int AudioDenoise::CheckPreprocessState(int frameSize, int sampleRate)
{
	if (SampleRate != sampleRate || FrameSize != frameSize || ST == NULL)
	{
		Uninit();
		Init(frameSize,sampleRate);
	}
	return 0;
}