#ifndef __AVCVIDEO_H
#define __AVCVIDEO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gst/gst.h>

#define	 	MAX_NAL_SIZE	512

extern int iAvcStreamLogLevel;

//===============================// Struct for SetDisplayAxis
typedef struct{
	int axis_left;		// Left side of CarPlay display
	int axis_top;		// Top side of CarPlay display
	int disp_width;		// Video width
	int disp_height;	// Video height
}stVideoAxis;

//===============================// AVC Player Parameters
typedef struct
{
	GstElement *pTinyMediaPlayer;
	GstElement *pAppSource;
	GstElement *pRtpBin;
	GstElement *pRtpDepay;
	GstElement *pVpuDec;
	GstElement *pVideoConv;
	GstElement *pVideoSink;
	GstElement *pVideoEnc;
	GstElement *pVideoparse;
	GstElement *pRtppay;
	GstElement *pUdpRtpsink;
	GstElement *pUdpRtcpsink;

	char 		*pURI;

	int 		iAppsrcBlockSize;
	int 		iAppsrcMaxBytes;
	bool		bAppsrcIsLive;
	int			iAppsrcMinPercent;

	bool		bNeedData;

	GstBuffer 	*gstVideoBuffer;

	bool		bNeedRecord;

	guint64 	u64V4lSinkMaxLateness;
	int			iV4lSinkBlocksize;
}stMediaParam;

extern stMediaParam g_PlayerParam;
//=====================================================================// For Old interface
int InitMediaPlayer(const char *strVideoSink, const char *strAudioSink, int iFlags, bool bSync);

int UninitCarPlay(void);

int StartCarPlayVideo(stVideoAxis *DispAxis, int bShowSurface);

void StopCarPlayVideo(void);

int SetVideoSurfaceAlpha(int iAlpha);

//=====================================================================//
// Initialize stream player, we will initialize GStreamer pipeline, default settings.
// bSyncOnClock: true: we will sync to GStreamer system clock
//		  		 false: we don't sync to GStreamer system clock
//=====================================================================//
int InitStreamPlayer(bool bSyncOnClock);

//=====================================================================//
// Uninit stream player and release all resources
//=====================================================================//
int UninitStreamPlayer(void);

//=====================================================================//
// Start h264 video stream. you can use your axis defined by DispAxis,
//	   you can show or hide video surface by set layer relationship.
// DispAxis:
//		Not NULL: the display axis
//		NULL:  we do not change the axis, default is full screen
// bShowSurface:
// 		true:  set overlay to top in order to show video surface
//		false: set HMI to top so that video surface could be hided by HMI;
//		-1	 : do not change layer configuration
//=====================================================================//
int StartVideoStream(stVideoAxis *DispAxis, int bShowSurface);

//=====================================================================//
// Stop stream pipeline only, could restart by StartVideoStream
//=====================================================================//
void StopVideoStream(void);

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

#ifdef __cplusplus
}
#endif

#endif
