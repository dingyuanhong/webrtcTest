
// MediaPeerDlg.cpp : implementation file
//


#include "stdafx.h"
#include "MediaPeer.h"
#include "MediaPeerDlg.h"
#include "afxdialogex.h"

#include "webrtc/base/ssladapter.h"
#include "webrtc/base/win32socketinit.h"
#include "webrtc/base/win32socketserver.h"
#include "webrtc/base/arraysize.h"
#include "webrtc/modules/audio_device/include/audio_device.h"

#include "..\MediaStreamApply\CodeTransport.h"
#include "..\MediaStreamApply\peer_connection_client.h"
#include "..\MediaStreamApply\conductor.h"
#include "..\MediaStreamApply\Configure.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

void CMediaPeerDlg::Init()
{
	InitThread();

	ui_ = CONNECT_TO_SERVER;
	transform_ = new PeerConnectionClient();
	conductor_ = new rtc::RefCountedObject<PeerConntionManager>(transform_, this);
}

void CMediaPeerDlg::Uninit()
{
	UninitThread();
}

void CMediaPeerDlg::RegisterObserver(FormWindowObserver* observer)
{
	callback_ = observer;
}

void CMediaPeerDlg::Error(std::string message)
{
	GetDlgItem(IDC_STATIC_ERROR)->SetWindowText(AToString(message).c_str());
}

bool CMediaPeerDlg::IsWindow()
{
	return ::IsWindow(GetSafeHwnd());
}

FormWindow::UI CMediaPeerDlg::current_ui()
{
	return ui_;
}

void CMediaPeerDlg::SwitchToConnectUI()
{
	ASSERT(IsWindow());
	ui_ = CONNECT_TO_SERVER;
	EnableWidow();
}

void CMediaPeerDlg::SwitchToPeerList(const Peers& peers)
{
	ASSERT(IsWindow());

	ui_ = LIST_PEERS;

	EnableWidow();
	FlushWindow();

	CListBox * list = (CListBox*)GetDlgItem(IDC_LIST_PEER);
	list->ResetContent();

	if (list->GetCount() > 0) {
		for (int i = list->GetCount(); i >= 0; i++)
			list->DeleteString(i);
	}
	

	Peers::const_iterator i = peers.begin();
	for (; i != peers.end(); ++i) {
		CString value = AToString(i->second).c_str();
		value.AppendFormat(_T("(%d)"), i->first);
		list->AddString(value);
		list->SetItemData(list->GetCount() - 1, i->first);
	}
}

void CMediaPeerDlg::SwitchToStreamingUI()
{
	ASSERT(IsWindow());
	ui_ = STREAMING;
	EnableWidow();
}

void CMediaPeerDlg::StartLocalRenderer(webrtc::VideoTrackInterface* local_video)
{
	local_renderer_.reset(new VideoRenderer(GetDlgItem(IDC_STATIC_LOCAL)->GetSafeHwnd(), 1, 1, local_video));
	local_renderer_->RegisterObserver(this);
	CComboBox * combox = (CComboBox*)GetDlgItem(IDC_COMBO_LOCAL);
	combox->Clear();
	rtc::scoped_refptr<webrtc::AudioDeviceModule> devices = conductor_->GetAudioDeviceModule();
	int16_t nDevices = devices->RecordingDevices();
	for (int16_t i = 0; i < nDevices; i++)
	{
		char name[webrtc::kAdmMaxDeviceNameSize] = { 0 };
		char guid[webrtc::kAdmMaxGuidSize] = { 0 };

		devices->RecordingDeviceName(i, name, guid);
		std::string utf8 = name;
		std::string Name = UTF82ASCII(utf8);
		CString strName = AToString(Name).c_str();

		combox->AddString(strName);
	}
	combox->SetCurSel(0);

	if (devices->Recording()) {
		devices->StopRecording();
		devices->SetRecordingDevice(0);
		devices->InitRecording();
		devices->StartRecording();
	}
}

void CMediaPeerDlg::StopLocalRenderer()
{
	local_renderer_.reset();

	CComboBox * combox = (CComboBox*)GetDlgItem(IDC_COMBO_LOCAL);
	combox->ResetContent();
	if (combox->GetCount() > 0) {
		for (int i = combox->GetCount(); i >= 0; i++)
			combox->DeleteString(i);
	}

	FlushLocalWindow();
}

