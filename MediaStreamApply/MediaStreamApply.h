#pragma once

#define NOMINMAX
#undef max
#undef min
#include <algorithm>

#include "webrtc/api/mediastreaminterface.h"
#include "webrtc/api/peerconnectioninterface.h"
#include "webrtc/base/scoped_ptr.h"
#include "LogObserver.h"

#include "class_api.h"

class CLASS_API MediaStreamApply
{
public:
	MediaStreamApply();
	~MediaStreamApply();
	bool InitializePeerConnection();
	void Close();
	int GetDeviceNames();
	const char * GetDeviceName(int index);
	void OpenStreams(int index);

	rtc::scoped_refptr<webrtc::VideoTrackInterface> GetVideoTrack();

	void RegisterObserver(LogObserver * observer);
	void UnregisterObserver(LogObserver * observer);
private:
	void DeletePeerConnection();
	
	cricket::VideoCapturer* OpenVideoCaptureDevice(int index);

private:
	rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory_;
	std::map<std::string, rtc::scoped_refptr<webrtc::MediaStreamInterface> > active_streams_;
	LogObserver *log_;
	int index_;

	std::vector<std::string> devices_;
};