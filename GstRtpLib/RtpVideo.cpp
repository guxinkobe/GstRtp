
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <pthread.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "RtpVideo.h"   

int iAvcStreamLogLevel = WARN_LEVEL;

static bool bNeedData = false;
static bool iGstVideoStoped = true;

//#define DEF_DEBUG_TIME
//#define DEF_BUILD_APP

void SetGstPluginRank(const char *pName, int iRank)
{
	GstPluginFeature *pPluginFeature = NULL;
	GstRegistry *pRegistry = NULL;

	if(NULL == pName)
	{
		ERROR("%s: Invalid pName!\n", __FUNCTION__);
		return;
	}

	pRegistry = gst_registry_get();
	pPluginFeature = gst_registry_find_feature(pRegistry, pName, GST_TYPE_ELEMENT_FACTORY);
	if(NULL != pPluginFeature)
	{
		int iCurrentRank = 0;
		gst_plugin_feature_set_rank(pPluginFeature, iRank);

		iCurrentRank = gst_plugin_feature_get_rank(pPluginFeature);
		printf("Rank of %s is %d !\n", pName, iCurrentRank);
		gst_object_unref(pPluginFeature);
	}
}

int bus_call(GstBus *bus, GstMessage *msg, void *data)
{
	printf("Got message\n");
	printf("Got [%s] message\n", GST_MESSAGE_TYPE_NAME (msg));

	GError *err;
	gchar *debug_info;

	switch(GST_MESSAGE_TYPE(msg)){
		case GST_MESSAGE_ERROR:{
			gst_message_parse_error(msg, &err, &debug_info);
			g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
			g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
			g_clear_error(&err);
			g_free(debug_info);

		}
			break;
		case GST_MESSAGE_EOS:{
			g_print("gstMW : End-Of-Stream reached.\n");
		}
			break;
		default:

			break;
	}
	return 1;
}


/*=========================================================================:
 *
 * class RtpSreamReciever,for recieve rtp stream
 *
==========================================================================*/
void on_recievepad_added(GstElement* object, GstPad* pad, gpointer user_data)
{
	INFO("src pad added\n");

	GstCaps *caps;
	GstStructure *str;
	gchar *name;
	GstPad *depaysink;
	GstPadLinkReturn ret;
	GstPad *peerpad;
	GstPad *udppad;
	GstElement *pRtpDepay;

	g_assert(user_data);

	pRtpDepay = (GstElement *)user_data;
	name = gst_pad_get_name(pad);

	//g_assert(str);

	//name = (gchar *)gst_structure_get_name(str);
	INFO("pad name:%s\n",name);

	if(g_strrstr(name, "recv_rtp_src"))
	{
		depaysink = gst_element_get_static_pad(pRtpDepay, "sink");
		g_assert(depaysink);
		peerpad = gst_pad_get_peer(depaysink);
		if(peerpad)
		{
			printf("unlink current pad\n");
			gst_pad_unlink(peerpad,depaysink);
			gst_object_unref(peerpad);
		}
		ret = gst_pad_link(pad, depaysink);
		printf("video gst_pad_link return:%d\n",ret);
		gst_object_unref(depaysink);
	}
	/*else if(g_strrstr(name, "send_rtp_src"))
	{
		udppad = gst_element_get_static_pad(g_PlayerParam.pUdpRtpsink, "sink");
		g_assert(depaysink);
		ret = gst_pad_link(pad, udppad);
		printf("video gst_pad_link return:%d\n",ret);
		gst_object_unref(udppad);
	}*/
	g_free(name);
	//gst_caps_unref(caps);
}

static void on_recievepad_removed(GstElement* object, GstPad* pad, gpointer user_data)
{
	INFO("pad removed\n");

	GstCaps *caps;
	GstStructure *str;
	gchar *name;
	GstPad *depaysink;
	GstPadLinkReturn ret;

	name = gst_pad_get_name(pad);

	//g_assert(str);
	//name = (gchar *)gst_structure_get_name(str);
	INFO("pad name:%s\n",name);
	g_free(name);

	//gst_caps_unref(caps);
}


