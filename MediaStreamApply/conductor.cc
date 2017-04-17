/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "stdafx.h"
#include "conductor.h"

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

Conductor::Conductor(PeerConnectionTransform* client, FormWindow* main_form)
  : peer_id_(-1),
    loopback_(false),
	loopbackrun_(false),
    client_(client),
	main_form_(main_form) {
  client_->RegisterObserver(this);
  main_form_->RegisterObserver(this);
}

Conductor::~Conductor() {
  ASSERT(peer_connection_.get() == NULL);
}

bool Conductor::connection_active() const {
  return peer_connection_.get() != NULL;
}

void Conductor::Close() {
  client_->SignOut();
  DeletePeerConnection();
}

void Conductor::Clear()
{
	Close();
	client_ = NULL;
	main_form_ = NULL;
}

bool Conductor::InitializePeerConnection() {
  ASSERT(peer_connection_factory_.get() == NULL);
  ASSERT(peer_connection_.get() == NULL);
  ASSERT(default_adm_.get() == NULL);

  default_adm_ = webrtc::AudioDeviceModuleImpl::Create(0);

  peer_connection_factory_  = webrtc::CreatePeerConnectionFactory(
	  rtc::ThreadManager::Instance()->CurrentThread(), //worker_thread_,
		rtc::ThreadManager::Instance()->CurrentThread(), 
		default_adm_.get(),NULL,NULL);

  if (!peer_connection_factory_.get()) {
	  main_form_->Error("Failed to initialize PeerConnectionFactory");
    DeletePeerConnection();
    return false;
  }

  if (!CreatePeerConnection(DTLS_ON)) {
	  main_form_->Error("CreatePeerConnection failed");
    DeletePeerConnection();
  }
  AddLocalStreams();
  return peer_connection_.get() != NULL;
}

bool Conductor::ReinitializePeerConnectionForLoopback() {
  loopback_ = true;
  rtc::scoped_refptr<webrtc::StreamCollectionInterface> streams(
      peer_connection_->local_streams());
  peer_connection_ = NULL;
  if (CreatePeerConnection(DTLS_OFF)) {
    for (size_t i = 0; i < streams->count(); ++i)
      peer_connection_->AddStream(streams->at(i));
    peer_connection_->CreateOffer(this, NULL);
  }
  return peer_connection_.get() != NULL;
}

bool Conductor::CreatePeerConnection(bool dtls) {
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

void Conductor::DeletePeerConnection() {
  peer_connection_ = NULL;
  active_streams_.clear();
  main_form_->StopLocalRenderer();
  main_form_->StopRemoteRenderer(-1);
  peer_connection_factory_ = NULL;
  default_adm_ = NULL;
  peer_id_ = -1;
  loopback_ = false;
}

cricket::VideoCapturer* Conductor::OpenVideoCaptureDevice() {
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
	for (const auto& name : device_names) {
		capturer = factory.Create(cricket::Device(name, 0));
		if (capturer) {
			break;
		}
	}
	return capturer;
}

void Conductor::AddLocalStreams() {
	if (active_streams_.find(kStreamLabel) != active_streams_.end())
		return;  // Already added.

	rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
		peer_connection_factory_->CreateAudioTrack(
			kAudioLabel, peer_connection_factory_->CreateAudioSource(NULL)));

	rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track(
		peer_connection_factory_->CreateVideoTrack(
			kVideoLabel,
			peer_connection_factory_->CreateVideoSource(OpenVideoCaptureDevice(),
				NULL)));
	main_form_->StartLocalRenderer(video_track);

	rtc::scoped_refptr<webrtc::MediaStreamInterface> stream =
		peer_connection_factory_->CreateLocalMediaStream(kStreamLabel);

	stream->AddTrack(audio_track);
	stream->AddTrack(video_track);
	if (!peer_connection_->AddStream(stream)) {
		LOG(LS_ERROR) << "Adding stream to PeerConnection failed";
	}

	typedef std::pair<std::string,
		rtc::scoped_refptr<webrtc::MediaStreamInterface> >
		MediaStreamPair;
	active_streams_.insert(MediaStreamPair(stream->label(), stream));
}

//
// PeerConnectionObserver implementation.
//

// Called when a remote stream is added
void Conductor::OnAddStream(webrtc::MediaStreamInterface* stream) {
  LOG(INFO) << __FUNCTION__ << " " << stream->label();

  stream->AddRef();
  NewRemoteStreamAdded(stream);
}

void Conductor::OnRemoveStream(webrtc::MediaStreamInterface* stream) {
  LOG(INFO) << __FUNCTION__ << " " << stream->label();
  stream->AddRef();
  RemoteStreamRemoved(stream);
}

