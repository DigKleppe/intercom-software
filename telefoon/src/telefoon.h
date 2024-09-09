/*
 * camera.h
 *
 *  Created on: Jan 27, 2019
 *      Author: dig
 */

#ifndef TELEFOON_H_
#define TELEFOON_H_


//#define SIM


#include <stdbool.h>
#include <stdint.h>

#define SOFTWAREVERSION 		39  // 2 = 0.2 10 = 1.0

#define VIDEOPORT1				5000	// camera1 to display (begane grond)
#define VIDEOPORT2				5001	// camera2 to display (eerste etage)
#define AUDIO_RX_PORT1			5002	// mic camera 1 to stations
#define AUDIO_RX_PORT2			5003	// mic camera 2 to stations
#define AUDIO_TX_PORT1			5004	// stations to door speaker camera 1
#define AUDIO_TX_PORT2			5005	// stations to door speaker camera 2
#define RECEIVE_STATIONSPORT1	5010	// incoming calls from stations to camera 1
#define RECEIVE_STATIONSPORT2	5011	// incoming calls from stations to camera 2
#define TCPCHATPORT				5012	// tcp socket for chatting between cameras and telephones
#define COMMANDPORT				5013	// tcp socket for commands to telephones
#define UDPCHATPORT				5014	// udp socket for chatting between cameras and telephones
#define UDPCHATPORT2			5015	// udp socket for chatting between cameras and telephones
#define UDPKEEPALIVEPORT		5016
#define UDPACKPORT1				5017	// acks from camera 1
#define UDPACKPORT2				5018	// acks from camera 2

#define UPDATEPORT				5050	// tcp socket for update files to telephones

#define MONITORTELEPHONESPORT	6003 // 6000 6001 used


#define BASEADDRESS				"192.168.2."
#define HOST 					"192.168.2.255"   // UDP
#define SPEAKER_CARD_NAME1 		"USB PnP Sound Device"	// https://nl.aliexpress.com/item/USB-2-0-Virtual-7-1-Kanaals-Externe-USB-Audio-Sound-Card-Adapter-voor-Laptop-PC/32890956230.html?spm=a2g0s.9042311.0.0.27424c4d4OGX7R
#define SPEAKER_CARD_NAME2 		"USB Audio Device"  //RV77 oid aliexpress  ( eerste proefzending)

#define TEST_INTERVAL			100  // (1 * 1000 * 1000 * 1000) // ms interval for checking stopping transmit/ receive threads

#define IPADDRESS_BASEFLOOR		"192.168.2.100"  	// socketaddress
#define IPADDRESS_FIRSTFLOOR	"192.168.2.101"


#define SPEAKERCHANNEL			1.0   // panorama
#define HANDSETCHANNEL			-1.0
#define BOTHCHANNELS			0
#define HANDSETCHANNEL			-1.0
#define TESTVOLUME				0.3

#ifdef SIM
#define CALL_TIME				10
#else
#define CALL_TIME				(2* 60) // seconds to switch off backlight
#endif

#define CHATINTERVAL			1000  	//ms

#define NOCOMM_TIMEOUT			(5 * 60) //sec ,restarts
#define UPDATE_TIMEOUT			10 // sec
#define RESTARTTIME				(60* 60 * 24) // restart every day ??

// needed for receiver
#define VorbisConfigStr "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"

typedef enum {RUNSTATE_RUN,  RUNSTATE_PAUSE } runState_t;

typedef enum { TASK_STOP, AUDIOTASK_RING, AUDIOTASK_LISTEN, AUDIOTASK_TALK , VIDEOTASK_STREAM, VIDEOTASK_SHOWMESSAGE } streamerTask_t;
typedef enum {ACT_STATE_IDLE,ACT_STATE_RINGING , ACT_STATE_TALKING ,ACT_STATE_RESTART} activeState_t;

typedef struct {
	int cardno;
	bool mustStop;
	bool run;
	bool pause;
	int task;
	void * info;
} threadStatus_t ;

// data sent to camera stations
typedef struct {
	uint32_t seqNo;
	uint32_t echoedCommand; // echoed for camera to ack sended command
	uint32_t keys;
	uint32_t uptime;
	uint32_t errors;
	uint32_t softwareversion;
} transmitData_t;  //  station at camera station

extern transmitData_t transmitData;
// data received from camera stations
typedef struct {
	uint32_t seqNo;
	uint32_t timeout;
	uint32_t echoedKeys; // echoed from camera to ack
	uint32_t command;
	uint32_t errors;
	uint32_t softwareversion;
} receiveData_t;

typedef enum { COMMAND_NONE, COMMAND_RING, COMMAND_TEST } command_t;
typedef enum {STATUS_SHOW_STARTUPSCREEN, STATUS_IDLE, STATUS_RINGING1, STATUS_RINGING2, STATUS_TALKING, STATUS_SETUP, STATUS_TESTMODE,STATUS_SETBELLVOLUME, STATUS_SETSPEECHVOLUME, STATUS_SETHOUSENO, STATUS_UPDATING, STATUS_ERROR ,STATUS_TEST } status_t;
typedef enum {NO_FLOOR, BASE_FLOOR, FIRST_FLOOR} floor_t;
typedef enum {NO_EXIT,ACK_TIMEOUT, OVERALL_TIMEOUT, MEM_ERROR } exitError_t;

extern receiveData_t receiveDataBaseFloor, receiveDataFirstFloor;
extern status_t status;

#define NR_STATIONS 31  // no 31 = test

extern float ringVolume;
#define MAXRINGVOLUME 		20
#define MINRINGVOLUME 		2
#define RINGVOLUMESTEP 		1

#define MAXSPEECHVOLUME 	10
#define MINSPEECHVOLUME 	0.2
#define SPEECHVOLUMESTEP 	0.1
#define CONNECTIONTIMEOUT	5 * 60 // seconds
#define COMMANDTIMEOUT		5 // seconds

#define MESSAGEBUFFERSIZE  	100 // max no chars in messagescreen

#define CPU_SPEED_LOW			400000
#define CPU_SPEED_HIGH			1400000

#define testMopje "/root/tiroler_holzhacker.mp3"  // voor opa Bolier
#define ringTone "/root/ringtone1.mp3"

extern int backLight,houseNo;
extern floor_t floorID;
extern volatile int exitCode;
extern uint32_t timeouts;
extern int exceptionCntr;

#define backLightOff() 	backLight = 0
#define backLightOn()  	backLight = 80
#define backLightHalf() backLight = 40  // 30 gaat soms niet goed

#define SETTINGSFILENAME "/root/telefoon.cfg"

void  print( const char *format, ...);
#endif /* CAMERA_H_ */
