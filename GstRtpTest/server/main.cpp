#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <pthread.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "RtpVideo.h"

RtpStreamSender *sender;
//========================================================================// APP
void *GenerateAVCDataThread(void *argv)
{
	DEBUG("GenerateAVCDataThread is running\n");

	FILE *fp = NULL;
	int iRet = -1;
//================================//
	int iVideoSize = 0;
	char *pVideoBuffer = NULL;
	int iCnt = 0;
	int iLastPos = 0;

	int tmpFlag = 0;
	//static stVideoAxis CarplayDispAxis[2] = {{0, 0, 800, 450}, {0, 0, 540, 960}};

	DEBUG("GenerateAVCDataThread is running 0\n");

	pthread_detach(pthread_self());

	INFO("Open file: %s...\n", (const char *)argv);
	fp = fopen((const char *)argv, "r");
	if(NULL == fp)
	{
		ERROR("Open file failed!\n");
		pthread_exit(&iRet);
		return NULL;
	}

//================================// Read video file
	DEBUG("GenerateAVCDataThread is running 2\n");
	fseek(fp, 0, SEEK_END);
	iVideoSize = ftell(fp);
	rewind(fp);

	pVideoBuffer = (char *)malloc(iVideoSize + 1);
	iRet = fread(pVideoBuffer, 1, iVideoSize, fp);
	if(iRet > 0)
	{
		DEBUG("bfw\n");
#ifdef FEED_FLV
		if(isFlvPacket(pVideoBuffer, iVideoSize))
		{
			FeedFlvPacket(pVideoBuffer, iVideoSize);
		}
#else
		while(1)
		{
			for(iCnt=1; iCnt < iVideoSize; iCnt++)
			{
				if( (0x00 == pVideoBuffer[iCnt])
					&& (0x00 == pVideoBuffer[iCnt + 1])
					&& (0x00 == pVideoBuffer[iCnt + 2])
					&& (0x01 == pVideoBuffer[iCnt + 3]))
				//if(0 == (iCnt % 10240))
				{
					DEBUG("bfd: %d!\n", iCnt - iLastPos);

					sender->FeedH264ToPlayer(pVideoBuffer + iLastPos, iCnt - iLastPos);
					usleep(40*1000);
					iLastPos = iCnt;

				}
			}
			DEBUG("bfd: %d!\n", iCnt - iLastPos);
			sender->FeedH264ToPlayer(pVideoBuffer + iLastPos, iCnt - iLastPos);
			iLastPos = 0;
		}
#endif

		DEBUG("fnw: %d\n", iRet);
	}
	else
	{
		ERROR("iRet: %d", iRet);
	}

	DEBUG("GenerateAVCDataThread is running 3\n");
	fclose(fp);
	free(pVideoBuffer);

	pthread_exit(NULL);
	//exit(0);
}

//#define STREAM_MODE

int main()
{
	pthread_t ptGenAVC;
	const char *pVideoFilePath = "/home/guxin/test.264";

#ifdef STREAM_MODE
	sender = new RtpStreamSender(6664,6665, STREAM_MODE);
#else
	sender = new RtpStreamSender(6664,6665, FILE_MODE);
#endif
	sender->InitRtpStreamSender(false, pVideoFilePath);
	sender->StartRtpStreamSender(NULL, 0);

#ifdef STREAM_MODE
	pthread_create(&ptGenAVC, NULL, &GenerateAVCDataThread, (char *)pVideoFilePath);
#endif

	sleep(1800);
	delete sender;
	sender = NULL;
	return 0;
}
