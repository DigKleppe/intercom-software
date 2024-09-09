/*
 * audioTransmitThread.c
 *
 *  Created on: Jan 27, 2019
 *      Author: dig
 */

#include "camera.h"
#include "videoThread.h"

#include <gst/gst.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

/*
logitec: naam van arecord -L
gst-launch-1.0 -v alsasrc device=hw:3 ! audioconvert ! "audio/x-raw,rate=24000" ! audioresample ! alsasink
gst-launch-1.0 -v alsasrc device=hw:3 ! audioconvert ! "audio/x-raw,rate=24000" ! audioresample ! alsasink device=hw:2
gst-launch-1.0 -v alsasrc device=hw:3 ! audioconvert ! "audio/x-raw,rate=24000" ! audioresample ! vorbisenc ! rtpvorbispay config-interval=1 ! rtpstreampay ! udpsink port=5001 host=192.168.2.255

gst-launch-1.0 -v alsasrc device=hw:2 ! audioconvert ! wavescope ! ximagesink

arecord --list-devices
arecord -L
alsamixer -c 1
sudo alsactl store

// USB micro
gst-launch-1.0 -v alsasrc device=hw:2 ! audioconvert ! "audio/x-raw,rate=44100" ! rtpL16pay ! udpsink host=192.168.2.255 port=5002

logitec:
gst-launch-1.0 -v alsasrc device=hw:1 ! audioconvert ! "audio/x-raw,rate=24000" ! rtpL16pay ! udpsink host=192.168.2.255 port=5002
gst-launch-1.0 -v audiotestsrc freq=1000 ! audioconvert ! "audio/x-raw,rate=24000" ! rtpL16pay ! udpsink host=192.168.2.255 port=5002

 */


static GstElement *audiopipeline = NULL;
static GstElement *audioSource, *audioconvert,* rtpL16pay, *audiosink;

static streamerTask_t actualTask = TASK_STOP;

bool setAudioTransmitTask( streamerTask_t task, int UDPport , int soundCardNo)
{
	char devicename[20];
	bool error = false;
	GstCaps *caps;
	GstStateChangeReturn ret;

	switch (task ){

	case TASK_STOP:
		if (audiopipeline != NULL ) {
			gst_element_set_state (audiopipeline, GST_STATE_NULL);
			if (audiopipeline !=  NULL )
				gst_object_unref(audiopipeline);
			audiopipeline = NULL;
		}
		break;

	case AUDIOTASK_TALK:
		if ( actualTask != AUDIOTASK_TALK) {
			sprintf(devicename, "hw:%d",soundCardNo);

			printf("audioTx started\n");

			//	audioSource = gst_element_factory_make ("audiotestsrc", "audioSource");
			audioSource = gst_element_factory_make ("alsasrc", "audioSource");
			g_object_set (audioSource,"device",devicename,NULL);

			audioconvert = gst_element_factory_make ("audioconvert", "audioconvert");
			rtpL16pay = gst_element_factory_make ("rtpL16pay", "rtpL16pay");
			audiosink = gst_element_factory_make ("udpsink", "udpsink");
			g_object_set (audiosink, "port", UDPport, NULL);

			g_object_set (audiosink, "host", HOST, NULL );  // broadcast

			if (!audioSource || !audioconvert || !rtpL16pay|| !audiosink ) {
				g_printerr ("Not all audio elements could be created.\n");
				error = true;
			}

			audiopipeline = gst_pipeline_new ("transmit audiopipeline");
			gst_bin_add_many (GST_BIN (audiopipeline), audioSource, audioconvert, rtpL16pay, audiosink ,NULL);

			if (gst_element_link (audioSource, audioconvert  ) != TRUE) {
				g_printerr ("Elements could not be linked.\n");
				gst_object_unref (audiopipeline);
				error = true;
			}
			caps = gst_caps_new_simple ("audio/x-raw",
					//	"rate", G_TYPE_INT,24000,  //
					"rate", G_TYPE_INT,44100,
					NULL);

			if (link_elements_with_filter (audioconvert, rtpL16pay,caps) != TRUE) {
				g_printerr ("Filter element could not be linked.\n");
				gst_object_unref (audiopipeline);
				error = true;
			}

			if (gst_element_link (rtpL16pay, audiosink  ) != TRUE) {
				g_printerr ("Elements could not be linked.\n");
				gst_object_unref (audiopipeline);
				error = true;
			}

			ret = gst_element_set_state (audiopipeline, GST_STATE_PLAYING);
			if (ret == GST_STATE_CHANGE_FAILURE) {
				g_printerr ("AudioTransmit: Unable to set the audio pipeline to the playing state.\n");
				gst_object_unref (audiopipeline);
				error = true;
			}
		}
		break;
	}
	actualTask = task;
	return error;
}

