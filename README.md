"# webrtcTest" 
window版本webrtc库编译请参照
#http://blog.csdn.net/yangzhenping/article/details/51276320
编译后将lib拷贝到当前目录下的lib中

本程序为webrtc版本P2P测试代码。

主要测试stun打洞流程。


代码使用vs2015编辑器编译

工程：

1. MediaPeer 点对点音视频通讯代码，基于MFC显示
1. MediaStreamApply  webrtc库封装模块
1. MediaTest		  音频回环播放模块
1. webrtc-cpp-sample webrtc数据通道打洞代码
