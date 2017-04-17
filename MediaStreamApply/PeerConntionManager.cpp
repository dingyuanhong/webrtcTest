#include "stdafx.h"
#include "webrtc/base/common.h"
#include "webrtc/base/json.h"
#include "webrtc/base/logging.h"
#include "webrtc/media/engine/webrtcvideocapturerfactory.h"
#include "webrtc/modules/video_capture/video_capture_factory.h"
#include "webrtc\modules\audio_device\audio_device_impl.h"

#include "PeerConntionManager.h"
#include "defaults.h"

PeerConntionManager::PeerConntionManager(PeerConnectionTransform* client, FormWindow* main_form)
	:client_(client), main_form_(main_form)
{
	client_->RegisterObserver(this);
	main_form_->RegisterObserver(this);
}

PeerConntionManager::~PeerConntionManager()
{
	ASSERT(peer_connection_factory_.get() == NULL);
	ASSERT(default_adm_.get() == NULL);
	ASSERT(local_stream_.get() == NULL);
	ASSERT(local_peer.get() == NULL);
	ASSERT(local_peer.get() == NULL);
}

void PeerConntionManager::Clear()
{
	Close();
	client_ = NULL;
	main_form_ = NULL;
}

//
// PeerConnectionTransformObserver implementation.
//
void PeerConntionManager::OnSignedIn()
{
	LOG(INFO) << __FUNCTION__;
	LocalLookBack();
	main_form_->SwitchToPeerList(client_->peers());
}

void PeerConntionManager::OnDisconnected()
{
	LOG(INFO) << __FUNCTION__;

	DisconnectPeers();
	if (main_form_->IsWindow())
		main_form_->SwitchToConnectUI();
}

//节点上线
void PeerConntionManager::OnPeerConnected(int id, const std::string& name)
{
	LOG(INFO) << __FUNCTION__;
	// Refresh the list if we're showing it.
	if (main_form_->current_ui() == FormWindow::LIST_PEERS)
		main_form_->SwitchToPeerList(client_->peers());
}

//节点下线
void PeerConntionManager::OnPeerDisconnected(int id)
{
	LOG(INFO) << __FUNCTION__;
	DisconnectFromPeer(id);

	// Refresh the list if we're showing it.
	if (main_form_->current_ui() == FormWindow::LIST_PEERS)
		main_form_->SwitchToPeerList(client_->peers());
}

//接收节点消息
void PeerConntionManager::OnMessageFromPeer(int peer_id, const std::string& message)
{
	ASSERT(!message.empty());
	
	PeerConnectionChannel * control = FindConnection(peer_id);
	if (control == NULL) {
		control = AddConnection(peer_id);
	}
	ASSERT(control != NULL);
	control->OnMessage(message);
}

void PeerConntionManager::OnMessageSent(int err)
{
	// Process the next pending message if any.
	std::string message;
	OnSendMessage(-1,message);
}

void PeerConntionManager::OnServerConnectionFailure()
{
	main_form_->Error(("Failed to connect to " + server_).c_str());
	main_form_->onMessage(CONNECT_STATE, (void*)FALSE);
}

void PeerConntionManager::OnServerConnectionSuccess()
{
	main_form_->onMessage(CONNECT_STATE, (void*)TRUE);
}

//
// FormWindowObserver implementation.
//
void PeerConntionManager::StartLogin(const std::string& server, int port)
{
	if (client_->is_connected())
		return;
	server_ = server;
	client_->Connect(server, port, GetPeerName());
}

void PeerConntionManager::DisconnectFromServer()
{
	if (client_->is_connected())
		client_->SignOut();
}

bool PeerConntionManager::IsConnectedPeer(int peer_id)
{
	std::map<int, PeerConnectionChannel *>::iterator it = remote_peers.find(peer_id);
	if (it != remote_peers.end())
	{
		return true;
	}
	return false;
}

//连接至节点
bool PeerConntionManager::ConnectToPeer(int peer_id)
{
	ASSERT(peer_id != -1);
	ASSERT(local_stream_.get() != NULL);

	std::map<int, PeerConnectionChannel *>::iterator it = remote_peers.find(peer_id);
	if (it != remote_peers.end())
	{
		return true;
	}

	PeerConnectionChannel * control = AddConnection(peer_id);
	if (control == NULL)
	{
		main_form_->Error("Failed to ConnectToPeer");
		return false;
	}
	control->Connect();

	return true;
}

void PeerConntionManager::DisconnectFromPeer(int peer_id)
{
	std::map<int, PeerConnectionChannel *>::iterator it = remote_peers.find(peer_id);
	if (it != remote_peers.end())
	{
		it->second->Close();
		delete it->second;
		remote_peers.erase(it);
	}
}

