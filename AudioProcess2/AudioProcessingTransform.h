#pragma once

#include "webrtc/base/ssladapter.h"
#include "webrtc/base/win32socketinit.h"
#include "webrtc/base/win32socketserver.h"
#include "webrtc/base/physicalsocketserver.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/signalthread.h"
#include "webrtc/base/sigslot.h"
#include "webrtc/base/arraysize.h"
#include "webrtc/transport.h"

#include "webrtc\common_audio\wav_file.h"
#include "webrtc\modules\audio_processing\ns\noise_suppression.h"

#include "class_api.h"
#include "AudioDenoise.h"
#include <Windows.h>
#include "DevicesInfo.h"
#include "AudioSampleBuffer.h"

#define MAX_RECORDING_SIZE 2000

class AudioProcessingTransform
	: public webrtc::AudioTransport
{
public:
	AudioProcessingTransform()
		:
		noiseWriter(NULL)
		, noNoiseWriter(NULL)
		, noiseProcesser_(NULL)
	{
		initNoise_ = false;
		soundCount_ = 0;
	}

	virtual ~AudioProcessingTransform()
	{
		for (size_t i = 0; i < Buffer.size(); i++)
		{
			AudioSampleBuffer * buff = Buffer.front();
			Buffer.pop_front();
			delete buff;
		}

		Buffer.clear();
	}

	void GenerateAudioFrame(const int16_t* audio,
		size_t samples_per_channel,
		size_t num_channels,
		int sample_rate_hz) {

		// We want to process at the lowest rate possible without losing information.
		// Choose the lowest native rate at least equal to the input and codec rates.
		const int min_processing_rate = sample_rate_hz;
		/*for (size_t i = 0; i < webrtc::AudioProcessing::kNumNativeSampleRates; ++i) {
			_audioFrame.sample_rate_hz_ = webrtc::AudioProcessing::kNativeSampleRatesHz[i];
			if (_audioFrame.sample_rate_hz_ >= min_processing_rate) {
				break;
			}
		}*/
		_audioFrame.sample_rate_hz_ = sample_rate_hz;
		_audioFrame.num_channels_ = num_channels;
		webrtc::voe::RemixAndResample(audio, samples_per_channel, num_channels, sample_rate_hz,
			&resampler_, &_audioFrame);
	}

	virtual int32_t RecordedDataIsAvailable(const void* audioSamples,
		const size_t nSamples,
		const size_t nBytesPerSample,
		const size_t nChannels,
		const uint32_t samplesPerSec,
		const uint32_t totalDelayMS,
		const int32_t clockDrift,
		const uint32_t currentMicLevel,
		const bool keyPressed,
		uint32_t& newMicLevel)
	{
		if (!initNoise_) {
			initNoise_ = true;

			noiseProcesser_ = WebRtcNs_Create();
			WebRtcNs_Init(noiseProcesser_, samplesPerSec);
			WebRtcNs_set_policy(noiseProcesser_,3);
		}

		printf("BytesPerSample:%d Channels:%d samplesPerSec:%d Samples:%d\n", nBytesPerSample, nChannels, samplesPerSec, nSamples);
		size_t packetCount = samplesPerSec / nSamples;

		if (Buffer.size() >= packetCount)
		{
			size_t removeCount = (Buffer.size() - packetCount + 1);
			for (size_t i = 0; i < removeCount; i++)
			{
				AudioSampleBuffer * buff = Buffer.front();
				Buffer.pop_front();
				delete buff;
			}
			printf("¶ªÆú:%d\n", removeCount);
		}

		AudioSampleBuffer * buff = NULL;
		bool bProcess = true;
		bool bProcessAPMCore = true;
		bool bProcessSpeexPre = false;
		bool bProcessSpeexAfter = false;

		if (bProcess == true) {
			GenerateAudioFrame((const int16_t*)audioSamples, nSamples, nChannels, samplesPerSec);

			if (bProcessSpeexPre) {
				doenoise.Process((char*)_audioFrame.data_, nBytesPerSample*nSamples, samplesPerSec, nBytesPerSample);
			}

			if (bProcessAPMCore) {
				size_t bands = nSamples / 160;
				for (size_t i = 0; i < bands; i++)
				{
					int16_t * data = (int16_t*)&_audioFrame.data_[i * 160];
					for (size_t j = 0; j < 160; j++)
					{
						process_in_buffer_[j] = (float)data[j];
					}
					float *data_in = process_in_buffer_;
					WebRtcNs_Analyze(noiseProcesser_, data_in);
					const float* const* in = &data_in;
					float * data_addr = (float*)process_buffer_;
					float * const * out = &data_addr;
					WebRtcNs_Process(noiseProcesser_, in,1,out);
					
					for (size_t j = 0; j < 160; j++)
					{
						data[j] = (int16_t)data_addr[j];
					}
				}
			}

			if (bProcessSpeexAfter) {
				doenoise.Process((char*)_audioFrame.data_, nBytesPerSample*nSamples, samplesPerSec, nBytesPerSample);
			}

			bool isMute = false;

			if (isMute) {
				buff = new AudioSampleBuffer(nSamples, nBytesPerSample, nChannels, samplesPerSec);
			}
			else {
				buff = new AudioSampleBuffer(_audioFrame.data_, nSamples, nBytesPerSample, nChannels, samplesPerSec);
			}
		}
		else
		{
			buff = new AudioSampleBuffer(audioSamples, nSamples, nBytesPerSample, nChannels, samplesPerSec);
		}

#ifdef _DEBUG
		if (soundCount_ <= MAX_RECORDING_SIZE) {
			char preName[64];
			DWORD time = GetTickCount();
			sprintf(preName,"./%02d%02d%02d_%03d_", (time / (60 * 60 * 1000)), (time / 60000) % 60, (time / 1000) % 60, time % 1000);
			if (noiseWriter == NULL)
			{
				std::string name = preName;
				name += "noiseFile.wav";
				noiseWriter = new webrtc::WavWriter(name, samplesPerSec, nChannels);
			}
			if (noNoiseWriter == NULL) {
				std::string name = preName;
				name += "noNoiseFile.wav";
				noNoiseWriter = new webrtc::WavWriter(name, samplesPerSec, nChannels);
			}
			noiseWriter->WriteSamples((int16_t*)audioSamples, nSamples);
			noNoiseWriter->WriteSamples((int16_t*)buff->audioSamples_, nSamples);
		}
		else {
			if (noiseWriter != NULL)
			{
				delete noiseWriter;
				noiseWriter = NULL;
			}
			if (noNoiseWriter != NULL) {
				delete noNoiseWriter;
				noNoiseWriter = NULL;
			}
		}
		soundCount_++;
		if (soundCount_ <= MAX_RECORDING_SIZE)
		{
			//return 0;
		}
#endif
		
		Buffer.push_back(buff);

		return 0;
	}

	virtual int32_t NeedMorePlayData(const size_t nSamples,
		const size_t nBytesPerSample,
		const size_t nChannels,
		const uint32_t samplesPerSec,
		void* audioSamples,
		size_t& nSamplesOut,
		int64_t* elapsed_time_ms,
		int64_t* ntp_time_ms)
	{
		printf("need:BytesPerSample:%d Channels:%d samplesPerSec:%d Samples:%d\n", nBytesPerSample, nChannels, samplesPerSec, nSamples);

		if (Buffer.size() == 0) {
			memset(audioSamples, 0, nBytesPerSample * nSamples);
			nSamplesOut = nSamples;
			return 0;
		}

		AudioSampleBuffer * buff = Buffer.front();
		if (buff == NULL)
		{
			memset(audioSamples, 0, nBytesPerSample * nSamples);
			nSamplesOut = nSamples;
			return 0;
		}
		int ret = buff->Transform(nSamples, nBytesPerSample, nChannels, samplesPerSec, audioSamples, nSamplesOut);
		if (ret == -1)
		{
			Buffer.pop_front();
			delete buff;
			return -1;
		}
		else if (ret == 0)
		{
			Buffer.pop_front();
			delete buff;
		}

		return 0;
	}
private:
	int capture_level;
	webrtc::AudioFrame _audioFrame;
	webrtc::PushResampler<int16_t> resampler_;
	std::list<AudioSampleBuffer*> Buffer;
	AudioDenoise doenoise;

	int soundCount_;
	webrtc::WavWriter *noiseWriter;
	webrtc::WavWriter *noNoiseWriter;
	bool initNoise_;
	rtc::CriticalSection crit_capture_;
	std::unique_ptr<webrtc::AudioBuffer> capture_audio_;

	float process_buffer_[480];
	float process_in_buffer_[480];
	NsHandle *noiseProcesser_;
};