bool audioTransmitterIsStopped ( void) {
	GstBus *bus;
	GstMessage *msg;
	GstStateChangeReturn ret;
	bool stopped = false;

	bus = gst_element_get_bus (audiopipeline);
	msg = gst_bus_pop(bus);
	gst_object_unref (bus);
	/* Parse message */
	if (msg != NULL) {
		GError *err;
		gchar *debug_info;
		switch (GST_MESSAGE_TYPE (msg)) {
		case GST_MESSAGE_ERROR:
			gst_message_parse_error (msg, &err, &debug_info);
			g_printerr ("AudioTransmit: Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
			g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
			g_clear_error (&err);
			g_free (debug_info);
			stopped = true;
			break;
		case GST_MESSAGE_EOS:
			g_print ("End-Of-Stream reached.\n");
			stopped = true;
			break;
		default:
			break;
		}
		gst_message_unref (msg);
	}
	return stopped;

}







//
//void* audioTransmitThread(void* args) {
////
//	GstElement *audiopipeline,  *audioSource, *audioconvert,* rtpL16pay, *audiosink;
//	GstBus *bus;
//	GstCaps *caps = NULL;
//	GstMessage *msg;
//	GstStateChangeReturn ret;
//	threadStatus_t  * pThreadStatus = args;
//	pThreadStatus->mustStop = false;
//	pThreadStatus->run = true;
//	char devicename[20];
//	bool error = false;
//
//
//	pThreadStatus->mustStop = false;
//	pThreadStatus->run = true;
//	runState_t runState = RUNSTATE_PAUSE;
//
//
//	print("audioTxthread started\n");
//
////audioSource = gst_element_factory_make ("audiotestsrc", "audioSource");
////	g_object_set (audioSource,"freq",1000.0f,NULL);
////	g_object_set (audioSource,"wave",0,NULL);
//
//	audioSource = gst_element_factory_make ("alsasrc", "audioSource");
//	sprintf(devicename, "hw:%d",pThreadStatus->microCardNo);
//	g_object_set (audioSource,"device",devicename,NULL);
//
//	audioconvert = gst_element_factory_make ("audioconvert", "audioconvert");
//	rtpL16pay = gst_element_factory_make ("rtpL16pay", "rtpL16pay");
//	audiosink = gst_element_factory_make ("udpsink", "udpsink");
//
//	if (myFloorID == BASE_FLOOR)
//		g_object_set (audiosink, "port", AUDIO_TX_PORT1, NULL);
//	else
//		g_object_set (audiosink, "port", AUDIO_TX_PORT2, NULL);
//
//	g_object_set (audiosink, "host", HOST, NULL );  // broadcast
//
//	if (!audioSource || !audioconvert || !rtpL16pay|| !audiosink ) {
//		g_printerr ("Not all audio elements could be created.\n");
//		error = true;
//	}
//
//	audiopipeline = gst_pipeline_new ("transmit audiopipeline");
//	gst_bin_add_many (GST_BIN (audiopipeline), audioSource, audioconvert, rtpL16pay, audiosink ,NULL);
//
//	if (gst_element_link (audioSource, audioconvert  ) != TRUE) {
//		g_printerr ("Elements could not be linked.\n");
//		gst_object_unref (audiopipeline);
//		error = true;
//	}
//	caps = gst_caps_new_simple ("audio/x-raw",
//		//	"rate", G_TYPE_INT,24000, // Logitec camera
//			"rate", G_TYPE_INT,44100, // USB audio
//			NULL);
//
//	if (link_elements_with_filter (audioconvert, rtpL16pay,caps) != TRUE) {
//		g_printerr ("Filter element could not be linked.\n");
//		gst_object_unref (audiopipeline);
//		error = true;
//	}
//
//	if (gst_element_link (rtpL16pay, audiosink  ) != TRUE) {
//		g_printerr ("Elements could not be linked.\n");
//		gst_object_unref (audiopipeline);
//		error = true;
//	}
//
////
//	while (1) {
//		usleep(10 * 1000);
//		pThreadStatus->runCntr++;
//		switch (runState) {
//		case RUNSTATE_RUN:
//			if (pThreadStatus->pause){
//				gst_element_set_state (audiopipeline, GST_STATE_NULL);
//				print ("audio paused\n");
//				runState = RUNSTATE_PAUSE;
//			}
//			break;
//
//		case RUNSTATE_PAUSE:
//			if (!pThreadStatus->pause){
//				ret = gst_element_set_state (audiopipeline, GST_STATE_PLAYING);
//				if (ret == GST_STATE_CHANGE_FAILURE) {
//					g_printerr ("Unable to set the audio pipeline to the playing state.\n");
//			//		error = true;
//				}
//				runState = RUNSTATE_RUN;
//				print ("audio running\n");
//			}
//			break;
//		}
//
////		ret = gst_element_set_state (audiopipeline, GST_STATE_PLAYING);
////		if (ret == GST_STATE_CHANGE_FAILURE) {
////			g_printerr ("Unable to set the audio pipeline to the playing state.\n");
////			gst_object_unref (audiopipeline);
////			error = true;
////		}
////		/* Wait until error or EOS */
////		bus = gst_element_get_bus (audiopipeline);
////		msg = gst_bus_pop(bus); // , GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
////		gst_object_unref (bus);
////		/* Parse message */
////		if (msg != NULL) {
////			GError *err;
////			gchar *debug_info;
////			switch (GST_MESSAGE_TYPE (msg)) {
////			case GST_MESSAGE_ERROR:
////				gst_message_parse_error (msg, &err, &debug_info);
////				g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
////				g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
////				g_clear_error (&err);
////				g_free (debug_info);
////				error = true;
////				break;
////			case GST_MESSAGE_EOS:
////				g_print ("End-Of-Stream reached.\n");
////				error = true;
////				break;
////			default:
////				/* We should not reach here because we only asked for ERRORs and EOS */
////				//g_printerr ("Unexpected message received.\n");
////				break;
////			}
////			gst_message_unref (msg);
////		}
//
//	}
//
//	print("audioTxthread stopped\n");
//	gst_element_set_state (audiopipeline, GST_STATE_NULL);
//	gst_object_unref(audiopipeline);
//	pThreadStatus->run = false;
//	pthread_exit(args);
//	return ( NULL);
//}
