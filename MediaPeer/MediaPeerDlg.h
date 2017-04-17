
// MediaPeerDlg.h : header file
//

#pragma once
#include "..\MediaStreamApply\conductor.h"
#include "..\MediaStreamApply\VideoRender.h"
#include "SplitScreen.h"
#include "SplitVideoRenderer.h"
#include "..\MediaStreamApply\PeerConntionManager.h"

// CMediaPeerDlg dialog
class CMediaPeerDlg : public CDialogEx,
	public FormWindow,
	public VideoRendererObserver
{
// Construction
public:
	CMediaPeerDlg(CWnd* pParent = NULL);	// standard constructor
	~CMediaPeerDlg();
// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MEDIAPEER_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

protected:
	//FormWindow
	virtual void RegisterObserver(FormWindowObserver* observer) ;
	virtual void Error(std::string message) ;

	virtual bool IsWindow() ;

	virtual UI current_ui() ;
	virtual void SwitchToConnectUI() ;
	virtual void SwitchToPeerList(const Peers& peers) ;
	virtual void SwitchToStreamingUI() ;

	virtual void StartLocalRenderer(webrtc::VideoTrackInterface* local_video) ;
	virtual void StopLocalRenderer() ;
	virtual void StartRemoteRenderer(
		webrtc::VideoTrackInterface* remote_video, int id) ;
	virtual void StopRemoteRenderer(int id) ;

	virtual void onMessage(int msg_id, void* data) ;
	//VideoRendererObserver
	virtual void OnFrame(VideoRenderer * render);

	void Init();
	void Uninit();

	void EnableWidow();
	void FlushWindow();
	void FlushLocalWindow();
	void FlushRemoteWindow();
	void DrawSelf();
private:
	rtc::scoped_refptr<PeerConntionManager> conductor_;
	PeerConnectionTransform * transform_;
	FormWindowObserver* callback_;
	FormWindow::UI ui_;

	rtc::scoped_ptr<VideoRenderer> local_renderer_;
	rtc::scoped_ptr<VideoRenderer> remote_renderer_;

	std::map<int, SplitRenderer* > splitmap_;
	SplitScreen * splitControl_;
// Implementation
protected:
	HICON m_hIcon;
	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnDestroy(void);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonConnect();
	afx_msg void OnBnClickedButtonSignout();
	afx_msg void OnLbnSelchangeListPeer();
	afx_msg void OnLbnDblclkListPeer();
	afx_msg void OnCbnSelchangeComboLocal();
	afx_msg void OnCbnSelchangeComboRemote();
};
