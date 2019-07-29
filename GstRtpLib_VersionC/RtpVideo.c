
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
#include "JenDebug.h"

int iAvcStreamLogLevel = WARN_LEVEL;
stMediaParam g_PlayerParam;

#define DEF_CARPLAY_MODE    0
#define DEF_AIRPLAY_MODE    1

static bool bIsFisrtNAL = true;


#define USE_CAPS_FILTER_PLUGIN
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

/*=========================================================================:

==========================================================================*/
static void on_pad_added(GstElement* object, GstPad* pad, gpointer user_data)
{
	INFO("pad added\n");

	GstCaps *caps;
	GstStructure *str;
	gchar *name;
	GstPad *depaysink;
	GstPadLinkReturn ret;
	GstPad *peerpad;
	GstPad *udppad;



	name = gst_pad_get_name(pad);

	//g_assert(str);

	//name = (gchar *)gst_structure_get_name(str);
	INFO("pad name:%s\n",name);

	if(g_strrstr(name, "recv_rtp_src"))
	{
		depaysink = gst_element_get_static_pad(g_PlayerParam.pRtpDepay, "sink");
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
	else if(g_strrstr(name, "send_rtp_src"))
	{
		udppad = gst_element_get_static_pad(g_PlayerParam.pUdpRtpsink, "sink");
		g_assert(depaysink);
		ret = gst_pad_link(pad, udppad);
		printf("video gst_pad_link return:%d\n",ret);
		gst_object_unref(udppad);
	}
	g_free(name);
	//gst_caps_unref(caps);
}

static void on_pad_removed(GstElement* object, GstPad* pad, gpointer user_data)
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

==========================================================================*/
int InitStreamPlayer(bool bSyncOnClock)
{
	printf("\n\e[33m ==> %s: Build Time: %s-%s! \e[0m\n", __FUNCTION__, __DATE__, __TIME__);

	GstCaps *x264Caps;
	char *pStrEnv = NULL;
	bool bRetValue = false;
	guint64 u64MaxBytes = 0;
	const char *pCarplayMode = NULL;
	guint bus_watch_id;
	GstBus *MsgBus;

	gst_init(NULL, NULL);

	memset(&g_PlayerParam, 0, sizeof(stMediaParam));

	SetDebugLogLevel("JEN_AVC_STREAM_LOGLEVEL", INFO_LEVEL);

	g_PlayerParam.pTinyMediaPlayer = gst_pipeline_new("TinyRtpPlayer");
	if(NULL == g_PlayerParam.pTinyMediaPlayer)
	{
		ERROR("pTinyMediaPlayer == NULL!\n");
		return -1;
	}
	MsgBus = gst_element_get_bus(g_PlayerParam.pTinyMediaPlayer);
	bus_watch_id = gst_bus_add_watch(MsgBus, bus_call, NULL);
	//==============================// appsrc
	g_PlayerParam.pAppSource = gst_element_factory_make("udpsrc", "TinyUdpsrc");
	if(NULL == g_PlayerParam.pAppSource)
	{
		ERROR("pAppSource == NULL!\n");
		return -2;
	}
	x264Caps = gst_caps_from_string("application/x-rtp,media=video,payload=96,clock-rate=90000,encoding-name=H264");
	g_object_set(g_PlayerParam.pAppSource, "caps", x264Caps, NULL);
	gst_caps_unref(x264Caps);

	g_object_set(g_PlayerParam.pAppSource, "port", 6664, NULL);

	g_PlayerParam.pRtpBin = gst_element_factory_make("rtpbin", "TinyRtpbin");
	g_signal_connect(g_PlayerParam.pRtpBin, "pad-added", G_CALLBACK(on_pad_added), NULL);
	g_signal_connect(g_PlayerParam.pRtpBin, "pad-removed", G_CALLBACK(on_pad_removed), NULL);


	g_PlayerParam.pRtpDepay = gst_element_factory_make("rtph264depay", "TinyRtpdepay");
	g_PlayerParam.pVpuDec = gst_element_factory_make("avdec_h264", "TinyVpudec");
	g_PlayerParam.pVideoConv = gst_element_factory_make("videoconvert", "TinyVideoconv");
	g_PlayerParam.pVideoSink = gst_element_factory_make("xvimagesink", "TinyVideosink");

	gst_bin_add_many(GST_BIN(g_PlayerParam.pTinyMediaPlayer),
				g_PlayerParam.pAppSource,
				g_PlayerParam.pRtpBin,
				g_PlayerParam.pRtpDepay,
				g_PlayerParam.pVpuDec,
				g_PlayerParam.pVideoConv,
				g_PlayerParam.pVideoSink,
				NULL);


		bRetValue = gst_element_link_many(
				g_PlayerParam.pAppSource,
				g_PlayerParam.pRtpBin,
				NULL);
		if(false == bRetValue)
		{
			ERROR("==> 3 gst_element_link_many failed!\n");
			return -3;
		}

		bRetValue = gst_element_link_many(
				g_PlayerParam.pRtpDepay,
				g_PlayerParam.pVpuDec,
				g_PlayerParam.pVideoConv,
				g_PlayerParam.pVideoSink,
				NULL);
		if(false == bRetValue)
		{
			ERROR("==> 4 gst_element_link_many failed!\n");
			return -4;
		}

	return 0;
}

/*=========================================================================: 

==========================================================================*/
int UninitStreamPlayer(void)
{
	DEBUG("UninitStreamPlayer...\n");

#ifdef DEF_DEBUG_TIME
	struct timeval StartTime, FinishTime;
    gettimeofday(&StartTime, NULL);
    DEBUG ("Start: %d-%d\n", StartTime.tv_sec, StartTime.tv_usec);
#endif

	StopVideoStream();

	gst_object_unref(g_PlayerParam.pTinyMediaPlayer);

#ifdef DEF_DEBUG_TIME
    gettimeofday(&FinishTime, NULL);
    DEBUG ("Finish: %d-%d\n", FinishTime.tv_sec, FinishTime.tv_usec);
#endif

	return 0;
}

/*=========================================================================:

==========================================================================*/
int StartVideoStream(stVideoAxis *DispAxis, int bShowSurface)
{
	INFO("StartCarPlayVideo..., show: %d\n", bShowSurface);

	gst_element_set_state(g_PlayerParam.pTinyMediaPlayer, GST_STATE_PLAYING);

	INFO("StartCarPlayVideo OK\n");

	return 0;
}

/*=========================================================================:

==========================================================================*/
void StopVideoStream(void)
{
    INFO("StopCarPlayVideo...\n");

	gst_element_set_state(g_PlayerParam.pTinyMediaPlayer, GST_STATE_NULL);
	gst_element_get_state(g_PlayerParam.pTinyMediaPlayer, NULL, NULL, 2);

    INFO("StopCarPlayVideo OK\n");
}


/*=========================================================================:
send rtp
==========================================================================*/

int InitRtpSender(bool bSyncOnClock)
{
	printf("\n\e[33m ==> %s: Build Time: %s-%s! \e[0m\n", __FUNCTION__, __DATE__, __TIME__);

	GstCaps *x264Caps;
	char *pStrEnv = NULL;
	bool bRetValue = false;
	guint64 u64MaxBytes = 0;
	const char *pCarplayMode = NULL;
	guint bus_watch_id;
	GstBus *MsgBus;

	gst_init(NULL, NULL);

	memset(&g_PlayerParam, 0, sizeof(stMediaParam));

	SetDebugLogLevel("JEN_AVC_STREAM_LOGLEVEL", INFO_LEVEL);

	g_PlayerParam.pTinyMediaPlayer = gst_pipeline_new("TinyRtpSender");
	if(NULL == g_PlayerParam.pTinyMediaPlayer)
	{
		ERROR("pTinyMediaPlayer == NULL!\n");
		return -1;
	}
	MsgBus = gst_element_get_bus(g_PlayerParam.pTinyMediaPlayer);
	bus_watch_id = gst_bus_add_watch(MsgBus, bus_call, NULL);
	//==============================// appsrc
	g_PlayerParam.pAppSource = gst_element_factory_make("filesrc", "Tinyfilesrc");
	if(NULL == g_PlayerParam.pAppSource)
	{
		ERROR("pAppSource == NULL!\n");
		return -2;
	}


	g_object_set(g_PlayerParam.pAppSource, "location", "/home/guxin/test.264", NULL);

	g_PlayerParam.pRtpBin = gst_element_factory_make("rtpbin", "TinyRtpbin");
	g_signal_connect(g_PlayerParam.pRtpBin, "pad-added", G_CALLBACK(on_pad_added), NULL);
	g_signal_connect(g_PlayerParam.pRtpBin, "pad-removed", G_CALLBACK(on_pad_removed), NULL);


	g_PlayerParam.pRtppay = gst_element_factory_make("rtph264pay", "TinyRtppay");
	g_PlayerParam.pVpuDec = gst_element_factory_make("avdec_h264", "TinyVpudec");
	g_PlayerParam.pVideoConv = gst_element_factory_make("videoconvert", "TinyVideoconv");
	g_PlayerParam.pVideoparse = gst_element_factory_make("h264parse", "Tinyh264parse");
	g_PlayerParam.pVideoEnc = gst_element_factory_make("x264enc", "Tinyx264enc");
	g_PlayerParam.pUdpRtpsink = gst_element_factory_make("udpsink", "Tinyudprtpsink");
	g_PlayerParam.pUdpRtcpsink = gst_element_factory_make("udpsink", "Tinyudprtcpsink");

	g_object_set(g_PlayerParam.pUdpRtpsink, "port", 6664, NULL);
	g_object_set(g_PlayerParam.pUdpRtcpsink, "port", 6665, NULL);
	g_object_set(g_PlayerParam.pUdpRtcpsink, "sync", false, NULL);
	g_object_set(g_PlayerParam.pUdpRtcpsink, "async", false, NULL);

	gst_bin_add_many(GST_BIN(g_PlayerParam.pTinyMediaPlayer),
				g_PlayerParam.pAppSource,
				g_PlayerParam.pVideoparse,
				g_PlayerParam.pVpuDec,
				g_PlayerParam.pVideoConv,
				g_PlayerParam.pVideoEnc,
				g_PlayerParam.pRtppay,
				g_PlayerParam.pRtpBin,
				g_PlayerParam.pUdpRtpsink,
				g_PlayerParam.pUdpRtcpsink,
				NULL);


		bRetValue = gst_element_link_many(
				g_PlayerParam.pAppSource,
				g_PlayerParam.pVideoparse,
				g_PlayerParam.pVpuDec,
				g_PlayerParam.pVideoConv,
				g_PlayerParam.pVideoEnc,
				g_PlayerParam.pRtppay,
				NULL);
		if(false == bRetValue)
		{
			ERROR("==> 3 gst_element_link_many failed!\n");
			return -3;
		}

		GstPad *sendpad;
		gchar *name;
		GstPad *sinkpad;

		sinkpad = gst_element_get_static_pad(g_PlayerParam.pRtppay, "src");
		g_assert(sinkpad);
		sendpad = gst_element_get_request_pad (g_PlayerParam.pRtpBin, "send_rtp_sink_%u");
		name = gst_pad_get_name (sendpad);
		g_print ("A new pad %s was created\n", name);
		g_free (name);

		gst_pad_link(sinkpad, sendpad);
		gst_object_unref (GST_OBJECT (sendpad));
		gst_object_unref (GST_OBJECT (sinkpad));
		sendpad = NULL;
		sinkpad = NULL;

		sinkpad = gst_element_get_static_pad(g_PlayerParam.pUdpRtcpsink, "sink");
		g_assert(sinkpad);
		sendpad = gst_element_get_request_pad (g_PlayerParam.pRtpBin, "send_rtcp_src_%u");
		name = gst_pad_get_name (sendpad);
		g_print ("rtcp A new pad %s was created\n", name);
		g_free (name);

		gst_pad_link(sendpad, sinkpad);
		gst_object_unref (GST_OBJECT (sendpad));
		gst_object_unref (GST_OBJECT (sinkpad));


	return 0;
}

/*=========================================================================:

==========================================================================*/
int UninitRtpSender(void)
{
	DEBUG("UninitStreamPlayer...\n");

#ifdef DEF_DEBUG_TIME
	struct timeval StartTime, FinishTime;
    gettimeofday(&StartTime, NULL);
    DEBUG ("Start: %d-%d\n", StartTime.tv_sec, StartTime.tv_usec);
#endif

	StopVideoStream();

	gst_object_unref(g_PlayerParam.pTinyMediaPlayer);
	g_PlayerParam.pTinyMediaPlayer = NULL;

#ifdef DEF_DEBUG_TIME
    gettimeofday(&FinishTime, NULL);
    DEBUG ("Finish: %d-%d\n", FinishTime.tv_sec, FinishTime.tv_usec);
#endif

	return 0;
}

/*=========================================================================:

==========================================================================*/
int StartRtpSender(stVideoAxis *DispAxis, int bShowSurface)
{
	INFO("StartCarPlayVideo..., show: %d\n", bShowSurface);

	gst_element_set_state(g_PlayerParam.pTinyMediaPlayer, GST_STATE_PLAYING);

	INFO("StartCarPlayVideo OK\n");

	return 0;
}

/*=========================================================================:

==========================================================================*/
void StopRtpSender(void)
{
    INFO("StopCarPlayVideo...\n");

	gst_element_set_state(g_PlayerParam.pTinyMediaPlayer, GST_STATE_NULL);
	gst_element_get_state(g_PlayerParam.pTinyMediaPlayer, NULL, NULL, 2);

    INFO("StopCarPlayVideo OK\n");
}

int main (int argc, char *argv[])
{
	int iResult = -1;
	pthread_t ptGenAVC;
	stVideoAxis CarplayDispAxis;
	const char *pVideoFilePath = "/home/guxin/test.264";

	DEBUG("==> AvcStream: Build Time: %s-%s!\n", __DATE__, __TIME__);

	InitRtpSender(false);
	StartRtpSender(NULL, true);

	sleep(600);

	getchar();

	return iResult;
}
