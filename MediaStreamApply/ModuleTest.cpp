#include "stdafx.h"
#include "ModuleTest.h"

#include "webrtc\modules\video_capture\video_capture_factory.h"
#include "webrtc/base/ssladapter.h"
#include "webrtc/base/win32socketinit.h"
#include "webrtc/base/win32socketserver.h"
#include "webrtc/base/physicalsocketserver.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/signalthread.h"
#include "webrtc/base/sigslot.h"
#include "webrtc/api/peerconnection.h"
#include "webrtc/modules/video_capture/video_capture_factory.h"
#include "webrtc/modules/video_capture/video_capture.h"
#include "webrtc/modules/video_capture/video_capture_defines.h"
#include "webrtc/media/base/videocommon.h"
#include "webrtc/base/arraysize.h"
#include <Windows.h>
#include <Objbase.h>

struct kVideoFourCCEntry {
	uint32_t fourcc;
	webrtc::RawVideoType webrtc_type;
};

// This indicates our format preferences and defines a mapping between
// webrtc::RawVideoType (from video_capture_defines.h) to our FOURCCs.
static kVideoFourCCEntry kSupportedFourCCs[] = {
	{ cricket::FOURCC_I420, webrtc::kVideoI420 },   // 12 bpp, no conversion.
	{ cricket::FOURCC_YV12, webrtc::kVideoYV12 },   // 12 bpp, no conversion.
	{ cricket::FOURCC_YUY2, webrtc::kVideoYUY2 },   // 16 bpp, fast conversion.
	{ cricket::FOURCC_UYVY, webrtc::kVideoUYVY },   // 16 bpp, fast conversion.
	{ cricket::FOURCC_NV12, webrtc::kVideoNV12 },   // 12 bpp, fast conversion.
	{ cricket::FOURCC_NV21, webrtc::kVideoNV21 },   // 12 bpp, fast conversion.
	{ cricket::FOURCC_MJPG, webrtc::kVideoMJPEG },  // compressed, slow conversion.
	{ cricket::FOURCC_ARGB, webrtc::kVideoARGB },   // 32 bpp, slow conversion.
	{ cricket::FOURCC_24BG, webrtc::kVideoRGB24 },  // 24 bpp, slow conversion.
};

static bool CapabilityToFormat(const webrtc::VideoCaptureCapability& cap,
	cricket::VideoFormat* format) {
	uint32_t fourcc = 0;
	for (size_t i = 0; i < arraysize(kSupportedFourCCs); ++i) {
		if (kSupportedFourCCs[i].webrtc_type == cap.rawType) {
			fourcc = kSupportedFourCCs[i].fourcc;
			break;
		}
	}
	if (fourcc == 0) {
		return false;
	}

	format->fourcc = fourcc;
	format->width = cap.width;
	format->height = cap.height;
	format->interval = cricket::VideoFormat::FpsToInterval(cap.maxFPS);
	return true;
}

static bool FormatToCapability(const cricket::VideoFormat& format,
	webrtc::VideoCaptureCapability* cap) {
	webrtc::RawVideoType webrtc_type = webrtc::kVideoUnknown;
	for (size_t i = 0; i < arraysize(kSupportedFourCCs); ++i) {
		if (kSupportedFourCCs[i].fourcc == format.fourcc) {
			webrtc_type = kSupportedFourCCs[i].webrtc_type;
			break;
		}
	}
	if (webrtc_type == webrtc::kVideoUnknown) {
		return false;
	}

	cap->width = format.width;
	cap->height = format.height;
	cap->maxFPS = cricket::VideoFormat::IntervalToFps(format.interval);
	cap->expectedCaptureDelay = 0;
	cap->rawType = webrtc_type;
	cap->codecType = webrtc::kVideoCodecUnknown;
	cap->interlaced = false;
	return true;
}

