#pragma once

#include <string.h>
#include "PeerConnectionTranform.h"

class FormWindowObserver {
public:
	//��¼
	virtual void StartLogin(const std::string& server, int port) = 0;
	//����
	virtual void DisconnectFromServer() = 0;
	//�������ڵ�
	virtual bool ConnectToPeer(int peer_id) = 0;
	//�ڵ��Ƿ�����
	virtual bool IsConnectedPeer(int peer_id) = 0;
	//�ڵ�Ͽ�����
	virtual void DisconnectFromPeer(int peer_id) = 0;
	//�Ͽ����нڵ�����
	virtual void DisconnectPeers() = 0;
	//�ر�
	virtual void Close() = 0;
protected:
	virtual ~FormWindowObserver() {}
};

class FormWindow
{
public:
	enum UI {
		CONNECT_TO_SERVER,
		LIST_PEERS,
		STREAMING,
	};

	virtual void RegisterObserver(FormWindowObserver* observer) = 0;
	virtual void Error(std::string message) = 0;

	virtual bool IsWindow() = 0;

	virtual UI current_ui() = 0;
	virtual void SwitchToConnectUI() = 0;
	virtual void SwitchToPeerList(const Peers& peers) = 0;
	virtual void SwitchToStreamingUI() = 0;
	//��������������Ⱦ
	virtual void StartLocalRenderer(webrtc::VideoTrackInterface* local_video) = 0;
	//�رձ���������Ⱦ
	virtual void StopLocalRenderer() = 0;
	//����Զ��������Ⱦ
	virtual void StartRemoteRenderer(
		webrtc::VideoTrackInterface* remote_video,int id) = 0;
	//ֹͣԶ��������Ⱦ
	virtual void StopRemoteRenderer(int id) = 0;
	//��Ϣ
	virtual void onMessage(int msg_id, void* data) = 0;
};