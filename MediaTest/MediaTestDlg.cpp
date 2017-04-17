
// MediaTestDlg.cpp : implementation file
//


#include "stdafx.h"
#include "MediaTest.h"
#include "MediaTestDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifndef NOMINMAX
#undef max
#undef min
#endif

#include <algorithm>

#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/sigslot.h"
#include "webrtc/base/ssladapter.h"
#include "webrtc/base/win32socketinit.h"
#include "webrtc/base/win32socketserver.h"
#include "webrtc/base/physicalsocketserver.h"
#include "webrtc/base/signalthread.h"
#include "webrtc/api/peerconnection.h"
#include "webrtc/base/arraysize.h"

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMediaTestDlg dialog



CMediaTestDlg::CMediaTestDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_MEDIATEST_DIALOG, pParent)
{
	index_ = -1;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CMediaTestDlg::~CMediaTestDlg()
{
	if (renderer != NULL)
	{
		renderer->Clear();
	}
	apply.Close();
	renderer.reset();
}

void CMediaTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMediaTestDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CMediaTestDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CMediaTestDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CMediaTestDlg::OnBnClickedButton3)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CMediaTestDlg::OnCbnSelchangeCombo1)
END_MESSAGE_MAP()


// CMediaTestDlg message handlers

BOOL CMediaTestDlg::OnInitDialog()
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

	std::vector<std::string> device_names;
	int nCount = apply.GetDeviceNames();

	CComboBox * box = (CComboBox*)GetDlgItem(IDC_COMBO1);
	for (int i = 0; i < nCount;i++) {
		CStringA n(apply.GetDeviceName(i));
		CString strName;
		strName = n;
		box->AddString(strName);
	}

	apply.InitializePeerConnection();
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMediaTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMediaTestDlg::OnPaint()
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
	}
}

void CMediaTestDlg::DrawPicture(const BITMAPINFO bmi, const uint8_t* image)
{
	int height = abs(bmi.bmiHeader.biHeight);
	int width = bmi.bmiHeader.biWidth;

	CWnd * pPicture = GetDlgItem(IDC_STATIC1);
	if (pPicture == NULL) return;

	HDC hdc = ::GetDC(pPicture->GetSafeHwnd());
	if (hdc == NULL) return;

	RECT rc;
	pPicture->GetClientRect(&rc);

	if (image != NULL) {
		HDC dc_mem = ::CreateCompatibleDC(hdc);
		::SetStretchBltMode(dc_mem, HALFTONE);

		// Set the map mode so that the ratio will be maintained for us.
		HDC all_dc[] = { hdc, dc_mem };
		for (int i = 0; i < arraysize(all_dc); ++i) {
			SetMapMode(all_dc[i], MM_ISOTROPIC);
			SetWindowExtEx(all_dc[i], width, height, NULL);
			SetViewportExtEx(all_dc[i], rc.right, rc.bottom, NULL);
		}

		HBITMAP bmp_mem = ::CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
		HGDIOBJ bmp_old = ::SelectObject(dc_mem, bmp_mem);

		POINT logical_area = { rc.right, rc.bottom };
		DPtoLP(hdc, &logical_area, 1);

		HBRUSH brush = ::CreateSolidBrush(RGB(0, 0, 0));
		RECT logical_rect = { 0, 0, logical_area.x, logical_area.y };
		::FillRect(dc_mem, &logical_rect, brush);
		::DeleteObject(brush);

		int x = (logical_area.x / 2) - (width / 2);
		int y = (logical_area.y / 2) - (height / 2);

		StretchDIBits(dc_mem, x, y, width, height,
			0, 0, width, height, image, &bmi, DIB_RGB_COLORS, SRCCOPY);

		/*if ((rc.right - rc.left) > 200 && (rc.bottom - rc.top) > 200) {
			int thumb_width = bmi.bmiHeader.biWidth / 4;
			int thumb_height = abs(bmi.bmiHeader.biHeight) / 4;
			StretchDIBits(dc_mem,
				logical_area.x - thumb_width - 10,
				logical_area.y - thumb_height - 10,
				thumb_width, thumb_height,
				0, 0, bmi.bmiHeader.biWidth, -bmi.bmiHeader.biHeight,
				image, &bmi, DIB_RGB_COLORS, SRCCOPY);
		}*/

		BitBlt(hdc, 0, 0, logical_area.x, logical_area.y,
			dc_mem, 0, 0, SRCCOPY);

		// Cleanup.
		::SelectObject(dc_mem, bmp_old);
		::DeleteObject(bmp_mem);
		::DeleteDC(dc_mem);
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMediaTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CMediaTestDlg::OnBnClickedButton1()
{

}


void CMediaTestDlg::OnBnClickedButton2()
{
	CComboBox * box = (CComboBox*)GetDlgItem(IDC_COMBO1);
	int index = box->GetCurSel();
	if (index == -1) {
		MessageBox(_T("请选择设备"),_T("提示"), MB_OK);
		return;
	}
	if (index_ == index) return;
	index_ = index;
	if (renderer != NULL)
	{
		renderer->Clear();
	}
	apply.OpenStreams(index);
	
	CWnd * pPicture = GetDlgItem(IDC_STATIC1);
	CRect rect;
	pPicture->GetWindowRect(&rect);

	renderer.reset(new VideoRenderer(pPicture->GetSafeHwnd(),rect.Width(), rect.Height(), apply.GetVideoTrack()));
	renderer->RegisterObserver(this);
}


void CMediaTestDlg::OnBnClickedButton3()
{
	if (renderer != NULL)
	{
		renderer->Clear();
		renderer.reset();
	}
}


void CMediaTestDlg::OnCbnSelchangeCombo1()
{
	
}


void CMediaTestDlg::OnFrame(VideoRenderer * render)
{
	if (render != renderer.get()) return;
	AutoLock<VideoRenderer> local_lock(render);
	const BITMAPINFO& bmi = render->bmi();
	const uint8_t* image = render->image();
	DrawPicture(bmi,image);
}

