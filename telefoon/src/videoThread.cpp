/*
 * video.c
 *
 *  Created on: Jan 27, 2019
 *      Author: dig
 */

#include <gst/gst.h>
#include "telefoon.h"
#include "videoThread.h"
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

gboolean link_elements_with_filter(GstElement *element1, GstElement *element2,
		GstCaps *caps) {
	gboolean link_ok;
	link_ok = gst_element_link_filtered(element1, element2, caps);
	gst_caps_unref(caps);

	if (!link_ok) {
		g_warning("Filter failed to link element1 and element2!");
	}
	return link_ok;
}

//gst-launch-1.0 v4l2src device=/dev/video0 ! image/jpeg,framerate=30/1,width=800,height=600 ! rtpjpegpay ! udpsink port=5001 host=192.168.2.255
//gst-launch-1.0 videotestsrc pattern=zone-plate kx2=20 ky2=20 kt=1 ! video/x-raw,width=800,height=600 ! jpegenc ! rtpjpegpay ! udpsink port=5001 host=192.168.2.255

//gst-launch-1.0 -v udpsrc port=5001 ! application/x-rtp,encoding-name=JPEG,payload=26 ! rtpjpegdepay ! jpegdec ! videoconvert !  videoflip method=counterclockwise ! nxvideosink
//gst-launch-1.0 -v udpsrc port=5001 ! application/x-rtp,encoding-name=JPEG,payload=26 ! rtpjpegdepay ! jpegdec ! videoconvert !  autovideosink

static GstElement *videopipeline = NULL;
static GstElement *videoSource, *rtpjpegdepay, *jpegdec, *videoconvert,
		*videoflip, *videosink;
static GstElement *textoverlay; // used for testscreen
static streamerTask_t actualTask;

static char textBuffer[MESSAGEBUFFERSIZE];
static int actualPort;

int updateTextExceptionCntr;
int setVideoExceptionCntr;
int setVideoTextExceptionCntr;

static void stopVideo() {
	if (videopipeline != NULL) {
		gst_element_set_state(videopipeline, GST_STATE_NULL);
		gst_object_unref(videopipeline);
		videopipeline = NULL;
		print("video stopped\n");
		usleep(1000);
	}
}

GstStateChangeReturn setVideoPort(int UDPport) {
	GstStateChangeReturn ret;
	if (UDPport == actualPort)
		return GST_STATE_CHANGE_SUCCESS;
	actualPort = UDPport;
	print("Video port changed to %d\n", UDPport);

	gst_element_set_state(videopipeline, GST_STATE_NULL);
	usleep(1000);
	g_object_set(videoSource, "port", UDPport, NULL);
	usleep(1000);
	ret = gst_element_set_state(videopipeline, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		g_printerr("Change port error.\n");
	}
	return ret;
}

bool updateText(char *pText) {
	GstStateChangeReturn ret;
	bool error = false;
	try {
		if (strcmp(pText, textBuffer) != 0) { // text changed ?
			gst_element_set_state(videopipeline, GST_STATE_NULL); // stop
			g_object_set(G_OBJECT(textoverlay), "text", pText, NULL);
			ret = gst_element_set_state(videopipeline, GST_STATE_PLAYING);
			if (ret == GST_STATE_CHANGE_FAILURE) {
				g_printerr(
						"Unable to set the video pipeline to the playing state.\n");
				error = true;
			}
			if (strlen(pText) < sizeof(textBuffer))
				strcpy(textBuffer, pText);
			else {
				strncpy(textBuffer, pText, sizeof(textBuffer - 1));
				textBuffer[sizeof(textBuffer) - 1] = 0;
			}
		}
	} catch (int e) {
		g_printerr("An exception occurred. Exception Nr. %d\n\r", e);
		updateTextExceptionCntr++;
		exceptionCntr++;
	}
	return error;
}

