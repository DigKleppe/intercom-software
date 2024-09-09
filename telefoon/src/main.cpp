
#include <gst/gst.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#include "telefoon.h"
#include "videoThread.h"
#include "audioTransmit.h"
#include "audioReceiveThread.h"
#include "connections.h"
#include "testThread.h"
#include "updateThread.h"
#include "io.h"
#include "keys.h"
#include "timerThread.h"

#include <time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <stdlib.h>

// cmake eclipse: cmake-gui , build binaries in parellel build folder.
// import project in build folder. From this folder build project.

/* in root :
./configure \
   --host=arm-none-linux-gnueabihf\
   PATH="/home/dig/nanoPiFire2A/tools/4.9.3/bin/:$PATH"
 */
/*
 ssh root@192.168.2.9

 gst-inspect-1.0
 audio
 tx
 gst-launch-1.0 audiotestsrc ! "audio/x-raw,rate=24000" ! vorbisenc ! rtpvorbispay config-interval=1 ! rtpstreampay ! udpsink port=5001 host=192.168.2.255

 255 is broadcast

 gst-launch-1.0 audiotestsrc ! "audio/x-raw,rate=24000" ! autoaudiosink
 gst-launch-1.0 audiotestsrc ! "audio/x-raw,rate=24000" ! audioresample ! alsasink device=hw:3


 card 1 =
 gst-launch-1.0 alsasrc device=hw:3 ! audioconvert ! "audio/x-raw,rate=24000" ! audioresample ! alsasink device=hw:0

 rx
 gst-launch-1.0 udpsrc port=5001 do-timestamp=true ! "application/x-rtp-stream,media=audio,clock-rate=24000,encoding-name=VORBIS" ! rtpstreamdepay ! rtpvorbisdepay ! decodebin ! audioconvert ! audioresample ! autoaudiosink
 gst-launch-1.0 udpsrc port=5000 do-timestamp=true ! "application/x-rtp-stream,media=audio,clock-rate=24000,encoding-name=VORBIS" ! rtpstreamdepay ! rtpvorbisdepay ! decodebin ! audioconvert ! audioresample ! autoaudiosink
 Zie Vorbis-receive.txt
 https://help.ubuntu.com/community/UbuntuStudio/UsbAudioDevices
 ssh: ssh pi@192.168.2.7  pi pi

 cat /proc/asound/cards
 alsa output devices: aplay -l
 alsa input devices: arecord --list-devices

 https://superuser.com/questions/53957/what-do-alsa-devices-like-hw0-0-mean-how-do-i-figure-out-which-to-use

 https://wiki.audacityteam.org/wiki/USB_mic_on_Linux
 alsamixer -c 1  (= card 1)

4-6-19
alsamixer USB audio: Speaker 6(-39db )
Capture  51 (19.5db)

 sudo alsactl store

 gst-launch-1.0 -v audiotestsrc !  alsasink
 gst-launch-1.0 filesrc location=audio8k16S.wav  ! wavparse ! audioconvert ! audioresample ! alsasink
 gst-launch-1.0 filesrc location=audio8k16S.wav  ! wavparse  ! deinterleave name=d  d.src0 ! audioconvert ! audioresample ! alsasink
 gst-launch-1.0 filesrc location=audio8k16S.wav ! decodebin ! audioconvert ! "audio/x-raw,channels=2" ! deinterleave name=d  d.src_0 ! queue ! audioconvert ! vorbisenc ! oggmux ! filesink location=channel1.ogg  d.src_1 ! queue ! audioconvert ! vorbisenc ! oggmux ! filesink location=channel2.ogg
 gst-launch-1.0 filesrc location=audio8k16S.wav  ! wavparse  ! audioconvert ! "audio/x-raw,channels=2" ! deinterleave name=d  d.src_0 ! queue ! audioconvert ! alsasink
 gst-launch-1.0 filesrc location=audio8k16S.wav ! decodebin ! audioconvert ! "audio/x-raw,channels=2" ! deinterleave name=d  d.src_0 ! queue ! audioconvert ! vorbisenc ! oggmux ! filesink location=channel1.ogg  d.src_1 ! queue ! audioconvert ! autoaudiosink

 video

 met jpeg ipv h264:
 gst-launch-1.0 --gst-debug -e -v v4l2src device=/dev/video0 ! image/jpeg,framerate=30/1,width=800,height=600 ! rtpjpegpay ! udpsink port=5000 host=192.168.2.255
 gst-launch-1.0 --gst-debug -e -v v4l2src device=/dev/video0 ! image/jpeg,framerate=20/1,width=800,height=600 ! jpegdec ! videoconvert ! autovideosink

 gst-launch-1.0 -v udpsrc port=5001 ! application/x-rtp,encoding-name=JPEG,payload=26 ! rtpjpegdepay ! jpegdec ! videoconvert !  videoflip method=counterclockwise ! nxvideosink
 gst-launch-1.0 -v udpsrc port=5000 ! application/x-rtp,encoding-name=JPEG,payload=26 ! rtpjpegdepay ! jpegdec ! videoconvert !  autovideosink
 gst-launch-1.0 udpsrc port=5001 ! rtph264depay  video/x-raw,format=I420,framerate=30/1,width=800,height=600 ! autovideosink

 */

