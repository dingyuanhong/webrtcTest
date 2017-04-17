/*
 *  Copyright 2011 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_EXAMPLES_PEERCONNECTION_CLIENT_PEER_CONNECTION_CLIENT_H_
#define WEBRTC_EXAMPLES_PEERCONNECTION_CLIENT_PEER_CONNECTION_CLIENT_H_
#pragma once

#include <map>
#include <string>

#include "webrtc/base/nethelpers.h"
#include "webrtc/base/physicalsocketserver.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/signalthread.h"
#include "webrtc/base/sigslot.h"

#include "class_api.h"
#include "PeerConnectionTranform.h"

class CLASS_API PeerConnectionClient : public sigslot::has_slots<>,
							 public PeerConnectionTransform,
                             public rtc::MessageHandler {
 public:
  enum State {
    NOT_CONNECTED,
    RESOLVING,
    SIGNING_IN,
    CONNECTED,
    SIGNING_OUT_WAITING,
    SIGNING_OUT,
  };

  PeerConnectionClient();
  ~PeerConnectionClient();

  int id() const;
  bool is_connected() const;
  const Peers& peers() const;

  void RegisterObserver(PeerConnectionTransformObserver* callback);

  void Connect(const std::string& server, int port,
               const std::string& client_name);

  bool SendToPeer(int peer_id, const std::string& message);
  bool SendHangUp(int peer_id);
  bool IsSendingMessage();

  bool SignOut();

  // implements the MessageHandler interface
  void OnMessage(rtc::Message* msg);

 protected:
  void DoConnect();
  void Close();
  void InitSocketSignals();
  bool ConnectControlSocket();
  void OnConnect(rtc::AsyncSocket* socket);
  
  void OnMessageFromPeer(int peer_id, const std::string& message);
protected:
  void OnRead(rtc::AsyncSocket* socket);

  // Returns true if the whole response has been read.
  bool ReadIntoBuffer(rtc::AsyncSocket* socket, std::string* data,
	  size_t* content_length);

  bool ParseServerResponse(const std::string& response, size_t content_length,
	  size_t* peer_id, size_t* eoh);

  int GetResponseStatus(const std::string& response);

  // Quick and dirty support for parsing HTTP header values.
  bool GetHeaderValue(const std::string& data, size_t eoh,
	  const char* header_pattern, size_t* value);

  bool GetHeaderValue(const std::string& data, size_t eoh,
	  const char* header_pattern, std::string* value);

  // Parses a single line entry in the form "<name>,<id>,<connected>"
  bool ParseEntry(const std::string& entry, std::string* name, int* id,
	  bool* connected);

  void OnHangingGetConnect(rtc::AsyncSocket* socket);

  void OnHangingGetRead(rtc::AsyncSocket* socket);

  void OnClose(rtc::AsyncSocket* socket, int err);

  void OnResolveResult(rtc::AsyncResolverInterface* resolver);

  PeerConnectionTransformObserver* callback_;
  rtc::SocketAddress server_address_;
  rtc::AsyncResolver* resolver_;
  rtc::scoped_ptr<rtc::AsyncSocket> control_socket_;
  rtc::scoped_ptr<rtc::AsyncSocket> hanging_get_;
  std::string onconnect_data_;
  std::string control_data_;
  std::string notification_data_;
  std::string client_name_;
  Peers peers_;
  State state_;
  int my_id_;
};

#endif  // WEBRTC_EXAMPLES_PEERCONNECTION_CLIENT_PEER_CONNECTION_CLIENT_H_