int RtpSreamReciever::InitRtpStreamReciever(bool bSyncOnClock)
{
	printf("\n\e[33m ==> %s: Build Time: %s-%s! \e[0m\n", __FUNCTION__, __DATE__, __TIME__);

	GstCaps *x264Caps;
	char *pStrEnv = NULL;
	bool bRetValue = false;
	guint bus_watch_id;
	GstBus *MsgBus;

	gst_init(NULL, NULL);


	SetDebugLogLevel("JEN_AVC_STREAM_LOGLEVEL", INFO_LEVEL);

	pRtpSreamReciever = gst_pipeline_new("RtpSreamReciever");
	if(NULL == pRtpSreamReciever)
	{
		ERROR("pTinyMediaPlayer == NULL!\n");
		return -1;
	}
	MsgBus = gst_element_get_bus(pRtpSreamReciever);
	bus_watch_id = gst_bus_add_watch(MsgBus, bus_call, NULL);
	//==============================// appsrc
	pAppSource = gst_element_factory_make("udpsrc", "TinyUdpsrc");
	if(NULL == pAppSource)
	{
		ERROR("pAppSource == NULL!\n");
		return -2;
	}
	x264Caps = gst_caps_from_string("application/x-rtp,media=video,payload=96,clock-rate=90000,encoding-name=H264");
	g_object_set(pAppSource, "caps", x264Caps, NULL);
	gst_caps_unref(x264Caps);

	g_object_set(pAppSource, "port", RtpSrcPort, NULL);

	pRtpBin = gst_element_factory_make("rtpbin", "TinyRtpbin");
	pRtpDepay = gst_element_factory_make("rtph264depay", "TinyRtpdepay");
	pVpuDec = gst_element_factory_make("avdec_h264", "TinyVpudec");
	pVideoConv = gst_element_factory_make("videoconvert", "TinyVideoconv");
	pVideoSink = gst_element_factory_make("xvimagesink", "TinyVideosink");

	g_signal_connect(pRtpBin, "pad-added", G_CALLBACK(on_recievepad_added), pRtpDepay);
	g_signal_connect(pRtpBin, "pad-removed", G_CALLBACK(on_recievepad_removed), NULL);

	gst_bin_add_many(GST_BIN(pRtpSreamReciever),
				pAppSource,
				pRtpBin,
				pRtpDepay,
				pVpuDec,
				pVideoConv,
				pVideoSink,
				NULL);


	bRetValue = gst_element_link_many(
			pAppSource,
			pRtpBin,
			NULL);
	if(false == bRetValue)
	{
		ERROR("==> 3 gst_element_link_many failed!\n");
		return -3;
	}

	bRetValue = gst_element_link_many(
			pRtpDepay,
			pVpuDec,
			pVideoConv,
			pVideoSink,
			NULL);
	if(false == bRetValue)
	{
		ERROR("==> 4 gst_element_link_many failed!\n");
		return -4;
	}

	return 0;
}


int RtpSreamReciever::UinitRtpStreamReciever(void)
{
	DEBUG("UninitStreamPlayer...\n");

#ifdef DEF_DEBUG_TIME
	struct timeval StartTime, FinishTime;
    gettimeofday(&StartTime, NULL);
    DEBUG ("Start: %d-%d\n", StartTime.tv_sec, StartTime.tv_usec);
#endif

    StopRtpStreamReciever();

	gst_object_unref(pRtpSreamReciever);

#ifdef DEF_DEBUG_TIME
    gettimeofday(&FinishTime, NULL);
    DEBUG ("Finish: %d-%d\n", FinishTime.tv_sec, FinishTime.tv_usec);
#endif

	return 0;
}


int RtpSreamReciever::StartRtpStreamReciever(stVideoAxis *DispAxis, int bShowSurface)
{
	INFO("StartCarPlayVideo..., show: %d\n", bShowSurface);

	gst_element_set_state(pRtpSreamReciever, GST_STATE_PLAYING);

	INFO("StartCarPlayVideo OK\n");

	return 0;
}


void RtpSreamReciever::StopRtpStreamReciever(void)
{
    INFO("StopCarPlayVideo...\n");

	gst_element_set_state(pRtpSreamReciever, GST_STATE_NULL);
	gst_element_get_state(pRtpSreamReciever, NULL, NULL, 2);

    INFO("StopCarPlayVideo OK\n");
}




