
#define _DEFAULT_SOURCE // usleep?

#define NO_UPDATES

#include "camera.h"
#include "videoThread.h"
#include "audioTransmit.h"
#include "audioReceiveThread.h"
#include "connections.h"
#include "i2cThread.h"
#include "timerThread.h"
#include "updateThread.h"
#include "testThread.h"
#include "ringThread.h"
#include "io.h"
#include "keys.h"
#include "timerThread.h"

#include <gst/gst.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

/*
./configure \
   --host=arm-none-linux-gnueabihf\
   --prefix=/build\
   PATH="/home/dig/nanoPiFire2A/tools/4.9.3/bin/:$PATH"
 */

/*
 ssh root@192.168.2.9

4-6-19
alsamixer USB audio: Speaker 64 (-9 db )
Capture  60 (22.5 db)

 ontvangen van telefoon
 gst-launch-1.0 udpsrc port=5004 caps='application/x-rtp, media=(string)audio, clock-rate=(int)44100, encoding-name=(string)L16, encoding-params=(string)1, channels=(int)1, payload=(int)96' ! rtpL16depay ! audioconvert ! wavescope ! ximagesink
 ontvangen van camera
 gst-launch-1.0 udpsrc port=5002 caps='application/x-rtp, media=(string)audio, clock-rate=(int)44100, encoding-name=(string)L16, encoding-params=(string)1, channels=(int)1, payload=(int)96' ! rtpL16depay ! audioconvert ! wavescope ! ximagesink

 gst-inspect-1.0
 audio
 tx
 gst-launch-1.0 audiotestsrc ! "audio/x-raw,rate=24000" ! vorbisenc ! rtpvorbispay config-interval=1 ! rtpstreampay ! udpsink port=5001 host=192.168.2.255

 255 is broadcast

 gst-launch-1.0 audiotestsrc ! "audio/x-raw,rate=24000" ! autoaudiosink
 gst-launch-1.0 audiotestsrc ! "audio/x-raw,rate=24000" ! audioresample ! alsasink device=hw:2


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
 /etc/asound.conf voor default USB audio device
 alsa
 pcm.!default {
 type hw
 card 1
 device 0
 }

 ctl.!default {
 type hw
 card 1
 }
 https://wiki.audacityteam.org/wiki/USB_mic_on_Linux
 alsamixer -c 1  (= card 1)

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

 gst-launch-1.0 --gst-debug -e -v v4l2src device=/dev/video0 ! image/jpeg,framerate=20/1,width=800,height=600 ! jpegdec ! videoconvert ! videoflip method=counterclockwise ! nxvideosink

 gst-launch-1.0 -v udpsrc port=5001 ! application/x-rtp,encoding-name=JPEG,payload=26 ! rtpjpegdepay ! jpegdec ! videoconvert ! videoflip method=counterclockwise ! nxvideosink
 gst-launch-1.0 udpsrc port=5001 ! rtph264depay  video/x-raw,format=I420,framerate=30/1,width=800,height=600 ! autovideosink
 */

volatile threadStatus_t i2cStatus = { 0, false, false,NULL };
volatile threadStatus_t testThreadStatus = { 0, false, false,NULL };
volatile threadStatus_t serverStatus = { 0, false, false,NULL };
volatile threadStatus_t timerThreadStatus = { 0,false, false,NULL };
volatile threadStatus_t updateThreadStatus = {0,  false, false,NULL };
volatile threadStatus_t keysThreadStatus = {0,  false, false,NULL };

pthread_t serverThreadID;
pthread_t i2cThreadID;
pthread_t timerThreadID;
pthread_t updateThreadID;
pthread_t testThreadID;
pthread_t keysThreadID;

void* i2cThread(void* args);

//#define TESTSTREAMS 1

station_t station[NR_STATIONS];  // no [0] not used , no [31] = test
void secToDay(int n , char *buf );

int bellKey;
int microCardNo, speakerCardNo;
floor_t  myFloorID = BASE_FLOOR;
volatile bool updateInProgress;
char message[MESSAGEBUFFERSIZE];
char mssg[MESSAGEBUFFERSIZE];
uint32_t oldSeqNo[NR_STATIONS];
time_t now, lastSec;
bool openDoor;
bool active = false;
receiveData_t receiveData;
transmitData_t transmitData;
int restarts;
int rings;
int ringTimer;
int backLight;

