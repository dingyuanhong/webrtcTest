#pragma once

#pragma once

#include <map>
#include <string>

#include "webrtc/base/nethelpers.h"
#include "webrtc/base/physicalsocketserver.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/signalthread.h"
#include "webrtc/base/sigslot.h"

#include "class_api.h"

typedef std::map<int, std::string> Peers;

struct PeerConnectionTransformObserver {
	//��¼�ɹ�
	virtual void OnSignedIn() = 0;  // Called when we're logged on.
	//�Ͽ�����
	virtual void OnDisconnected() = 0;
	//�ڵ�����
	virtual void OnPeerConnected(int id, const std::string& name) = 0;
	//�ڵ�Ͽ�
	virtual void OnPeerDisconnected(int peer_id) = 0;
	//�ڵ���Ϣ
	virtual void OnMessageFromPeer(int peer_id, const std::string& message) = 0;
	//��Ϣ����ʧ��
	virtual void OnMessageSent(int err) = 0;
	//����������ʧ��
	virtual void OnServerConnectionFailure() = 0;
	//���������ӳɹ�
	virtual void OnServerConnectionSuccess() = 0;
protected:
	virtual ~PeerConnectionTransformObserver() {}
};

class PeerConnectionTransform
{
public:
	virtual ~PeerConnectionTransform() {}
	//ע��ص�
	virtual void RegisterObserver(PeerConnectionTransformObserver* callback) = 0;
	
	virtual int id() const = 0;
	//�Ƿ�����
	virtual bool is_connected() const = 0;
	//�������������
	virtual void Connect(const std::string& server, int port,
		const std::string& client_name) = 0 ;
	//�˳�
	virtual bool SignOut() = 0;
	//���нڵ�
	virtual const Peers& peers() const = 0;
	//������Ϣ���ڵ�
	virtual bool SendToPeer(int peer_id, const std::string& message) = 0;
	//�Ҷ�
	virtual bool SendHangUp(int peer_id) = 0;
	//�Ƿ����ڷ�����Ϣ
	virtual bool IsSendingMessage() = 0;
};