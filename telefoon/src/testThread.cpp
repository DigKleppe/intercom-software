/*
 * testThread.c
 *
 *  Created on: Apr 6, 2019
 *      Author: dig
 */

#include "telefoon.h"
#include "io.h"
#include "keys.h"
#include "connections.h"

#include <gst/gst.h>

#include <stdio.h>
#include <pthread.h>
#include "videoThread.h"

#include <unistd.h>
#include <string.h> /* for strncpy */
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>



#define TEST_INTERVAL2			1000 // ms   * 1000 * 1000LL) // interval for checking stopping transmit/ receive threads in nanosecons


typedef enum { TEST_ETH, TEST_SPKR, TEST_SPKR_HANDSET, TEST_SWITCHES, TEST_MIC, TEST_END, TEST_RING} test_t ;

const char testText [TEST_END][20] = {
		{"test ethernet" },
		{"test luidspreker" },
		{"test hoornspkr" },
		//	{"test bel" },
		{"test schakelaars" },
		{"test microfoon" }
};

//echo userspace > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
//echo 800000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
//cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq
//Available frequencies :
//cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies
//1400000 1200000 1000000 800000 700000 600000 500000 400000
// videoflip method=counterclockwise
//
//gst-launch-1.0 videotestsrc ! video/x-raw,width=800,height=480 !  videoflip method=counterclockwise ! nxvideosink
//gst-launch-1.0 videotestsrc ! video/x-raw,width=800,height=480 ! textoverlay text="Room A \n Room B" ! videoflip method=counterclockwise ! nxvideosink
//gst-launch-1.0 videotestsrc ! textoverlay text="Room A \n Room B" valignment=top halignment=left font-desc="Sans, 72" ! video/x-raw,width=800,height=480 ! videoflip method=counterclockwise ! nxvideosink
//gst-launch-1.0 -v videotestsrc ! textoverlay text="Room A" valignment=top halignment=left font-desc="Sans, 72" ! autovideosink
//gst-launch-1.0 videotestsrc ! video/x-raw,width=800,height=600 ! jpegenc ! rtpjpegpay ! udpsink port=5001 host=192.168.2.255
//gst-launch-1.0 filesrc location=/home/root/audio/tiroler_holzhacker.mp3 ! mpegaudioparse ! mpg123audiodec ! audioconvert ! alsasink device=hw:1
//gst-launch-1.0 filesrc location=/home/root/audio/office_phone_4.mp3 ! mpegaudioparse ! mpg123audiodec ! audioconvert ! alsasink device=hw:1
//gst-launch-1.0 alsasrc device=hw:1 ! audioconvert ! audiopanorama panorama=1.00 ! alsasink device=hw:1
//gst-launch-1.0 alsasrc device=hw:1 ! audioconvert ! "audio/x-raw,rate=24000" ! audioresample ! wavescope ! videoconvert ! videoflip method=counterclockwise ! nxvideosink
//gst-launch-1.0 alsasrc device=hw:1 ! queue ! audioconvert !  wavescope !  video/x-raw,width=800,height=480 ! videoconvert ! videoflip method=counterclockwise ! nxvideosink
// gst-launch-1.0 -v alsasrc device=hw:1  ! audioconvert ! alsasink device=hw:1
//gst-launch-1.0 videotestsrc ! textoverlay text="Room A \n Room B" valignment=top halignment=left font-desc="Sans, 72" ! video/x-raw,width=800,height=480 ! videoflip method=counterclockwise ! nxvideosink