/*=========================================================================:
 *
 * class RtpStreamSender,for send rtp stream
 *
==========================================================================*/
void on_sendpad_added(GstElement* object, GstPad* pad, gpointer user_data)
{
	INFO("sender pad added\n");

	GstCaps *caps;
	GstStructure *str;
	gchar *name;
	GstPadLinkReturn ret;
	GstPad *peerpad;
	GstPad *udppad;
	GstElement *pUdpRtpsink;

	g_assert(user_data);

	pUdpRtpsink = (GstElement *)user_data;
	name = gst_pad_get_name(pad);

	//name = (gchar *)gst_structure_get_name(str);
	INFO("pad name:%s\n",name);

	if(g_strrstr(name, "send_rtp_src"))
	{
		udppad = gst_element_get_static_pad(pUdpRtpsink, "sink");
		g_assert(udppad);
		ret = gst_pad_link(pad, udppad);
		printf("video gst_pad_link return:%d\n",ret);
		gst_object_unref(udppad);
	}
	g_free(name);
	//gst_caps_unref(caps);
}

static void on_sendpad_removed(GstElement* object, GstPad* pad, gpointer user_data)
{
	INFO("pad removed\n");

	GstCaps *caps;
	GstStructure *str;
	gchar *name;
	GstPad *depaysink;
	GstPadLinkReturn ret;

	name = gst_pad_get_name(pad);

	//g_assert(str);
	//name = (gchar *)gst_structure_get_name(str);
	INFO("pad name:%s\n",name);
	g_free(name);

	//gst_caps_unref(caps);
}

static void callbackNeedDataAppsource(GstElement* appsource,
        guint arg0,
        gpointer user_data)
{
	DEBUG("Need data!\n");
	bNeedData = true;

}


static void callbackEnoughDataAppsource(GstElement* object,
        gpointer user_data)
{
	DEBUG("Enough data!\n");
	bNeedData = false;
}

void RtpStreamSender::request_sender_pad()
{
	GstPad *sendpad;
	gchar *name;
	GstPad *sinkpad;

	sinkpad = gst_element_get_static_pad(pRtppay, "src");
	g_assert(sinkpad);
	sendpad = gst_element_get_request_pad (pRtpBin, "send_rtp_sink_%u");
	name = gst_pad_get_name (sendpad);
	g_print ("A new pad %s was created\n", name);
	g_free (name);

	gst_pad_link(sinkpad, sendpad);
	gst_object_unref (GST_OBJECT (sendpad));
	gst_object_unref (GST_OBJECT (sinkpad));
	sendpad = NULL;
	sinkpad = NULL;

	sinkpad = gst_element_get_static_pad(pUdpRtcpsink, "sink");
	g_assert(sinkpad);
	sendpad = gst_element_get_request_pad (pRtpBin, "send_rtcp_src_%u");
	name = gst_pad_get_name (sendpad);
	g_print ("rtcp A new pad %s was created\n", name);
	g_free (name);

	gst_pad_link(sendpad, sinkpad);
	gst_object_unref (GST_OBJECT (sendpad));
	gst_object_unref (GST_OBJECT (sinkpad));
}