/*
MVideoCaptureDataCallback
*/
class MVideoCaptureDataCallback
	: public webrtc::VideoCaptureDataCallback
{
public:
	void OnIncomingCapturedFrame(const int32_t id,
		const webrtc::VideoFrame& videoFrame) {
		if (ft.QuadPart == 0) {
			if (!QueryPerformanceCounter(&ft))
			{
				assert(FALSE);
			}
			last_tick_ = GetTickCount();
		}
		else {
			LARGE_INTEGER fm;
			if (!QueryPerformanceCounter(&fm))
			{
				assert(FALSE);
			}
			DWORD tm = GetTickCount();
			printf("毫秒:%d %I64d\n", tm-last_tick_ , (fm.QuadPart - ft.QuadPart)*1000*1000/ fc.QuadPart);
			ft = fm;
			last_tick_ = tm;
		}
		return;
	}

	void OnCaptureDelayChanged(const int32_t id,
		const int32_t delay)  {
		return;
	}

	MVideoCaptureDataCallback()
	{
		last_tick_ = 0;
		if (!QueryPerformanceFrequency(&fc)) {
		}
		ft.QuadPart = 0;
	}

	~MVideoCaptureDataCallback() {
		
	}
private:
	DWORD last_tick_;
	LARGE_INTEGER fc;
	LARGE_INTEGER ft;
};


void CLASS_API TestVideoCapture()
{
	CoInitialize(NULL);
	rtc::EnsureWinsockInit();
	rtc::Win32Thread w32_thread;
	rtc::ThreadManager::Instance()->SetCurrentThread(&w32_thread);
	
	rtc::InitializeSSL();
	
	char * Arr_device_names[] = {
		"Insta360 Virtual Camera",
		"Virtual Camera (By evomotion)"
	};

	std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
		webrtc::VideoCaptureFactory::CreateDeviceInfo(0));
	if (!info) {
		return;
	}

	int test_index = 1;

	int index = -1;
	std::vector<std::string> device_names;
	std::vector<cricket::VideoFormat> supported;
	const uint32_t kSize = 256;
	char name[kSize] = { 0 };
	char id[kSize] = { 0 };

	int num_devices = info->NumberOfDevices();
	for (int i = 0; i < num_devices; ++i) {
		
		
		if (info->GetDeviceName(i, name, kSize, id, kSize) != -1) {
			device_names.push_back(name);
			if (strcmp(name, Arr_device_names[test_index]) == 0) {
				index = i;

				int32_t num_caps = info->NumberOfCapabilities(id);
				for (int32_t i = 0; i < num_caps; ++i) {
					webrtc::VideoCaptureCapability cap;
					if (info->GetCapability(id, i, cap) != -1) {
						cricket::VideoFormat format;
						if (CapabilityToFormat(cap, &format)) {
							supported.push_back(format);
						}
						else {
							LOG(LS_WARNING) << "Ignoring unsupported WebRTC capture format "
								<< cap.rawType;
						}
					}
				}

				break;
			}
		}
	}
	info.reset();

	rtc::scoped_refptr<webrtc::VideoCaptureModule> capturer;
	capturer = webrtc::VideoCaptureFactory::Create(index, id);
	if (capturer.get() == NULL) return;


	webrtc::VideoCaptureCapability cap;
	if (!FormatToCapability(supported[0], &cap)) {
		LOG(LS_ERROR) << "Invalid capture format specified";
		return;
	}
	MVideoCaptureDataCallback *datacallback = new MVideoCaptureDataCallback();
	uint32_t start = rtc::Time();
	capturer->RegisterCaptureDataCallback(*datacallback);
	if (capturer->StartCapture(cap) != 0) {
		LOG(LS_ERROR) << "Camera '" << 0 << "' failed to start";
		capturer->DeRegisterCaptureDataCallback();
		return;
	}


	w32_thread.Run();
	capturer.release();
	delete datacallback;

	rtc::CleanupSSL();
}

#include "webrtc\voice_engine\include\voe_base.h"
#include "webrtc\voice_engine\voice_engine_impl.h"
#include "webrtc\modules\audio_device\audio_device_impl.h"
#include "webrtc\modules\audio_processing\include\audio_processing.h"
#include "CodeTransport.h"

