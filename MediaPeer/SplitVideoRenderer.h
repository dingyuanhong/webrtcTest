#pragma once
#include "SplitScreen.h"
#include "..\MediaStreamApply\VideoRender.h"


void DrawPicture(CWnd * wnd, const BITMAPINFO bmi, const uint8_t* image);

class SplitRenderer :
	public SplitItem,
	public VideoRendererObserver
{
public:
	virtual ~SplitRenderer();
	void CreateRender(webrtc::VideoTrackInterface* remote_video);
	void Start();
	void Stop();
	//VideoRendererObserver
	virtual void OnFrame(VideoRenderer * render);

	void OnDrawSelf();
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
private:
	
	rtc::scoped_ptr<VideoRenderer> renderer_;
};