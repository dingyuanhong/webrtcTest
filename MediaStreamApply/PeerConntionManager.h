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
	//节点上线
	virtual void OnPeerConnected(int id, const std::string& name);
	//节点下线
	virtual void OnPeerDisconnected(int id);
	//接收节点消息
	virtual void OnMessageFromPeer(int peer_id, const std::string& message);
	//消息发送错误
	virtual void OnMessageSent(int err);
	//信令服务器连接失败
	virtual void OnServerConnectionFailure();
	//信令服务器连接成功
	virtual void OnServerConnectionSuccess();

	//
	// FormWindowObserver implementation.
	//
	//登录
	virtual void StartLogin(const std::string& server, int port);
	//退出
	virtual void DisconnectFromServer();
	//连接至节点
	virtual bool ConnectToPeer(int peer_id);
	//节点退出
	virtual void DisconnectFromPeer(int peer_id);
	//关闭所有节点连接
	virtual void DisconnectPeers();
	//关闭
	virtual void Close();

	//
	//PeerConnectionControlObserver implementation
	//
	void OnSendMessage(int peer_id,std::string& message);
	void OnAddedStream(int peer_id, webrtc::MediaStreamTrackInterface * track);
	void OnRemovedStream(int peer_id);
	void OnDeletePeer(int peer_id);
private:
	//本地回环连接
	void LocalLookBack();
	//节点是否连接
	bool IsConnectedPeer(int peer_id);
	//搜索P2P节点
	PeerConnectionChannel * FindConnection(int peer_id);
	//添加新的节点
	PeerConnectionChannel * AddConnection(int peer_id);
	//初始化
	void Initinalize();
	//创建本地刘
	void CreateLocalStreams();
	//打开视频采集设备
	cricket::VideoCapturer* OpenVideoCaptureDevice();
private:
	rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
		peer_connection_factory_;									//工厂
	rtc::scoped_refptr<webrtc::AudioDeviceModule> default_adm_;		//音频设备
	rtc::scoped_refptr<webrtc::MediaStreamInterface> local_stream_;	//本地流

	rtc::scoped_refptr<PeerConnectionChannel> local_peer;			//本地音频回环播放节点
	std::map<int, PeerConnectionChannel *> remote_peers;			//远端P2P节点
	//待发送的消息
	std::deque<PendingMessage> pending_messages_;
	//外部通讯模块
	PeerConnectionTransform* client_;
	//显示主窗体
	FormWindow* main_form_;

	//服务名
	std::string server_;
};