void* testThread(void* args)
{
	GstElement *audiopipeline,  *audioSource, *mpegaudioparser,* mpg123audiodec, *audioconvert;
	GstElement *volume , *audiopanorama ,*audiosink ;
	GstElement *videopipeline,*videoSource,  *wavescope, *queue,  *videoconvert, *videosink , *videoflip, *textoverlay;
	GstCaps *caps;
	GstBus *bus;
	GstMessage *msg;
	bool stop = false;
	GstStateChangeReturn ret;
	threadStatus_t  * pThreadStatus = (threadStatus_t  * )args;
	test_t  * ptest =  (test_t *) pThreadStatus->info;
	test_t test = *ptest;
	char str[50];
	uint32_t oldSwitches = 0;
	uint32_t oldCntr = 9999;
	float oldringVolume = ringVolume;
	ringVolume = 0.01;

	pThreadStatus->mustStop = false;
	pThreadStatus->run = true;
	videopipeline = NULL;
	audiopipeline = NULL;

	videoconvert = gst_element_factory_make ("videoconvert", "videoconvert");
	videosink = gst_element_factory_make ("nxvideosink", "nxvideosink");
	videoflip = gst_element_factory_make("videoflip", "videoflip");
	g_object_set (G_OBJECT (videoflip), "method",GST_VIDEO_FLIP_METHOD_90L ,NULL);

	switch (test) {
	case TEST_SPKR:
	case TEST_SPKR_HANDSET:
	case TEST_RING:
	case TEST_MIC:  // make audiopipeline
		if ( pThreadStatus->cardno > 0 ) {  // else no USB audiocard found
			audioconvert = gst_element_factory_make ("audioconvert", "audioconvert");
			volume = gst_element_factory_make ("volume", "volume");
			audiopanorama = gst_element_factory_make ("audiopanorama", "audiopanorama"); // balance
			audiosink = gst_element_factory_make ("alsasink", "alsasink");
			g_object_set (G_OBJECT (audiosink), "device","hw:1",NULL);

			audiopipeline = gst_pipeline_new ("testAudiopipeline");

			if (!audiopipeline || !audioconvert || !volume || !audiopanorama || !audiosink) {
				g_printerr ("Not all audio elements could be created.\n");
				stop = true;
			}

			switch (test) {
			case TEST_SPKR:
			case TEST_RING:
			case TEST_SPKR_HANDSET:
				//gst-launch-1.0 filesrc location=/home/root/audio/tiroler_holzhacker.mp3 ! mpegaudioparse ! mpg123audiodec !  volume volume=0.5 ! audioconvert  ! audiopanorama panorama=1.00 ! alsasink device=hw:1
				//gst-launch-1.0 filesrc location=/home/root/audio/office_phone_4.mp3 ! mpegaudioparse ! mpg123audiodec !  volume volume=0.01 ! audioconvert  ! audiopanorama panorama=1.00 ! alsasink device=hw:1

				audioSource = gst_element_factory_make ("filesrc", "filesrc");
				switch (test) {
				case TEST_SPKR:
					g_object_set (G_OBJECT (audioSource), "location",(gchar *) testMopje,NULL);
					g_object_set (G_OBJECT (volume), "volume",TESTVOLUME,NULL);
					g_object_set (G_OBJECT (audiopanorama), "panorama",SPEAKERCHANNEL,NULL);
					break;
				case TEST_SPKR_HANDSET:
					g_object_set (G_OBJECT (audioSource), "location",(gchar *) testMopje,NULL);
					g_object_set (G_OBJECT (volume), "volume",TESTVOLUME,NULL);
					g_object_set (G_OBJECT (audiopanorama), "panorama",HANDSETCHANNEL,NULL);
					break;
				case TEST_RING:
					g_object_set (G_OBJECT (audioSource), "location",(gchar *) ringTone,NULL);
					g_object_set (G_OBJECT (volume), "volume",ringVolume,NULL);
					g_object_set (G_OBJECT (audiopanorama), "panorama",SPEAKERCHANNEL,NULL);
					break;
				}
				mpegaudioparser = gst_element_factory_make ("mpegaudioparse", "mpegaudioparse");
				mpg123audiodec = gst_element_factory_make ("mpg123audiodec", "mpg123audiodec");
				if (!audioSource  || !mpegaudioparser || !mpg123audiodec ){
					g_printerr ("Not all audio elements could be created.\n");
					stop = true;
				}
				gst_bin_add_many (GST_BIN (audiopipeline), audioSource,mpegaudioparser,mpg123audiodec, volume, audioconvert, audiopanorama, audiosink ,NULL);

				if (gst_element_link (audioSource, mpegaudioparser  ) != TRUE)
					stop = true;

				if (gst_element_link (mpegaudioparser, mpg123audiodec ) != TRUE)
					stop = true;

				if (gst_element_link (mpg123audiodec, volume) != TRUE)
					stop = true;

				if (gst_element_link (volume, audioconvert) != TRUE)
					stop = true;

				if (gst_element_link (audioconvert, audiopanorama) != TRUE)
					stop = true;

				if (gst_element_link (audiopanorama, audiosink) != TRUE)
					stop = true;

				break;

				case TEST_MIC:
					//gst-launch-1.0 alsasrc device=hw:1 ! queue ! audioconvert !  wavescope !  video/x-raw,width=800,height=480 ! videoconvert ! videoflip method=counterclockwise ! nxvideosink
					audioSource = gst_element_factory_make ("alsasrc", "audioSource");
					char devicename[20];
					sprintf(devicename, "hw:%d",pThreadStatus->cardno);
					g_object_set (audioSource,"device",devicename,NULL);

					queue = gst_element_factory_make ("queue", "queue");
					audioconvert = gst_element_factory_make ("audioconvert", "audioconvert");

					wavescope = gst_element_factory_make ("wavescope", "wavescope");
					g_object_set (wavescope, "shader", 0, "style", 1, NULL);

					gst_bin_add_many (GST_BIN (audiopipeline), audioSource, queue, audioconvert, wavescope, videoconvert,videoflip, videosink ,NULL);

					if (gst_element_link (audioSource, queue  ) != TRUE)
						stop = true;
					if (gst_element_link (queue, audioconvert  ) != TRUE)
						stop = true;

					if (gst_element_link (audioconvert, wavescope  ) != TRUE)
						stop = true;

					caps = gst_caps_new_simple ("video/x-raw",
							//	"framerate",GST_TYPE_FRACTION, 30, 1,
							"width", G_TYPE_INT, 800,
							"height", G_TYPE_INT, 480,
							NULL);
					if (link_elements_with_filter (wavescope, videoconvert, caps ) != TRUE)
						stop = true;

					if (gst_element_link (videoconvert, videoflip ) != TRUE)
						stop = true;

					if (gst_element_link (videoflip, videosink  ) != TRUE)
						stop = true;
					break;
			} // end case test 2
			ret = gst_element_set_state (audiopipeline, GST_STATE_PLAYING);

			if (ret == GST_STATE_CHANGE_FAILURE) {
				g_printerr ("Unable to set the audio pipeline to the playing state.\n");
				stop = true;
			}
		} // end if (pThreadStatus->cardno > 0 )
	break;
	case TEST_SWITCHES: // no audio
		break;
	} // end case test 1


	// video
	switch (test){
	case TEST_SPKR:
	case TEST_SPKR_HANDSET:
	case TEST_RING:
	case TEST_SWITCHES:
	case TEST_ETH:
		videopipeline = gst_pipeline_new ("testVideopipeline");
		textoverlay = gst_element_factory_make("textoverlay", "textoverlay" );
		g_object_set (G_OBJECT (textoverlay), "valignment",GST_BASE_TEXT_OVERLAY_VALIGN_TOP,NULL);
		g_object_set (G_OBJECT (textoverlay), "halignment",GST_BASE_TEXT_OVERLAY_HALIGN_LEFT,NULL);
		g_object_set (G_OBJECT (textoverlay), "font-desc","Sans, 72",NULL);

		videoSource = gst_element_factory_make ("videotestsrc", "videoSource");
		gst_bin_add_many (GST_BIN (videopipeline),videoSource,textoverlay, videoflip, videosink, NULL);
		caps = gst_caps_new_simple ("video/x-raw",
				//	"framerate",GST_TYPE_FRACTION, 30, 1,
				"width", G_TYPE_INT, 800,
				"height", G_TYPE_INT, 480,
				NULL);
		if (link_elements_with_filter (videoSource, textoverlay, caps ) != TRUE)
			stop = true;
		if (gst_element_link (textoverlay , videoflip) != TRUE)
			stop = true;
		if (gst_element_link (videoflip , videosink) != TRUE)
			stop = true;
//
		if (pThreadStatus->cardno < 0)
			g_object_set (G_OBJECT (textoverlay), "text"," Geen USB geluidskaart",NULL);
		else
			g_object_set (G_OBJECT (textoverlay), "text",testText[test],NULL);

		usleep(1000);  // delay, else segm fault?

		ret = gst_element_set_state (videopipeline, GST_STATE_PLAYING);

		if (ret == GST_STATE_CHANGE_FAILURE) {
			g_printerr ("Unable to set the video pipeline to the playing state.\n");
			stop= true;
		}

		break;

	case TEST_MIC:  // no video pipeline
		backLightOn();
		break;
	}


	if ( stop)
		g_printerr ("linking error.\n");


	while( !pThreadStatus->mustStop && !stop ) {
		usleep(10 * 1000);
		msg = NULL;
		if ( audiopipeline  ) { // check audiopipeline
			bus = gst_element_get_bus (audiopipeline);
			msg = gst_bus_pop (bus);
			gst_object_unref (bus);
		}
		if ( videopipeline  ) {
			if ( msg == NULL) {  // test videopipeline if no troubles with audio
				bus = gst_element_get_bus (videopipeline);
				msg = gst_bus_pop (bus);
				gst_object_unref (bus);
			}
		}

		/* Parse message */

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
				stop = true;
				break;
			case GST_MESSAGE_EOS:
				g_print ("End-Of-Stream reached.\n");
				stop = true;
				break;
			default:

				//	g_printerr ("Unexpected message received.\n");
				break;
			}
			gst_message_unref (msg);

		}

		switch (test) {
		case TEST_SWITCHES:
			if ( keysRT != oldSwitches) {
				sprintf(str, "toets:\n");
				if (keysRT & KEY_P1)
					sprintf(str+ strlen (str), " sleutel\n");
				if (keysRT & KEY_P2)
					sprintf(str+ strlen( str), " 1\n");
				if (keysRT & KEY_P3)
					sprintf(str+ strlen( str), " 2\n");
				gst_element_set_state (videopipeline, GST_STATE_NULL);
				g_object_set (G_OBJECT (textoverlay), "text",str,NULL);
				gst_element_set_state (videopipeline, GST_STATE_PLAYING);
				oldSwitches = keysRT;
			}
			break;
		case TEST_ETH:
			if ( oldCntr != connectCntrBaseFloor) {
			//	if ( connectCntr1 == 5 ) {
					sprintf(str, " berichten:\n  %d\n", connectCntrBaseFloor);
				//	sprintf(str, "Ethernet OK", connectCntr1);
					gst_element_set_state (videopipeline, GST_STATE_NULL);
					g_object_set (G_OBJECT (textoverlay), "text",str,NULL);
					gst_element_set_state (videopipeline, GST_STATE_PLAYING);

			//	}
				printf("connects: %d\n",connectCntrBaseFloor);
				oldCntr = connectCntrBaseFloor;
			}
			break;

		}
	}

	printf("testThread stopped\n");

	if (audiopipeline) {
		gst_element_set_state (audiopipeline, GST_STATE_NULL);
		gst_object_unref(audiopipeline);
	}
	if ( videopipeline ){
		gst_element_set_state (videopipeline, GST_STATE_NULL);
		gst_object_unref(videopipeline);
	}


