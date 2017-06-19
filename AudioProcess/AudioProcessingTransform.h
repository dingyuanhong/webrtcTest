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
#include "webrtc/api/peerconnection.h"

#include "webrtc\voice_engine\include\voe_base.h"
#include "webrtc\voice_engine\voice_engine_impl.h"
#include "webrtc\voice_engine\utility.h"
#include "webrtc\voice_engine\include\voe_video_sync.h"
#include "webrtc\common_audio\wav_file.h"
#include "webrtc\modules\audio_device\audio_device_impl.h"
#include "webrtc\modules\audio_processing\include\audio_processing.h"
#include "webrtc\modules\audio_processing\noise_suppression_impl.h"
#include "webrtc\modules\audio_processing\audio_buffer.h"

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
		, noNoiseWriter(NULL),
		noiseSuppression_(&crit_capture_)
	{
		webrtc::Config config;
		webrtc::Intelligibility *intelligibility = new webrtc::Intelligibility;
		intelligibility->enabled = true;
		config.Set(intelligibility);
		apm.reset(webrtc::AudioProcessing::Create());
		apm->set_output_will_be_muted(true);
		apm->level_estimator()->Enable(false);//启用重试次数估计组件
		apm->echo_cancellation()->Enable(false);//启用回声消除组件
		apm->echo_cancellation()->enable_metrics(false);//
		apm->echo_cancellation()->enable_drift_compensation(false);//启用时钟补偿模块（声音捕捉设备的时钟频率和播放设备的时钟频率可能不一样）
		apm->gain_control()->Enable(false);//启用增益控制组件，client必须启用哦！
		apm->high_pass_filter()->Enable(true);//高通过滤器组件，过滤DC偏移和低频噪音，client必须启用
		apm->voice_detection()->Enable(false);//启用语音检测组件，检测是否有说话声
		apm->voice_detection()->set_likelihood(webrtc::VoiceDetection::kVeryLowLikelihood);//设置语音检测的阀值，阀值越大，语音越不容易被忽略，同样一些噪音可能被当成语音。
		apm->noise_suppression()->Enable(true);//噪声抑制组件，client必须启用
		apm->Initialize();//保留所有用户设置的情况下重新初始化apm的内部状态，用于开始处理一个新的音频流。第一个流创建之后不一定需要调用此方法。

		apm->noise_suppression()->set_level(webrtc::NoiseSuppression::kVeryHigh);

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
		for (size_t i = 0; i < webrtc::AudioProcessing::kNumNativeSampleRates; ++i) {
			_audioFrame.sample_rate_hz_ = webrtc::AudioProcessing::kNativeSampleRatesHz[i];
			if (_audioFrame.sample_rate_hz_ >= min_processing_rate) {
				break;
			}
		}
		_audioFrame.num_channels_ = num_channels;
		webrtc::voe::RemixAndResample(audio, samples_per_channel, num_channels, sample_rate_hz,
			&resampler_, &_audioFrame);
	}

	void GenerateAudioConfig(size_t nSamples,
		size_t nChannels,
		int samplesPerSec)
	{
		//webrtc::StreamConfig input_config, output_config;
		//input_config.set_has_keyboard(false);
		//input_config.set_num_channels(nChannels);
		////input_config. = nSamples;
		//input_config.set_sample_rate_hz(samplesPerSec);
		//output_config = input_config;
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
			noiseSuppression_.Initialize(nChannels, samplesPerSec);
			noiseSuppression_.set_level(webrtc::NoiseSuppression::kVeryHigh);
			noiseSuppression_.Enable(true);
			initNoise_ = true;
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
			printf("丢弃:%d\n", removeCount);
		}

		AudioSampleBuffer * buff = NULL;
		bool bProcess = true;
		bool bProcessAPM = true;
		bool bProcessSpeex = true;
		if (bProcess == true) {
			GenerateAudioFrame((const int16_t*)audioSamples, nSamples, nChannels, samplesPerSec);

			if (bProcessSpeex) {
				doenoise.Process((char*)_audioFrame.data_, nBytesPerSample*nSamples, samplesPerSec, nBytesPerSample);
			}

			if (bProcessAPM) {
				/*if (capture_audio_.get() == NULL) {
				capture_audio_.reset(
				new webrtc::AudioBuffer(nSamples,
				nChannels,
				nSamples,
				nChannels,
				nSamples));
				}
				capture_audio_->DeinterleaveFrom(&_audioFrame);
				noiseSuppression_.AnalyzeCaptureAudio(capture_audio_.get());
				noiseSuppression_.ProcessCaptureAudio(capture_audio_.get());
				capture_audio_->InterleaveTo(&_audioFrame, true);*/

				int err = apm->ProcessStream(&_audioFrame);
				if (err != 0)
				{
					printf("ProcessStream Error:%d \n", err);
					return 0;
				}
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
			if (noiseWriter == NULL)
			{
				std::string name = "./noiseFile.wav";
				noiseWriter = new webrtc::WavWriter(name, samplesPerSec, nChannels);
			}
			if (noNoiseWriter == NULL) {
				std::string name = "./noNoiseFile.wav";
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
	std::unique_ptr<webrtc::AudioProcessing> apm;
	int capture_level;
	webrtc::AudioFrame _audioFrame;
	webrtc::PushResampler<int16_t> resampler_;
	std::list<AudioSampleBuffer*> Buffer;
	AudioDenoise doenoise;

	int soundCount_;
	webrtc::WavWriter *noiseWriter;
	webrtc::WavWriter *noNoiseWriter;
	bool initNoise_;
	webrtc::NoiseSuppressionImpl noiseSuppression_;
	rtc::CriticalSection crit_capture_;
	std::unique_ptr<webrtc::AudioBuffer> capture_audio_;

};