void CMediaPeerDlg::StartRemoteRenderer(
	webrtc::VideoTrackInterface* remote_video, int id)
{
	if (remote_renderer_.get() != NULL) return;
	if (id == transform_->id()) return;

	SplitRenderer * render = new SplitRenderer();
	TCHAR number[64];
	_itot(id, number, 10);
	render->SetID(number);
	render->Create(WS_CHILDWINDOW | WS_EX_COMPOSITED, CRect(-10,-10,0,0), splitControl_);
	render->CreateRender(remote_video);
	splitmap_.insert(std::pair<int, SplitRenderer* >(id, render));
	splitControl_->AddItem(render);
	render->ShowWindow(SW_SHOW);
	render->Start();
	GetDlgItem(IDC_LIST_PEER)->EnableWindow(TRUE);

	/*remote_renderer_.reset(new VideoRenderer(GetDlgItem(IDC_STATIC_REMOTE)->GetSafeHwnd(), 1, 1, remote_video, this));

	GetDlgItem(IDC_LIST_PEER)->EnableWindow(TRUE);*/

	if (splitmap_.size() <= 1) {
		CComboBox * combox = (CComboBox*)GetDlgItem(IDC_COMBO_REMOTE);
		combox->Clear();
		rtc::scoped_refptr<webrtc::AudioDeviceModule> devices = conductor_->GetAudioDeviceModule();
		int16_t nDevices = devices->PlayoutDevices();
		for (int16_t i = 0; i < nDevices; i++)
		{
			char name[webrtc::kAdmMaxDeviceNameSize] = { 0 };
			char guid[webrtc::kAdmMaxGuidSize] = { 0 };

			devices->PlayoutDeviceName(i, name, guid);
			std::string utf8 = name;
			std::string Name = UTF82ASCII(utf8);
			CString strName = AToString(Name).c_str();
			combox->AddString(strName);
		}
		combox->SetCurSel(0);

		if (devices->Playing()) {
			devices->StopPlayout();
			devices->SetPlayoutDevice(0);
			devices->InitPlayout();
			devices->StartPlayout();
		}
	}
}

void CMediaPeerDlg::StopRemoteRenderer(int id)
{
	std::map<int, SplitRenderer* >::iterator it =  splitmap_.find(id);
	if (it != splitmap_.end())
	{
		splitControl_->RemoveItem(it->second);
		it->second->Stop();
		it->second->ShowWindow(SW_HIDE);
		it->second->DestroyWindow();
		delete it->second;
		splitmap_.erase(it);
	}

	if (splitControl_ != NULL) {
		splitControl_->InvalidateRect(NULL, TRUE);
		splitControl_->UpdateWindow();
	}

	remote_renderer_.reset();

	if (splitmap_.size() <= 0) {
		CComboBox * combox = (CComboBox*)GetDlgItem(IDC_COMBO_REMOTE);
		combox->ResetContent();
		if (combox->GetCount() > 0) {
			for (int i = combox->GetCount(); i >= 0; i++)
				combox->DeleteString(i);
		}
	}
	FlushRemoteWindow();
}

void CMediaPeerDlg::onMessage(int msg_id, void* data)
{
	if (msg_id == Conductor::CONNECT_STATE) {
		if (data == NULL) {
			GetDlgItem(IDC_BUTTON_CONNECT)->EnableWindow(TRUE);
			GetDlgItem(IDC_BUTTON_SIGNOUT)->EnableWindow(FALSE);

			GetDlgItem(IDC_LIST_PEER)->EnableWindow(FALSE);
		}
		else 
		{
			
		}
	}
}

//VideoRendererObserver
void CMediaPeerDlg::OnFrame(VideoRenderer * render)
{
	RECT rc;
	GetDlgItem(IDC_STATIC_LOCAL)->GetWindowRect(&rc);
	ScreenToClient(&rc);
	InvalidateRect(&rc,FALSE);
	return;
}

