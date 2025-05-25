/*
 * video.c
 *
 *  Created on: Jan 27, 2019
 *      Author: dig
 */

#include "camera.h"
#include "videoThread.h"
#include <gst/gst.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define NOSCREEN
//
#define CAMERAWIDTH 800  //logitech C260
#define CAMERAHEIGHT 600

//#define CAMERAWIDTH 1280
//#define CAMERAHEIGHT 720

//#define CAMERAWIDTH 1920
//#define CAMERAHEIGHT 1080

#define STREAMWIDTH		800
#define STREAMHEIGHT	600

gboolean link_elements_with_filter(GstElement *element1, GstElement *element2,
		GstCaps *caps) {
	gboolean link_ok;
	link_ok = gst_element_link_filtered(element1, element2, caps);
	gst_caps_unref(caps);
	if (!link_ok) {
		g_warning("Filter failed to link video element1 and element2!");
	}
	return link_ok;
}

/*
 gst-launch-1.0 v4l2src device=/dev/video0 ! image/jpeg,framerate=30/1,width=800,height=600 ! rtpjpegpay ! udpsink port=5000 host=192.168.2.255

 gst-launch-1.0 v4l2src device=/dev/video0 ! image/jpeg,framerate=30/1,width=800,height=600 ! tee name=t  ! queue ! jpegdec  ! videoconvert ! videoflip method=counterclockwise ! nxvideosink  t. ! queue  ! rtpjpegpay ! udpsink port=5000 host=192.168.2.255

 gst-launch-1.0 v4l2src device=/dev/video0 ! image/jpeg,framerate=30/1,width=800,height=600 ! tee name=t ! queue ! rtpjpegpay ! udpsink port=5000 host=192.168.2.255  t. ! queue !  jpegdec  ! videoconvert ! videoflip method=counterclockwise ! nxvideosink

 gst-launch-1.0 videotestsrc ! video/x-raw,width=800,height=480 ! jpegenc ! tee name=t ! queue ! rtpjpegpay ! udpsink port=5000 host=192.168.2.255  t. ! queue !  jpegdec  ! videoconvert ! videoflip method=counterclockwise ! nxvideosink

 gst-launch-1.0 videotestsrc ! video/x-raw,width=800,height=480 ! jpegenc ! rtpjpegpay ! udpsink port=5000 host=192.168.2.255

 sudo gst-launch-1.0 -e v4l2src device=/dev/video0 ! video/x-raw,format=I420,framerate=30/1,width=800,height=600 ! videoflip method=counterclockwise ! nxvideosink

 gst-launch-1.0 v4l2src ! videoconvert ! videoscale add-borders=true ! video/x-raw,width=800,height=600 ! videoflip method=3 ! nxvideosink
 gst-launch-1.0 v4l2src ! video/x-raw,width=800,height=600 ! videoconvert ! x264enc ! rtph264pay config-interval=1 pt=96 ! udpsink host=127.0.0.1 port=5000

 gst-launch-1.0 v4l2src ! videoconvert ! videoscale add-borders=true ! video/x-raw,width=800,height=600 ! videoflip method=3 ! udpsink host=192.168.2.255 port=5000
 gst-launch-1.0 v4l2src ! videoconvert ! videoscale add-borders=true ! video/x-raw,width=800,height=600 ! videoflip method=3 ! udpsink host=192.168.2.255 port=5000


 gst-launch-1.0 v4l2src ! videoconvert ! videoscale add-borders=true ! video/x-raw,width=800,height=600 ! videoflip method=3 ! queue ! rtpjpegpay ! udpsink host=192.168.2.255 port=5000
 gst-launch-1.0 v4l2src ! video/x-raw,format=YUY2,width=640,height=480 ! videoconvert ! jpegenc ! rtpjpegpay ! udpsink host=127.0.0.1 port=5000gst-launch-1.0 v4l2src ! video/x-raw,format=YUY2,width=640,height=480 ! videoconvert ! jpegenc ! rtpjpegpay ! udpsink host=127.0.0.1 port=5000
 gst-launch-1.0 \
  v4l2src device=/dev/video0 ! \
  videoconvert ! \
  videoscale ! video/x-raw,width=800,height=600 ! \
  jpegenc ! \
  rtpjpegpay ! \
  udpsink host=192.168.2.255 port=5000

 gst-launch-1.0 videotestsrc ! jpegenc ! video/x-raw,width=800,height=600 ! rtpjpegpay ! udpsink  host=192.168.2.255 port=5000

 */

static GstElement *videopipeline = NULL;
static GstElement *videoSource;
static GstElement *textoverlay, *videoflip,	*videosink;

static streamerTask_t actualTask;
static char textBuffer[MESSAGEBUFFERSIZE];