void Conductor::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
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
// PeerConnectionClientObserver implementation.
//

void Conductor::OnSignedIn() {
  LOG(INFO) << __FUNCTION__;
  main_form_->SwitchToPeerList(client_->peers());
}

void Conductor::OnDisconnected() {
  LOG(INFO) << __FUNCTION__;

  DeletePeerConnection();
  connectingIDs.clear();
  connectedIDs.clear();
  if (main_form_->IsWindow())
    main_form_->SwitchToConnectUI();

  loopback_ = false;
  loopbackrun_ = false;
}

void Conductor::OnPeerConnected(int id, const std::string& name) {
  LOG(INFO) << __FUNCTION__;
  // Refresh the list if we're showing it.
  if (main_form_->current_ui() == FormWindow::LIST_PEERS)
    main_form_->SwitchToPeerList(client_->peers());
}

void Conductor::OnPeerDisconnected(int id) {
  LOG(INFO) << __FUNCTION__;

  bool finded = false;
  //删除下线节点
  std::vector<int>::iterator result = std::find(connectedIDs.begin(), connectedIDs.end(), id);
  if (result != connectedIDs.end()) {
	  connectedIDs.erase(result);
	  finded = true;
  }
  result = std::find(connectingIDs.begin(), connectingIDs.end(), id);
  if (result != connectingIDs.end()) {
	  connectingIDs.erase(result);
	  finded = true;
  }

  if (finded) {
    LOG(INFO) << "Our peer disconnected";
	PeerConnectingClosed(id);
  } else {
    // Refresh the list if we're showing it.
    if (main_form_->current_ui() == FormWindow::LIST_PEERS)
      main_form_->SwitchToPeerList(client_->peers());
  }
}

void Conductor::OnMessageFromPeer(int peer_id, const std::string& message) {
  ASSERT(peer_id_ == peer_id || peer_id_ == -1);
  ASSERT(!message.empty());

  if (!peer_connection_.get()) {
    ASSERT(peer_id_ == -1);
    peer_id_ = peer_id;

    if (!InitializePeerConnection()) {
      LOG(LS_ERROR) << "Failed to initialize our PeerConnection instance";
      client_->SignOut();
      return;
    }
  } else if (peer_id_ != -1 && peer_id != peer_id_) {
    ASSERT(peer_id_ != -1);
    LOG(WARNING) << "Received a message from unknown peer while already in a "
                    "conversation with a different peer.";
    return;
  }

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
	//环回
    if (type == "offer-loopback") {
      // This is a loopback call.
      // Recreate the peerconnection with DTLS disabled.
      if (!ReinitializePeerConnectionForLoopback()) {
        LOG(LS_ERROR) << "Failed to initialize our PeerConnection instance";
        DeletePeerConnection();
        client_->SignOut();
      }
      return;
    }
	
	//获取SDP
    std::string sdp;
    if (!rtc::GetStringFromJsonObject(jmessage, kSessionDescriptionSdpName,
                                      &sdp)) {
      LOG(WARNING) << "Can't parse received session description message.";
      return;
    }
	//如果已经启用环回,则不用注册RemoteDescription
	//if (!loopback_) 
	{
		const webrtc::SessionDescriptionInterface* remote_description = peer_connection_->remote_description();

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
		  peer_connection_->CreateAnswer(this, NULL);
		}
	}
    return;
  } else {
	//添加ICE申请
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

	//增加连接人
	std::vector<int>::iterator result = std::find(connectingIDs.begin(), connectingIDs.end(), peer_id);
	if (result != connectingIDs.end()) {
		connectingIDs.erase(result);
	}
	connectedIDs.push_back(peer_id);
    return;
  }
}

void Conductor::OnMessageSent(int err) {
  // Process the next pending message if any.
  main_form_->onMessage(SEND_MESSAGE_TO_PEER, NULL);
}

void Conductor::OnServerConnectionFailure() {
    main_form_->Error(("Failed to connect to " + server_).c_str());
	main_form_->onMessage(CONNECT_STATE, (void*)FALSE);
}

void Conductor::OnServerConnectionSuccess()
{
	main_form_->onMessage(CONNECT_STATE, (void*)TRUE);
}

//
// MainWndCallback implementation.
//

void Conductor::StartLogin(const std::string& server, int port) {
  if (client_->is_connected())
    return;
  server_ = server;
  client_->Connect(server, port, GetPeerName());

  InitializePeerConnection();
  ReinitializePeerConnectionForLoopback();
}

void Conductor::DisconnectFromServer() {
  if (client_->is_connected())
    client_->SignOut();
}