//	gst_object_unref (caps);
	ringVolume = oldringVolume;
	pThreadStatus->run = false;
	pthread_exit(args);
	return ( NULL);
}


void* testModeThread(void* args)
{
	bool stop= false;
	int test = TEST_ETH;
	int result;
	volatile threadStatus_t testStatus =  * (threadStatus_t *) args; // to pass to actual test - thread
	testStatus.info = &test; // set info to testno to exec

	threadStatus_t  * pThreadStatus = (threadStatus_t  * ) args;

	pThreadStatus->mustStop = false;
	pThreadStatus->run = true;

	pthread_t testThreadID;

	usleep(100 * 1000); // read keys
	key ( ALLKEYS );  // read all keys

	while(! stop){
		usleep(1000);
		result = pthread_create(&testThreadID, NULL, &testThread, (void *) &testStatus);
		if (result == 0) {
			printf("subtestThread created successfully.\n");
		} else {
			printf("subtestThread not created.\n");
			stop= true;
		}
		while (!testStatus.run) // wait until task runs
			usleep(1000);

		while ( testStatus.run){
			usleep(10000);
			if (key (KEY_HANDSET)) {
				testStatus.mustStop = true;
				while ( testStatus.run)
					usleep(1000);
				test++;
			}
		}
//		if (test == TEST_END)
//			test= TEST_SPKR;
		if (( pThreadStatus->mustStop) || (test == TEST_END)){
			testStatus.mustStop = true;
			while ( testStatus.run)
				usleep(1000);
			stop = true;
		}
	}
	pThreadStatus->run = false;
	pthread_exit(args);
	return ( NULL);
}