void CMediaPeerDlg::EnableWidow()
{
	if (ui_ == CONNECT_TO_SERVER) {
		GetDlgItem(IDC_BUTTON_CONNECT)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_SIGNOUT)->EnableWindow(FALSE);

		GetDlgItem(IDC_LIST_PEER)->EnableWindow(FALSE);

		GetDlgItem(IDC_STATIC_STATE)->SetWindowText(_T(""));
		GetDlgItem(IDC_STATIC_ERROR)->SetWindowText(_T(""));

		CListBox * list = (CListBox*)GetDlgItem(IDC_LIST_PEER);
		list->ResetContent();
		for (int i = 0; i < list->GetCount(); i++)
			list->DeleteString(i);
	}else
	if (ui_ == LIST_PEERS) {
		GetDlgItem(IDC_BUTTON_CONNECT)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_SIGNOUT)->EnableWindow(TRUE);

		GetDlgItem(IDC_LIST_PEER)->EnableWindow(TRUE);

		GetDlgItem(IDC_STATIC_STATE)->SetWindowText(_T(""));
		GetDlgItem(IDC_STATIC_ERROR)->SetWindowText(_T(""));
	}else
	if (ui_ == STREAMING) {
		GetDlgItem(IDC_BUTTON_CONNECT)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_SIGNOUT)->EnableWindow(TRUE);

		GetDlgItem(IDC_LIST_PEER)->EnableWindow(TRUE);

		GetDlgItem(IDC_STATIC_STATE)->SetWindowText(_T(""));
		GetDlgItem(IDC_STATIC_ERROR)->SetWindowText(_T(""));
	}
}

void CMediaPeerDlg::FlushWindow()
{
	if (!(ui_ == CONNECT_TO_SERVER)) return;

	FlushLocalWindow();
	FlushRemoteWindow();
}

void CMediaPeerDlg::FlushLocalWindow()
{
	GetDlgItem(IDC_STATIC_LOCAL)->InvalidateRect(NULL, TRUE);
	GetDlgItem(IDC_STATIC_LOCAL)->UpdateWindow();
	RECT rc;
	GetDlgItem(IDC_STATIC_LOCAL)->GetWindowRect(&rc);
	ScreenToClient(&rc);
	InvalidateRect(&rc);
	UpdateWindow();
}

void CMediaPeerDlg::FlushRemoteWindow()
{
	GetDlgItem(IDC_STATIC_REMOTE)->InvalidateRect(NULL);
	GetDlgItem(IDC_STATIC_REMOTE)->UpdateWindow();
	if (splitControl_ != NULL) {
		splitControl_->InvalidateRect(NULL);
		splitControl_->UpdateWindow();
	}
	RECT rc;
	GetDlgItem(IDC_STATIC_REMOTE)->GetWindowRect(&rc);
	ScreenToClient(&rc);
	InvalidateRect(&rc);
	UpdateWindow();
}

void CMediaPeerDlg::DrawSelf()
{
	if (current_ui() == CONNECT_TO_SERVER) return;
	VideoRenderer *render = local_renderer_.get();
	if (render == NULL) return;

	AutoLock<VideoRenderer> local_lock(render);
	const BITMAPINFO& bmi = render->bmi();
	const uint8_t* image = render->image();
	CWnd wnd;
	wnd.Attach(GetDlgItem(IDC_STATIC_LOCAL)->GetSafeHwnd());
	DrawPicture(&wnd, bmi, image);
	wnd.Detach();
}

// CMediaPeerDlg dialog
CMediaPeerDlg::CMediaPeerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_MEDIAPEER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}


CMediaPeerDlg::~CMediaPeerDlg()
{
	Uninit();

	if (splitControl_ != NULL) {
		delete splitControl_;
		splitControl_ = NULL;
	}
}

void CMediaPeerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMediaPeerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_CONNECT, &CMediaPeerDlg::OnBnClickedButtonConnect)
	ON_BN_CLICKED(IDC_BUTTON_SIGNOUT, &CMediaPeerDlg::OnBnClickedButtonSignout)
	ON_LBN_SELCHANGE(IDC_LIST_PEER, &CMediaPeerDlg::OnLbnSelchangeListPeer)
	ON_LBN_DBLCLK(IDC_LIST_PEER, &CMediaPeerDlg::OnLbnDblclkListPeer)
	ON_CBN_SELCHANGE(IDC_COMBO_LOCAL, &CMediaPeerDlg::OnCbnSelchangeComboLocal)
	ON_CBN_SELCHANGE(IDC_COMBO_REMOTE, &CMediaPeerDlg::OnCbnSelchangeComboRemote)
END_MESSAGE_MAP()


