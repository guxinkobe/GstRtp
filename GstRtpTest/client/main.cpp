#include <unistd.h>
#include "RtpVideo.h"

int main()
{
	RtpSreamReciever reciever(6664);
	reciever.InitStreamPlayer();
	reciever.StartStreamPlayer();

	sleep(1800);
	return 0;
}
