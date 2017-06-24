#include "stdafx.h"
#include "AudioLookback.h"

#include "webrtc/base/ssladapter.h"
#include "webrtc/base/win32socketinit.h"
#include "webrtc/base/win32socketserver.h"
#include "webrtc/base/physicalsocketserver.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/signalthread.h"
#include "webrtc/base/sigslot.h"
#include "webrtc/base/arraysize.h"
#include "webrtc/transport.h"

#include "webrtc\voice_engine\include\voe_base.h"
#include "webrtc\voice_engine\voice_engine_impl.h"
#include "webrtc\voice_engine\utility.h"
#include "webrtc\voice_engine\include\voe_video_sync.h"
#include "webrtc\common_audio\wav_file.h"
#include "webrtc\modules\audio_device\audio_device_impl.h"
#include "webrtc\modules\audio_processing\include\audio_processing.h"
#include "webrtc\modules\audio_processing\noise_suppression_impl.h"
#include "webrtc\modules\audio_processing\audio_buffer.h"

#include "DevicesInfo.h"
#include "AudioSampleBuffer.h"
#include "AudioProcessingTransform.h"
#include "AudioPlayTransform.h"

#include <Objbase.h>

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

	AudioProcessingTransform transform;
	audioDevice->RegisterAudioCallback(&transform);

	audioDevice->SetPlayoutDevice(0);
	audioDevice->InitPlayout();

	int32_t ret = audioDevice->StartRecording();
	ret = audioDevice->StartPlayout();

	w32_thread.Run();

	audioDevice->Terminate();

	rtc::CleanupSSL();
}

void TestAudioPlay()
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

	AudioPlayTransform transform;
	
	audioDevice->RegisterAudioCallback(&transform);

	audioDevice->SetPlayoutDevice(0);
	audioDevice->InitPlayout();

	int32_t ret = audioDevice->StartPlayout();

	transform.Open("./014956_923_noiseFile.wav");

	w32_thread.Run();

	audioDevice->Terminate();

	rtc::CleanupSSL();
}