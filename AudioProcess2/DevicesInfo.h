#pragma once

#include "webrtc\modules\audio_device\audio_device_impl.h"
#include "CodeTransport.h"

static void printDevices(webrtc::AudioDeviceModule* device)
{
	if (device == NULL) return;

	int16_t playerCount = device->PlayoutDevices();
	printf("播放设备数:%d\n", playerCount);
	for (int16_t i = 0; i < playerCount; i++)
	{
		char name[webrtc::kAdmMaxDeviceNameSize] = { 0 };
		char guid[webrtc::kAdmMaxGuidSize] = { 0 };

		device->PlayoutDeviceName(i, name, guid);
		std::string utf8 = name;
		std::string strName = UTF82ASCII(utf8);
		printf("播放设备：%d %s , %s\n", i, strName.c_str(), guid);
	}

	int16_t recordCount = device->RecordingDevices();
	printf("采集设备数:%d\n", recordCount);
	for (int16_t i = 0; i < recordCount; i++)
	{
		char name[webrtc::kAdmMaxDeviceNameSize] = { 0 };
		char guid[webrtc::kAdmMaxGuidSize] = { 0 };
		device->RecordingDeviceName(i, name, guid);
		std::string utf8 = name;
		std::string strName = UTF82ASCII(utf8);
		printf("采集设备：%d %s , %s\n", i, strName.c_str(), guid);
	}
}