//volatile threadStatus_t videoThreadStatus = { 0,false, false, NULL };
//volatile threadStatus_t audioTransmitThreadStatus = {0, false, false, NULL };
//volatile threadStatus_t audioReceiveThreadStatus = { 0, false, false, NULL };
volatile threadStatus_t testModeThreadStatus = { 0,  false, false, NULL };
//volatile threadStatus_t ringThreadStatus = {0, false, false, NULL };
//volatile threadStatus_t messageThreadStatus = {0, false, false, NULL };
volatile threadStatus_t updateThreadStatus = { 0, false, false, NULL };
volatile threadStatus_t timerThreadStatus = { 0, false, false, NULL };

pthread_t testModeThreadID;
pthread_t keysThreadID;
pthread_t updateThreadID;
pthread_t timerThreadID;

uint32_t serverTimeoutTimerBaseFloor,serverTimeoutTimerFirstFloor;
uint32_t ackBaseFloorTimer=COMMANDTIMEOUT,ackFirstFloorTimer=COMMANDTIMEOUT;

uint32_t overallTimeout;
status_t status = STATUS_SHOW_STARTUPSCREEN;
int subStatus;
activeState_t activeState;

receiveData_t receiveDataBaseFloor, receiveDataFirstFloor;
transmitData_t transmitData;

int bellKey;
int microCardNo, speakerCardNo, houseNo;
float ringVolume = 1.0;
floor_t floorID = NO_FLOOR;
uint32_t restarts;
uint32_t timeouts;
int lastExitCode;
int lastActiveState;
volatile int exitCode;
uint32_t rings;

int exceptionCntr;

int backLight;
int oldBackLight;

char message[MESSAGEBUFFERSIZE];
char mssg[100];

#define MAINLOOPTIME 10 // ms
//#define TESTTHREADS 1


//#define TESTTHREADS 1  // audio and video streams on
//cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies
//1400000 1200000 1000000 800000 700000 600000 500000 400000

void setCPUSpeed ( uint32_t speed) {
	printf("speed %d\n",speed);
	writeIntValueToFile((const char *) "/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed",speed);
}


void secToDay(int n , char *buf )
{
	int day = n / (24 * 3600);

	n = n % (24 * 3600);
	int hour = n / 3600;

	n %= 3600;
	int min = n / 60 ;

	n %= 60;
	int sec = n;

	sprintf(buf,"%d days %d:%d:%d", day, hour,min,sec);
}


void  print( const char *format, ...) {
	va_list args;
	va_start(args, format);
	vprintf(format,args);
	sprintf( mssg, "%2d: ", houseNo );
	vsnprintf(mssg+strlen(mssg), sizeof(mssg)-4, format,args);
	UDPsendMessage( mssg);
	va_end(args);
}


void printExtMessage (const char *format, ...) {
	va_list args;
	va_start(args, format);
	vprintf(format,args);
	sprintf( mssg, "%2d: ", houseNo );
	vsnprintf(mssg+strlen(mssg), sizeof(mssg)-4, format,args);
	UDPsendExtendedMessage( mssg);
	va_end(args);
}

void printDebug(void){  // send tot port 6003
	char buf [50];
	static int debugpresc= 10;
	int station = 0;
	if (--debugpresc == 0 ){
		debugpresc= 10;
		secToDay(upTime, buf);
		print("%2d up: %s  \t rs:%u\trings:%d\tlas:%d to:%u \n", houseNo, buf, restarts,rings, lastActiveState, timeouts);
	}

	//	const int stations[] = {2,4,6,8,10,30,0 };
	//	const int *p = stations;
	//	int n;
	//
	//printf("Up: %d\n", upTime);
	//	do {
	//		n = *p;
	//		printf( "%d: %4d %4d ", n, commOKCntr[n/2], commErrCntr[n/2]);
	//		p++;
	//	} while (*p > 0);
	//
	//	printf("\n");
}

void printExtendedDebug( void) { // send to port 6000+ houseNo

//	char buf[100];
//	int n;
//	n = sprintf ( buf, "%d\t",houseNo);
//	n += sprintf ( buf+n, "st:%d\tsst:%d\tast:%d\t",status,subStatus,activeState);
//	n += sprintf ( buf+n, "bl:%d\tks:%u\t", backLight, keysThreadCntr);
//	n += sprintf ( buf+n, "a1:%u\ta2:%u\t", ackCntrBaseFloor, ackCntrFirstFloor);
//	n +=sprintf ( buf+n, "e:%u\t", exceptionCntr);
//	n += sprintf ( buf+n,"\n");
//	UDPsendExtendedMessage(buf);

}


