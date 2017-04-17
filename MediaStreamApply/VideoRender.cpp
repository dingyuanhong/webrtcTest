#include "stdafx.h"

#include <algorithm>

#include "videorender.h"

#include <math.h>

#include "webrtc/api/mediastreaminterface.h"
#include "webrtc/base/win32.h"
#include "webrtc/media/base/mediachannel.h"
#include "webrtc/media/base/videocommon.h"
#include "webrtc/media/base/videoframe.h"
#include "webrtc/base/arraysize.h"
#include "webrtc/base/common.h"
#include "webrtc/base/logging.h"

VideoRenderer::VideoRenderer(
	HWND wnd, int width, int height,
	rtc::scoped_refptr<webrtc::VideoTrackInterface> track_to_render)
	: wnd_(wnd), rendered_track_(track_to_render), callback_(NULL)
{
	::InitializeCriticalSection(&buffer_lock_);
	ZeroMemory(&bmi_, sizeof(bmi_));
	bmi_.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi_.bmiHeader.biPlanes = 1;
	bmi_.bmiHeader.biBitCount = 32;
	bmi_.bmiHeader.biCompression = BI_RGB;
	bmi_.bmiHeader.biWidth = width;
	bmi_.bmiHeader.biHeight = -height;
	bmi_.bmiHeader.biSizeImage = width * height *
		(bmi_.bmiHeader.biBitCount >> 3);
}

void VideoRenderer::RegisterObserver(VideoRendererObserver * observer)
{
	callback_ = observer;
}

void VideoRenderer::UnregisterObserver()
{
	callback_ = NULL;
}

void VideoRenderer::Begin()
{
	rendered_track_->AddOrUpdateSink(this, rtc::VideoSinkWants());
}

void VideoRenderer::End()
{
	if (rendered_track_.get() != NULL) rendered_track_->RemoveSink(this);
}

VideoRenderer::~VideoRenderer() {
	::DeleteCriticalSection(&buffer_lock_);
}

void VideoRenderer::Clear()
{
	if (rendered_track_ != NULL && rendered_track_.get() != NULL) rendered_track_->RemoveSink(this);
	rendered_track_ = NULL;
}

void VideoRenderer::SetSize(int width, int height) {
	if (width == bmi_.bmiHeader.biWidth && height == abs(bmi_.bmiHeader.biHeight)) {
		return;
	}
	bmi_.bmiHeader.biWidth = width;
	bmi_.bmiHeader.biHeight = -height;
	bmi_.bmiHeader.biSizeImage = width * height *
		(bmi_.bmiHeader.biBitCount >> 3);
	image_.reset(new uint8_t[bmi_.bmiHeader.biSizeImage]);
}

void VideoRenderer::OnFrame(const cricket::VideoFrame& video_frame) 
{
	
	{
		AutoLock<VideoRenderer> lock(this);

		const cricket::VideoFrame* frame =
			video_frame.GetCopyWithRotationApplied();
		SetSize(frame->width(), frame->height());

		ASSERT(image_.get() != NULL);
		frame->ConvertToRgbBuffer(cricket::FOURCC_ARGB,
			image_.get(),
			bmi_.bmiHeader.biSizeImage,
			bmi_.bmiHeader.biWidth *
			bmi_.bmiHeader.biBitCount / 8);
	}

	if (callback_ != NULL) {
		callback_->OnFrame(this);
	}
	else 
	{
		this->DrawPicture();
	}
}

void VideoRenderer::DrawPicture()
{
	int height = abs(this->bmi_.bmiHeader.biHeight);
	int width = this->bmi_.bmiHeader.biWidth;
	if (height == 0 || width == 0) return;

	HDC hdc = ::GetDC(this->wnd_);
	if (hdc == NULL) return;

	RECT rc;
	::GetClientRect(this->wnd_, &rc);
	const uint8_t* imagedata = this->image();

	if (imagedata != NULL) {
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
			0, 0, width, height, imagedata, &(this->bmi_), DIB_RGB_COLORS, SRCCOPY);

		BitBlt(hdc, 0, 0, logical_area.x, logical_area.y,
			dc_mem, 0, 0, SRCCOPY);

		// Cleanup.
		::SelectObject(dc_mem, bmp_old);
		::DeleteObject(bmp_mem);
		::DeleteDC(dc_mem);
	}
}

