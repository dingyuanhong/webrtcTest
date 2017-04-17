/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_
#define WEBRTC_EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_
#pragma once

#include <deque>
#include <map>
#include <set>
#include <string>

#include "webrtc/api/mediastreaminterface.h"
#include "webrtc/api/peerconnectioninterface.h"
#include "webrtc/base/scoped_ptr.h"
#include "FormWindow.h"
#include "PeerConnectionTranform.h"
#include "class_api.h"

namespace webrtc {
class VideoCaptureModule;
}  // namespace webrtc

namespace cricket {
class VideoRenderer;
}  // namespace cricket

class CLASS_API Conductor
  : public webrtc::PeerConnectionObserver,
    public webrtc::CreateSessionDescriptionObserver,
    public PeerConnectionTransformObserver,
	public FormWindowObserver
{
public:
  enum CallbackID {
    MEDIA_CHANNELS_INITIALIZED = 1,
    PEER_CONNECTION_CLOSED,
    SEND_MESSAGE_TO_PEER,	//string
    NEW_STREAM_ADDED,		//MediaStreamInterface*
    STREAM_REMOVED,			//MediaStreamInterface*
	CONNECT_STATE,
  };

  Conductor(PeerConnectionTransform* client, FormWindow* main_form);
  webrtc::AudioDeviceModule * GetAudioDeviceModule() { return default_adm_.get(); }

  bool connection_active() const;

  virtual void Close();
  void Clear();
protected:
  ~Conductor();
  bool InitializePeerConnection();
  //开启本地回音
  bool ReinitializePeerConnectionForLoopback();
  //开启本地音视频
  void AddLocalStreams();
  //打开视频涉笔
  cricket::VideoCapturer* OpenVideoCaptureDevice();
  //创建P2P连接
  bool CreatePeerConnection(bool dtls);
  //删除P2P连接
  void DeletePeerConnection();
  
  //
  // PeerConnectionObserver implementation.
  //
  void OnSignalingChange(
      webrtc::PeerConnectionInterface::SignalingState new_state) override{};
  void OnAddStream(webrtc::MediaStreamInterface* stream) override;
  void OnRemoveStream(webrtc::MediaStreamInterface* stream) override;
  void OnDataChannel(webrtc::DataChannelInterface* channel) override {}
  void OnRenegotiationNeeded() override {}
  void OnIceConnectionChange(
      webrtc::PeerConnectionInterface::IceConnectionState new_state) override{};
  void OnIceGatheringChange(
      webrtc::PeerConnectionInterface::IceGatheringState new_state) override{};
  void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
  void OnIceConnectionReceivingChange(bool receiving) override {}

  //
  // PeerConnectionTransformObserver implementation.
  //
  virtual void OnSignedIn();

  virtual void OnDisconnected();
  //节点上线
  virtual void OnPeerConnected(int id, const std::string& name);
  //节点下线
  virtual void OnPeerDisconnected(int id);
  //接收节点消息
  virtual void OnMessageFromPeer(int peer_id, const std::string& message);

  virtual void OnMessageSent(int err);

  virtual void OnServerConnectionFailure();
  virtual void OnServerConnectionSuccess();
  //
  // FormWindowObserver implementation.
  //
  virtual void StartLogin(const std::string& server, int port);

  virtual void DisconnectFromServer();

  bool IsConnectedPeer(int peer_id);
  //连接至节点
  virtual bool ConnectToPeer(int peer_id);

  virtual void DisconnectFromPeer(int peer_id);

  virtual void DisconnectPeers();
  //
  // CreateSessionDescriptionObserver implementation.
  //
  virtual void OnSuccess(webrtc::SessionDescriptionInterface* desc);
  virtual void OnFailure(const std::string& error);
private:
	void PeerConnectingClosed(int peer_id);
	void SendMessageToPeer(std::string &MSG);
	void NewRemoteStreamAdded(webrtc::MediaStreamInterface* stream);
	void RemoteStreamRemoved(webrtc::MediaStreamInterface* stream);
protected:

  int peer_id_;
  bool loopback_;
  bool loopbackrun_;
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
      peer_connection_factory_;
  PeerConnectionTransform* client_;
  FormWindow* main_form_;
  std::deque<std::string*> pending_messages_;
  std::map<std::string, rtc::scoped_refptr<webrtc::MediaStreamInterface> >
      active_streams_;
  std::string server_;
  rtc::scoped_refptr<webrtc::AudioDeviceModule> default_adm_;


  std::vector<int>  connectingIDs;
  std::vector<int>	connectedIDs;
};

#endif  // WEBRTC_EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_
