#ifndef __AVCVIDEO_H
#define __AVCVIDEO_H


#include <string>
#include <gst/gst.h>

extern "C"{
#include "JenDebug.h"


}
#define	 	MAX_NAL_SIZE	512
extern int iAvcStreamLogLevel;
//===============================// Struct for SetDisplayAxis
typedef struct{
	int axis_left;		// Left side of CarPlay display
	int axis_top;		// Top side of CarPlay display
	int disp_width;		// Video width
	int disp_height;	// Video height
}stVideoAxis;



//=====================================================================//
// Change video axis,
// Since we have to pause pipeline, at the end, we will set the pipeline to pending state if pending state is not GST_STATE_VOID_PENDING.
//      else we will set pipeline to previous current state
// DisplayAxis:
	// int axis_left;		// Left side of video display
	// int axis_top;		// Top side of video display
	// int disp_width;		// Video width
	// int disp_height;		// Video height
//=====================================================================//
int SetDisplayAxis(stVideoAxis *DisplayAxis);

//=====================================================================//
// Set overlay to top in order to Show video surface
//=====================================================================//
int ShowVideoSurface(void);

//=====================================================================//
// Set HMI layer to top in order to Hide video surface (If HMI is opaque)
//=====================================================================//
int HideVideoSurface(void);

//=====================================================================//
// Send H264 raw video to AVC Stream Player
// pVideoBuffer: H264 video buffer
// iBufferSize: size of video
//
// Return:
//   0:   if successfully
//	 < 0: fail
// Note:
// 1. You have to check the return value to secure the operation !
// 2. Call this method only after StartCarPlayVideo
// 3. Feed complete SPS and PPS NAL header together, without any other frames, we will record it as Nal header, may reuse it !!!
//    It's better to ensure each NAL buffer (pVideoBuffer) is complete even though we use h264 parser !!!
//
//=====================================================================//
int FeedH264ToPlayer(void *pVideoBuffer, int iBufferSize);


class RtpSreamReciever{

public:
	RtpSreamReciever(int port){
		RtpSrcPort = port;

		pRtpSreamReciever = NULL;
		pAppSource = NULL;
		pRtpBin = NULL;
		pRtpDepay = NULL;
		pVpuDec = NULL;
		pVideoConv = NULL;
		pVideoSink = NULL;
	}
	int InitRtpStreamReciever(bool bSyncOnClock);
	int UinitRtpStreamReciever(void);
	int StartRtpStreamReciever(stVideoAxis *DispAxis, int bShowSurface);
	void StopRtpStreamReciever();

private:

	GstElement *pRtpSreamReciever;
	GstElement *pAppSource;
	GstElement *pRtpBin;
	GstElement *pRtpDepay;
	GstElement *pVpuDec;
	GstElement *pVideoConv;
	GstElement *pVideoSink;

	int RtpSrcPort;
};

typedef enum{

	FILE_MODE,
	STREAM_MODE

}SENDER_MODE;

class RtpStreamSender{
public:
	RtpStreamSender(int rtpport, int rtcpport, SENDER_MODE mode){
		RtpsinkPort = rtpport;
		RtcpsinkPort = rtcpport;
		sendMode = mode;

		pRtpStreamSender = NULL;
		pSource = NULL;
		pRtpBin = NULL;
		pVpuDec = NULL;
		pVideoConv = NULL;
		pVideoEnc = NULL;
		pVideoparse = NULL;
		pRtppay = NULL;
		pUdpRtpsink = NULL;
		pUdpRtcpsink = NULL;
		gstVideoBuffer = NULL;
	}
	~RtpStreamSender(){}

	int InitRtpStreamSender(bool bSyncOnClock, std::string filepath);
	int UinitRtpStreamSender(void);
	int StartRtpStreamSender(stVideoAxis *DispAxis, int bShowSurface);
	void StopRtpStreamSender();
	int FeedH264ToPlayer(void *pVideoBuffer, int iBufferSize);

private:
	void request_sender_pad();

	GstElement *pRtpStreamSender;
	GstElement *pSource;
	GstElement *pRtpBin;
	GstElement *pVpuDec;
	GstElement *pVideoConv;
	GstElement *pVideoEnc;
	GstElement *pVideoparse;
	GstElement *pRtppay;
	GstElement *pUdpRtpsink;
	GstElement *pUdpRtcpsink;
	GstBuffer *gstVideoBuffer;

	int RtpsinkPort;
	int RtcpsinkPort;
	SENDER_MODE sendMode;

};

#endif
