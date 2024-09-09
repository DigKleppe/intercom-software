/*
 * audioReceiveThread.c
 *
 *  Created on: Apr 28, 2019
 *      Author: dig
 */

// balance:
//gst-launch-1.0 audiotestsrc wave=saw ! audioconvert ! audiopanorama panorama=-1.00 ! audioconvert ! alsasink

#define HANDSETVOLUME 1.0
/*
logitec: naam van arecord -L
gst-launch-1.0 -v alsasrc device=hw:3 ! audioconvert ! "audio/x-raw,rate=24000" ! audioresample ! alsasink
gst-launch-1.0 -v alsasrc device=hw:3 ! audioconvert ! "audio/x-raw,rate=24000" ! audioresample ! alsasink device=hw:2
gst-launch-1.0 -v alsasrc device=hw:3 ! audioconvert ! "audio/x-raw,rate=24000" ! audioresample ! vorbisenc ! rtpvorbispay config-interval=1 ! rtpstreampay ! udpsink port=5001 host=192.168.2.255

transmitter side station1:
gst-launch-1.0 -v alsasrc device=hw:1 ! audioconvert ! "audio/x-raw,rate=24000" ! audioresample ! vorbisenc ! rtpvorbispay config-interval=1 ! rtpstreampay ! udpsink port=5002 host=192.168.2.255

receiver side:
gst-launch-1.0 udpsrc port=5002 do-timestamp=true ! "application/x-rtp-stream,media=audio,clock-rate=24000,encoding-name=VORBIS" ! rtpstreamdepay ! rtpvorbisdepay ! decodebin ! audioconvert ! audioresample ! alsasink device=hw:1

gst-launch-1.0 udpsrc port=5002 do-timestamp=true ! "application/x-rtp-stream,media=audio,clock-rate=24000,encoding-name=VORBIS,configuration=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" ! rtpstreamdepay ! rtpvorbisdepay ! decodebin ! audioconvert ! audioresample ! alsasink device=hw:1
gst-launch-1.0 udpsrc port=5002 ! "application/x-rtp-stream,media=audio,clock-rate=24000,encoding-name=VORBIS,configuration=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" ! rtpstreamdepay ! rtpvorbisdepay ! decodebin ! audioconvert ! audioresample ! alsasink device=hw:1
arecord --list-devices
arecord -L
alsamixer -c 1
sudo alsactl store
aplay -l

gst-launch-1.0 -v audiotestsrc ! audioconvert ! "audio/x-raw,rate=44100" ! rtpL16pay ! udpsink host=192.168.2.255 port=5002
gst-launch-1.0 -v udpsrc port=5002 ! "application/x-rtp,media=(string)audio, clock-rate=(int)444100, width=16, height=16, encoding-name=(string)L16, encoding-params=(string)1, channels=(int)1, channel-positions=(int)1, payload=(int)96" ! rtpL16depay ! audioconvert ! alsasink device=hw:1,0 sync=false &

gst-launch-1.0 -v audiotestsrc ! audioconvert ! "audio/x-raw,rate=44100" ! rtpL16pay ! udpsink host=192.168.2.255 port=5002

gst-launch-1.0 -v audiotestsrc ! audioconvert ! "audio/x-raw,rate=44100,encoding-name=S16BE" ! rtpL16pay ! udpsink host=192.168.2.255 port=5002
Setting pipeline to PAUSED ...
Pipeline is PREROLLING ...
/GstPipeline:pipeline0/GstAudioTestSrc:audiotestsrc0.GstPad:src: caps = "audio/x-raw\,\ layout\=\(string\)interleaved\,\ rate\=\(int\)44100\,\ format\=\(string\)S16BE\,\ channels\=\(int\)1"
/GstPipeline:pipeline0/GstAudioConvert:audioconvert0.GstPad:src: caps = "audio/x-raw\,\ layout\=\(string\)interleaved\,\ rate\=\(int\)44100\,\ format\=\(string\)S16BE\,\ channels\=\(int\)1"
/GstPipeline:pipeline0/GstCapsFilter:capsfilter0.GstPad:src: caps = "audio/x-raw\,\ layout\=\(string\)interleaved\,\ rate\=\(int\)44100\,\ format\=\(string\)S16BE\,\ channels\=\(int\)1"
/GstPipeline:pipeline0/GstRtpL16Pay:rtpl16pay0.GstPad:src: caps = "application/x-rtp\,\ media\=\(string\)audio\,\ clock-rate\=\(int\)44100\,\ encoding-name\=\(string\)L16\,\ encoding-params\=\(string\)1\,\ channels\=\(int\)1\,\ payload\=\(int\)96\,\ ssrc\=\(uint\)1900555378\,\ timestamp-offset\=\(uint\)3541881202\,\ seqnum-offset\=\(uint\)26844"
/GstPipeline:pipeline0/GstUDPSink:udpsink0.GstPad:sink: caps = "application/x-rtp\,\ media\=\(string\)audio\,\ clock-rate\=\(int\)44100\,\ encoding-name\=\(string\)L16\,\ encoding-params\=\(string\)1\,\ channels\=\(int\)1\,\ payload\=\(int\)96\,\ ssrc\=\(uint\)1900555378\,\ timestamp-offset\=\(uint\)3541881202\,\ seqnum-offset\=\(uint\)26844"
/GstPipeline:pipeline0/GstRtpL16Pay:rtpl16pay0.GstPad:sink: caps = "audio/x-raw\,\ layout\=\(string\)interleaved\,\ rate\=\(int\)44100\,\ format\=\(string\)S16BE\,\ channels\=\(int\)1"
/GstPipeline:pipeline0/GstCapsFilter:capsfilter0.GstPad:sink: caps = "audio/x-raw\,\ layout\=\(string\)interleaved\,\ rate\=\(int\)44100\,\ format\=\(string\)S16BE\,\ channels\=\(int\)1"
/GstPipeline:pipeline0/GstAudioConvert:audioconvert0.GstPad:sink: caps = "audio/x-raw\,\ layout\=\(string\)interleaved\,\ rate\=\(int\)44100\,\ format\=\(string\)S16BE\,\ channels\=\(int\)1"
/GstPipeline:pipeline0/GstRtpL16Pay:rtpl16pay0: timestamp = 3541881202
/GstPipeline:pipeline0/GstRtpL16Pay:rtpl16pay0: seqnum = 26844


port 5002 van camera

gst-launch-1.0 -v audiotestsrc freq=1000 ! audioconvert ! "audio/x-raw,rate=24000" ! rtpL16pay ! udpsink host=192.168.2.255 port=5002
gst-launch-1.0 -v alsasrc device=hw:1 ! audioconvert ! "audio/x-raw,rate=24000" ! rtpL16pay ! udpsink host=192.168.2.255 port=5002


gst-launch-1.0 -v audiotestsrc ! audioconvert ! "audio/x-raw,rate=441000" ! rtpL16pay ! udpsink host=192.168.2.255 port=5004
gst-launch-1.0 udpsrc port=5002 caps='application/x-rtp, media=(string)audio, clock-rate=(int)24000, encoding-name=(string)L16, encoding-params=(string)1, channels=(int)1, payload=(int)96' ! rtpL16depay ! audioconvert ! audioresample ! audiopanorama panorama=-1 ! alsasink device=hw:1
gst-launch-1.0 udpsrc port=5002 caps='application/x-rtp, media=(string)audio, clock-rate=(int)44100, encoding-name=(string)L16, encoding-params=(string)1, channels=(int)2, payload=(int)96' ! rtpL16depay ! audioconvert ! audioresample ! audiopanorama panorama=-1 !  alsasink device=hw:1

gst-launch-1.0 udpsrc port=5002 ! "application/x-rtp, media=(string)audio, clock-rate=(int)24000, encoding-name=(string)L16, encoding-params=(string)1, channels=(int)2, payload=(int)96" ! rtpL16depay ! audioconvert ! audioresample ! audiopanorama panorama=-1 ! autoaudiosink
 */

