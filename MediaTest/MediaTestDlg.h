
// MediaTestDlg.h : header file
//

#pragma once
#include "../MediaStreamApply/MediaStreamApply.h"
#include "../MediaStreamApply/VideoRender.h"

// CMediaTestDlg dialog
class CMediaTestDlg : public CDialogEx, public VideoRendererObserver
{
// Construction
public:
	CMediaTestDlg(CWnd* pParent = NULL);	// standard constructor
	~CMediaTestDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MEDIATEST_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	void DrawPicture(const BITMAPINFO bmi, const uint8_t* image);
private:
	MediaStreamApply apply;
	rtc::scoped_ptr<VideoRenderer> renderer;
	int index_;
private:
	virtual void OnFrame(VideoRenderer * render);
// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnCbnSelchangeCombo1();
};
