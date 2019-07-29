#include <unistd.h>
#include "RtpVideo.h"

int main()
{
	RtpSreamReciever reciever(6664);
	reciever.InitRtpStreamReciever(false);
	reciever.StartRtpStreamReciever(NULL, 0);

	sleep(1800);
	return 0;
}