/*
 * audioReceiveThread.c
 *
 *  Created on: Apr 28, 2019
 *      Author: dig
 */

// balance:
//gst-launch-1.0 audiotestsrc wave=saw ! audioconvert ! audiopanorama panorama=-1.00 ! audioconvert ! alsasink

#include "telefoon.h"
#include "videoThread.h"
#include "audioReceiveThread.h"

#include <stdbool.h>
#include <gst/gst.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

int stopAudioReceiveExceptionCntr;
int setAudioReceiveTaskExceptionCntr;
int audioReceiverIsStoppedExceptionCntr;

//#define SPKRVOLUME 0.3 volume by alsa
// arm-cortexa9-linuxgnueabihf

//#define WAVESCOPE

/*
logitec: naam van arecord -L
gst-launch-1.0 -v alsasrc device=hw:3 ! audioconvert ! "audio/x-raw,rate=24000" ! audioresample ! alsasink
gst-launch-1.0 -v alsasrc device=hw:3 ! audioconvert ! "audio/x-raw,rate=24000" ! audioresample ! alsasink device=hw:2
gst-launch-1.0 -v alsasrc device=hw:3 ! audioconvert ! "audio/x-raw,rate=24000" ! audioresample ! vorbisenc ! rtpvorbispay config-interval=1 ! rtpstreampay ! udpsink port=5001 host=192.168.2.255

transmitter side station1:
gst-launch-1.0 -v alsasrc device=hw:1 ! audioconvert ! "audio/x-raw,rate=24000" ! audioresample ! vorbisenc ! rtpvorbispay config-interval=1 ! rtpstreampay ! udpsink port=5002 host=192.168.2.255

receiver side:
gst-launch-1.0 udpsrc port=5004 do-timestamp=true ! "application/x-rtp-stream,media=audio,clock-rate=44100,encoding-name=VORBIS" ! rtpstreamdepay ! rtpvorbisdepay ! decodebin ! audioconvert ! audioresample ! alsasink device=hw:1

gst-launch-1.0 udpsrc port=5004 do-timestamp=true ! "application/x-rtp-stream,media=audio,clock-rate=44100,encoding-name=VORBIS,configuration=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" ! rtpstreamdepay ! rtpvorbisdepay ! decodebin ! audioconvert ! audioresample ! alsasink device=hw:2
gst-launch-1.0 udpsrc port=5002 ! "application/x-rtp-stream,media=audio,clock-rate=24000,encoding-name=VORBIS,configuration=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" ! rtpstreamdepay ! rtpvorbisdepay ! decodebin ! audioconvert ! audioresample ! alsasink device=hw:1
arecord --list-devices
arecord -L
alsamixer -c 1
aplay -l

gst-launch-1.0 -v audiotestsrc ! audioconvert ! "audio/x-raw,rate=44100" ! rtpL16pay ! udpsink host=192.168.2.255 port=5002
gst-launch-1.0 -v udpsrc port=5002 ! "application/x-rtp,media=(string)audio, clock-rate=(int)444100, width=16, height=16, encoding-name=(string)L16, encoding-params=(string)1, channels=(int)1, channel-positions=(int)1, payload=(int)96" ! rtpL16depay ! audioconvert ! alsasink device=hw:1,0 sync=false &

gst-launch-1.0 -v audiotestsrc ! audioconvert ! "audio/x-raw,rate=441000" ! rtpL16pay ! udpsink host=192.168.2.255 port=5004


gst-launch-1.0 -v audiotestsrc ! audioconvert ! "audio/x-raw,rate=24000" ! rtpL16pay ! udpsink host=192.168.2.255 port=5002
gst-launch-1.0 udpsrc port=5004 caps='application/x-rtp, media=(string)audio, clock-rate=(int)24000, encoding-name=(string)L16, encoding-params=(string)1, channels=(int)1, payload=(int)96' ! rtpL16depay ! audioconvert ! audioresample ! alsasink device=hw:1
gst-launch-1.0 udpsrc port=5004 caps='application/x-rtp, media=(string)audio, clock-rate=(int)44100, encoding-name=(string)L16, encoding-params=(string)1, channels=(int)1, payload=(int)96' ! rtpL16depay ! audioconvert ! audioresample ! alsasink device=hw:2

gst-launch-1.0 udpsrc port=5004 caps='application/x-rtp, media=(string)audio, clock-rate=(int)44100, encoding-name=(string)L16, encoding-params=(string)1, channels=(int)1, payload=(int)96' ! rtpL16depay ! audioconvert ! wavescope ! ximagesink
 */

