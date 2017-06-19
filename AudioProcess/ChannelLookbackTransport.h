#pragma once

#include "webrtc/base/arraysize.h"
#include "webrtc/transport.h"

#include "webrtc\voice_engine\include\voe_base.h"
#include "webrtc\voice_engine\voice_engine_impl.h"
#include "webrtc\voice_engine\utility.h"

class TransportInput
{
public:
	// Start implementation of TransportData.
	virtual void IncomingRTPPacket(const uint8_t* incoming_rtp_packet,
		const size_t packet_length) = 0;

	virtual void IncomingRTCPPacket(const uint8_t* incoming_rtcp_packet,
		const size_t packet_length) = 0;
};

class ChannelLookbackTransport : public webrtc::Transport, TransportInput
{
public:
	ChannelLookbackTransport(webrtc::VoENetwork* network, int playoutChannel, int captureChannel)
		:network_(network), playoutChannel_(playoutChannel), captureChannel_(captureChannel)
	{
	}

	virtual bool SendRtp(const uint8_t* packet,
		size_t length,
		const webrtc::PacketOptions& options)
	{
		IncomingRTPPacket(packet, length);
		return true;
	}

	virtual bool SendRtcp(const uint8_t* packet, size_t length)
	{
		IncomingRTCPPacket(packet, length);
		return true;
	}

	// Start implementation of TransportData.
	void IncomingRTPPacket(const uint8_t* incoming_rtp_packet,
		const size_t packet_length) override
	{
		network_->ReceivedRTPPacket(playoutChannel_, incoming_rtp_packet, packet_length);
	}

	void IncomingRTCPPacket(const uint8_t* incoming_rtcp_packet,
		const size_t packet_length) override
	{
		network_->ReceivedRTCPPacket(captureChannel_, incoming_rtcp_packet, packet_length);
		network_->ReceivedRTCPPacket(playoutChannel_, incoming_rtcp_packet, packet_length);
	}
private:
	webrtc::VoENetwork* network_;
	int playoutChannel_;
	int captureChannel_;
};