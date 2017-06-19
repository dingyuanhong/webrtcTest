#ifndef AUDIODENOISE_H
#define AUDIODENOISE_H

#include "speex\speex_echo.h"
#include "speex\speex_preprocess.h"

class AudioDenoise
{
public:
	AudioDenoise();
	~AudioDenoise();
	int Process(char *in, int size, int sampleRate,int sampleByte);
private:
	int Init(int frameSize, int sampleRate);
	void Uninit();
	int CheckPreprocessState(int frameSize, int sampleRate);
private:
	SpeexPreprocessState *ST;
	int SampleRate;
	int FrameSize;
};

#endif