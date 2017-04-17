/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "stdafx.h"
#include <algorithm>
#include <memory>
#include <vector>
#include "LoopBackTransport.h"

#include "webrtc\voice_engine\include\voe_base.h"
#include "webrtc\voice_engine\channel.h"
#include "webrtc\voice_engine\voice_engine_impl.h"

namespace webrtc {

LoopBackTransport::LoopBackTransport(webrtc::VoiceEngine *engine, int playoutChannel, int captureChannel)
:engine_(engine), playoutChannel_(playoutChannel), captureChannel_(captureChannel),packet_loss_(0), count_(0)
{
	webrtc::voe::ChannelOwner Channel = ((webrtc::VoiceEngineImpl*)engine_)->channel_manager().GetChannel(captureChannel_);
	RtpReceiver* rtp_receiver = NULL;
	Channel.channel()->GetRtpRtcp(&rtp_rtcp_module_,&rtp_receiver);
}

void LoopBackTransport::DropEveryNthPacket(int n) {
  packet_loss_ = n;
}

bool LoopBackTransport::SendRtp(const uint8_t* data,
                                size_t len,
                                const PacketOptions& options) {
  count_++;
  if (packet_loss_ > 0) {
    if ((count_ % packet_loss_) == 0) {
      return true;
    }
  }

  webrtc::PacketTime optiontime;
  webrtc::voe::ChannelOwner channel = ((webrtc::VoiceEngineImpl*)engine_)->channel_manager().GetChannel(playoutChannel_);
  channel.channel()->ReceivedRTPPacket((const int8_t*)data, len, optiontime);
  return true;
}

bool LoopBackTransport::SendRtcp(const uint8_t* data, size_t len) {
	if (rtp_rtcp_module_ && rtp_rtcp_module_->IncomingRtcpPacket((const uint8_t*)data, len) < 0) {
		return false;
	}
	return true;
	webrtc::voe::ChannelOwner channel = ((webrtc::VoiceEngineImpl*)engine_)->channel_manager().GetChannel(captureChannel_);
	return channel.channel()->ReceivedRTCPPacket((const int8_t*)data, len);
}

bool NullTransport::SendRtp(const uint8_t* packet,
	size_t length,
	const PacketOptions& options) {
	return true;
}

bool NullTransport::SendRtcp(const uint8_t* packet, size_t length) {
	return true;
}

}  // namespace webrtc
