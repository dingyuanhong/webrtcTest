#include "stdafx.h"
#include <utility>
#include <vector>

#include "webrtc/base/common.h"
#include "webrtc/base/json.h"
#include "webrtc/base/logging.h"
#include "defaults.h"
#include "webrtc/media/engine/webrtcvideocapturerfactory.h"
#include "webrtc/modules/video_capture/video_capture_factory.h"
#include "webrtc\modules\audio_device\audio_device_impl.h"
#include "fakeconstraints.h"
#include "PeerConnectionChannel.h"
#include "DummySetSessionDescriptionObserver.h"

// Names used for a IceCandidate JSON object.
const char kCandidateSdpMidName[] = "sdpMid";
const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
const char kCandidateSdpName[] = "candidate";

// Names used for a SessionDescription JSON object.
const char kSessionDescriptionTypeName[] = "type";
const char kSessionDescriptionSdpName[] = "sdp";

#define DTLS_ON  true
#define DTLS_OFF false


#define OnError

//
// PeerConnectionObserver implementation.
//

// Called when a remote stream is added
void PeerConnectionChannel::OnAddStream(webrtc::MediaStreamInterface* stream) {
	LOG(INFO) << __FUNCTION__ << " " << stream->label();

	stream->AddRef();
	NewRemoteStreamAdded(stream);
}

void PeerConnectionChannel::OnRemoveStream(webrtc::MediaStreamInterface* stream) {
	LOG(INFO) << __FUNCTION__ << " " << stream->label();
	stream->AddRef();
	RemoteStreamRemoved(stream);
}

void PeerConnectionChannel::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
	LOG(INFO) << __FUNCTION__ << " " << candidate->sdp_mline_index();
	// For loopback test. To save some connecting delay.
	if (loopback_) {
		if (!peer_connection_->AddIceCandidate(candidate)) {
			LOG(WARNING) << "Failed to apply the received candidate";
		}
		return;
	}

	Json::StyledWriter writer;
	Json::Value jmessage;

	jmessage[kCandidateSdpMidName] = candidate->sdp_mid();
	jmessage[kCandidateSdpMlineIndexName] = candidate->sdp_mline_index();
	std::string sdp;
	if (!candidate->ToString(&sdp)) {
		LOG(LS_ERROR) << "Failed to serialize candidate";
		return;
	}
	jmessage[kCandidateSdpName] = sdp;

	std::string json_object = writer.write(jmessage);
	SendMessageToPeer(json_object);
}

//
// CreateSessionDescriptionObserver implementation.
//
void PeerConnectionChannel::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
	//this->AddRef();

	peer_connection_->SetLocalDescription(
		DummySetSessionDescriptionObserver::Create(), desc);

	std::string sdp;
	desc->ToString(&sdp);

	// For loopback test. To save some connecting delay.
	if (loopback_) {
		// Replace message type from "offer" to "answer"
		webrtc::SessionDescriptionInterface* session_description(
			webrtc::CreateSessionDescription("answer", sdp, nullptr));
		peer_connection_->SetRemoteDescription(
			DummySetSessionDescriptionObserver::Create(), session_description);
		return;
	}

	Json::StyledWriter writer;
	Json::Value jmessage;
	jmessage[kSessionDescriptionTypeName] = desc->type();
	jmessage[kSessionDescriptionSdpName] = sdp;

	std::string json_object = writer.write(jmessage);
	SendMessageToPeer(json_object);
}

void PeerConnectionChannel::OnFailure(const std::string& error) {
	LOG(LERROR) << error;
}

// private function

void PeerConnectionChannel::SendMessageToPeer(std::string &MSG)
{
	ASSERT(observer_ != NULL);
	observer_->OnSendMessage(peer_id_,MSG);
}

void PeerConnectionChannel::NewRemoteStreamAdded(webrtc::MediaStreamInterface* stream)
{
	ASSERT(observer_ != NULL);
	webrtc::VideoTrackVector tracks = stream->GetVideoTracks();
	// Only render the first track.
	if (!tracks.empty()) {
		webrtc::VideoTrackInterface* track = tracks[0];
		observer_->OnAddedStream(peer_id_,track);
	}
	stream->Release();
}

void PeerConnectionChannel::RemoteStreamRemoved(webrtc::MediaStreamInterface* stream)
{
	ASSERT(observer_ != NULL);
	// Remote peer stopped sending a stream.
	webrtc::VideoTrackVector tracks = stream->GetVideoTracks();
	if (!tracks.empty()) {
		observer_->OnRemovedStream(peer_id_);
	}
	stream->Release();
}

PeerConnectionChannel::PeerConnectionChannel(webrtc::PeerConnectionFactoryInterface*factory, int peer_id)
	:peer_connection_factory_(factory),peer_id_(peer_id), loopback_(false)
{

}

PeerConnectionChannel::~PeerConnectionChannel()
{
	ASSERT(peer_connection_.get() == NULL);
	/*Close();
	peer_id_ = -1;
	peer_connection_factory_ = NULL;
	observer_ = NULL;*/
}

bool PeerConnectionChannel::IsConnected()
{
	return peer_connection_.get() != NULL;
}

bool PeerConnectionChannel::HasLocalStream()
{
	return IsConnected() && peer_connection_->local_streams()->count() > 0;
}