void printDevices(webrtc::AudioDeviceModule* device)
{
	if (device == NULL) return;

	int16_t playerCount = device->PlayoutDevices();
	printf("播放设备数:%d\n", playerCount);
	for (int16_t i = 0; i < playerCount; i++)
	{
		char name[webrtc::kAdmMaxDeviceNameSize] = { 0 };
		char guid[webrtc::kAdmMaxGuidSize] = { 0 };

		device->PlayoutDeviceName(i, name, guid);
		std::string utf8 = name;
		std::string strName = UTF82ASCII(utf8);
		printf("播放设备：%d %s , %s\n", i, strName.c_str(), guid);
	}

	int16_t recordCount = device->RecordingDevices();
	printf("采集设备数:%d\n", recordCount);
	for (int16_t i = 0; i < recordCount; i++)
	{
		char name[webrtc::kAdmMaxDeviceNameSize] = { 0 };
		char guid[webrtc::kAdmMaxGuidSize] = { 0 };
		device->RecordingDeviceName(i, name, guid);
		std::string utf8 = name;
		std::string strName = UTF82ASCII(utf8);
		printf("采集设备：%d %s , %s\n", i, strName.c_str(), guid);
	}
}

void CLASS_API TestAudioDevice()
{
	CoInitialize(NULL);
	rtc::EnsureWinsockInit();
	rtc::Win32Thread w32_thread;
	rtc::ThreadManager::Instance()->SetCurrentThread(&w32_thread);

	rtc::InitializeSSL();

	rtc::scoped_refptr<webrtc::AudioDeviceModule> audioDevice = webrtc::AudioDeviceModuleImpl::Create(0);

	if (!audioDevice.get()) {
		return;
	}

	audioDevice->Init();

	printDevices(audioDevice.get());

	w32_thread.Run();

	audioDevice->Terminate();

	rtc::CleanupSSL();
}

class MAudioBuffer
{
public:
	MAudioBuffer(const size_t nSamples,
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
		memset(audioSamples_,0, nSamples*nBytesPerSample);
		nSamplesAvailable_ = nSamples_ = nSamples;
		nBytesPerSample_ = nBytesPerSample;
		nChannels_ = nChannels;
		samplesPerSec_ = samplesPerSec;
	}