void PeerConntionManager::DisconnectPeers()
{
	std::map<int, PeerConnectionChannel *>::iterator it = remote_peers.begin();
	for (; it != remote_peers.end(); it++)
	{
		it->second->Close();
	}

	if (local_peer.get()) {
		local_peer = NULL;
	}
	local_stream_ = NULL;

	it = remote_peers.begin();
	for (; it != remote_peers.end(); it++)
	{
		delete it->second;
	}
	remote_peers.clear();

	for (size_t i = 0; i < pending_messages_.size(); i++) {
		delete pending_messages_[i].msg;
	}
	pending_messages_.clear();

	main_form_->StopLocalRenderer();

	peer_connection_factory_ = NULL;
	default_adm_ = NULL;
}

void PeerConntionManager::Close()
{
	ASSERT(client_ != NULL);
	client_->SignOut();
	DisconnectPeers();
}

//
//PeerConnectionControlObserver implementation
//
void PeerConntionManager::OnSendMessage(int peer_id, std::string& message)
{
	LOG(INFO) << "SEND_MESSAGE_TO_PEER";
	//需加锁

	if (message.length() > 0 && peer_id != -1) {
		// For convenience, we always run the message through the queue.
		// This way we can be sure that messages are sent to the server
		// in the same order they were signaled without much hassle.
		std::string* msg = new std::string(message);
		PendingMessage pmsg = { peer_id,msg };
		pending_messages_.push_back(pmsg);
	}

	if (!pending_messages_.empty() && !client_->IsSendingMessage()) {
		PendingMessage pmsg = pending_messages_.front();
		pending_messages_.pop_front();

		if (!client_->SendToPeer(pmsg.peer_id, *pmsg.msg) && pmsg.peer_id != -1) {
			LOG(LS_ERROR) << "SendToPeer failed";
			DisconnectFromServer();
		}
		delete pmsg.msg;
	}
}

void PeerConntionManager::OnAddedStream(int peer_id, webrtc::MediaStreamTrackInterface * track)
{
	main_form_->StartRemoteRenderer((webrtc::VideoTrackInterface*)track, peer_id);
}

void PeerConntionManager::OnRemovedStream(int peer_id)
{
	main_form_->StopRemoteRenderer(peer_id);
}

void PeerConntionManager::OnDeletePeer(int peer_id)
{
}

//private function
void PeerConntionManager::LocalLookBack()
{
	PeerConnectionChannel * control = AddConnection(client_->id());
	ASSERT(control != NULL);
	control->InitializeForLoopback();
	control->Connect();
}

PeerConnectionChannel * PeerConntionManager::FindConnection(int peer_id)
{
	std::map<int, PeerConnectionChannel *>::iterator it = remote_peers.find(peer_id);
	if (it != remote_peers.end())
	{
		PeerConnectionChannel * control = it->second;
		return control;
	}
	return NULL;
}

PeerConnectionChannel * PeerConntionManager::AddConnection(int peer_id)
{
	if (peer_connection_factory_.get() == NULL)
	{
		Initinalize();
		if (peer_connection_factory_.get() == NULL)
		{
			return NULL;
		}
	}

	ASSERT(peer_connection_factory_.get() != NULL);
	ASSERT(local_stream_.get() != NULL);

	std::map<int, PeerConnectionChannel *>::iterator it = remote_peers.find(peer_id);
	if (it != remote_peers.end())
	{
		PeerConnectionChannel * control = it->second;
		if (!control->IsConnected())
			control->Initialize();
		if(!control->HasLocalStream())
			control->AddLocalStreams(local_stream_);
		return control;
	}
	else
	{
		PeerConnectionChannel * control = new rtc::RefCountedObject<PeerConnectionChannel>(peer_connection_factory_.get(),peer_id);
		remote_peers.insert(std::pair<int, PeerConnectionChannel*>(peer_id, control));
		control->RegisterObserver(this);
		control->Initialize();
		control->AddLocalStreams(local_stream_);
		return control;
	}
}

void PeerConntionManager::Initinalize()
{
	ASSERT(peer_connection_factory_.get() == NULL);
	ASSERT(default_adm_.get() == NULL);

	default_adm_ = webrtc::AudioDeviceModuleImpl::Create(0);

	peer_connection_factory_ = webrtc::CreatePeerConnectionFactory(
		rtc::ThreadManager::Instance()->CurrentThread(), //worker_thread_,
		rtc::ThreadManager::Instance()->CurrentThread(),
		default_adm_.get(), NULL, NULL);

	if (!peer_connection_factory_.get()) {
		main_form_->Error("Failed to initialize PeerConnectionFactory");
		DisconnectPeers();
	}

	CreateLocalStreams();
}

void PeerConntionManager::CreateLocalStreams()
{
	ASSERT(!local_stream_.get());

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

	local_stream_ = stream;
}

cricket::VideoCapturer* PeerConntionManager::OpenVideoCaptureDevice()
{
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