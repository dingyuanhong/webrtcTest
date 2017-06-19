#include "stdafx.h"
#include "AudioLookback2.h"

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

#include "DevicesInfo.h"
#include "ChannelLookbackTransport.h"

#include <Objbase.h>

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

	audioProcessing->SetRxNsStatus(playout, true, webrtc::kNsModerateSuppression);

	ChannelLookbackTransport transport(network, playout, capture);
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