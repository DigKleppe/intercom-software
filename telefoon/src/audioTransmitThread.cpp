/*
 * audioThread.c
 *
 *  Created on: Jan 27, 2019
 *      Author: dig
 */
// balance:
//gst-launch-1.0 audiotestsrc wave=saw ! audioconvert ! audiopanorama panorama=-1.00 ! audioconvert ! alsasink

#include "telefoon.h"
#include "videoThread.h"
#include <stdbool.h>
#include <gst/gst.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
/*
logitec: naam van arecord -L
gst-launch-1.0 -v alsasrc device=hw:3 ! audioconvert ! "audio/x-raw,rate=24000" ! audioresample ! alsasink
gst-launch-1.0 -v alsasrc device=hw:3 ! audioconvert ! "audio/x-raw,rate=24000" ! audioresample ! alsasink device=hw:2
gst-launch-1.0 -v alsasrc device=hw:1 ! audioconvert ! "audio/x-raw,rate=44100" ! audioresample ! vorbisenc ! rtpvorbispay config-interval=1 ! rtpstreampay ! udpsink port=5004 host=192.168.2.255

gst-launch-1.0 -v alsasrc device=hw:1 ! audioconvert ! "audio/x-raw,rate=44100,format=(string)S16BE" ! rtpL16pay  ! udpsink host=192.168.2.255 port=5004
gst-launch-1.0 -v alsasrc device=hw:1 ! audioconvert ! "audio/x-raw,rate=44100,format=(string)S16BE" ! volume volume=2 ! audioconvert ! rtpL16pay  ! udpsink host=192.168.2.255 port=5004
gst-launch-1.0 -v alsasrc device=hw:2 ! volume volume=2 ! audioconvert ! "audio/x-raw,rate=44100" ! rtpL16pay  ! udpsink host=192.168.2.255 port=5004

gst-launch-1.0 -v alsasrc device=hw:2 ! volume volume=1 ! audioconvert ! "audio/x-raw,rate=44100" ! audioresample ! alsasink device=hw:2

gst-launch-1.0 -v alsasrc device=hw:1 ! audioconvert ! "audio/x-raw,rate=44100" ! alsasink device=hw:1

gst-launch-1.0 -v audiotestsrc freq=1000 ! audioconvert ! "audio/x-raw,rate=44100,format=(string)S16BE" ! rtpL16pay  ! udpsink host=192.168.2.255 port=5004
arecord --list-devices
arecord -L
alsamixer -c 1


gst-inspect-1.0 alsasrc
 */

static GstElement *audiopipeline = NULL;
static GstElement *audioSource, *audioconvert,* rtpL16pay, *audiosink;

#define PRINTSTEP() 	printf(" %d ",step++); usleep(1000)

static streamerTask_t actualTask = TASK_STOP;

int audioTransmitterIsStoppedkExceptionCntr;
int setAudioTransmitTaskExceptionCntr;

void setAudioTransmitTask( streamerTask_t task, int UDPport , int SoundCardNo)
{
	char devicename[20];
	bool error = false;
	GstCaps *caps;
	GstStateChangeReturn ret;
	int step = 1;

	try {
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
				sprintf(devicename, "hw:%d",SoundCardNo);

				printf("audioTx started\n");

				//	audioSource = gst_element_factory_make ("audiotestsrc", "audioSource");
				audioSource = gst_element_factory_make ("alsasrc", "audioSource");

				PRINTSTEP();
				g_object_set (audioSource,"device",devicename,NULL);

				PRINTSTEP();
				audioconvert = gst_element_factory_make ("audioconvert", "audioconvert");

				PRINTSTEP();
				rtpL16pay = gst_element_factory_make ("rtpL16pay", "rtpL16pay");

				PRINTSTEP();
				audiosink = gst_element_factory_make ("udpsink", "udpsink");

				PRINTSTEP();
				g_object_set (audiosink, "port", UDPport, NULL);

				PRINTSTEP();
				g_object_set (audiosink, "host", HOST, NULL );  // broadcast


				if (!audioSource || !audioconvert || !rtpL16pay|| !audiosink ) {
					g_printerr ("Not all audio elements could be created.\n");
					error = true;
				}

				PRINTSTEP();

				audiopipeline = gst_pipeline_new ("transmit audiopipeline");
				gst_bin_add_many (GST_BIN (audiopipeline), audioSource, audioconvert, rtpL16pay, audiosink ,NULL);

				PRINTSTEP();
				if (gst_element_link (audioSource, audioconvert  ) != TRUE) {
					g_printerr ("Elements could not be linked.\n");
					gst_object_unref (audiopipeline);
					error = true;
				}

				PRINTSTEP();
				caps = gst_caps_new_simple ("audio/x-raw",
						//	"rate", G_TYPE_INT,24000,  //
						"rate", G_TYPE_INT,44100,
						NULL);
				PRINTSTEP();
				if (link_elements_with_filter (audioconvert, rtpL16pay,caps) != TRUE) {
					g_printerr ("Filter element could not be linked.\n");
					gst_object_unref (audiopipeline);
					error = true;
				}
				PRINTSTEP();

				if (gst_element_link (rtpL16pay, audiosink  ) != TRUE) {
					g_printerr ("Elements could not be linked.\n");
					gst_object_unref (audiopipeline);
					error = true;
				}
				PRINTSTEP();
				ret = gst_element_set_state (audiopipeline, GST_STATE_PLAYING);
				if (ret == GST_STATE_CHANGE_FAILURE) {
					g_printerr ("AudioTransmit: Unable to set the audio pipeline to the playing state.\n");
					gst_object_unref (audiopipeline);
					error = true;
				}
				PRINTSTEP();
			}
			break;
		default:
			break;
		}
	}

	catch (int e)
	{
		g_printerr("An exception occurred. Exception Nr. %d\n\r",e);
		setAudioTransmitTaskExceptionCntr++;
		exceptionCntr++;
	}

	actualTask = task;
}

bool audioTransmitterIsStopped ( void) {
	GstBus *bus;
	GstMessage *msg;
	GstStateChangeReturn ret;
	bool stopped = false;
	try  {
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
	}
	catch (int e)
	{
		g_printerr("An exception occurred. Exception Nr. %d\n\r",e);
		audioTransmitterIsStoppedkExceptionCntr++;
		exceptionCntr++;
	}

	return stopped;

}

