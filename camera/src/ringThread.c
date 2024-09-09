///*
// * ringThread.c
// *
// *  Created on: May 5, 2019
// *      Author: dig
// */
//
//#include "camera.h"
//#include "keys.h"
//#include "connections.h"
//
//#include <gst/gst.h>
//#include <stdio.h>
//#include <pthread.h>
//#include <unistd.h>
//
//#define RINGVOLUME 0.1 // local to feedback button
//
//#define ringTone "/root/ringtone1.mp3"
//
//int ring ( void * args) {
//	GstElement *audiopipeline,  *audioSource, *mpegaudioparser,* mpg123audiodec, *audioconvert;
//	GstElement *volume , *audiosink ;
//	GstBus *bus;
//	GstMessage *msg;
//	bool stop = false;
//	GstStateChangeReturn ret;
//	threadStatus_t  * pThreadStatus = args;
//	char devicename[20];
//	int retries = 5;
//
//	pThreadStatus->mustStop = false;
//
//	if ( pThreadStatus->spkrCardNo == -1 ) // no USB audiocard found
//		return ( -1 );
//
//	audiopipeline = gst_pipeline_new ("audiopipeline");
//	audioSource = gst_element_factory_make ("filesrc", "filesrc");
//	g_object_set (G_OBJECT (audioSource), "location",(gchar *) ringTone,NULL);
//	mpegaudioparser = gst_element_factory_make ("mpegaudioparse", "mpegaudioparse");
//	mpg123audiodec = gst_element_factory_make (alsactl store"mpg123audiodec", "mpg123audiodec");
//
//	volume = gst_element_factory_make ("volume", "volume");
//	g_object_set (G_OBJECT (volume), "volume",RINGVOLUME,NULL);
//
//	audioconvert = gst_element_factory_make ("audioconvert", "audioconvert");
//
//	audiosink = gst_element_factory_make ("alsasink", "alsasink");
//	sprintf(devicename, "hw:%d",pThreadStatus->spkrCardNo);
//	g_object_set (G_OBJECT (audiosink), "device",devicename,NULL);
//
//	if (!audiopipeline || !audioconvert || !volume || !audiosink) {
//		g_printerr ("Not all audio elements could be created.\n");
//		stop = true;
//	}
//	if (!audioSource  || !mpegaudioparser || !mpg123audiodec ){
//		g_printerr ("Not all audio elements could be created.\n");
//		stop = true;
//	}
//
//	gst_bin_add_many (GST_BIN (audiopipeline), audioSource,mpegaudioparser,mpg123audiodec, volume, audioconvert, audiosink ,NULL);
//
//	if (gst_element_link (audioSource, mpegaudioparser  ) != TRUE)
//		stop = true;
//
//	if (gst_element_link (mpegaudioparser, mpg123audiodec ) != TRUE)
//		stop = true;
//
//	if (gst_element_link (mpg123audiodec, volume) != TRUE)
//		stop = true;
//
//	if (gst_element_link (volume, audioconvert) != TRUE)
//		stop = true;
//
//	if (gst_element_link (audioconvert, audiosink) != TRUE)
//		stop = true;
//
//	if (!stop) {
//		ret = gst_element_set_state (audiopipeline, GST_STATE_PLAYING);
//		if (ret == GST_STATE_CHANGE_FAILURE) {
//			g_printerr ("Rngtone:  Unable to set the audio pipeline to the playing state.\n");
//			stop = true;
//		}
//	}
//	if ( stop)
//		print ("ringThread error\n");
//	else
//		pThreadStatus->run = true;
//
//	while( !pThreadStatus->mustStop && !stop ) {
//		usleep(10 * 1000);
//		msg = NULL;
//		/* Wait until error or EOS */
//		bus = gst_element_get_bus (audiopipeline);
//
//		msg = gst_bus_pop (bus);
//		gst_object_unref (bus);
//		if (msg != NULL) {
//			GError *err;
//			gchar *debug_info;
//			switch (GST_MESSAGE_TYPE (msg)) {
//			case GST_MESSAGE_ERROR:
//				gst_message_parse_error (msg, &err, &debug_info);
//				g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
//				g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
//				g_clear_error (&err);
//				g_free (debug_info);
//				stop = true;
//				break;
//			case GST_MESSAGE_EOS:
//				g_print ("End-Of-Stream reached.\n");
//				stop = true;
//				//	gst_element_set_state (audiopipeline, GST_STATE_PLAYING); // start again
//				break;
//			default:
//				//	g_printerr ("Unexpected message received.\n");
//				break;
//			}
//			gst_message_unref (msg);
//		}
//		if ( pThreadStatus->mustStop )
//			stop = true;
//	}
//	gst_element_set_state (audiopipeline, GST_STATE_NULL);
//	gst_object_unref(audiopipeline);
//
//	pThreadStatus->run = false;
//	print ("ringThread ended\n");
//	return 0;
//	//	usleep(10000);
//	//	pthread_exit(args);
//	//	return ( NULL);
//}
//
//void* ringThread(void* args){
//	GstElement *audiopipeline,  *audioSource, *mpegaudioparser,* mpg123audiodec, *audioconvert;
//	GstElement *volume , *audiosink ;
//	GstBus *bus;
//	GstMessage *msg;
//	bool stop = false;
//	GstStateChangeReturn ret;
//	threadStatus_t  * pThreadStatus = args;
//	char devicename[20];
//	int retries = 5;
//
//	pThreadStatus->mustStop = false;
//
//	if ( pThreadStatus->spkrCardNo == -1 ) // no USB audiocard found
//		stop = true;
//
//	while ( retries > 0 ) {
//		audiopipeline = gst_pipeline_new ("audiopipeline");
//
//		audioSource = gst_element_factory_make ("filesrc", "filesrc");
//		g_object_set (G_OBJECT (audioSource), "location",(gchar *) ringTone,NULL);
//
//		mpegaudioparser = gst_element_factory_make ("mpegaudioparse", "mpegaudioparse");
//		mpg123audiodec = gst_element_factory_make ("mpg123audiodec", "mpg123audiodec");
//
//		volume = gst_element_factory_make ("volume", "volume");
//		g_object_set (G_OBJECT (volume), "volume",RINGVOLUME,NULL);
//
//		audioconvert = gst_element_factory_make ("audioconvert", "audioconvert");
//
//		audiosink = gst_element_factory_make ("alsasink", "alsasink");
//		sprintf(devicename, "hw:%d",pThreadStatus->spkrCardNo);
//		g_object_set (G_OBJECT (audiosink), "device",devicename,NULL);
//
//		if (!audiopipeline || !audioconvert || !volume || !audiosink) {
//			g_printerr ("Not all audio elements could be created.\n");
//			stop = true;
//		}
//		if (!audioSource  || !mpegaudioparser || !mpg123audiodec ){
//			g_printerr ("Not all audio elements could be created.\n");
//			stop = true;
//		}
//
//		gst_bin_add_many (GST_BIN (audiopipeline), audioSource,mpegaudioparser,mpg123audiodec, volume, audioconvert, audiosink ,NULL);
//
//		if (gst_element_link (audioSource, mpegaudioparser  ) != TRUE)
//			stop = true;
//
//		if (gst_element_link (mpegaudioparser, mpg123audiodec ) != TRUE)
//			stop = true;
//
//		if (gst_element_link (mpg123audiodec, volume) != TRUE)
//			stop = true;
//
//		if (gst_element_link (volume, audioconvert) != TRUE)
//			stop = true;
//
//		if (gst_element_link (audioconvert, audiosink) != TRUE)
//			stop = true;
//
//		if ( !stop) {
//			ret = gst_element_set_state (audiopipeline, GST_STATE_PLAYING);
//			if (ret == GST_STATE_CHANGE_FAILURE)
//				g_printerr ("Rngtone:  Unable to set the audio pipeline to the playing state.\n");
//			else
//				retries = 0;
//		}
//	}
//	if (!stop) {
//		print ("ringThread started\n");
//		pThreadStatus->run = true;
//
//		while( !pThreadStatus->mustStop && !stop ) {
//			usleep(10 * 1000);
//			msg = NULL;
//			/* Wait until error or EOS */
//			bus = gst_element_get_bus (audiopipeline);
//
//			msg = gst_bus_pop (bus);
//			gst_object_unref (bus);
//			if (msg != NULL) {
//				GError *err;
//				gchar *debug_info;
//				switch (GST_MESSAGE_TYPE (msg)) {
//				case GST_MESSAGE_ERROR:
//					gst_message_parse_error (msg, &err, &debug_info);
//					g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
//					g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
//					g_clear_error (&err);
//					g_free (debug_info);
//					stop = true;
//					break;
//				case GST_MESSAGE_EOS:
//					g_print ("End-Of-Stream reached.\n");
//					stop = true;
//					//	gst_element_set_state (audiopipeline, GST_STATE_PLAYING); // start again
//					break;
//				default:
//					//	g_printerr ("Unexpected message received.\n");
//					break;
//				}
//				gst_message_unref (msg);
//			}
//			if ( pThreadStatus->mustStop )
//				stop = true;
//		}
//	}
//	gst_element_set_state (audiopipeline, GST_STATE_NULL);
//	gst_object_unref(audiopipeline);
//
//	pThreadStatus->run = false;
//	print ("ringThread ended\n");
//	usleep(10000);
//	pthread_exit(args);
//	return ( NULL);
//}
//
//
//