bool PeerConnectionChannel::Initialize()
{
	ASSERT(peer_connection_factory_.get() != NULL);

	peer_connection_ = NULL;
	if (!CreatePeerConnection(DTLS_ON)) {
		OnError("CreatePeerConnection failed");
		Close();
		if (observer_ != NULL) {
			observer_->OnDeletePeer(peer_id_);
		}
	}
	return peer_connection_.get() != NULL;
}

bool PeerConnectionChannel::InitializeForLoopback() {
	ASSERT(peer_connection_.get() != NULL);

	loopback_ = true;
	rtc::scoped_refptr<webrtc::StreamCollectionInterface> streams(
		peer_connection_->local_streams());
	peer_connection_ = NULL;
	if (CreatePeerConnection(DTLS_OFF)) {
		for (size_t i = 0; i < streams->count(); ++i)
			peer_connection_->AddStream(streams->at(i));
	}
	return peer_connection_.get() != NULL;
}

void PeerConnectionChannel::Close()
{
	if (observer_ != NULL) {
		if(peer_connection_ != NULL && peer_connection_->remote_description() != NULL)
			observer_->OnRemovedStream(peer_id_);
	}
	peer_connection_ = NULL;
	loopback_ = false;
}

void PeerConnectionChannel::Connect()
{
	if (peer_connection_.get())
	{
		this->AddRef();
		peer_connection_->CreateOffer(this, NULL);
	}
	else {
		OnError("Failed to initialize PeerConnection");
	}
}

bool PeerConnectionChannel::CreatePeerConnection(bool dtls)
{
	ASSERT(peer_connection_factory_.get() != NULL);
	ASSERT(peer_connection_.get() == NULL);

	webrtc::PeerConnectionInterface::RTCConfiguration config;
	webrtc::PeerConnectionInterface::IceServer server;
	server.uri = GetPeerConnectionString();
	config.servers.push_back(server);

	webrtc::FakeConstraints constraints;
	if (dtls) {
		constraints.AddOptional(webrtc::MediaConstraintsInterface::kEnableDtlsSrtp,
			"true");
	}
	else {
		constraints.AddOptional(webrtc::MediaConstraintsInterface::kEnableDtlsSrtp,
			"false");
	}

	peer_connection_ = peer_connection_factory_->CreatePeerConnection(
		config, &constraints, NULL, NULL, this);
	return peer_connection_.get() != NULL;
}

void PeerConnectionChannel::AddLocalStreams(webrtc::MediaStreamInterface* stream)
{
	ASSERT(peer_connection_.get() != NULL);

	if (!peer_connection_->AddStream(stream)) {
		LOG(LS_ERROR) << "Adding stream to PeerConnection failed";
	}
}

void PeerConnectionChannel::OnMessage(const std::string & message)
{
	Json::Reader reader;
	Json::Value jmessage;
	if (!reader.parse(message, jmessage)) {
		LOG(WARNING) << "Received unknown message. " << message;
		return;
	}
	std::string type;
	std::string json_object;

	rtc::GetStringFromJsonObject(jmessage, kSessionDescriptionTypeName, &type);
	if (!type.empty()) {
		if (type == "offer-loopback") {
			// This is a loopback call.
			// Recreate the peerconnection with DTLS disabled.
			if (!InitializeForLoopback()) {
				LOG(LS_ERROR) << "Failed to initialize our PeerConnection instance";
				Close();
				if(observer_ != NULL) observer_->OnDeletePeer(peer_id_);
			}
			return;
		}

		std::string sdp;
		if (!rtc::GetStringFromJsonObject(jmessage, kSessionDescriptionSdpName,
			&sdp)) {
			LOG(WARNING) << "Can't parse received session description message.";
			return;
		}
		webrtc::SdpParseError error;
		webrtc::SessionDescriptionInterface* session_description(
			webrtc::CreateSessionDescription(type, sdp, &error));
		if (!session_description) {
			LOG(WARNING) << "Can't parse received session description message. "
				<< "SdpParseError was: " << error.description;
			return;
		}
		LOG(INFO) << " Received session description :" << message;
		peer_connection_->SetRemoteDescription(
			DummySetSessionDescriptionObserver::Create(), session_description);
		if (session_description->type() ==
			webrtc::SessionDescriptionInterface::kOffer) {
			this->AddRef();
			peer_connection_->CreateAnswer(this, NULL);
		}
		return;
	}
	else {
		std::string sdp_mid;
		int sdp_mlineindex = 0;
		std::string sdp;
		if (!rtc::GetStringFromJsonObject(jmessage, kCandidateSdpMidName,
			&sdp_mid) ||
			!rtc::GetIntFromJsonObject(jmessage, kCandidateSdpMlineIndexName,
				&sdp_mlineindex) ||
			!rtc::GetStringFromJsonObject(jmessage, kCandidateSdpName, &sdp)) {
			LOG(WARNING) << "Can't parse received message.";
			return;
		}
		webrtc::SdpParseError error;
		rtc::scoped_ptr<webrtc::IceCandidateInterface> candidate(
			webrtc::CreateIceCandidate(sdp_mid, sdp_mlineindex, sdp, &error));
		if (!candidate.get()) {
			LOG(WARNING) << "Can't parse received candidate message. "
				<< "SdpParseError was: " << error.description;
			return;
		}
		if (!peer_connection_->AddIceCandidate(candidate.get())) {
			LOG(WARNING) << "Failed to apply the received candidate";
			return;
		}
		LOG(INFO) << " Received candidate :" << message;
		return;
	}
}