void getIpAddress(char * dest) {

	int fd;
	struct ifreq ifr;

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	/* I want to get an IPv4 IP address */
	ifr.ifr_addr.sa_family = AF_INET;

	/* I want IP address attached to "eth0" */
	strncpy(ifr.ifr_name, "eth0", IFNAMSIZ - 1);

	ioctl(fd, SIOCGIFADDR, &ifr);

	close(fd);

	/* display result */
	sprintf(dest, "%s\n", inet_ntoa(((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr));
}
/*
//etc/network/interfaces

auto lo
iface lo inet loopback
auto eth0
iface eth0 inet static
address 192.168.2.130
netmask 255.255.255.0
gateway 192.168.2.254
nameserver 192.168.2.254
nameserver 8.8.8.8
nameserver 127.0.0.53
dns-search lan
 */


void saveIPaddresss(int houseNo ){
	FILE *fptr;
	FILE *fptr2;
	ssize_t result;
	char str[50];
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	fptr = fopen("/etc/network/interfaces","r");
	fptr2 = fopen("/etc/network/temp","w");

	if((fptr == NULL) || (fptr2 == NULL))
	{
		printf("File open Error !\n");
		return;
	}
	do {
		read = getline(&line, &len, fptr);
		if ( read != -1) {
			if ( strstr( line,"address") > 0)
				fprintf( fptr2, "address %s%d\n", BASEADDRESS, 100+houseNo);// new ip = 100+ houseno
			else
				fprintf( fptr2, "%s", line);
		}
	} while (read > 0 );
	free(line);

	fclose(fptr);
	fclose(fptr2);
	remove("/etc/network/interfaces");
	system("sync");
	rename("/etc/network/temp","/etc/network/interfaces");
	printf("IPaddress changed \n");
	system("sync");
	return;
}



// check ipaddress xxx.xxx.xxx.nnn   nnn= houseno + 100

void getHouseNo() {
	char ipAddress[17];
	int val;
	getIpAddress(ipAddress);
	int n = sscanf(ipAddress, "%d.%d.%d.%d", &val, &val, &val, &val);
	if (val >= 100)
		houseNo = val - 100;
	else {
		printf("Illegal IP address: %d default to set to 30\n", val);
		houseNo = 30;
	}
}

void saveSettings() {

	FILE *fptr;
	fptr = fopen(SETTINGSFILENAME,"w");
	if(fptr == NULL)
	{
		printf("File write open Error!\n");
		return;
	}
	fprintf(fptr,"ringvolume:%d",(int) (ringVolume));
	fclose(fptr);
	return;
}

void loadSettings(){
	FILE *fptr;
	int temp;
	int n;

	fptr = fopen(SETTINGSFILENAME,"r");
	if(fptr == NULL)
	{
		printf("Load settings file read Error!\n");
		ringVolume = 10;
		return;
	}
	n = fscanf(fptr,"ringvolume:%d",&temp);
	if ( n == 1)
		ringVolume = (float)temp;

	fclose(fptr);
	return;
}

void saveState(int state, int floor ){
	static int oldState;
	static int oldloor;
	if ((oldState == state) &&( oldloor == floor))  // not changed
		return;

	oldState = state;
	oldloor = floor;


	FILE *fptr;
	ssize_t result;
	char str[50];
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	fptr = fopen("/root/laststate.txt","w");
	if(fptr == NULL)
	{
		print("Save File open Error !\n");
		return;
	}
	fprintf( fptr,"Restarts__: %d\n", restarts);
	fprintf( fptr,"timeouts: %d\n", timeouts);
	fprintf( fptr,"lastexit: %d\n", lastExitCode);
	fprintf( fptr,"state: %d\n", state);
	fprintf( fptr,"floor: %d\n", floor);
	fprintf( fptr,"rings: %d\n", rings);
	fclose(fptr);
	print ( "State %d saved %d\n", state, floor);
	usleep( 10000);
	system("sync");
	return;
}


bool restoreState( int * state, int* floor ){
	FILE *fptr;

	ssize_t result;
	char str[50];
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	fptr = 	fptr = fopen("/root/laststate.txt","r");
	if(fptr == NULL)
	{
		printf("Restore File open Error !\n");
		return true;
	}
	do {
		read = getline(&line, &len, fptr);
		if ( read != -1) {
			sscanf(line,"Restarts__: %d\n", &restarts);
			sscanf(line,"timeouts: %d\n", &timeouts);
			sscanf(line,"lastexit: %d\n", &lastExitCode);
			sscanf(line,"rings: %d\n", &rings);
			sscanf(line,"state: %d\n",state);
			sscanf(line,"floor: %d\n", floor);
		}
	} while (read > 0 );

	free(line);
	print( "restore state: %d\n",*state);
	lastActiveState = *state;
	fclose(fptr);
	return false;
}


int detectAudioCardNo(const char * cardName) {
	bool found = false;
	GstElement *audioSource;
	char *card;
	char deviceName[15];
	int n;

	audioSource = gst_element_factory_make("alsasrc", "audioSource");

	for (n = 0; n < 5 && !found; n++) { // find card with name
		sprintf(deviceName, "hw:%d", n);
		g_object_set(audioSource, "device", deviceName, NULL);
		g_object_get(audioSource, "card-name", &card, NULL);
		if (!(card == (char *) NULL)) {
			if (strncmp(card, cardName, strlen(cardName)) == 0) {
				found = true;
			}
			g_free(card);
		}
	}
	gst_object_unref(audioSource);

	if (!found) {
		return -1;
	} else
		return n - 1;

}
//https://randu.org/tutorials/threads/



int init(void) {

	int result;

	printf("Version %3.1f %s %s\n", SOFTWAREVERSION/10.0, __DATE__ , __TIME__);

	writeValueToFile((const char *)"/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", "userspace");
	//	setCPUSpeed ( CPU_SPEED_LOW);

	initIo();
	getHouseNo();
	transmitData.softwareversion = SOFTWAREVERSION;
	loadSettings();

	speakerCardNo = detectAudioCardNo(SPEAKER_CARD_NAME1);
	if (speakerCardNo == -1) {
		speakerCardNo = detectAudioCardNo(SPEAKER_CARD_NAME2);
		if (speakerCardNo == -1) {
			printf("%s card not found \n\r", SPEAKER_CARD_NAME1);
			speakerCardNo = -1;
		}
	}
	testModeThreadStatus.cardno = speakerCardNo;

	result = pthread_create(&keysThreadID, NULL, &keysThread, NULL);
	if (result == 0)
		printf("keysThread created successfully.\n");
	else {
		printf("keysThread not created.\n");
		return -1;
	}

	pthread_create(&timerThreadID, NULL, &timerThread, NULL);

	result = pthread_create(&updateThreadID, NULL, &updateServerThread, (void *)&updateThreadStatus);
	if (result == 0)
		printf("updateServerThread created successfully.\n");
	else {
		printf("updateServerThread not created.\n");
		return -1;
	}

	startConnectionThreads();

	if ( keysRT & KEY_HANDSET) { // then horn is off, enter testmode
		result = pthread_create(&testModeThreadID, NULL, &testModeThread, (void *) &testModeThreadStatus);
		if (result == 0)
			printf("testThread created successfully.\n");
		else {
			printf("testThread not created.\n");
			return -1;			if ( keysRT & KEY_HANDSET) { // then horn is off, enter testmode
				result = pthread_create(&testModeThreadID, NULL, &testModeThread, (void *) &testModeThreadStatus);
				if (result == 0)
					printf("testThread created successfully.\n");
				else {
					printf("testThread not created.\n");
					return -1;
				}
			}
		}
	}

	/*creating thread*/
	return 0;
}

time_t startTime, now, startCall, lastSec; // startCall gaat mis als ie in main staat?????


void handleDoorKey( floor_t floorID){

	static bool doorKeyAcceptedbyBaseFloor = false;
	static bool doorKeyAcceptedbyFirstFloor = false;
	static bool doorKeyPressed = false;

	if (( keysRT & KEY_P1) && (doorKeyTimer == 0)) {
		print("doorkey pressed\n");
		doorKeyPressed = true;
		doorKeyAcceptedbyFirstFloor = false;
		doorKeyAcceptedbyBaseFloor = false;
		doorKeyTimer = 3;
	}

	if ( doorKeyPressed ) {
		if ( floorID == BASE_FLOOR) {
			doorKeyAcceptedbyFirstFloor = true;
			if (receiveDataBaseFloor.echoedKeys & KEY_P1) { // key is accepted
				doorKeyAcceptedbyBaseFloor = true;
			}
			if (!doorKeyAcceptedbyBaseFloor)
				transmitData.keys = keysRT | KEY_P1;
			else
				transmitData.keys = keysRT;
		}
		else {
			doorKeyAcceptedbyBaseFloor = true;
			if (receiveDataFirstFloor.echoedKeys & KEY_P1) { // key is accepted
				doorKeyAcceptedbyFirstFloor = true;
			}
			if (!doorKeyAcceptedbyFirstFloor)
				transmitData.keys = keysRT | KEY_P1;
			else
				transmitData.keys = keysRT;
		}
	}
	else
		transmitData.keys = keysRT;
}
int result;

int main(int argc, char *argv[]) {
	writeIntValueToFile ("/dev/backlight-1wire", 0);
	usleep( 1000);

	gst_init(&argc, &argv);
	init();
	startTime = time(NULL);
	int result, n;
	int oldhouseNo;
	uint32_t lastConnectCntr1=0,lastConnectCntr2=0;
	uint32_t mssgPresc = 1;

	if ( restoreState((int *)  &activeState,(int *)  &floorID ) == true )
		activeState = ACT_STATE_IDLE;  // no file
	if ( activeState == ACT_STATE_RESTART) {
		status = STATUS_IDLE;  // do not show startup screen
	}


#ifdef TESTTHREADS
	status = 999;
#endif

	while (1) {
		usleep(MAINLOOPTIME * 1000);
#ifdef SIM
		ackFirstFloorTimer = 100;
		serverTimeoutTimerFirstFloor = 0;
		serverTimeoutTimerBaseFloor = 0;
#endif
		if ( exitCode != NO_EXIT) {  // thn troubles
			lastExitCode = exitCode;
			print("exit %d\n",exitCode);
			saveState(status,floorID);
			exit( exitCode); // restart throuh script
		}

		if ( mssgPresc-- == 0){
			mssgPresc = 100/MAINLOOPTIME;  // keepalive
			UDPSend( BASE_FLOOR);
			UDPSend( FIRST_FLOOR);
		}

		if ( backLight != oldBackLight ) {
			writeIntValueToFile ("/dev/backlight-1wire", backLight);
			system("sync");
			oldBackLight = backLight;
		}
		now = time(NULL);
//		setVideoTask(VIDEOTASK_STREAM, VIDEOPORT1,NULL);
//		backLight = 80;

		if ( lastSec != now ){
			lastSec = now;
			printDebug();
			printExtendedDebug();

//			if ( status == (status_t) 999){
//				setVideoPort( VIDEOPORT1);
//				status = (status_t) 998;
//			}
//			else {
//				setVideoPort( VIDEOPORT2);
//				status = (status_t) 999;
//			}


			transmitData.uptime = now - startTime; // statistics
			if (connectCntrBaseFloor != lastConnectCntr1) {
				lastConnectCntr1 = connectCntrBaseFloor;
				serverTimeoutTimerBaseFloor = 0;
			}
			else
				serverTimeoutTimerBaseFloor++;

			if (connectCntrFirstFloor != lastConnectCntr2) {
				lastConnectCntr2 = connectCntrFirstFloor;
				serverTimeoutTimerFirstFloor = 0;
			}
			else
				serverTimeoutTimerFirstFloor++;

			if (ackBaseFloorTimer > 0 )
				ackBaseFloorTimer--;

			if (ackFirstFloorTimer > 0 )
				ackFirstFloorTimer--;
		}

		switch (status) {
		case STATUS_SHOW_STARTUPSCREEN:
			switch (subStatus) {
			case 0:
				subStatus++;
				restarts++;
				switch (activeState) {
				case ACT_STATE_RINGING:
					if ( floorID == BASE_FLOOR ) {
						setVideoTask(VIDEOTASK_STREAM, VIDEOPORT1,NULL);
					}
					else {
						setVideoTask(VIDEOTASK_STREAM, VIDEOPORT2,NULL);
					}
					commandTimer = COMMANDTIMEOUT;
					startCall = now;
					setAudioReceiveTask ( AUDIOTASK_RING, 0, speakerCardNo);
					status = STATUS_RINGING1;
					print("ring\n");
					status = STATUS_RINGING2;
					backLightOn();
					break;

				case ACT_STATE_TALKING:
					if ( floorID == BASE_FLOOR ) {
						setVideoTask(VIDEOTASK_STREAM, VIDEOPORT1,NULL);
						setAudioReceiveTask ( AUDIOTASK_LISTEN, AUDIO_RX_PORT1, speakerCardNo);
						setAudioTransmitTask( AUDIOTASK_TALK, AUDIO_TX_PORT1 , speakerCardNo);
					}
					else {
						setVideoTask(VIDEOTASK_STREAM, VIDEOPORT2,NULL);
						setAudioReceiveTask ( AUDIOTASK_LISTEN, AUDIO_RX_PORT2, speakerCardNo);
						setAudioTransmitTask( AUDIOTASK_TALK, AUDIO_TX_PORT2 , speakerCardNo);
					}
					startCall = now;
					status = STATUS_TALKING;
					backLightOn();

					break;
				case ACT_STATE_IDLE:
					n = sprintf(message, "\r  Huisnummer: %d\r\n\n", houseNo);
					n+= sprintf(&message[n], "  Softwareversie: %1.1f\r\n\n", SOFTWAREVERSION * 0.1);
					sprintf(&message[n], " %s %s\n", __DATE__ , __TIME__);
					setVideoTask(VIDEOTASK_SHOWMESSAGE, 0, message);
					print( message);
					subStatus++;
					backLightOn();
					break;

				default:
					break;
				} // end switch activestae
				break;

				case 300:
				backLightOff();
				setVideoTask(TASK_STOP,0,NULL);
				subStatus = 0;
				status = STATUS_IDLE;
				break;

			default:
				if ( subStatus < 300 )
					subStatus++; // timer
				break;
			}
			break; // end STATUS_SHOW_STARTUPSCREEN

			case STATUS_IDLE:

//				if ( upTime > RESTARTTIME) {
//					saveState(ACT_STATE_RESTART, floorID); //
//					exit(0); // restart fresh
//				}

				handleDoorKey(floorID);// and fill transmitdata with keys
				setAudioReceiveTask ( TASK_STOP,0,0);
				setAudioTransmitTask( TASK_STOP,0,0);
				saveState(ACT_STATE_IDLE, floorID); // in case of reset..

				if ((keysRT & KEY_HANDSET)){// && !(getVideoTask() == VIDEOTASK_STREAM) ) {  //handset off  and not streaming?
					status = STATUS_SETUP;
					subStatus = 0;
				}
				else {  // horn on the hook
			//		if (getVideoTask() == VIDEOTASK_STREAM) { // still on from last time
						if (now - startCall >= CALL_TIME) { // video off
							backLightOff();
				//			setVideoTask( TASK_STOP, 0 , NULL);
						}
				//	}
				//	else { // nothing to do , check timeouts
						if ((serverTimeoutTimerBaseFloor > CONNECTIONTIMEOUT) || (serverTimeoutTimerFirstFloor > CONNECTIONTIMEOUT)) {
							n = sprintf(message, "  Geen verbinding met\n");
							n += sprintf(&message[n], "  deurpost\n\n");
							if (serverTimeoutTimerBaseFloor > CONNECTIONTIMEOUT)
								n+= sprintf(&message[n], "  begane grond\n\n");
							else
								overallTimeoutTimer = NOCOMM_TIMEOUT;

							if (serverTimeoutTimerFirstFloor > CONNECTIONTIMEOUT)
								n+= sprintf(&message[n], "  eerste verdieping\n");
							else
								overallTimeoutTimer = NOCOMM_TIMEOUT;
							setVideoTask(VIDEOTASK_SHOWMESSAGE, 0, message);
							backLightHalf();
						}
						else { // no timeouts
							overallTimeoutTimer = NOCOMM_TIMEOUT;
							if ( ringVolume == 0) { // show screen if bell off
								switch (subStatus) {
								case 0:
									n = sprintf(message, "  Bel staat uit\n\n");
									n+= sprintf(&message[n], "  Neem de hoorn van de haak\n");
									n+= sprintf(&message[n], "  om de bel aan te zetten");
									setVideoTask(VIDEOTASK_SHOWMESSAGE, 0, message);
									backLightHalf();
									subStatus++;
									break;
								case 1:
									break;
								default:
									break;
								}
							}
							else {
					//		backLightOff();
					//			setVideoTask(TASK_STOP, 0,NULL);
								subStatus = 0;
							}
						}
				//	}
				} // end if horn on the hook
				if ((receiveDataBaseFloor.command == COMMAND_RING) || (receiveDataFirstFloor.command == COMMAND_RING)){
					floor_t oldFloor = floorID;
					rings++;
					saveState(ACT_STATE_RINGING, floorID); // in case of reset..

					if (receiveDataBaseFloor.command == COMMAND_RING) {
						floorID = BASE_FLOOR;
					//	setVideoTask(TASK_STOP,0,NULL);
						setVideoTask(VIDEOTASK_STREAM, VIDEOPORT1,NULL);
						setVideoPort(VIDEOPORT1);
					}
					else {
						floorID = FIRST_FLOOR;
					//	setVideoTask(TASK_STOP,0,NULL);
						setVideoTask(VIDEOTASK_STREAM, VIDEOPORT2,NULL);
						setVideoPort(VIDEOPORT2);
					}
					commandTimer = COMMANDTIMEOUT;
					startCall = now;
					setAudioReceiveTask ( AUDIOTASK_RING, 0, speakerCardNo);
					status = STATUS_RINGING1;
					print("ring\n");
				}
				break;  // end STATUS_IDLE

			case STATUS_RINGING1: // wait until ringcommand is acknowlegded
				backLightOn();

				if (floorID == BASE_FLOOR ){
					if (receiveDataBaseFloor.command == COMMAND_NONE)
						status = STATUS_RINGING2;
				}
				else {
					if (receiveDataFirstFloor.command == COMMAND_NONE)
						status = STATUS_RINGING2;
				}

				if ( commandTimer == 0) {
					status = STATUS_RINGING2;  // go on
					receiveDataBaseFloor.command = COMMAND_NONE;
					receiveDataFirstFloor.command = COMMAND_NONE;
				}

				if( (ackBaseFloorTimer == 0) || (ackFirstFloorTimer == 0)) {
			//		exitCode = ACK_TIMEOUT;
					timeouts++;
				}
				break;

			case STATUS_RINGING2:  // wait until horn gets off
				if (receiveDataBaseFloor.command == COMMAND_RING) {
					rings++;
					if( floorID == FIRST_FLOOR) { // then caller changed
						floorID = BASE_FLOOR;
						setVideoPort(VIDEOPORT1);
					//	setVideoTask(TASK_STOP,0,NULL);
					//	setVideoTask(VIDEOTASK_STREAM, VIDEOPORT1,NULL);

					}
					startCall = now;
					printf("ring base\n");
					status = STATUS_RINGING1;
					commandTimer = COMMANDTIMEOUT;
					setAudioReceiveTask ( AUDIOTASK_RING, 0, speakerCardNo);
				}
				if (receiveDataFirstFloor.command == COMMAND_RING) {
					rings++;
					if( floorID == BASE_FLOOR) { // then caller changed
						floorID = FIRST_FLOOR;
						setVideoPort(VIDEOPORT2);
					//	setVideoTask(TASK_STOP,0,NULL);
					//	setVideoTask(VIDEOTASK_STREAM, VIDEOPORT2,NULL);
					}
					startCall = now;
					printf("ring first\n");
					status = STATUS_RINGING1;
					commandTimer = COMMANDTIMEOUT;
					setAudioReceiveTask ( AUDIOTASK_RING, 0, speakerCardNo);
				}
				if (keysRT & KEY_HANDSET) { // then horn is off
					saveState(ACT_STATE_TALKING, floorID); // in case of reset..
					if( floorID == BASE_FLOOR) {
						setAudioReceiveTask ( AUDIOTASK_LISTEN, AUDIO_RX_PORT1, speakerCardNo);
						setAudioTransmitTask( AUDIOTASK_TALK, AUDIO_TX_PORT1 , speakerCardNo);
					}
					else {
						setAudioReceiveTask ( AUDIOTASK_LISTEN, AUDIO_RX_PORT2, speakerCardNo);
						setAudioTransmitTask( AUDIOTASK_TALK, AUDIO_TX_PORT2 , speakerCardNo);
					}
					status = STATUS_TALKING;
				}
				if (now - startCall >= CALL_TIME) { // taken too long to answer, stop
					backLightOff();
				//	setVideoTask(TASK_STOP, 0, 0 );
					setAudioReceiveTask ( TASK_STOP,0,0);
					setAudioTransmitTask( TASK_STOP,0,0);
					status = STATUS_IDLE;
				}
				handleDoorKey(floorID);
				UDPSend(floorID);

				if( (ackBaseFloorTimer == 0) || (ackFirstFloorTimer == 0)) {
		//			exitCode = ACK_TIMEOUT;
					timeouts++;
				}

				if ( audioReceiverIsStopped () )
					setAudioReceiveTask ( TASK_STOP, 0,0);

				break;
			case STATUS_TALKING:


				if (!(keysRT & KEY_HANDSET)) { // then horn is on again
					status = STATUS_IDLE;
				}

				if (now - startCall >= CALL_TIME) { // stop
					status = STATUS_IDLE;
			//		print("exit\n");
			//		writeIntValueToFile ("/dev/backlight-1wire", 0); // backlight immideate  off
			//		saveState(ACT_STATE_RESTART, floorID); //
			//		exit(0);
				}
				if( (ackBaseFloorTimer == 0) || (ackFirstFloorTimer == 0)) {
				//	exitCode = ACK_TIMEOUT;
					timeouts++;
				}
				handleDoorKey(floorID);
				UDPSend(floorID);
				break;

			case STATUS_SETUP:
				transmitData.keys = 0;
				switch (subStatus) {
				case 0:
					n = sprintf(message, "  Sleutel: belvolume (%d)\n\n", (int)ringVolume);
					//	n += sprintf( &message[n] ,"2: spraakvolume\n");
					n += sprintf(&message[n], "  1: test\n\n");
					//	n += sprintf(&message[n], "2+3: huisnummer (%d)", houseNo);
					setVideoTask(VIDEOTASK_SHOWMESSAGE, 0, message);
					subStatus++;
					backLightOn();
					break;

				case 1:
					switch (keysRT) {
					case (KEY_P1 + KEY_HANDSET):
						status = STATUS_SETBELLVOLUME;
						setCPUSpeed ( CPU_SPEED_HIGH);
						subStatus = 0;
					break;
					case (KEY_P2 + KEY_HANDSET):
						status = STATUS_TESTMODE;
						subStatus = 0;
					break;
					case (KEY_P2 + KEY_P3 + KEY_HANDSET):
						status = STATUS_SETHOUSENO;
						subStatus = 0;
						setCPUSpeed ( CPU_SPEED_HIGH);
					break;
					default:
						break;

					}
					if (!(keysRT & KEY_HANDSET)) { // then horn is on again
						backLightOff();
						setVideoTask(TASK_STOP, 0,NULL);
						subStatus = 0;
						status = STATUS_IDLE;
					}
					break;
					default:
						break;
				} // end switch subStatus
				break;

			case STATUS_TESTMODE:
				transmitData.keys = 0;
				switch (subStatus) {
				case 0:
					backLightOff();
					setCPUSpeed(CPU_SPEED_HIGH);
					setVideoTask(TASK_STOP, 0,NULL);
					setAudioReceiveTask(TASK_STOP, 0,0);
					setAudioTransmitTask(TASK_STOP, 0,0);

					pthread_create(&testModeThreadID, NULL, &testModeThread, (void *) &testModeThreadStatus);
					backLightOn();
					while( !testModeThreadStatus.run)
						usleep(1000);
					subStatus++;
					startCall = now;
					break;
				case 1:
					if (((now - startCall) > 300) || (!testModeThreadStatus.run)) {
						backLightOff();
						//	stopMessageThread();
						subStatus = 0;
						status = STATUS_SHOW_STARTUPSCREEN;
						setCPUSpeed(CPU_SPEED_LOW);
					}
					break;
				default:
					break;
				}
				break;

			case STATUS_SETBELLVOLUME:
				transmitData.keys = 0;
				keyRepeats = KEY_P2 | KEY_P3;
				n = sprintf(message, "  Instellen belvolume\n");
				n += sprintf(&message[n], "  1: harder\n");
				n += sprintf(&message[n], "  2: zachter\n");
				n += sprintf(&message[n], "  stand: %d\n\n", (int) ringVolume);
				n += sprintf(&message[n], "  plaats de hoorn terug\n  om af te sluiten");
				switch (subStatus) {
				case 0:
					subStatus++;
					setVideoTask(VIDEOTASK_SHOWMESSAGE, 0, message);
					//	break;
				case 1:
					setAudioReceiveTask ( TASK_STOP, 0, 0);
					setAudioReceiveTask ( AUDIOTASK_RING, 0, speakerCardNo);
					setVideoText(message);
					subStatus++;

				case 2:
					if (!(keysRT & KEY_HANDSET)) {
						backLightOff();
						setVideoTask(TASK_STOP, 0,NULL);
						saveSettings();
						status = STATUS_IDLE;

						setCPUSpeed(CPU_SPEED_LOW);
						subStatus = 0;
					}
					if (key(KEY_P2)) {
						if (ringVolume < MAXRINGVOLUME)
							ringVolume++;
						subStatus = 1;
					}
					if (key(KEY_P3)) {
						if (ringVolume > 0 ) {
							ringVolume--;
							subStatus = 1;
						}
					}
					break;
				default:
					break;
				}
				break;

			case STATUS_SETHOUSENO:
				transmitData.keys = 0;
				keyRepeats = KEY_P2 | KEY_P3;
				n = sprintf(message, "  Instellen huisnummer\n");
				n += sprintf(&message[n], "  1: op\n");
				n += sprintf(&message[n], "  2: neer\n\n");
				n += sprintf(&message[n], "  huisnummer: %d\n", houseNo);
				n += sprintf(&message[n], "  plaats de hoorn terug\n  om af te sluiten");
				switch (subStatus) {
				case 0:
					subStatus++;
					oldhouseNo=houseNo;
					key(KEY_P2);
					key(KEY_P3);
					setVideoTask(VIDEOTASK_SHOWMESSAGE, 0, message);
					break;
				case 1:
					setVideoText(message);
					subStatus++;

				case 2:
					if (!(keysRT & KEY_HANDSET)) {
						backLightOff();
						//	stopMessageThread();
						if ( oldhouseNo != houseNo) {
							saveIPaddresss(houseNo);
							system ("reboot now");
						}
						status = STATUS_IDLE;
						setCPUSpeed(CPU_SPEED_LOW);
						subStatus = 1;
					}
					if (key(KEY_P2)) {
						if (houseNo < 62)  // 2 - 60  , 62 = park IPADRESSS
							houseNo += 2;
						subStatus = 1;
					}
					if (key(KEY_P3)) {
						if (houseNo > 2)
							houseNo -= 2;
						subStatus = 1;
					}
					break;
				default:
					break;
				}
				break;
			default:
				break;
		} // end switch status
	}  // end while 1
	return 0;
}