static void stopVideo() {
	if (videopipeline != NULL) {
		gst_element_set_state(videopipeline, GST_STATE_NULL);
		gst_object_unref(videopipeline);
		videopipeline = NULL;
		print("video stopped\n");
		usleep(20000);
	}
}

bool updateText(char *pText) {
	GstStateChangeReturn ret;
	bool error = false;

	if (strcmp(pText, textBuffer) != 0) { // text changed ?
		gst_element_set_state(videopipeline, GST_STATE_NULL); // error
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
	return error;
}

bool setVideoTask(streamerTask_t task, int UDPport, char *pText, int cameraCard) {
	bool error = false;

	GstCaps *caps;
	GstStateChangeReturn ret;

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
			g_object_set(G_OBJECT(textoverlay), "font-desc", "Sans, 30", NULL);
			g_object_set(G_OBJECT(textoverlay), "text", pText, NULL);
			videoflip = gst_element_factory_make("videoflip", "videoflip");
			g_object_set(G_OBJECT(videoflip), "method",
					GST_VIDEO_FLIP_METHOD_90L, NULL);
			videosink = gst_element_factory_make("nxvideosink", "nxvideosink");
			gst_bin_add_many(GST_BIN(videopipeline), videoSource, textoverlay,
					videoflip, videosink, NULL);

			caps = gst_caps_new_simple("video/x-raw",
					"width", G_TYPE_INT, 800, "height", G_TYPE_INT, 480,
					NULL);
			if (link_elements_with_filter(videoSource, textoverlay,
					caps) != TRUE)
				error = true;

			if (gst_element_link(textoverlay, videoflip) != TRUE)
				error = true;
			if (gst_element_link(videoflip, videosink) != TRUE)
				error = true;

			ret = gst_element_set_state(videopipeline, GST_STATE_PLAYING);

			if (ret == GST_STATE_CHANGE_FAILURE) {
				g_printerr(
						"Unable to set the video pipeline to the playing state.\n");
				error = true;
			}
		} else
			updateText(pText); // update text if needed

		break;

	case VIDEOTASK_STREAM:
		if (actualTask != VIDEOTASK_STREAM) { // do nothing if already showing
			stopVideo();
			error = false;
			char str[30];
			char pipelinedescriptor[300];

			int len;
			len = sprintf(str, " host =%s ", HOST);

			if (myFloorID == BASE_FLOOR)

				sprintf(str + len, "port= %d", VIDEOPORT1);
			else
				sprintf(str + len, "port=%d", VIDEOPORT2);

		if (cameraCard >= 0) // then camera present
			sprintf(pipelinedescriptor,
						"gst-launch-1.0 v4l2src ! videoconvert ! videoscale ! video/x-raw,width=800,height=600 ! jpegenc ! rtpjpegpay ! udpsink");
			else
				sprintf(pipelinedescriptor,
						"gst-launch-1.0 videotestsrc pattern=spokes is-live=true ! videoconvert ! videoscale method=1 !"
						" video/x-raw,width=800,height=600,framerate=30/1 "
						"! jpegenc quality=75 idct-method=ifast ! rtpjpegpay ! udpsink ");

			strcat(pipelinedescriptor, str);
			videopipeline = gst_parse_launch(pipelinedescriptor, NULL);

			ret = gst_element_set_state(videopipeline, GST_STATE_PLAYING);
			if (ret == GST_STATE_CHANGE_FAILURE) {
				g_printerr(
						"Unable to set the pipeline to the playing state.\n");
				error = true;
			}
		}
		break;

	case VIDEOTASK_SHOWCAMERA:
		if (actualTask != VIDEOTASK_SHOWCAMERA) {
			stopVideo();
			error = false;
			videopipeline = gst_parse_launch(
							"v4l2src ! videoconvert ! videoscale ! video/x-raw,width=800,height=600"
							" ! videoflip method=3 ! nxvideosink",NULL);
			ret = gst_element_set_state(videopipeline, GST_STATE_PLAYING);
			if (ret == GST_STATE_CHANGE_FAILURE) {  // no camera
				videopipeline = gst_parse_launch("gst-launch-1.0 videotestsrc pattern=spokes is-live=true ! videoconvert ! videoscale method=1"
						" ! video/x-raw,width=800,height=600,framerate=30/1 ! videoflip method=3 ! nxvideosink",NULL);
				ret = gst_element_set_state(videopipeline, GST_STATE_PLAYING);
				if (ret == GST_STATE_CHANGE_FAILURE) {
					g_printerr( "Unable to set the pipeline to the playing state.\n");
					error = true;
				}
			}
			break;
		}
		break;
	}
	actualTask = task;
	return error;
}

streamerTask_t getVideoTask() {
	return actualTask;
}

