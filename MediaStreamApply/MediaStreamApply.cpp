#include "stdafx.h"

#define NOMINMAX
#undef max
#undef min
#include <algorithm>

#include "webrtc/api/test/fakeconstraints.h"
#include "webrtc/base/common.h"
#include "webrtc/base/json.h"
#include "webrtc/base/logging.h"
#include "webrtc/media/engine/webrtcvideocapturerfactory.h"
#include "webrtc/modules/video_capture/video_capture_factory.h"

#include "MediaStreamApply.h"
#include "defaults.h"

// Names used for a IceCandidate JSON object.
const char kCandidateSdpMidName[] = "sdpMid";
const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
const char kCandidateSdpName[] = "candidate";

// Names used for a SessionDescription JSON object.
const char kSessionDescriptionTypeName[] = "type";
const char kSessionDescriptionSdpName[] = "sdp";

#define DTLS_ON  true
#define DTLS_OFF false

MediaStreamApply::MediaStreamApply()
	:log_(NULL),
	index_(-1)
{
}

MediaStreamApply::~MediaStreamApply()
{
}

void MediaStreamApply::RegisterObserver(LogObserver * observer)
{
	log_ = observer;
}

void MediaStreamApply::UnregisterObserver(LogObserver * observer)
{
	log_ = NULL;
}

void MediaStreamApply::Close()
{
	DeletePeerConnection();
}

int MediaStreamApply::GetDeviceNames()
{
	std::vector<std::string> device_names;
	{
		std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
			webrtc::VideoCaptureFactory::CreateDeviceInfo(0));
		if (!info) {
			return 0;
		}
		int num_devices = info->NumberOfDevices();
		for (int i = 0; i < num_devices; ++i) {
			const uint32_t kSize = 256;
			char name[kSize] = { 0 };
			char id[kSize] = { 0 };
			if (info->GetDeviceName(i, name, kSize, id, kSize) != -1) {
				device_names.push_back(name);
			}
		}
	}

	devices_ = device_names;

	return devices_.size();
}

const char * MediaStreamApply::GetDeviceName(int index)
{
	return devices_[index].c_str();
}

bool MediaStreamApply::InitializePeerConnection() {
	ASSERT(peer_connection_factory_.get() == NULL);

	peer_connection_factory_ = webrtc::CreatePeerConnectionFactory();

	if (!peer_connection_factory_.get()) {
		if (log_ != NULL) log_->OnLogMessageA("Error",
			"Failed to initialize PeerConnectionFactory");
		DeletePeerConnection();
		return false;
	}
	return true;
}

void MediaStreamApply::DeletePeerConnection() {
	active_streams_.clear();

	peer_connection_factory_ = NULL;
}

void MediaStreamApply::OpenStreams(int index) {
	bool addAudioTrack = true;
	bool addVideoTrack = true;
	if (index_ == index) return;

	rtc::scoped_refptr<webrtc::MediaStreamInterface> stream;
	active_streams_.clear();
	if (active_streams_.find(kStreamLabel) != active_streams_.end()) {
		stream = active_streams_.find(kStreamLabel)->second;
		rtc::scoped_refptr<webrtc::VideoTrackInterface> videoTrack = stream->FindVideoTrack(kVideoLabel);
		stream->RemoveTrack(videoTrack);
		videoTrack->GetSource()->Stop();
		addAudioTrack = false;
	}
	else 
	{
		stream = peer_connection_factory_->CreateLocalMediaStream(kStreamLabel);
	}

	if (addAudioTrack) {
		rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
			peer_connection_factory_->CreateAudioTrack(
				kAudioLabel, peer_connection_factory_->CreateAudioSource(NULL)));

		stream->AddTrack(audio_track);
	}

	if (addVideoTrack) {
		rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track(
			peer_connection_factory_->CreateVideoTrack(
				kVideoLabel,
				peer_connection_factory_->CreateVideoSource(OpenVideoCaptureDevice(index),
					NULL)));

		index_ = index;
		stream->AddTrack(video_track);
	}

	if (active_streams_.find(kStreamLabel) != active_streams_.end()) {
		return;  // Already added.
	}

	typedef std::pair<std::string,
		rtc::scoped_refptr<webrtc::MediaStreamInterface> >
		MediaStreamPair;
	active_streams_.insert(MediaStreamPair(stream->label(), stream));
}

rtc::scoped_refptr<webrtc::VideoTrackInterface> MediaStreamApply::GetVideoTrack()
{
	rtc::scoped_refptr<webrtc::MediaStreamInterface> stream;

	if (active_streams_.find(kStreamLabel) != active_streams_.end()) {
		stream = active_streams_.find(kStreamLabel)->second;
		return stream->FindVideoTrack(kVideoLabel);
	}
	else {
		return NULL;
	}
}

cricket::VideoCapturer* MediaStreamApply::OpenVideoCaptureDevice(int index) {
	std::vector<std::string> device_names;
	{
		std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
			webrtc::VideoCaptureFactory::CreateDeviceInfo(0));
		if (!info) {
			return nullptr;
		}
		int num_devices = info->NumberOfDevices();
		for (int i = 0; i < num_devices; ++i) {
			const uint32_t kSize = 256;
			char name[kSize] = { 0 };
			char id[kSize] = { 0 };
			if (info->GetDeviceName(i, name, kSize, id, kSize) != -1) {
				device_names.push_back(name);
			}
		}
	}

	cricket::WebRtcVideoDeviceCapturerFactory factory;
	cricket::VideoCapturer* capturer = nullptr;
	
	int i = -1;
	if (capturer == NULL) {
		for (const auto& name : device_names) {
			i++;
			if (i != index) continue;
			capturer = factory.Create(cricket::Device(name, i));
			if (capturer) {
				break;
			}
		}
	}
	return capturer;
}