int UDPVideoPort;
int UDPAudioRxPort;
int UDPAudioTxPort;

int cameraCard;

void  print( const char *format, ...) {
	va_list args;
	va_start(args, format);
	vprintf(format,args);
	vsnprintf(mssg, sizeof(mssg), format,args);
	mssg[MESSAGEBUFFERSIZE-1] = 0;
	UDPsendMessage( mssg);
	va_end(args);
}

void printDebug(void){
	char buf [50];
	static int debugpresc= 10;
//	int station = 0;
	if (--debugpresc == 0 ){
		debugpresc= 10;
		secToDay(upTime, buf);
		print("%d: restarts:%d rings:%d uptime: %s",myFloorID, restarts, rings, buf);
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

//cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies
//1400000 1200000 1000000 800000 700000 600000 500000 400000

//echo userspace > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
//echo 400000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed

void setCPUSpeed ( uint32_t speed) {
	writeIntValueToFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed",speed);
}

void saveState(int state, int actTelefoon ){
	static int oldState;
	static int oldTel;
	if ((oldState == state) &&( oldTel == actTelefoon))  // not changed
		return;

	oldState = state;
	oldTel = actTelefoon;

	FILE *fptr;

	fptr = fopen("/root/laststate.txt","w");
	if(fptr == NULL)
	{
		print("Save File open Error !\n");
		return;
	}
	fprintf( fptr, "Restarts_: %d\n", restarts);
	fprintf( fptr, "rings: %d\n", rings);
	fprintf( fptr, "state: %d\n", state);
	fprintf( fptr, "actTelefoon: %d\n", actTelefoon);

	fclose(fptr);
	system("sync");
	print ( "State %d saved %d\n", state, actTelefoon);
	return;
}

bool restoreState( int * state, int* actTelefoon ){
	FILE *fptr;
	int n,val;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	fptr =  fopen("/root/laststate.txt","r");
	if(fptr == NULL)
	{
		printf("Restore File open Error !\n");
		return true;
	}
	do {
		read = getline(&line, &len, fptr);
		if ( read != -1) {
			n = sscanf(line,"Restarts_: %d\n", &val);
			if (n==1) restarts = val;
			n = sscanf(line,"rings: %d\n", &val);
			if (n==1) rings = val;
			n = sscanf(line,"state: %d\n", &val);
			if (n==1) *state = val;
			n = sscanf(line,"actTelefoon: %d\n", &val);
			if (n==1) *actTelefoon = val;
		}
	} while (read > 0 );

	free(line);
	fclose(fptr);
	return false;
}


int detectAudioCardNo(char * cardName) {
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


//void hang(){
//	while (1){
//		printf( "Hello \n");
//		backLightOff();
//		sleep (1);
//		backLightOn();
//		sleep (1);
//	}
//}

int init(void) {

	int result;

	writeValueToFile("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", "userspace");

	setCPUSpeed ( CPU_SPEED_LOW);
	initIo();

	if ( fopen ("/dev/video0","r" ) == NULL ){
		cameraCard = -1;
		printf("camera not found \n\r");
	}
	else
		cameraCard = 0;
//	cameraCard = 0; // todo
	microCardNo = detectAudioCardNo(SPEAKER_CARD_NAME1); // detect audio interface

	if (microCardNo == -1) {
		microCardNo = detectAudioCardNo(SPEAKER_CARD_NAME2);
		if (microCardNo == -1)
			printf("%s microphone card not found \n\r", CAMERA_MIC_CARD_NAME);
	}

	testThreadStatus.spkrCardNo = microCardNo;
	testThreadStatus.microCardNo =  microCardNo;

	result = pthread_create(&i2cThreadID, NULL, &i2cThread, (void *) &i2cStatus);
	if (result == 0) {
		printf("i2cThread created successfully.\n");
	} else {
		printf("i2cThread not created.\n");
		return -1;
	}

	result = pthread_create(&timerThreadID, NULL, &timerThread, (void *) &timerThreadStatus);
	if (result == 0) {
		printf("timerThread created successfully.\n");
	} else {
		printf("timerThread not created.\n");
		return -1;
	}

	//	hang();

	result = pthread_create(&keysThreadID, NULL, &keysThread, (void *) &keysThreadStatus);
	if (result == 0) {
		printf("keysThread created successfully.\n");
	} else {
		printf("keysThread not created.\n");
		return -1;
	}

	getMyIpAddress(message + strlen(message));
	if ( strstr( message, "100") > 0) { // ip 192.168.2.100
		myFloorID = BASE_FLOOR;
		UDPVideoPort = VIDEOPORT1;
		UDPAudioRxPort = AUDIO_RX_PORT1;
		UDPAudioTxPort = AUDIO_TX_PORT1;
#ifndef	NO_UPDATES
		result = pthread_create(&updateThreadID, NULL, &updateThread, (void *) &updateThreadStatus);
		if (result == 0) {
			printf("updateThread created successfully.\n");
		} else {
			printf("updateThread not created.\n");
			return -1;
		}
#endif
	}
	else {
		myFloorID = FIRST_FLOOR;  // ip 192.168.2.101
		UDPVideoPort = VIDEOPORT2;
		UDPAudioRxPort = AUDIO_RX_PORT2;
		UDPAudioTxPort = AUDIO_TX_PORT2;
	}
	startConnectionThreads();

	return 0;
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

	sprintf(buf,"%d days %d:%d:%d\r\n", day, hour,min,sec);
}

bool bellSimButtonIn ( int stationID) {
	if (station[stationID]. keys & KEY_HANDSET)
		return false; // may only be activated if handset is on

	if( myFloorID == BASE_FLOOR) {
		if ( station[stationID].keys & KEY_P3) // key "2" pressed
			return true;
	}
	else {
		if (station[stationID].keys & KEY_P2) // key "1" pressed
			return true;
	}
	return false;
}


void test( void) {
	//	static int presc = 1;
	//	if ( --presc == 0 ){
	//		presc = 30;
	//		station[15].keys |= KEY_P3; // key "2" pressed
	//	}
	//	if (presc == 28)
	//		station[15].keys = 0;
}


void startRing ( int p) {

	memset( activeTimer,0, sizeof ( activeTimer) ); // clear others
	activeTimer[p]=ACTIVETIME;
	if (commandTimer[p] == 0) {
		commandTimer[p] = COMMANDTIME;
		transmitData.command = COMMAND_RING;
		print("ring to %d\n", p*2);
		setVideoTask(VIDEOTASK_STREAM,UDPVideoPort,NULL,cameraCard);
		setAudioTransmitTask(AUDIOTASK_TALK,UDPAudioTxPort,microCardNo);
		active = true;
		if (!testThreadStatus.run) {
			setAudioReceiveTask(AUDIOTASK_RING,UDPAudioRxPort,microCardNo); // local speaker
			ringTimer = RINGTIME;
		}
		active = true;
		saveState(ACT_STATE_RINGING, p); // in case of reset....
		rings++;
	}
}
int main(int argc, char *argv[]) {
	uint32_t oldConnectCntr =0;
	uint32_t mask;
	int presc = (10000/MAINLOOPTIME);
	int testmodeTimer = 0;
	status_t status = STATUS_SHOW_STARTUPSCREEN;
	int subStatus = 0;
	int activeTelephone;
	activeState_t activeState;

	int oldBackLight= -1;
	uint32_t oldBellButtons;

	bool lastActive = false;
	backLightOn();

	gst_init(&argc, &argv);
	init();
	setCameraLEDS(true);

#ifdef TESTSTREAMS
	status = 9999;
#endif
	while (1) {
		usleep(MAINLOOPTIME * 1000);
		now = time(NULL);

		if ( backLight != oldBackLight ) {
			writeIntValueToFile ("/dev/backlight-1wire", backLight);
			system("sync");
			oldBackLight = backLight;
		}
		switch (status) {
		case STATUS_SHOW_STARTUPSCREEN:
			switch (subStatus) {
			case 0:
				if ( restoreState((int *)  &activeState, &activeTelephone ) == true )
					activeState = ACT_STATE_IDLE;  // no file
				restarts++;
				switch (activeState) {
				case ACT_STATE_RINGING:
					startRing(activeTelephone);
					status = STATUS_IDLE;
					backLightOn();
					break;
				case ACT_STATE_TALKING:
					activeTimer[activeTelephone]=ACTIVETIME;
					setVideoTask(VIDEOTASK_STREAM,UDPVideoPort,NULL,cameraCard);
					setAudioTransmitTask(AUDIOTASK_TALK,UDPAudioTxPort,microCardNo);
					setAudioReceiveTask(AUDIOTASK_LISTEN,UDPAudioRxPort,microCardNo);
					active = true;
					status = STATUS_IDLE;
					backLightOn();
					break;
				case ACT_STATE_IDLE:
					sprintf(message, "\r Intercom\r\n ");
					if (myFloorID == BASE_FLOOR)
						sprintf(message + strlen(message)," Begane grond");
					else
						sprintf(message + strlen(message)," Eerste verdieping");

					sprintf(message + strlen(message), "\r\n\n Softwareversie: %1.1f", SOFTWAREVERSION * 0.1);
					sprintf(message + strlen(message), "\r\n %s %s\n", __DATE__ , __TIME__);

					UDPsendMessage(message);
					setVideoTask(VIDEOTASK_SHOWMESSAGE, 0, message, cameraCard);
					printf("Version %3.1f %s %s\n", SOFTWAREVERSION/10.0, __DATE__ , __TIME__);
					subStatus++;
					break;
				}
				break;
				case 20:
					setVideoTask(TASK_STOP,0,NULL, cameraCard);
					//	subStatus++;
					subStatus = 0;
					status = STATUS_IDLE;
					backLightOff();
					//		backLightHalf();
					break;

				case 30:
					subStatus = 0;
					break;

				default:
					subStatus++;

			}
			break;

			case STATUS_IDLE:
				if( updateInProgress){
					status = STATUS_UPDATING;
					subStatus = 0;
					break;
				}
				if( !active && (!testThreadStatus.run) ) {
					LEDD4 = false;
					setAudioReceiveTask( TASK_STOP,0,0);
					setAudioTransmitTask(TASK_STOP,0,0);

					setVideoTask(TASK_STOP,0,NULL,cameraCard); // mag weg
					backLightOff();


					int stationID = 1;  // no nr 0
					bool unConnected = false;
					do {
						if ( timeoutTimer[stationID] == 0)
							unConnected = true;
						stationID++;
					} while ( stationID < NR_STATIONS-1);  // skip last (test station)

					if (! unConnected){ // all ok screen off (memleak messageThread)
						backLightOff();
						setVideoTask(TASK_STOP,0,NULL,cameraCard);
						if ( presc)
							presc--;
						else {
							presc = (10000/MAINLOOPTIME);
						}
					}
					else {
						if ( presc)
							presc--;
						else {
							presc = (10000/MAINLOOPTIME);

							int stationID = 1;  // no nr 0
							print ("\n Not connected: ");
							sprintf (message ,"Geen verbinding met:\r\n\n"); // first
							do {
								if (timeoutTimer[stationID] == 0 ){
									print (" %d",stationID * 2 );
									sprintf( message + strlen(message), " %d" ,stationID*2);
								}
								stationID++;
							}  while ( stationID < NR_STATIONS-1);  // skip last (test station)
							//			backLightHalf();
							//			setVideoTask(VIDEOTASK_SHOWMESSAGE, 0, message,cameraCard);
							print("\n" );
						}
					}
				} // end if !active

#ifdef TESTSTREAMS
				if(0) {
#else
					if ( lastSec != now ){
#endif
						test();
						printDebug();
						lastSec = now;
						active = false;
						for ( int n = 1; n < NR_STATIONS; n++) {
							if ( activeTimer[n] > 0){
								active = true;  // if any active telephone
							}
						}
						if ( lastActive != active ){
							if ( active )
								setCPUSpeed ( CPU_SPEED_HIGH);
							else {
								saveState(ACT_STATE_IDLE, activeTelephone); // in case of reset....
								setCPUSpeed ( CPU_SPEED_LOW);
							}
							lastActive = active;
						}

						if ( testThreadStatus.run)  { // testmode running]
							testmodeTimer--;
							if ( testmodeTimer <= 0 ) {
								testThreadStatus.mustStop = true;
								while ( testThreadStatus.run)
										usleep( 10000);
								setCPUSpeed ( CPU_SPEED_LOW);
							}
							if ((testThreadStatus.run) && key( KEY_SW2)) {  // stopped with button
								testmodeTimer = 0;
								setVideoTask(TASK_STOP, 0 ,0, 0);
								setCPUSpeed ( CPU_SPEED_LOW);
								testThreadStatus.run = false;
							}
						}
						else {
							if ( key (KEY_SW1)) {  // testkey, abort active
								print("SW1 Key pressed\n");

								backLightOn();
								active = false;
								setCPUSpeed ( CPU_SPEED_HIGH);
								setAudioReceiveTask (TASK_STOP, 0 ,0);
								setAudioTransmitTask(TASK_STOP, 0 ,0);
								setVideoTask(TASK_STOP, 0 ,0, 0);
								usleep(10000);
								pthread_create(&testThreadID, NULL, &testModeThread, (void *) &testThreadStatus);
								testmodeTimer = TESTMODEMAXTIME;
							}
//							if ( key (KEY_SW2)) {  // testkey, abort active
//								print("SW2 Key pressed");
//								backLightOn();
//								active = false;
//								setCPUSpeed ( CPU_SPEED_HIGH);
//								setAudioReceiveTask (TASK_STOP, 0 ,0);
//								setAudioTransmitTask(TASK_STOP, 0 ,0);
//								setVideoTask(VIDEOTASK_SHOWCAMERA, 0 ,0, 0);
//								usleep(10000);
//								testmodeTimer = TESTMODEMAXTIME;
//								testThreadStatus.run = true;
//							}

							if (active ) {
								LEDD4 = !LEDD4;
								//backLightOn();
								if ( ringTimer > 0) {
									ringTimer--;
									if ( ringTimer == 0 ) {
										saveState(ACT_STATE_TALKING, activeTelephone); // in case of reset....
										setAudioReceiveTask(AUDIOTASK_LISTEN,UDPAudioRxPort,microCardNo);  // listen
									}
								}
							}
						}
					} // end every second

					if ( oldConnectCntr != connectCntr ){
						LEDD5 = !LEDD5;
						oldConnectCntr = connectCntr;
					}
					openDoor = false;

					if (!testThreadStatus.run ) {
						oldBellButtons &= bellButtons; // reset old pressed

						mask = 1;
						for ( int p = 1; p < NR_STATIONS; p++) {
							//	transmitData.command = COMMAND_NONE; // cleared if receiver received it
							if ( (bellButtons & mask) || bellSimButtonIn(p)){  // bell key pressed
								if ( ((bellButtons & mask) != (oldBellButtons & mask)) || bellSimButtonIn(p)) {  //prevent continous pressing
									bellButtons &= ~oldBellButtons; // disable already pressed key
									activeTelephone = p;
									oldBellButtons |= mask;
									startRing(p);
									if (!testThreadStatus.run) {
										setAudioReceiveTask(AUDIOTASK_RING,UDPAudioRxPort,microCardNo); // local speaker
										ringTimer = RINGTIME;
									}
								}
							}

							receiveData.keys = 0;
							if ( !updateInProgress)   {
								if (activeTimer[p] > 0 ) { // only send to active stations
									if( oldSeqNo[p] != station[p].seqNo) { // then new data
										oldSeqNo[p] = station[p].seqNo;
										commOKCntr[p]++;
										transmitData.echoedKeys = station[p].keys; // echo back to ack
										if ( station[p].keys & KEY_P1 ) {  // keykey in telephone pressed
											openDoor = true;
											print("Key pressed %d\n",p*2);
											station[p].keys = 0;
										}
									}
									else {
										sprintf (mssg , "No data %d\n", p*2); // sends no data when hang ip
										print( mssg);
										commErrCntr[p]++;
									}
									UDPsendToTelephone ( p*2 ,COMMANDPORT, (uint8_t *)&transmitData, sizeof(transmitData ),(uint8_t *)&receiveData , sizeof(receiveData));
									if ( transmitData.command > 0 )
										printf( "UDP to %d:  %d %d\n", p*2, transmitData.command, station[p].receivedCommand);
									if (station[p].receivedCommand == transmitData.command )
										transmitData.command = 0; // clear command if received on other end
								}
								else
									station[p].keys = 0;
							}
							mask <<= 1;
						}
					} // end if not testing

					if ( openDoor) { // closed in timer
						openDoorTimer = OPENDOORTIME;
						setDooropen( true);
					}

					break;  // end STATUS_IDLE
			case STATUS_UPDATING:
				if ( !updateInProgress) {
					backLightOff();
					status = STATUS_IDLE;
				}

				break;

			case STATUS_ERROR:
				break;


				}// end of

		} // end switch status;
		// end application

		print ("Main ended !\n");

		usleep(100000);

		return 0;

	}