bool setVideoTask(streamerTask_t task, int UDPport, char *pText) {
	bool error = false;
	GstCaps *caps;
	GstStateChangeReturn ret;

	try {

		switch (task) {
		case TASK_STOP:
			stopVideo();
			break;

		case VIDEOTASK_SHOWMESSAGE:

			if (actualTask != VIDEOTASK_SHOWMESSAGE) { // do nothing if already showing
				stopVideo();
				print("message started\n");
				if (strlen(pText) < sizeof(textBuffer))
					strcpy(textBuffer, pText);
				else {
					strncpy(textBuffer, pText, sizeof(textBuffer - 1));
					textBuffer[sizeof(textBuffer) - 1] = 0;
				}
				videopipeline = gst_pipeline_new("messageVideoPipeline");
				videoSource = gst_element_factory_make("videotestsrc",
						"videoSource");
				g_object_set(G_OBJECT(videoSource), "pattern",
						GST_VIDEO_TEST_SRC_BLUE, NULL);
				textoverlay = gst_element_factory_make("textoverlay",
						"textoverlay");
				g_object_set(G_OBJECT(textoverlay), "valignment",
						GST_BASE_TEXT_OVERLAY_VALIGN_TOP, NULL);
				g_object_set(G_OBJECT(textoverlay), "halignment",
						GST_BASE_TEXT_OVERLAY_HALIGN_LEFT, NULL);
				g_object_set(G_OBJECT(textoverlay), "font-desc", "Sans, 30",
				NULL);
				g_object_set(G_OBJECT(textoverlay), "text", pText, NULL);
				videoflip = gst_element_factory_make("videoflip", "videoflip");
				g_object_set(G_OBJECT(videoflip), "method",
						GST_VIDEO_FLIP_METHOD_90L, NULL);
				videosink = gst_element_factory_make("nxvideosink",
						"nxvideosink");
				if (videosink == NULL)
					videosink = gst_element_factory_make("autovideosink",
							"autovideosink");
				if (videosink == NULL) {
					error = true;
					g_printerr("Unable to make videosink.\n");
				} else {
					gst_bin_add_many(GST_BIN(videopipeline), videoSource,
							textoverlay, videoflip, videosink, NULL);

					caps = gst_caps_new_simple("video/x-raw",
					//	"framerate",GST_TYPE_FRACTION, 30, 1,
							"width", G_TYPE_INT, 800, "height", G_TYPE_INT, 480,
							NULL);
					if (link_elements_with_filter(videoSource, textoverlay,
							caps) != TRUE)
						error = true;

					if (gst_element_link(textoverlay, videoflip) != TRUE)
						error = true;
					if (gst_element_link(videoflip, videosink) != TRUE)
						error = true;

					ret = gst_element_set_state(videopipeline,
							GST_STATE_PLAYING);

					if (ret == GST_STATE_CHANGE_FAILURE) {
						g_printerr(
								"Unable to set the video pipeline to the playing state.\n");
						error = true;
					}
				}
			} else
				updateText(pText); // update text if needed

			break;

		case VIDEOTASK_STREAM:
			if (actualTask != VIDEOTASK_STREAM) { // do nothing if already showing
				actualPort = UDPport;
				stopVideo();
				videopipeline = gst_pipeline_new("videopipeline");
				videoSource = gst_element_factory_make("udpsrc", "udpsrc");
				g_object_set(videoSource, "port", UDPport, NULL);

				print("video stream started port %d\n", UDPport);

				rtpjpegdepay = gst_element_factory_make("rtpjpegdepay",
						"rtpjpegdepay");
				jpegdec = gst_element_factory_make("jpegdec", "jpegdec");
				videoconvert = gst_element_factory_make("videoconvert",
						"videoconvert");
				videoflip = gst_element_factory_make("videoflip", "videoflip");
				g_object_set(videoflip, "method", 3, NULL);

				videosink = gst_element_factory_make("nxvideosink",
						"nxvideosink");

				if (!videopipeline || !videoSource || !rtpjpegdepay || !jpegdec
						|| !videoconvert || !videoflip || !videosink) {
					g_printerr("Not all elements could be created.\n");
					error = true;
				}
				if (!error) {
					/* Build the pipeline */
					gst_bin_add_many(GST_BIN(videopipeline), videoSource,
							rtpjpegdepay, jpegdec, videoconvert, videoflip,
							videosink, NULL);

					caps = gst_caps_new_simple("application/x-rtp",
					//	"framerate",GST_TYPE_FRACTION, 30, 1,
							"encoding-name", G_TYPE_STRING, "JPEG", "payload",
							G_TYPE_INT, 26,
							NULL);
					if (link_elements_with_filter(videoSource, rtpjpegdepay,
							caps) != TRUE) {
						g_printerr(
								"rtpjpegdepay elements could not be linked.\n");
						error = true;
					}

					if (gst_element_link(rtpjpegdepay, jpegdec) != TRUE) {
						g_printerr(
								"jpegenc and videopayloader could not be linked.\n");
						error = true;
					}

					if (gst_element_link(jpegdec, videoconvert) != TRUE) {
						g_printerr(
								"jpegenc and videopayloader could not be linked.\n");
						error = true;
					}

					if (gst_element_link(videoconvert, videoflip) != TRUE) {
						g_printerr(
								"jpegenc and videopayloader could not be linked.\n");
						error = true;
					}

					if (gst_element_link(videoflip, videosink) != TRUE) {
						g_printerr(
								"jpegenc and videopayloader could not be linked.\n");
						error = true;
					}
				}
				ret = gst_element_set_state(videopipeline, GST_STATE_PLAYING);

				if (ret == GST_STATE_CHANGE_FAILURE) {
					g_printerr(
							"Unable to set the video pipeline to the playing state.\n");
					error = true;
				}
			}
			break;
		default:
			break;
		}

	} catch (int e) {
		g_printerr("An exception occurred. Exception Nr. %d\n\r", e);
		setVideoExceptionCntr++;
		exceptionCntr++;
	}
	actualTask = task;
	return error;
}

