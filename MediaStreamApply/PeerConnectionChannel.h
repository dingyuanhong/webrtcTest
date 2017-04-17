#pragma once

#include <deque>
#include <map>
#include <set>
#include <string>

#include "webrtc/api/mediastreaminterface.h"
#include "webrtc/api/peerconnectioninterface.h"
#include "webrtc/base/scoped_ptr.h"
#include "class_api.h"

#include "FormWindow.h"
#include "PeerConnectionTranform.h"

enum {
	LOCAL = 0,
	REMOTE
};

class PeerConnectionChannelObserver
{
public:
	virtual void OnSendMessage(int peer_id, std::string& message) = 0;
	virtual void OnAddedStream(int peer_id, webrtc::MediaStreamTrackInterface * track) = 0;
	virtual void OnRemovedStream(int peer_id) = 0;
	virtual void OnDeletePeer(int peer_id) = 0;
protected:
	virtual ~PeerConnectionChannelObserver() {}
};

class CLASS_API PeerConnectionChannel
	:public webrtc::PeerConnectionObserver,
	public webrtc::CreateSessionDescriptionObserver
{
public:
	PeerConnectionChannel(webrtc::PeerConnectionFactoryInterface*factory,int peer_id);
	virtual ~PeerConnectionChannel();

	void RegisterObserver(PeerConnectionChannelObserver* callback) { observer_ = callback; }
	//是否连接
	bool IsConnected();
	//存在本地流
	bool HasLocalStream();
	//初始化
	bool Initialize();
	//初始化本地音频汇播
	bool InitializeForLoopback();
	//添加本地流
	void AddLocalStreams(webrtc::MediaStreamInterface* stream);
	//开始P2P连接
	void Connect();
	//关闭
	void Close();
	//处理消息
	void OnMessage(const std::string & message);
protected:
	//
	// PeerConnectionObserver implementation.
	//
	void OnSignalingChange(
		webrtc::PeerConnectionInterface::SignalingState new_state) override {};
	void OnAddStream(webrtc::MediaStreamInterface* stream) override;
	void OnRemoveStream(webrtc::MediaStreamInterface* stream) override;
	void OnDataChannel(webrtc::DataChannelInterface* channel) override {}
	void OnRenegotiationNeeded() override {}
	void OnIceConnectionChange(
		webrtc::PeerConnectionInterface::IceConnectionState new_state) override {};
	void OnIceGatheringChange(
		webrtc::PeerConnectionInterface::IceGatheringState new_state) override {};
	void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
	void OnIceConnectionReceivingChange(bool receiving) override {}
	//
	// CreateSessionDescriptionObserver implementation.
	//
	virtual void OnSuccess(webrtc::SessionDescriptionInterface* desc);
	virtual void OnFailure(const std::string& error);
private:
	//发送消息给对方
	void SendMessageToPeer(std::string &MSG);
	//对端流添加
	void NewRemoteStreamAdded(webrtc::MediaStreamInterface* stream);
	//对端流移除
	void RemoteStreamRemoved(webrtc::MediaStreamInterface* stream);
	//创建P2P连接实例
	bool CreatePeerConnection(bool dtls);
private:
	int peer_id_;	//对端ID
	bool loopback_;	//音频回播

	rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;//p2p连接
	rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>	
		peer_connection_factory_;										//工厂类

	PeerConnectionChannelObserver *observer_;
};