int RtpStreamSender::InitRtpStreamSender(bool bSyncOnClock, std::string filepath)
{
	printf("\n\e[33m ==> %s: Build Time: %s-%s! \e[0m\n", __FUNCTION__, __DATE__, __TIME__);

	GstCaps *x264Caps;
	bool bRetValue = false;
	guint64 u64MaxBytes = 0;
	guint bus_watch_id;
	GstBus *MsgBus;

	gst_init(NULL, NULL);

	SetDebugLogLevel("JEN_AVC_STREAM_LOGLEVEL", INFO_LEVEL);

	pRtpStreamSender = gst_pipeline_new("RtpStreamSender");
	if(NULL == pRtpStreamSender)
	{
		ERROR("pTinyMediaPlayer == NULL!\n");
		return -1;
	}
	MsgBus = gst_element_get_bus(pRtpStreamSender);
	bus_watch_id = gst_bus_add_watch(MsgBus, bus_call, NULL);

	switch(sendMode)
	{
		case FILE_MODE:
		{

			pSource = gst_element_factory_make("filesrc", "Tinyfilesrc");
			if(NULL == pSource)
			{
				ERROR("pSource == NULL!\n");
				return -1;
			}
			g_object_set(pSource, "location", filepath.c_str(), NULL);
			break;
		}
		case STREAM_MODE:
		{

			pSource = gst_element_factory_make("appsrc", "Tinyappsrc");
			if(NULL == pSource)
			{
				ERROR("pSource == NULL!\n");
				return -1;
			}
			g_signal_connect(pSource, "need-data", G_CALLBACK(callbackNeedDataAppsource), NULL);
			g_signal_connect(pSource, "enough-data", G_CALLBACK(callbackEnoughDataAppsource), NULL);
			break;
		}
		default:
			ERROR("no support mode set, return\n");
			break;

	}
	//==============================// appsrc


	pRtpBin = gst_element_factory_make("rtpbin", "TinyRtpbin");
	pRtppay = gst_element_factory_make("rtph264pay", "TinyRtppay");
	pVpuDec = gst_element_factory_make("avdec_h264", "TinyVpudec");
	pVideoConv = gst_element_factory_make("videoconvert", "TinyVideoconv");
	pVideoparse = gst_element_factory_make("h264parse", "Tinyh264parse");
	pVideoEnc = gst_element_factory_make("x264enc", "Tinyx264enc");
	pUdpRtpsink = gst_element_factory_make("udpsink", "Tinyudprtpsink");
	pUdpRtcpsink = gst_element_factory_make("udpsink", "Tinyudprtcpsink");

	g_signal_connect(pRtpBin, "pad-added", G_CALLBACK(on_sendpad_added), pUdpRtpsink);
	g_signal_connect(pRtpBin, "pad-removed", G_CALLBACK(on_sendpad_removed), NULL);

	g_object_set(pUdpRtpsink, "port", RtpsinkPort, NULL);
	g_object_set(pUdpRtcpsink, "port", RtcpsinkPort, NULL);
	g_object_set(pUdpRtcpsink, "sync", false, NULL);
	g_object_set(pUdpRtcpsink, "async", false, NULL);

	gst_bin_add_many(GST_BIN(pRtpStreamSender),
				pSource,
				pVideoparse,
				pVpuDec,
				pVideoConv,
				pVideoEnc,
				pRtppay,
				pRtpBin,
				pUdpRtpsink,
				pUdpRtcpsink,
				NULL);


	bRetValue = gst_element_link_many(
			pSource,
			pVideoparse,
			pVpuDec,
			pVideoConv,
			pVideoEnc,
			pRtppay,
			NULL);
	if(false == bRetValue)
	{
		ERROR("==> 3 gst_element_link_many failed!\n");
		return -3;
	}


	request_sender_pad();

	return 0;
}


int RtpStreamSender::UinitRtpStreamSender(void)
{
	DEBUG("UninitStreamPlayer...\n");

#ifdef DEF_DEBUG_TIME
	struct timeval StartTime, FinishTime;
    gettimeofday(&StartTime, NULL);
    DEBUG ("Start: %d-%d\n", StartTime.tv_sec, StartTime.tv_usec);
#endif

    StopRtpStreamSender();

	gst_object_unref(pRtpStreamSender);
	pRtpStreamSender = NULL;

#ifdef DEF_DEBUG_TIME
    gettimeofday(&FinishTime, NULL);
    DEBUG ("Finish: %d-%d\n", FinishTime.tv_sec, FinishTime.tv_usec);
#endif

	return 0;
}


int RtpStreamSender::StartRtpStreamSender(stVideoAxis *DispAxis, int bShowSurface)
{
	INFO("StartCarPlayVideo..., show: %d\n", bShowSurface);

	gst_element_set_state(pRtpStreamSender, GST_STATE_PLAYING);
	iGstVideoStoped = false;
	INFO("StartCarPlayVideo OK\n");

	return 0;
}


void RtpStreamSender::StopRtpStreamSender(void)
{
    INFO("StopCarPlayVideo...\n");

	gst_element_set_state(pRtpStreamSender, GST_STATE_NULL);
	gst_element_get_state(pRtpStreamSender, NULL, NULL, 2);
	iGstVideoStoped = true;
    INFO("StopCarPlayVideo OK\n");
}