	MAudioBuffer(const void* audioSamples,
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

	~MAudioBuffer()
	{
		if (audioSamples_ != NULL)
		{
			free(audioSamples_);
		}
		audioSamplesAvailable_ = audioSamples_ = NULL;
		nSamplesAvailable_ = nSamples_ = 0;
	}

	int TransformTo(const size_t nSamples,
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
				size_t Bytes = nBytesPerSample_ / nChannels_;
				size_t count = nBytesPerSample / nBytesPerSample_;

				switch (Bytes)
				{
				case 1:
					for (size_t i = 0; i < copySamples; i++)
					{
						size_t j = 0;
						for (; j < count; j++)
						{
							*((char*)audioSamples + j) = *((char*)audioSamplesAvailable_);
						}
						audioSamplesAvailable_ = (char*)audioSamplesAvailable_ + nBytesPerSample_;
						audioSamples = (char*)audioSamples + nBytesPerSample;
					}
					break;
				case 2:
					for (size_t i = 0; i < copySamples; i++)
					{
						size_t j = 0;
						for (; j < count; j++)
						{
							*((short*)audioSamples + j) = *((short*)audioSamplesAvailable_);
						}
						audioSamplesAvailable_ = (char*)audioSamplesAvailable_ + nBytesPerSample_;
						audioSamples = (char*)audioSamples + nBytesPerSample;
					}
					break;
				case 4:
					for (size_t i = 0; i < copySamples; i++)
					{
						size_t j = 0;
						for (; j < count; j++)
						{
							*((int*)audioSamples + j) = *((int*)audioSamplesAvailable_);
						}
						audioSamplesAvailable_ = (char*)audioSamplesAvailable_ + nBytesPerSample_;
						audioSamples = (char*)audioSamples + nBytesPerSample;
					}
					break;
				case 8:
					for (size_t i = 0; i < copySamples; i++)
					{
						size_t j = 0;
						for (; j < count; j++)
						{
							*((LONG64*)audioSamples + j) = *((LONG64*)audioSamplesAvailable_);
						}
						audioSamplesAvailable_ = (char*)audioSamplesAvailable_ + nBytesPerSample_;
						audioSamples = (char*)audioSamples + nBytesPerSample;
					}
					break;
				default:
					return -1;
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
	void* audioSamples_;
	size_t nSamples_;
	void* audioSamplesAvailable_;
	size_t nSamplesAvailable_;
	size_t nBytesPerSample_;
	size_t nChannels_;
	uint32_t samplesPerSec_;
};

#include "webrtc\voice_engine\utility.h"
class MAudioTransform
	: public webrtc::AudioTransport
{
public:
	MAudioTransform()
		:apm(webrtc::AudioProcessing::Create())
	{
		apm->level_estimator()->Enable(true);//启用重试次数估计组件
		//apm->echo_cancellation()->Enable(true);//启用回声消除组件
		//apm->echo_cancellation()->enable_metrics(true);//
		//apm->echo_cancellation()->enable_drift_compensation(true);//启用时钟补偿模块（声音捕捉设备的时钟频率和播放设备的时钟频率可能不一样）
		apm->gain_control()->Enable(true);//启用增益控制组件，client必须启用哦！
		apm->high_pass_filter()->Enable(true);//高通过滤器组件，过滤DC偏移和低频噪音，client必须启用
		//apm->voice_detection()->Enable(true);//启用语音检测组件，检测是否有说话声
		//apm->voice_detection()->set_likelihood(webrtc::VoiceDetection::kModerateLikelihood);//设置语音检测的阀值，阀值越大，语音越不容易被忽略，同样一些噪音可能被当成语音。
		apm->noise_suppression()->Enable(true);//噪声抑制组件，client必须启用
		apm->Initialize();//保留所有用户设置的情况下重新初始化apm的内部状态，用于开始处理一个新的音频流。第一个流创建之后不一定需要调用此方法。
		
		apm->noise_suppression()->set_level(webrtc::NoiseSuppression::kVeryHigh);

		capture_level = 0;
		webrtc::GainControl* agc = apm->gain_control();
		if (agc->set_analog_level_limits(webrtc::kMinVolumeLevel, webrtc::kMaxVolumeLevel) != 0) {
			LOG_F(LS_ERROR) << "Failed to set analog level limits with minimum: "
				<< webrtc::kMinVolumeLevel << " and maximum: " << webrtc::kMaxVolumeLevel;
		}
		if (agc->set_mode(webrtc::kDefaultAgcMode) != 0) {
			LOG_F(LS_ERROR) << "Failed to set mode: " << webrtc::kDefaultAgcMode;
		}
		if (agc->Enable(webrtc::kDefaultAgcState) != 0) {
			LOG_F(LS_ERROR) << "Failed to set agc state: " << webrtc::kDefaultAgcState;
		}

		//int delay_ms, extra_delay_ms;
		//int drift_samples;
		//apm->set_stream_delay_ms(delay_ms + extra_delay_ms);//设置本地和远端音频流之间的延迟，单位毫秒。这个延迟是远端音频流和本地音频流之间的时差，计算方法为：
		//apm->echo_cancellation()->set_stream_drift_samples(drift_samples);//设置音频设备捕捉和播放的采样率的差值。（drift组件启用时必须调用）
	}

	virtual ~MAudioTransform() 
	{
		for (size_t i = 0; i < Buffer.size(); i++)
		{
			MAudioBuffer * buff = Buffer.front();
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
		//printf("Capture:%d %d %d %d\n", nBytesPerSample, nChannels, samplesPerSec, nSamples);
		size_t packetCount = samplesPerSec / nSamples;

		if (Buffer.size() > packetCount)
		{
			size_t removeCount = (Buffer.size() - packetCount + 1);
			for (size_t i = 0; i < removeCount; i++)
			{
				MAudioBuffer * buff = Buffer.front();
				Buffer.pop_front();
				delete buff;
			}
			printf("丢弃:%d\n", removeCount);
		}

		MAudioBuffer * buff = NULL;
		bool bProcess = true;
		if (bProcess == true) {
			buff = new MAudioBuffer(nSamples, nBytesPerSample, nChannels, samplesPerSec);

			GenerateAudioFrame((const int16_t*)audioSamples,nSamples,nChannels,samplesPerSec);

			apm->gain_control()->set_stream_analog_level(capture_level);
			int err = apm->ProcessStream(&_audioFrame);
			if (err == 0) {
				memcpy(buff->audioSamples_, _audioFrame.data_, nBytesPerSample*nSamples);
			}
			else
			{
				delete buff;
				return 0;
			}
			capture_level = apm->gain_control()->stream_analog_level();//模拟模式下，必须在ProcessStream之后调用此方法，获取新的音频HAL的推荐模拟值。
			bool stream_has_voice = apm->voice_detection()->stream_has_voice();//检测是否有语音，必须在ProcessStream之后调用此方法
			float ns_speech_prob = apm->noise_suppression()->speech_probability();//返回内部计算出的当前frame的人声优先概率。
		}
		else
		{
			buff = new MAudioBuffer(audioSamples,nSamples, nBytesPerSample, nChannels, samplesPerSec);
		}

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

		MAudioBuffer * buff = Buffer.front();
		if (buff == NULL) 
		{
			memset(audioSamples, 0, nBytesPerSample * nSamples);
			nSamplesOut = nSamples;
			return 0;
		}
		int ret = buff->TransformTo(nSamples,nBytesPerSample,nChannels,samplesPerSec,audioSamples,nSamplesOut);
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
	std::list<MAudioBuffer*> Buffer;
};

void CLASS_API TestAudioCapture()
{
	CoInitialize(NULL);
	rtc::EnsureWinsockInit();
	rtc::Win32Thread w32_thread;
	rtc::ThreadManager::Instance()->SetCurrentThread(&w32_thread);

	rtc::InitializeSSL();

	rtc::scoped_refptr<webrtc::AudioDeviceModule> audioDevice = webrtc::AudioDeviceModuleImpl::Create(0);

	if (!audioDevice.get()) {
		return;
	}

	audioDevice->Init();
	audioDevice->Initialized();
	audioDevice->InitMicrophone();
	audioDevice->InitSpeaker();

	printDevices(audioDevice.get());
	
	audioDevice->SetRecordingDevice(0);
	
	audioDevice->InitRecording();

	MAudioTransform transform;
	audioDevice->RegisterAudioCallback(&transform);

	audioDevice->SetPlayoutDevice(0);
	audioDevice->InitPlayout();

	int32_t ret = audioDevice->StartRecording();
	ret = audioDevice->StartPlayout();

	w32_thread.Run();

	audioDevice->Terminate();

	rtc::CleanupSSL();
}

class TransportData
{
public:
	// Start implementation of TransportData.
	virtual void IncomingRTPPacket(const uint8_t* incoming_rtp_packet,
		const size_t packet_length) = 0;

	virtual void IncomingRTCPPacket(const uint8_t* incoming_rtcp_packet,
		const size_t packet_length) = 0;
};

class MTransport : public webrtc::Transport, TransportData
{
public:
	MTransport(webrtc::VoENetwork* network, int playoutChannel,int captureChannel)
		:network_(network), playoutChannel_(playoutChannel), captureChannel_(captureChannel)
	{
	}

	virtual bool SendRtp(const uint8_t* packet,
		size_t length,
		const webrtc::PacketOptions& options)
	{
		IncomingRTPPacket(packet,length);
		return true;
	}

	virtual bool SendRtcp(const uint8_t* packet, size_t length)
	{
		IncomingRTCPPacket(packet,length);
		return true;
	}

	// Start implementation of TransportData.
	void IncomingRTPPacket(const uint8_t* incoming_rtp_packet,
		const size_t packet_length) override
	{
		network_->ReceivedRTPPacket(playoutChannel_, incoming_rtp_packet, packet_length);
	}

	void IncomingRTCPPacket(const uint8_t* incoming_rtcp_packet,
		const size_t packet_length) override
	{
		network_->ReceivedRTCPPacket(captureChannel_, incoming_rtcp_packet, packet_length);
		network_->ReceivedRTCPPacket(playoutChannel_, incoming_rtcp_packet, packet_length);
	}
private:
	webrtc::VoENetwork* network_;
	int playoutChannel_;
	int captureChannel_;
};

void CLASS_API TestAudioLoopCapture()
{
	CoInitialize(NULL);
	rtc::EnsureWinsockInit();
	rtc::Win32Thread w32_thread;
	rtc::ThreadManager::Instance()->SetCurrentThread(&w32_thread);

	rtc::InitializeSSL();

	webrtc::VoiceEngine *engine = webrtc::VoiceEngine::Create();

	if (!engine) {
		return;
	}
	webrtc::VoEBase * base = webrtc::VoEBase::GetInterface(engine);
	webrtc::VoENetwork * network = webrtc::VoENetwork::GetInterface(engine);
	webrtc::VoERTP_RTCP* rtcp = webrtc::VoERTP_RTCP::GetInterface(engine);
	webrtc::VoEHardware * hardware = webrtc::VoEHardware::GetInterface(engine);
	webrtc::VoECodec * codec = webrtc::VoECodec::GetInterface(engine);
	webrtc::VoEAudioProcessing *audioProcessing = webrtc::VoEAudioProcessing::GetInterface(engine);

	rtc::scoped_refptr<webrtc::AudioDeviceModule> audioDevice = webrtc::AudioDeviceModuleImpl::Create(0);

	if (!audioDevice.get()) {
		return;
	}

	audioDevice->Init();
	
	printDevices(audioDevice.get());

	base->Init(audioDevice);

	audioDevice->Initialized();
	audioDevice->InitMicrophone();
	audioDevice->InitSpeaker();
	audioDevice->SetRecordingDevice(0);
	audioDevice->InitRecording();
	audioDevice->SetPlayoutDevice(0);
	audioDevice->InitPlayout();


	audioProcessing->SetNsStatus(true, webrtc::kNsHighSuppression);
	audioProcessing->SetAgcStatus(true, webrtc::kAgcFixedDigital);
	audioProcessing->EnableDriftCompensation(true);
	audioProcessing->SetEcStatus(true, webrtc::kEcAec);
	audioProcessing->SetAecmMode(webrtc::kAecmQuietEarpieceOrHeadset, true);
	audioProcessing->EnableHighPassFilter(true);
	audioProcessing->SetEcMetricsStatus(true);
	audioProcessing->SetTypingDetectionStatus(true);
	audioProcessing->EnableStereoChannelSwapping(true);

	int capture = base->CreateChannel();
	int playout = base->CreateChannel();

	audioProcessing->SetRxNsStatus(playout,true, webrtc::kNsModerateSuppression);

	MTransport transport(network, playout, capture);
	network->RegisterExternalTransport(capture, transport);
	network->RegisterExternalTransport(playout, transport);

	base->AssociateSendChannel(playout, capture);
	base->StartPlayout(playout);
	base->StartReceive(capture);
	base->StartSend(capture);

	w32_thread.Run();

	webrtc::VoiceEngine::Delete(engine);

	rtc::CleanupSSL();
}

#include "LoopBackTransport.h"
#include "webrtc\voice_engine\include\voe_video_sync.h"
#include "webrtc\call\transport_adapter.h"

void CLASS_API TestAudioLoopBack()
{
	CoInitialize(NULL);
	rtc::EnsureWinsockInit();
	rtc::Win32Thread w32_thread;
	rtc::ThreadManager::Instance()->SetCurrentThread(&w32_thread);

	rtc::InitializeSSL();

	webrtc::VoiceEngine *engine = webrtc::VoiceEngine::Create();

	if (!engine) {
		return;
	}
	webrtc::VoEBase * base = webrtc::VoEBase::GetInterface(engine);
	webrtc::VoENetwork * network = webrtc::VoENetwork::GetInterface(engine);
	webrtc::VoERTP_RTCP* rtcp = webrtc::VoERTP_RTCP::GetInterface(engine);
	webrtc::VoEHardware * hardware = webrtc::VoEHardware::GetInterface(engine);
	webrtc::VoECodec * codec = webrtc::VoECodec::GetInterface(engine);
	webrtc::VoEAudioProcessing *audioProcessing = webrtc::VoEAudioProcessing::GetInterface(engine);
	webrtc::VoEVideoSync * videoSync = webrtc::VoEVideoSync::GetInterface(engine);

	rtc::scoped_refptr<webrtc::AudioDeviceModule> audioDevice = webrtc::AudioDeviceModuleImpl::Create(0);

	if (!audioDevice.get()) {
		return;
	}

	audioDevice->Init();

	printDevices(audioDevice.get());

	base->Init(audioDevice);

	audioDevice->Initialized();
	audioDevice->InitMicrophone();
	audioDevice->InitSpeaker();
	audioDevice->SetRecordingDevice(0);
	audioDevice->InitRecording();
	audioDevice->SetPlayoutDevice(0);
	audioDevice->InitPlayout();


	//audioProcessing->SetNsStatus(true, webrtc::kNsConference);
	//audioProcessing->SetAgcStatus(true, webrtc::kAgcUnchanged);
	//audioProcessing->EnableDriftCompensation(true);
	//audioProcessing->SetEcStatus(true, webrtc::kEcUnchanged);
	////audioProcessing->SetAecmMode(webrtc::kAecmQuietEarpieceOrHeadset, true);
	//audioProcessing->EnableHighPassFilter(true);
	//audioProcessing->SetEcMetricsStatus(true);
	//audioProcessing->SetTypingDetectionStatus(true);
	//audioProcessing->EnableStereoChannelSwapping(true);
	
	int capture = base->CreateChannel();
	int playout = base->CreateChannel();

	webrtc::LoopBackTransport loopback(engine, capture,capture);
	webrtc::NullTransport transport;
	network->RegisterExternalTransport(capture, loopback);
	network->RegisterExternalTransport(playout, loopback);

	webrtc::internal::TransportAdapter adapter(&transport);

	base->AssociateSendChannel(playout, capture);
	base->StartPlayout(capture);
	base->StartReceive(capture);
	base->StartSend(capture);

	w32_thread.Run();

	webrtc::VoiceEngine::Delete(engine);

	rtc::CleanupSSL();
}

rtc::Win32Thread * g_w32_thread = NULL;

void InitThread()
{
	rtc::EnsureWinsockInit();
	rtc::ThreadManager::Instance();
	if (g_w32_thread == NULL) {
		g_w32_thread = new rtc::Win32Thread();
		g_w32_thread->Start();
	}
	rtc::ThreadManager::Instance()->SetCurrentThread(g_w32_thread);
	//rtc::ThreadManager::Instance()->CurrentThread();
	rtc::InitializeSSL();
}

void UninitThread()
{
	rtc::ThreadManager::Instance()->SetCurrentThread(NULL);
	if (g_w32_thread != NULL) {
		g_w32_thread->Quit();
		g_w32_thread->Stop();
		delete g_w32_thread;
		g_w32_thread = NULL;
	}
	rtc::CleanupSSL();
}