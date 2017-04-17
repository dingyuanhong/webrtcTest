#pragma once

#include <string.h>
#include "PeerConnectionTranform.h"

class FormWindowObserver {
public:
	//登录
	virtual void StartLogin(const std::string& server, int port) = 0;
	//断连
	virtual void DisconnectFromServer() = 0;
	//连接至节点
	virtual bool ConnectToPeer(int peer_id) = 0;
	//节点是否连接
	virtual bool IsConnectedPeer(int peer_id) = 0;
	//节点断开连接
	virtual void DisconnectFromPeer(int peer_id) = 0;
	//断开所有节点连接
	virtual void DisconnectPeers() = 0;
	//关闭
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
	//开启本地数据渲染
	virtual void StartLocalRenderer(webrtc::VideoTrackInterface* local_video) = 0;
	//关闭本地数据渲染
	virtual void StopLocalRenderer() = 0;
	//开启远端数据渲染
	virtual void StartRemoteRenderer(
		webrtc::VideoTrackInterface* remote_video,int id) = 0;
	//停止远端数据渲染
	virtual void StopRemoteRenderer(int id) = 0;
	//消息
	virtual void onMessage(int msg_id, void* data) = 0;
};