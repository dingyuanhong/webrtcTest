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
	//�Ƿ�����
	bool IsConnected();
	//���ڱ�����
	bool HasLocalStream();
	//��ʼ��
	bool Initialize();
	//��ʼ��������Ƶ�㲥
	bool InitializeForLoopback();
	//��ӱ�����
	void AddLocalStreams(webrtc::MediaStreamInterface* stream);
	//��ʼP2P����
	void Connect();
	//�ر�
	void Close();
	//������Ϣ
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
	//������Ϣ���Է�
	void SendMessageToPeer(std::string &MSG);
	//�Զ������
	void NewRemoteStreamAdded(webrtc::MediaStreamInterface* stream);
	//�Զ����Ƴ�
	void RemoteStreamRemoved(webrtc::MediaStreamInterface* stream);
	//����P2P����ʵ��
	bool CreatePeerConnection(bool dtls);
private:
	int peer_id_;	//�Զ�ID
	bool loopback_;	//��Ƶ�ز�

	rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;//p2p����
	rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>	
		peer_connection_factory_;										//������

	PeerConnectionChannelObserver *observer_;
};