// CMediaPeerDlg message handlers
void CMediaPeerDlg::OnDestroy(void)
{
	{
		if (callback_ != NULL)
		{
			callback_->DisconnectFromServer();
		}
	}

	//if (current_ui() == STREAMING) 
	{
		if (callback_ != NULL)
		{
			callback_->DisconnectPeers();
		}
	}
	
	if (transform_ != NULL)
	{
		MSG lpMsg;
		while (TRUE) {
			if (GetMessage(&lpMsg,NULL,0,0)) {
				TranslateMessage(&lpMsg);
				DispatchMessage(&lpMsg);
			}

			if (!transform_->is_connected()) 
				break;
		}
	}

	if (conductor_.get() != NULL) {
		conductor_->Clear();
	}

	if (transform_ != NULL)
	{
		delete transform_;
		transform_ = NULL;
	}
	splitControl_->Detach();
}

BOOL CMediaPeerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	GetDlgItem(IDC_EDIT_IP)->SetWindowText(_T("localhost"));
	GetDlgItem(IDC_EDIT_PORT)->SetWindowText(_T("8888"));

	splitControl_ = new SplitScreen();
	splitControl_->Attach(GetDlgItem(IDC_STATIC_REMOTE)->GetSafeHwnd());

	Init();
	EnableWidow();
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMediaPeerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMediaPeerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
		DrawSelf();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMediaPeerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CMediaPeerDlg::OnBnClickedButtonConnect()
{
	CString strIP;
	CString strPort;
	GetDlgItem(IDC_EDIT_IP)->GetWindowText(strIP);
	GetDlgItem(IDC_EDIT_PORT)->GetWindowText(strPort);
	if (strIP.GetLength() == 0) {
		AfxMessageBox(_T("请输入IP地址"));
		return;
	}
	if (strPort.GetLength() == 0) {
		AfxMessageBox(_T("请输入端口"));
		return;
	}
	int port = _ttoi(strPort.GetBuffer());
	if (port < 0)
	{
		AfxMessageBox(_T("请输入端口"));
		return;
	}

	if (callback_ != NULL)
	{
		std::string ip = toString(strIP.GetBuffer());
		callback_->StartLogin(ip,port);

		GetDlgItem(IDC_BUTTON_CONNECT)->EnableWindow(FALSE);
	}
}

void CMediaPeerDlg::OnBnClickedButtonSignout()
{
	if (current_ui() == STREAMING) {
		if (callback_ != NULL)
		{
			callback_->DisconnectPeers();

			ui_ = LIST_PEERS;
			EnableWidow();
			FlushWindow();
		}
	}
	else {
		if (callback_ != NULL)
		{
			callback_->DisconnectFromServer();

			ui_ = CONNECT_TO_SERVER;
			EnableWidow();
		}
	}
}

void CMediaPeerDlg::OnLbnSelchangeListPeer()
{

}

void CMediaPeerDlg::OnLbnDblclkListPeer()
{
	CListBox * list = (CListBox*)GetDlgItem(IDC_LIST_PEER);

	int index = list->GetCurSel();
	LRESULT peer_id = list->GetItemData(index);
	if (peer_id != -1 && callback_) {
		bool ret  = callback_->IsConnectedPeer(peer_id);
		if (!ret) {
			ret = callback_->ConnectToPeer(peer_id);
			if (ret) {
				GetDlgItem(IDC_LIST_PEER)->EnableWindow(FALSE);
			}
		}
	}
}

void CMediaPeerDlg::OnCbnSelchangeComboLocal()
{
	CComboBox * combox = (CComboBox*)GetDlgItem(IDC_COMBO_LOCAL);
	int index = combox->GetCurSel();
	if (index == -1) return;
	rtc::scoped_refptr<webrtc::AudioDeviceModule> devices = conductor_->GetAudioDeviceModule();

	if (devices->Recording()) {
		devices->StopRecording();
		devices->SetRecordingDevice(index);
		devices->InitRecording();
		devices->StartRecording();
	}
}

void CMediaPeerDlg::OnCbnSelchangeComboRemote()
{
	CComboBox * combox = (CComboBox*)GetDlgItem(IDC_COMBO_REMOTE);
	int index = combox->GetCurSel();
	if (index == -1) return;

	rtc::scoped_refptr<webrtc::AudioDeviceModule> devices = conductor_->GetAudioDeviceModule();
	
	if (devices->Playing()) {
		devices->StopPlayout();
		devices->SetPlayoutDevice(index);
		devices->InitPlayout();
		devices->StartPlayout();
	}
}