int RtpStreamSender::FeedH264ToPlayer(void *pVideoBuffer, int iBufferSize)
{
#ifdef DEF_DEBUG_TIME
	struct timeval StartTime, FinishTime;
	int iTimeDiv = 0;
	gettimeofday(&StartTime, NULL);
#endif

	LOG("feed start\n");

	static FILE *fp = NULL;
	GstFlowReturn gstResult;
	//const char *pNALBuffer = NULL;

    if(NULL == pVideoBuffer)
    {
        ERROR("Video buffer address should great than 0!\n");
        return -1;
    }

	if(NULL == pSource)
	{
		ERROR("Video srouce has not been initalized!\n");
		return -2;
	}

	if(iBufferSize <= 0)
	{
		ERROR("iBufferSize should not be less than 1!\n");
		return -3;
	}

	if(sendMode != STREAM_MODE)
	{
		ERROR("send mode does not match[%d]!\n",sendMode);
		return -6;
	}

	/*pNALBuffer = pVideoBuffer;
	if(  ( (0x7 == (0x1F & pNALBuffer[4])) || (0x8 == (0x1F & pNALBuffer[4])) ) //sps/pps
		&& (0x00 == pNALBuffer[0])
		&& (0x00 == pNALBuffer[1])
		&& (0x00 == pNALBuffer[2])
		&& (0x01 == pNALBuffer[3]) )
	{

		if (0 == memcmp(LastNALHeader, pVideoBuffer, iBufferSize))
		{
			INFO("###### The same NAL Header found!\n");
			return 0;
		}
		else
		{
			if(true == bIsFisrtNAL)
			{
				INFO("--> First NAL Header.\n");
				bIsFisrtNAL = false;
			}
			else
			{
				DEBUG("--> New NAL Header.\n");
				if(DEF_AIRPLAY_MODE == g_iCarPlayType)
				{
				    INFO("A Mode, we will restart renderor!\n");
				    StopVideoStream();             //--jerry for airplay
				    StartVideoStream(NULL, -1);    //--jerry for airplay
				}
			}

			if(iBufferSize > MAX_NAL_SIZE)
			{
				WARN("NAL size is too large: %d !\n", iBufferSize);
				memcpy(LastNALHeader, pVideoBuffer, MAX_NAL_SIZE);
			}
			else
			{
				memcpy(LastNALHeader, pVideoBuffer, iBufferSize);
			}
		}
	}
*/
// copy video data

	GstMapInfo BufferMap;

	gstVideoBuffer = gst_buffer_new_and_alloc(iBufferSize);

	gst_buffer_map (gstVideoBuffer, &BufferMap, GST_MAP_WRITE);
	memcpy(BufferMap.data, pVideoBuffer, iBufferSize);
	gst_buffer_unmap (gstVideoBuffer, &BufferMap);
// wait for need data
	while(false == bNeedData)
	{
		usleep(1000);
		printf(".");
		if(true == iGstVideoStoped)
		{
		    DEBUG("Gst Video Stoped!\n");
		    gst_buffer_unref(gstVideoBuffer);
		    return -5;
		}
	}
// push data to gstreamer
	DEBUG("bm_push: %d\n", gst_buffer_get_size(gstVideoBuffer));
	g_signal_emit_by_name(pSource, "push-buffer", gstVideoBuffer, &gstResult);
	if(GST_FLOW_OK != gstResult)
	{
		ERROR("\n=================================================> Push buffer failed!\n");
	}
	gst_buffer_unref(gstVideoBuffer);
	gstVideoBuffer = NULL;

	//g_PlayerParam.bNeedData = false;

	/*if(true == g_PlayerParam.bNeedRecord)
	{
		if(NULL == fp)
		{
			fp = fopen("/usr/app/gstCarplay.rec", "w");
			if(NULL == fp)
			{
				ERROR("Open file for AVC record failed!\n");
			}
		}

		if(NULL != fp)
		{
			fwrite(pVideoBuffer, 1, iBufferSize, fp);
			DEBUG("==> Rec: %d\n", iBufferSize);
			WriteCnt++;
			if (0 == (WriteCnt % 10))
			{
				DEBUG("==> flush Video data!");
				fflush(fp);
			}
		}
	}*/
#ifdef DEF_DEBUG_TIME
	gettimeofday(&FinishTime, NULL);
	iTimeDiv = 1000000*(FinishTime.tv_sec - StartTime.tv_sec) + (FinishTime.tv_usec - StartTime.tv_usec);
	if(iTimeDiv >= 200)
	{
	    LOG("tdiv>200us: %d\n", iTimeDiv);
	}
#endif

	return 0;
}
