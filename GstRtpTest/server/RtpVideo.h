#ifndef __AVCVIDEO_H
#define __AVCVIDEO_H

#include <iostream>
#include <string>
#include <gst/gst.h>

extern "C"{
#include "JenDebug.h"
}
#define	 	MAX_NAL_SIZE	512
extern int iAvcStreamLogLevel;


class StreamPlayer{

public:
	StreamPlayer(){
		std::cout<< "StreamPlayer construct" <<std::endl;

		pStreamPipeline = NULL;
		pSource = NULL;
		pVpuDec = NULL;
		pVideoConv = NULL;
		pVideoEnc = NULL;
		pVideoparse = NULL;
		pVideoSink = NULL;

		pAudioparse = NULL;
		pAudioSink = NULL;
		pAudioEnc = NULL;
		pAudioConv = NULL;
		pAudioDec = NULL;
	}
	virtual ~StreamPlayer(){}

	/*
	 * init player,request resource
	 */
	virtual int InitStreamPlayer(){
		gst_init(NULL, NULL);
		return 0;
	}
	/*
	 * uinit player,clear resource
	 */
	virtual int UinitStreamPlayer() = 0;
	/*
	 * do the stream player start operation
	 */
	virtual int StartStreamPlayer() = 0;

	/*
	 * stop the stream player operation
	 */
	virtual void StopStreamPlayer() = 0;


	GstElement *pStreamPipeline;
	GstElement *pSource;

	GstElement *pVpuDec;
	GstElement *pVideoConv;
	GstElement *pVideoEnc;
	GstElement *pVideoparse;
	GstElement *pVideoSink;

	GstElement *pAudioparse;
	GstElement *pAudioSink;
	GstElement *pAudioEnc;
	GstElement *pAudioConv;
	GstElement *pAudioDec;

};

typedef struct{
	int axis_left;		// Left side of CarPlay display
	int axis_top;		// Top side of CarPlay display
	int disp_width;		// Video width
	int disp_height;	// Video height
}stVideoAxis;


class RtpSreamReciever : public StreamPlayer{

public:
	RtpSreamReciever(int port, bool syncclock = false){
		std::cout<< "RtpSreamReciever construct, port:" << port <<std::endl;
		RtpSrcPort = port;
		bSyncOnClock = syncclock;

		pRtpBin = NULL;
		pRtpDepay = NULL;
	}

	int InitStreamPlayer();
	int StartStreamPlayer();
	int UinitStreamPlayer();
	void StopStreamPlayer();

	static void on_recievepad_added(GstElement* object, GstPad* pad, gpointer user_data);
	static void on_recievepad_removed(GstElement* object, GstPad* pad, gpointer user_data);


private:

	GstElement *pRtpBin;
	GstElement *pRtpDepay;

	int RtpSrcPort;
	bool bSyncOnClock;
};

typedef enum{

	FILE_MODE,
	STREAM_MODE

}SENDER_MODE;

class RtpStreamSender : public StreamPlayer{
public:
	RtpStreamSender(int rtpport, int rtcpport, SENDER_MODE mode, std::string path=" ", bool syncclock=false){

		std::cout<< "RtpStreamSender construct, rtpport:" << rtpport <<std::endl;
		RtpsinkPort = rtpport;
		RtcpsinkPort = rtcpport;
		sendMode = mode;
		bSyncOnClock = syncclock;
		filepath = path;

		pRtpBin = NULL;
		pRtppay = NULL;
		pUdpRtpsink = NULL;
		pUdpRtcpsink = NULL;
		gstVideoBuffer = NULL;
	}
	~RtpStreamSender(){}

	int InitStreamPlayer();
	int StartStreamPlayer();
	int UinitStreamPlayer();
	void StopStreamPlayer();

	int FeedH264ToPlayer(void *pVideoBuffer, int iBufferSize);

	static void on_sendpad_removed(GstElement* object, GstPad* pad, gpointer user_data);
	static void on_sendpad_added(GstElement* object, GstPad* pad, gpointer user_data);
	static void callbackNeedDataAppsource(GstElement* appsource, guint arg0, gpointer user_data);
	static void callbackEnoughDataAppsource(GstElement* object, gpointer user_data);

private:
	void request_sender_pad();

	GstElement *pRtpBin;
	GstElement *pRtppay;
	GstElement *pUdpRtpsink;
	GstElement *pUdpRtcpsink;
	GstBuffer *gstVideoBuffer;

	int RtpsinkPort;
	int RtcpsinkPort;
	SENDER_MODE sendMode;
	bool bSyncOnClock;
	std::string filepath;

};



class RtmpReciever : public StreamPlayer{

public:
	int InitStreamPlayer();
	int StartStreamPlayer();
	int UinitStreamPlayer();
	void StopStreamPlayer();

	static void on_pad_added(GstElement* object, GstPad* pad, gpointer user_data);

private:
	GstElement *pDemux;

};

#endif