bool Conductor::IsConnectedPeer(int peer_id)
{
	ASSERT(peer_id != -1);
	if (peer_id_ == peer_id) return true;
	return false;
}

//本地连接指定节点
bool Conductor::ConnectToPeer(int peer_id) {
  ASSERT(peer_id_ == -1);
  ASSERT(peer_id != -1);
  
  if (peer_connection_.get() || InitializePeerConnection())
  {
	connectingIDs.push_back(peer_id);
    peer_id_ = peer_id;
    peer_connection_->CreateOffer(this, NULL);
	return true;
  } else {
    main_form_->Error("Failed to initialize PeerConnection");
	return false;
  }
}

void Conductor::DisconnectFromPeer(int peer_id)
{
	std::vector<int>::iterator result = std::find(connectedIDs.begin(), connectedIDs.end(), peer_id);
	if (result != connectedIDs.end()) {
		connectedIDs.erase(result);
	}
}

void Conductor::DisconnectPeers() {
  LOG(INFO) << __FUNCTION__;
  if (peer_connection_.get()) {
    client_->SendHangUp(peer_id_);
    DeletePeerConnection();
  }

  if (main_form_->IsWindow())
    main_form_->SwitchToPeerList(client_->peers());
}

// CreateSessionDescriptionObserver implementation.
void Conductor::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
  peer_connection_->SetLocalDescription(
      DummySetSessionDescriptionObserver::Create(), desc);

  std::string sdp;
  desc->ToString(&sdp);

  // For loopback test. To save some connecting delay.
  if (loopback_ && !loopbackrun_) {
    // Replace message type from "offer" to "answer"
    webrtc::SessionDescriptionInterface* session_description(
        webrtc::CreateSessionDescription("answer", sdp, nullptr));
	//session_description->description()->GetTransportInfoByName("audio")->description.connection_role = cricket::CONNECTIONROLE_ACTIVE;
	//session_description->description()->GetTransportInfoByName("video")->description.connection_role = cricket::CONNECTIONROLE_ACTIVE;
    peer_connection_->SetRemoteDescription(
        DummySetSessionDescriptionObserver::Create(), session_description);
	loopbackrun_ = true;
    return;
  }

  Json::StyledWriter writer;
  Json::Value jmessage;
  jmessage[kSessionDescriptionTypeName] = desc->type();
  jmessage[kSessionDescriptionSdpName] = sdp;

  std::string json_object = writer.write(jmessage);
  SendMessageToPeer(json_object);
}

void Conductor::OnFailure(const std::string& error) {
    LOG(LERROR) << error;
	loopbackrun_ = false;
}

// private function

void Conductor::PeerConnectingClosed(int peer_id)
{
	LOG(INFO) << "PEER_CONNECTION_CLOSED";
	DeletePeerConnection();

	ASSERT(active_streams_.empty());

	if (main_form_->IsWindow()) {
		if (client_->is_connected()) {
			main_form_->SwitchToPeerList(client_->peers());
		}
		else {
			main_form_->SwitchToConnectUI();
		}
	}
	else {
		DisconnectFromServer();
	}
}

void Conductor::SendMessageToPeer(std::string &MSG)
{
	LOG(INFO) << "SEND_MESSAGE_TO_PEER";
	std::string* msg = new std::string(MSG);
	if (msg) {
		// For convenience, we always run the message through the queue.
		// This way we can be sure that messages are sent to the server
		// in the same order they were signaled without much hassle.
		pending_messages_.push_back(msg);
	}

	if (!pending_messages_.empty() && !client_->IsSendingMessage()) {
		msg = pending_messages_.front();
		pending_messages_.pop_front();

		if (!client_->SendToPeer(peer_id_, *msg) && peer_id_ != -1) {
			LOG(LS_ERROR) << "SendToPeer failed";
			DisconnectFromServer();
		}
		delete msg;
	}

	if (!peer_connection_.get())
		peer_id_ = -1;
}

void Conductor::NewRemoteStreamAdded(webrtc::MediaStreamInterface* stream)
{
	webrtc::VideoTrackVector tracks = stream->GetVideoTracks();
	// Only render the first track.
	if (!tracks.empty()) {
		webrtc::VideoTrackInterface* track = tracks[0];
		main_form_->StartRemoteRenderer(track, peer_id_);
		connectedIDs.push_back(peer_id_);
		peer_id_ = -1;
	}
	stream->Release();
}

void Conductor::RemoteStreamRemoved(webrtc::MediaStreamInterface* stream)
{
	// Remote peer stopped sending a stream.
	stream->Release();
}