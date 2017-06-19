#include "stdafx.h"
#include "Configure.h"

#include "webrtc/base/ssladapter.h"
#include "webrtc/base/win32socketinit.h"
#include "webrtc/base/win32socketserver.h"
#include "webrtc/base/physicalsocketserver.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/signalthread.h"
#include "webrtc/base/sigslot.h"

#define USE_EXTERN
#ifdef USE_EXTERN
#include "webrtc\extern\VCMQmResolutionFactory.h"
#include "webrtc\modules\video_coding\qm_select.h"
#endif

rtc::Win32Thread * g_w32_thread = NULL;

void InitThread()
{
	rtc::EnsureWinsockInit();
	rtc::ThreadManager::Instance();
	if (g_w32_thread == NULL) {
		g_w32_thread = new rtc::Win32Thread();
		g_w32_thread->Start();
	}
	rtc::ThreadManager::Instance()->SetCurrentThread(g_w32_thread);
	//rtc::ThreadManager::Instance()->CurrentThread();
	rtc::InitializeSSL();
}

void UninitThread()
{
	rtc::ThreadManager::Instance()->SetCurrentThread(NULL);
	if (g_w32_thread != NULL) {
		g_w32_thread->Quit();
		g_w32_thread->Stop();
		delete g_w32_thread;
		g_w32_thread = NULL;
	}
	rtc::CleanupSSL();
}

#ifdef USE_EXTERN
namespace webrtc
{
	VCMQmResolution * VCMQmResolutionFactory::Create()
	{
		return new VCMQmResolution();
	}
}
#endif