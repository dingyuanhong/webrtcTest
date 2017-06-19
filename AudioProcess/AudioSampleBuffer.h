#pragma once

#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/signalthread.h"
#include "webrtc/base/sigslot.h"
#include "webrtc/base/arraysize.h"

class AudioSampleBuffer
{
public:
	AudioSampleBuffer(const size_t nSamples,
		const size_t nBytesPerSample,
		const size_t nChannels,
		const uint32_t samplesPerSec
		)
	{
		audioSamplesAvailable_ = audioSamples_ = malloc(nSamples*nBytesPerSample);
		if (audioSamples_ == NULL)
		{
			printf("malloc false.\n");
		}
		memset(audioSamples_, 0, nSamples*nBytesPerSample);
		nSamplesAvailable_ = nSamples_ = nSamples;
		nBytesPerSample_ = nBytesPerSample;
		nChannels_ = nChannels;
		samplesPerSec_ = samplesPerSec;
	}

	AudioSampleBuffer(const void* audioSamples,
		const size_t nSamples,
		const size_t nBytesPerSample,
		const size_t nChannels,
		const uint32_t samplesPerSec
		)
	{
		audioSamplesAvailable_ = audioSamples_ = malloc(nSamples*nBytesPerSample);
		if (audioSamples_ == NULL)
		{
			printf("malloc false.\n");
		}

		nSamplesAvailable_ = nSamples_ = nSamples;
		nBytesPerSample_ = nBytesPerSample;
		nChannels_ = nChannels;
		samplesPerSec_ = samplesPerSec;

		memcpy(audioSamples_, audioSamples, nSamples*nBytesPerSample);
	}

	~AudioSampleBuffer()
	{
		if (audioSamples_ != NULL)
		{
			free(audioSamples_);
		}
		audioSamplesAvailable_ = audioSamples_ = NULL;
		nSamplesAvailable_ = nSamples_ = 0;
	}

	int Transform(const size_t nSamples,
		const size_t nBytesPerSample,
		const size_t nChannels,
		const uint32_t samplesPerSec,
		void* audioSamples,
		size_t& nSamplesOut)
	{
		if (nSamplesAvailable_ == 0) return -1;

		if ((samplesPerSec == samplesPerSec_) && (nBytesPerSample / nChannels == nBytesPerSample_ / nChannels_))
		{
			size_t copySamples = std::min(nSamples, nSamplesAvailable_);
			if (copySamples == 0)
			{
				return 0;
			}
			if (nBytesPerSample > nBytesPerSample_)
			{
				size_t perChannelBytes = nBytesPerSample_ / nChannels_;
				size_t channels = nBytesPerSample / nBytesPerSample_;

				for (size_t i = 0; i < copySamples; i++)
				{

					for (size_t j = 0; j < channels; j++)
					{
						memcpy(((char*)audioSamples + j), (char*)audioSamplesAvailable_, perChannelBytes);
					}
					audioSamplesAvailable_ = (char*)audioSamplesAvailable_ + nBytesPerSample_;
					audioSamples = (char*)audioSamples + nBytesPerSample;
				}

				nSamplesOut = copySamples;
				nSamplesAvailable_ -= copySamples;

				if (nSamplesAvailable_ > 0)
				{
					return 1;
				}
			}
			else if (nBytesPerSample == nBytesPerSample_)
			{
				memcpy(audioSamples, audioSamplesAvailable_, copySamples*nBytesPerSample_);
				nSamplesOut = copySamples;
				nSamplesAvailable_ -= copySamples;

				if (nSamplesAvailable_ > 0)
				{
					return 1;
				}
			}
			else if (nBytesPerSample < nBytesPerSample_)
			{
				for (size_t i = 0; i < copySamples; i++)
				{
					memcpy((char*)audioSamples, audioSamplesAvailable_, nBytesPerSample);
					audioSamplesAvailable_ = (char*)audioSamplesAvailable_ + nBytesPerSample_;
					audioSamples = (char*)audioSamples + nBytesPerSample;
				}

				nSamplesOut = copySamples;
				nSamplesAvailable_ -= copySamples;

				if (nSamplesAvailable_ > 0)
				{
					return 1;
				}
			}
		}
		else
		{
			return -1;
		}

		return 0;
	}
public:
	void* audioSamples_;			//音频样本
	size_t nSamples_;				//样本数
	void* audioSamplesAvailable_;	//可用样本
	size_t nSamplesAvailable_;		//可用样本数
	size_t nBytesPerSample_;		//样本字节数
	size_t nChannels_;				//通道数
	uint32_t samplesPerSec_;		//每秒样本数
};
