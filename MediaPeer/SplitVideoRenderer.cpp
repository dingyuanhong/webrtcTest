#include "stdafx.h"
#include "SplitVideoRenderer.h"
#include "webrtc/base/arraysize.h"

void DrawPicture(CWnd * wnd, const BITMAPINFO bmi, const uint8_t* image)
{
	int height = abs(bmi.bmiHeader.biHeight);
	int width = bmi.bmiHeader.biWidth;
	if (image == NULL) return;

	CWnd * pPicture = wnd;
	if (pPicture == NULL) return;

	HDC hdc = ::GetDC(pPicture->GetSafeHwnd());
	if (hdc == NULL) return;

	RECT rc;
	pPicture->GetClientRect(&rc);

	{
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

		BitBlt(hdc, 0, 0, logical_area.x, logical_area.y,
			dc_mem, 0, 0, SRCCOPY);

		// Cleanup.
		::SelectObject(dc_mem, bmp_old);
		::DeleteObject(bmp_mem);
		::DeleteDC(dc_mem);
	}
	::ReleaseDC(pPicture->GetSafeHwnd(), hdc);
}

BEGIN_MESSAGE_MAP(SplitRenderer, SplitItem)
	ON_WM_PAINT()
END_MESSAGE_MAP()

void SplitRenderer::OnFrame(VideoRenderer * render)
{
	//PostMessage(WM_PAINT);
	InvalidateRect(NULL);
	return;

	OnDrawSelf();
}

void SplitRenderer::CreateRender(webrtc::VideoTrackInterface* remote_video)
{
	ASSERT(GetSafeHwnd() != NULL);
	ASSERT(IsWindow(GetSafeHwnd()));
	ASSERT(IsWindow(ctr_picture_->GetSafeHwnd()));

	renderer_.reset(new VideoRenderer(ctr_picture_->GetSafeHwnd(), 0, 0, remote_video));
	renderer_->RegisterObserver(this);
}

void SplitRenderer::Start()
{
	if (renderer_.get() != NULL)
	{
		renderer_->Begin();
	}
}

void SplitRenderer::Stop()
{
	if (renderer_.get() != NULL)
	{
		renderer_->End();
	}
}

SplitRenderer::~SplitRenderer()
{
	renderer_.reset();
}


void SplitRenderer::OnDrawSelf()
{
	if (!show_) {
		RECT rc;
		this->GetClientRect(&rc);
		CDC* dc = GetDC();
		dc->FillSolidRect(0, 0, rc.right, rc.bottom, RGB(0, 0, 0));
		ReleaseDC(dc);
		return;
	}
	int height = 0;
	int width = 0;
	bool drawBlack = true;
	if (renderer_.get() != NULL) {
		VideoRenderer * render = renderer_.get();
		AutoLock<VideoRenderer> local_lock(render);
		const BITMAPINFO& bmi = render->bmi();
		const uint8_t* image = render->image();
		height = abs(bmi.bmiHeader.biHeight);
		width = bmi.bmiHeader.biWidth;

		if (ctr_picture_ != NULL && height > 0 && width > 0) {
			drawBlack = false;
			DrawPicture(ctr_picture_, bmi, image);
		}
	}

	if(drawBlack && ctr_picture_ != NULL)
	{
		RECT rc;
		ctr_picture_->GetClientRect(&rc);
		CDC* dc = ctr_picture_->GetDC();
		if (dc != NULL) {
			dc->FillSolidRect(0, 0, rc.right, rc.bottom, RGB(0, 0, 0));
			ctr_picture_->ReleaseDC(dc);
		}
	}

	if (ctr_id_ != NULL) {
		RECT rc;
		ctr_id_->GetClientRect(&rc);

		CDC* dc = ctr_id_->GetDC();
		if (dc != NULL) {
			//dc->SetBkMode(TRANSPARENT);
			dc->SetBkColor(RGB(0, 0, 0));
			dc->FillSolidRect(0,0, rc.right,rc.bottom,RGB(0,0,0));
			dc->SetTextColor(RGB(255, 0, 255));
			//dc->SetTextAlign(dc->GetTextAlign() | TA_CENTER | VTA_CENTER);
			CString text = id_;
			if (width == 0 || height == 0) {
				text.AppendFormat(_T(" (???*???)"));
			}
			else {
				text.AppendFormat(_T(" (%d*%d)"), width, height);
			}
			dc->DrawText(text, &rc, DT_CENTER | DT_SINGLELINE);
			ctr_id_->ReleaseDC(dc);
		}
	}
}

void SplitRenderer::OnPaint(void)
{
	SplitItem::OnPaint();
	OnDrawSelf();
}