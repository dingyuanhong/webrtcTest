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
	//登录成功
	virtual void OnSignedIn() = 0;  // Called when we're logged on.
	//断开连接
	virtual void OnDisconnected() = 0;
	//节点连接
	virtual void OnPeerConnected(int id, const std::string& name) = 0;
	//节点断开
	virtual void OnPeerDisconnected(int peer_id) = 0;
	//节点消息
	virtual void OnMessageFromPeer(int peer_id, const std::string& message) = 0;
	//消息发送失败
	virtual void OnMessageSent(int err) = 0;
	//服务器连接失败
	virtual void OnServerConnectionFailure() = 0;
	//服务器连接成功
	virtual void OnServerConnectionSuccess() = 0;
protected:
	virtual ~PeerConnectionTransformObserver() {}
};

class PeerConnectionTransform
{
public:
	virtual ~PeerConnectionTransform() {}
	//注册回调
	virtual void RegisterObserver(PeerConnectionTransformObserver* callback) = 0;
	
	virtual int id() const = 0;
	//是否连接
	virtual bool is_connected() const = 0;
	//连接信令服务器
	virtual void Connect(const std::string& server, int port,
		const std::string& client_name) = 0 ;
	//退出
	virtual bool SignOut() = 0;
	//所有节点
	virtual const Peers& peers() const = 0;
	//发送消息给节点
	virtual bool SendToPeer(int peer_id, const std::string& message) = 0;
	//挂断
	virtual bool SendHangUp(int peer_id) = 0;
	//是否正在发送消息
	virtual bool IsSendingMessage() = 0;
};