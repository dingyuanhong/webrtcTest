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

#include "PeerConnectionChannel.h"


typedef struct PEDDINGMESSAGE{
	int peer_id;
	std::string * msg;
}PendingMessage;

class CLASS_API  PeerConntionManager
	:public rtc::RefCountInterface,
	public PeerConnectionTransformObserver,
	public PeerConnectionChannelObserver,
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

	PeerConntionManager(PeerConnectionTransform* client, FormWindow* main_form);
	webrtc::AudioDeviceModule * GetAudioDeviceModule() { return default_adm_.get(); }

	void Clear();
protected:
	virtual ~PeerConntionManager();
	//
	// PeerConnectionTransformObserver implementation.
	//
	virtual void OnSignedIn();

	virtual void OnDisconnected();
	//�ڵ�����
	virtual void OnPeerConnected(int id, const std::string& name);
	//�ڵ�����
	virtual void OnPeerDisconnected(int id);
	//���սڵ���Ϣ
	virtual void OnMessageFromPeer(int peer_id, const std::string& message);
	//��Ϣ���ʹ���
	virtual void OnMessageSent(int err);
	//�������������ʧ��
	virtual void OnServerConnectionFailure();
	//������������ӳɹ�
	virtual void OnServerConnectionSuccess();

	//
	// FormWindowObserver implementation.
	//
	//��¼
	virtual void StartLogin(const std::string& server, int port);
	//�˳�
	virtual void DisconnectFromServer();
	//�������ڵ�
	virtual bool ConnectToPeer(int peer_id);
	//�ڵ��˳�
	virtual void DisconnectFromPeer(int peer_id);
	//�ر����нڵ�����
	virtual void DisconnectPeers();
	//�ر�
	virtual void Close();

	//
	//PeerConnectionControlObserver implementation
	//
	void OnSendMessage(int peer_id,std::string& message);
	void OnAddedStream(int peer_id, webrtc::MediaStreamTrackInterface * track);
	void OnRemovedStream(int peer_id);
	void OnDeletePeer(int peer_id);
private:
	//���ػػ�����
	void LocalLookBack();
	//�ڵ��Ƿ�����
	bool IsConnectedPeer(int peer_id);
	//����P2P�ڵ�
	PeerConnectionChannel * FindConnection(int peer_id);
	//����µĽڵ�
	PeerConnectionChannel * AddConnection(int peer_id);
	//��ʼ��
	void Initinalize();
	//����������
	void CreateLocalStreams();
	//����Ƶ�ɼ��豸
	cricket::VideoCapturer* OpenVideoCaptureDevice();
private:
	rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
		peer_connection_factory_;									//����
	rtc::scoped_refptr<webrtc::AudioDeviceModule> default_adm_;		//��Ƶ�豸
	rtc::scoped_refptr<webrtc::MediaStreamInterface> local_stream_;	//������

	rtc::scoped_refptr<PeerConnectionChannel> local_peer;			//������Ƶ�ػ����Žڵ�
	std::map<int, PeerConnectionChannel *> remote_peers;			//Զ��P2P�ڵ�
	//�����͵���Ϣ
	std::deque<PendingMessage> pending_messages_;
	//�ⲿͨѶģ��
	PeerConnectionTransform* client_;
	//��ʾ������
	FormWindow* main_form_;

	//������
	std::string server_;
};