#define RINGVOLUME 0.1 // local to feedback button

#define ringTone "/root/ringtone1.mp3"

static GstElement *audiopipeline = NULL;
static GstElement *audioSource, *rtpL16depay, *audioconvert,*volume, * audiopanorama, * audioresample ,*audiosink;
static GstElement *mpegaudioparser,* mpg123audiodec;

static streamerTask_t actualTask;

static bool stopAudioReceive() {
	try {

		if (audiopipeline !=  NULL ) {
			gst_element_set_state (audiopipeline, GST_STATE_NULL);
			gst_object_unref(audiopipeline);
			audiopipeline = NULL;
			print("audio stopped\n");
		}
	}
	catch (int e)
	{
		g_printerr("An exception occurred. Exception Nr. %d\n\r",e);
		stopAudioReceiveExceptionCntr++;
		exceptionCntr++;
	}
}

bool setAudioReceiveTask ( streamerTask_t task, int UDPport , int SoundCardNo) {

	bool error = false;
	GstCaps *caps = NULL;
	GstStateChangeReturn ret;
	char devicename[20];

	sprintf(devicename, "hw:%d",SoundCardNo);

	try {

		switch (task ){

		case TASK_STOP:
			stopAudioReceive();
			break;

		case AUDIOTASK_RING:
			if ( actualTask != AUDIOTASK_RING) {
				stopAudioReceive();
				audiopipeline = gst_pipeline_new ("audiopipeline");
				audioSource = gst_element_factory_make ("filesrc", "filesrc");
				g_object_set (G_OBJECT (audioSource), "location",(gchar *) ringTone,NULL);
				mpegaudioparser = gst_element_factory_make ("mpegaudioparse", "mpegaudioparse");
				mpg123audiodec = gst_element_factory_make ("mpg123audiodec", "mpg123audiodec");

				volume = gst_element_factory_make ("volume", "volume");
				g_object_set (G_OBJECT (volume), "volume",ringVolume/10.0,NULL);

				audioconvert = gst_element_factory_make ("audioconvert", "audioconvert");
				audiosink = gst_element_factory_make ("alsasink", "alsasink");

				audiopanorama = gst_element_factory_make ("audiopanorama", "audiopanorama");
				g_object_set (audiopanorama,"panorama",SPEAKERCHANNEL,NULL);  // sound to speaker

				g_object_set (G_OBJECT (audiosink), "device",devicename,NULL);

				if (!audiopipeline || !audioconvert || !volume || !audiosink) {
					g_printerr ("Not all audio elements could be created.\n");
					error = true;
				}
				if (!audioSource  || !mpegaudioparser || !mpg123audiodec ){
					g_printerr ("Not all audio elements could be created.\n");
					error = true;
				}

				gst_bin_add_many (GST_BIN (audiopipeline), audioSource,mpegaudioparser,mpg123audiodec, volume, audioconvert,audiopanorama, audiosink ,NULL);

				if (gst_element_link (audioSource, mpegaudioparser  ) != TRUE)
					error = true;

				if (gst_element_link (mpegaudioparser, mpg123audiodec ) != TRUE)
					error = true;

				if (gst_element_link (mpg123audiodec, volume) != TRUE)
					error = true;

				if (gst_element_link (volume, audioconvert) != TRUE)
					error = true;

				if (gst_element_link (audioconvert, audiopanorama  ) != TRUE)
					error = true;

				if (gst_element_link (audiopanorama, audiosink) != TRUE)
					error = true;

				if (!error) {
					ret = gst_element_set_state (audiopipeline, GST_STATE_PLAYING);
					if (ret == GST_STATE_CHANGE_FAILURE) {
						g_printerr ("Rngtone:  Unable to set the audio pipeline to the playing state.\n");
						error = true;
					}
				}
				if ( error)
					print("ringThread error\n");
				else {
					print( "ring started\n");
				}

			}
			break;


		case AUDIOTASK_LISTEN:
			if ( actualTask != AUDIOTASK_LISTEN) {
				stopAudioReceive();
				print("audio listen started port %d\n",UDPport);

				audioSource = gst_element_factory_make ("udpsrc", "udpsrc");
				g_object_set (audioSource, "port", UDPport, NULL);

				rtpL16depay = gst_element_factory_make ("rtpL16depay", "rtpL16depay");

				audioconvert = gst_element_factory_make ("audioconvert", "audioconvert");
				audioresample = gst_element_factory_make ("audioresample", "audioresample");
				audiopanorama = gst_element_factory_make ("audiopanorama", "audiopanorama");
				g_object_set (audiopanorama,"panorama",HANDSETCHANNEL,NULL);  // sound to handset

				volume = gst_element_factory_make ("volume", "volume");
				g_object_set (volume,"volume",HANDSETVOLUME,NULL);

				audiosink = gst_element_factory_make ("alsasink", "alsasink");
				g_object_set (audiosink,"device",devicename,NULL);

				if (!audioSource || !rtpL16depay ||!audioconvert || !audioresample || !audiopanorama || !audiosink || !volume) {
					g_printerr ("Not all audio elements could be created.\n");
					error = true;
				}

				audiopipeline = gst_pipeline_new ("receive audiopipeline");
				gst_bin_add_many (GST_BIN (audiopipeline), audioSource, rtpL16depay,
						audioconvert, audioresample, volume, audiopanorama, audiosink ,NULL);


				caps = gst_caps_new_simple ("application/x-rtp",
						"media", G_TYPE_STRING,"audio",
						//	"clock-rate", G_TYPE_INT,24000, // logitec
						"clock-rate", G_TYPE_INT,44100, // USB sound
						"encoding-name",G_TYPE_STRING,"L16",
						"encoding-params",(G_TYPE_STRING),"1",
						"channels", G_TYPE_INT, 1,
						"payload", G_TYPE_INT,96,
						NULL);

				if (link_elements_with_filter (audioSource, rtpL16depay, caps) != TRUE)
					error = true;

				if (gst_element_link (rtpL16depay, audioconvert  ) != TRUE)
					error = true;

				if (gst_element_link (audioconvert, audioresample  ) != TRUE)
					error = true;

				if (gst_element_link (audioresample, volume  ) != TRUE)
					error = true;

				if (gst_element_link (volume, audiopanorama  ) != TRUE)
					error = true;

				if (gst_element_link (audiopanorama, audiosink  ) != TRUE)
					error = true;

				usleep(10 * 1000);
				if (error)
					g_printerr ("Elements could not be linked.\n");
				else {
					ret = gst_element_set_state (audiopipeline, GST_STATE_PLAYING);
					if (ret == GST_STATE_CHANGE_FAILURE) {
						g_printerr (" audio Receive Unable to set the audio pipeline to the playing state.\n");
						error = true;
					}
				}
			}
			break;

		default:
			break;
		}
	}
	catch (int e)
	{
		g_printerr("An exception occurred. Exception Nr. %d\n\r",e);
		setAudioReceiveTaskExceptionCntr++;
		exceptionCntr++;
	}

	actualTask = task;
	return error;
}

