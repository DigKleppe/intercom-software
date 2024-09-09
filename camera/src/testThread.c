/*
 * testThread.c
 *
 *  Created on: Apr 6, 2019
 *      Author: dig
 */

#include "camera.h"
#include "io.h"
#include "connections.h"

#include <gst/gst.h>

#include <stdio.h>
#include <pthread.h>
#include "videoThread.h"
#include "i2cThread.h"
#include "keys.h"

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#define TEST_INTERVAL2			1000 // ms   * 1000 * 1000LL) // interval for checking stopping transmit/ receive threads in nanosecons


typedef enum {TEST_SPKR,TEST_MIC, TEST_BELLBUTTONS, TEST_END} test_t ;

const char testText [TEST_END][20] = {
		//		{"test ethernet" },
		{"test luidspreker" },
		{"test microfoon" },
		{"test beltoetsen" }
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
//gst-launch-1.0 filesrc location=/root/tiroler_holzhacker.mp3 ! mpegaudioparse ! mpg123audiodec ! audioconvert ! alsasink device=hw:1
//gst-launch-1.0 filesrc location=/home/root/audio/office_phone_4.mp3 ! mpegaudioparse ! mpg123audiodec ! audioconvert ! alsasink device=hw:1
//gst-launch-1.0 alsasrc device=hw:1 ! audioconvert ! audiopanorama panorama=1.00 ! alsasink device=hw:1
//
//gst-launch-1.0 alsasrc device=hw:1 ! queue ! audioconvert !  wavescope !  video/x-raw,width=800,height=480 ! videoconvert ! videoflip method=counterclockwise ! nxvideosink
// gst-launch-1.0 -v alsasrc device=hw:1  ! audioconvert ! alsasink device=hw:1
//gst-launch-1.0 videotestsrc ! textoverlay text="Room A \n Room B" valignment=top halignment=left font-desc="Sans, 72" ! video/x-raw,width=800,height=480 ! videoflip method=counterclockwise ! nxvideosink

/*
gst-launch-1.0 filesrc location=/root/tiroler_holzhacker.mp3 ! mpegaudioparse ! mpg123audiodec ! audioconvert ! "audio/x-raw, format="F32LE" rate=24000" ! audioconvert ! audiocheblimit mode=high-pass cutoff=400 ripple=0.2 !  alsasink device=hw:1

*/
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
	threadStatus_t  * pThreadStatus = args;
	test_t  * ptest =  (test_t *) pThreadStatus->info;
	test_t test = *ptest;
	char str[50];
	char devicename[20];
	uint32_t oldBellButtons = 0;
	uint32_t oldCntr = 9999;
	uint32_t mask;

	char text[100] = {0};

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
	case TEST_MIC:  // make audiopipeline
		if ( pThreadStatus->microCardNo > 0 ) {  // else no USB audiocard found
			audioconvert = gst_element_factory_make ("audioconvert", "audioconvert");	//	result = pthread_create(&testThreadID, NULL, &testModeThread, (void *) &testThreadStatus);
			volume = gst_element_factory_make ("volume", "volume");
			audiopanorama = gst_element_factory_make ("audiopanorama", "audiopanorama"); // balance
			audiosink = gst_element_factory_make ("alsasink", "alsasink");

			sprintf(devicename, "hw:%d",pThreadStatus->spkrCardNo);
			g_object_set (audiosink,"device",devicename,NULL);

			audiopipeline = gst_pipeline_new ("testAudiopipeline");

			if (!audiopipeline || !audioconvert || !volume || !audiopanorama || !audiosink) {
				g_printerr ("Not all audio elements could be created.\n");
				stop = true;
			}
			switch (test) {
			case TEST_SPKR:
				audioSource = gst_element_factory_make ("filesrc", "filesrc");
				switch (test) {
				case TEST_SPKR:
					g_object_set (G_OBJECT (audioSource), "location",(gchar *) testMopje,NULL);
					g_object_set (G_OBJECT (volume), "volume",TESTVOLUME,NULL);
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
					sprintf(devicename, "hw:%d",pThreadStatus->microCardNo);
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

					caps = gst_caps_new_simple ("audio/x-raw",
						//	"rate", G_TYPE_INT, 24000,  // logitec
							"rate", G_TYPE_INT, 44100,  // USB audio
							NULL);

					if (link_elements_with_filter (audioconvert, wavescope, caps ) != TRUE)
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
	case TEST_BELLBUTTONS: // no audio
		break;
	} // end case test 1


	// video
	switch (test){
	case TEST_SPKR:
	case TEST_BELLBUTTONS:
		//	case TEST_ETH:
		videopipeline = gst_pipeline_new ("testVideopipeline");
		textoverlay = gst_element_factory_make("textoverlay", "textoverlay" );
		g_object_set (G_OBJECT (textoverlay), "valignment",GST_BASE_TEXT_OVERLAY_VALIGN_TOP,NULL);
		g_object_set (G_OBJECT (textoverlay), "halignment",GST_BASE_TEXT_OVERLAY_HALIGN_LEFT,NULL);
		g_object_set (G_OBJECT (textoverlay), "font-desc","Sans, 30",NULL);

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
		if (pThreadStatus->microCardNo < 0)
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
		}
		if ( videopipeline  ) {
			if ( msg == NULL) {  // test videopipeline if no troubles with audio
				bus = gst_element_get_bus (videopipeline);
				msg = gst_bus_pop (bus);
			}
		}
		/* Parse message */
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
		case TEST_BELLBUTTONS:  // add pressed buttons to list (print housen nos)
			mask = 1;
			int n = 2;
			bool changed = false;
			do {
				if ( bellButtons & mask) {
					if ( !( oldBellButtons & mask)) {
						sprintf( text + strlen(text), " %d" ,n);
						oldBellButtons |= mask;
						changed = true;
					}
				}
				n+=2;
				mask <<= 1;
			} while ( mask != (1<<31));

			if (bellButtons & 1 )
				openDoor = true;
			else
				openDoor = false;

			LEDD4 = bellButtons;

			if ( changed) {
				gst_element_set_state (videopipeline, GST_STATE_NULL);
				g_object_set (G_OBJECT (textoverlay), "text",text,NULL);
				gst_element_set_state (videopipeline, GST_STATE_PLAYING);
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
//	gst_object_unref (bus);
	pThreadStatus->run = false;
	pthread_exit(args);
	return ( NULL);
}


void* testModeThread(void* args)
{
	bool stop = false;
	test_t test = TEST_SPKR;
//	test_t test = TEST_MIC;
	int result;
	volatile threadStatus_t testStatus =  * (threadStatus_t *) args; // to pass to actual test - thread
	testStatus.info = &test;

	threadStatus_t  * pThreadStatus = args;

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
			if (key (KEY_SW1)) {
				testStatus.mustStop = true;
				while ( testStatus.run)
					usleep(1000);
				test++;
			}
		}
		if (( pThreadStatus->mustStop) || (test == TEST_END)){
			testStatus.mustStop = true;
			stop = true;
			while ( testStatus.run)
				usleep(1000);
		}
	}
	printf("testmode ended.\n");
	pThreadStatus->run = false;
	pthread_exit(args);
	return ( NULL);
}
