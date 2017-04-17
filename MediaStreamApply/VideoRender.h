#pragma once

#include "webrtc\media\base\videoframe.h"
#include "webrtc\media\base\videosinkinterface.h"
#include "webrtc\api\mediastreaminterface.h"
#include "class_api.h"

class VideoRendererObserver;

template <typename T>
class AutoLock {
public:
	explicit AutoLock(T* obj) : obj_(obj) { obj_->Lock(); }
	~AutoLock() { obj_->Unlock(); }
protected:
	T* obj_;
};

class CLASS_API VideoRenderer : public rtc::VideoSinkInterface<cricket::VideoFrame> {
public:
	VideoRenderer(HWND wnd, int width, int height,
		rtc::scoped_refptr<webrtc::VideoTrackInterface> track_to_render);
	virtual ~VideoRenderer();
	void RegisterObserver(VideoRendererObserver * observer);
	void UnregisterObserver();
	void Clear();
	
	void Begin();
	void End();

	void Lock() {
		::EnterCriticalSection(&buffer_lock_);
	}

	void Unlock() {
		::LeaveCriticalSection(&buffer_lock_);
	}
	// VideoSinkInterface implementation
	void OnFrame(const cricket::VideoFrame& frame) override;
	void DrawPicture();

	const BITMAPINFO& bmi() const { return bmi_; }
	const uint8_t* image() const { return image_.get(); }
protected:
	void SetSize(int width, int height);

	enum {
		SET_SIZE,
		RENDER_FRAME,
	};

	HWND wnd_;
	BITMAPINFO bmi_;
	rtc::scoped_ptr<uint8_t[]> image_;
	CRITICAL_SECTION buffer_lock_;
	rtc::scoped_refptr<webrtc::VideoTrackInterface> rendered_track_;

	VideoRendererObserver *callback_;
};

class VideoRendererObserver
{
public:
	virtual void OnFrame(VideoRenderer * render) = 0;
protected:
	virtual ~VideoRendererObserver() {}
};