bool audioReceiverIsStopped ( void) {

	GstBus *bus = NULL;
	GstMessage *msg;
	bool stopped = false;

	try{

		if ( audiopipeline != NULL ){
			bus = gst_element_get_bus (audiopipeline);
			msg = gst_bus_pop (bus);
			gst_object_unref (bus);
			if (msg != NULL) {
				GError *err;
				gchar *debug_info;
				switch (GST_MESSAGE_TYPE (msg)) {
				case GST_MESSAGE_ERROR:
					gst_message_parse_error (msg, &err, &debug_info);
					g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
					g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
					g_clear_error (&err);
					g_free (debug_info);
					stopped = true;
					break;
				case GST_MESSAGE_EOS:
					printf("End-Of-Stream reached.\n");
					stopped = true;
					//	gst_element_set_state (audiopipeline, GST_STATE_PLAYING); // start again
					break;
				default:
					//	g_printerr ("Unexpected message received.\n");
					break;
				}
				gst_message_unref (msg);
			}
		}
		else
			stopped = true;
	}

	catch (int e)
	{
		g_printerr("An exception occurred. Exception Nr. %d\n\r",e);
		audioReceiverIsStoppedExceptionCntr++;
		exceptionCntr++;
	}

	return stopped;
}

streamerTask_t getAudioReceiveTask() {
	return actualTask;
}