bool videoIsStopped(void) {

	GstBus *bus;
	GstMessage *msg;
	bool stopped = false;
	if (videopipeline != NULL) {

		bus = gst_element_get_bus(videopipeline);
		msg = gst_bus_pop(bus);
		gst_object_unref(bus);
		/* Parse message */
		if (msg != NULL) {
			GError *err;
			gchar *debug_info;

			switch (GST_MESSAGE_TYPE(msg)) {
			case GST_MESSAGE_ERROR:
				gst_message_parse_error(msg, &err, &debug_info);
				g_printerr("Error received from element %s: %s\n",
						GST_OBJECT_NAME(msg->src), err->message);
				g_printerr("Debugging information: %s\n",
						debug_info ? debug_info : "none");
				g_clear_error(&err);
				g_free(debug_info);
				stopped = true;
				break;
			case GST_MESSAGE_EOS:
				g_print("End-Of-Stream reached.\n");
				stopped = true;
				break;
			default:
				break;
			}
			gst_message_unref(msg);
		}
	} else
		stopped = true;
	return stopped;
}

bool setVideoText(char *newText) {
	bool error = false;
	GstStateChangeReturn ret;
	try {

		if (actualTask == VIDEOTASK_SHOWMESSAGE) {
			gst_element_set_state(videopipeline, GST_STATE_NULL);
			g_object_set(G_OBJECT(textoverlay), "text", newText, NULL);
			ret = gst_element_set_state(videopipeline, GST_STATE_PLAYING);
			if (ret == GST_STATE_CHANGE_FAILURE) {
				g_printerr(
						"Unable to set the video pipeline to the playing state.\n");
				error = true;
			}
		} else
			error = true;
	} catch (int e) {
		g_printerr("An exception occurred. Exception Nr. %d\n\r", e);
		setVideoTextExceptionCntr++;
		exceptionCntr++;
	}

	return error;
}

streamerTask_t getVideoTask() {
	return actualTask;
}

