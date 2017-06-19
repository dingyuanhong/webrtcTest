/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef WEBRTC_MODULES_RTP_RTCP_TEST_TESTAPI_TEST_API_H_
#define WEBRTC_MODULES_RTP_RTCP_TEST_TESTAPI_TEST_API_H_

#define NOMINMAX

#include "webrtc/base/scoped_ptr.h"
#include "webrtc/common_types.h"
#include "webrtc/transport.h"
#include "webrtc/modules/rtp_rtcp/include/receive_statistics.h"
#include "webrtc/modules/rtp_rtcp/include/rtp_header_parser.h"
#include "webrtc/modules/rtp_rtcp/include/rtp_payload_registry.h"
#include "webrtc/modules/rtp_rtcp/include/rtp_receiver.h"
#include "webrtc/modules/rtp_rtcp/include/rtp_rtcp.h"
#include "webrtc/modules/rtp_rtcp/include/rtp_rtcp_defines.h"
#include "webrtc/voice_engine/include/voe_base.h"

namespace webrtc {

// This class sends all its packet straight to the provided RtpRtcp module.
// with optional packet loss.
class LoopBackTransport : public Transport {
 public:
	LoopBackTransport(VoiceEngine *engine, int playoutChannel, int captureChannel);
	void DropEveryNthPacket(int n);
	bool SendRtp(const uint8_t* data,
				size_t len,
				const PacketOptions& options) override;
	bool SendRtcp(const uint8_t* data, size_t len) override;

private:
	int count_;
	int packet_loss_;
	int playoutChannel_;
	int captureChannel_;
	webrtc::VoiceEngine *engine_;
	RtpRtcp* rtp_rtcp_module_;
};

class NullTransport : public Transport {
public:
	bool SendRtp(const uint8_t* packet,
		size_t length,
		const PacketOptions& options) override;
	bool SendRtcp(const uint8_t* packet, size_t length) override;
};

}  // namespace webrtc
#endif  // WEBRTC_MODULES_RTP_RTCP_TEST_TESTAPI_TEST_API_H_
