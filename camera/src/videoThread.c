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


gboolean link_elements_with_filter (GstElement *element1, GstElement *element2, GstCaps *caps)
{
	gboolean link_ok;
	link_ok = gst_element_link_filtered (element1, element2, caps);
	gst_caps_unref (caps);
	if (!link_ok) {
		g_warning ("Filter failed to link video element1 and element2!");
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

 */

//static GstElement *videopipeline = NULL;
//static GstElement *videoSource, *rtpjpegdepay,  *jpegdec ,*videoconvert , *videoflip,*videosink;
//static GstElement *jpegenc, *textoverlay; // used for testscreen

static GstElement *videopipeline, *videoSource, *videopayloader, *udpSink;
static GstElement *jpegenc , *jpegdec, *videoconvert ; // used for testscreen
static GstElement *tee , *udpQueue, *videoQueue, *textoverlay, *videoflip,*videosink ;

static GstElement *image_filter;
static GstElement *image_sink;


static GstPad *tee_udp_pad, *tee_video_pad, *tee_app_pad;
static GstPad *queue_udp_pad, *queue_video_pad;



static streamerTask_t actualTask;
static char textBuffer[MESSAGEBUFFERSIZE];

static void stopVideo (){
	if (videopipeline != NULL ) {
		gst_element_set_state (videopipeline, GST_STATE_NULL);
		gst_object_unref(videopipeline);
		videopipeline = NULL;
		print("video stopped\n");
		usleep( 20000);
	}
}


bool  updateText ( char * pText) {
	GstStateChangeReturn ret;
	bool error = false;

	if ( strcmp (pText, textBuffer) != 0 ){ // text changed ?
		gst_element_set_state (videopipeline, GST_STATE_NULL); // error
		g_object_set (G_OBJECT (textoverlay), "text",pText,NULL);
		ret = gst_element_set_state (videopipeline, GST_STATE_PLAYING);
		if (ret == GST_STATE_CHANGE_FAILURE) {
			g_printerr ("Unable to set the video pipeline to the playing state.\n");
			error = true;
		}
		if ( strlen (pText ) < sizeof (textBuffer))
			strcpy( textBuffer, pText);
		else {
			strncpy( textBuffer, pText, sizeof (textBuffer-1));
			textBuffer[sizeof (textBuffer)-1] = 0;
		}
	}
	return error;
}

bool setVideoTask( streamerTask_t task, int UDPport, char * pText, int cameraCard)
{
	bool error = false;

	GstCaps *caps;
	GstStateChangeReturn ret;

	switch (task){
	case TASK_STOP:
		stopVideo();
		break;

	case VIDEOTASK_SHOWMESSAGE:

		if (actualTask != VIDEOTASK_SHOWMESSAGE) { // do nothing if already showing
			stopVideo();
			print("message started\n");
			if ( strlen (pText ) < sizeof (textBuffer))
				strcpy( textBuffer, pText);
			else {
				strncpy( textBuffer, pText, sizeof (textBuffer-1));
				textBuffer[sizeof (textBuffer)-1] = 0;
			}
			videopipeline = gst_pipeline_new ("messageVideoPipeline");
			videoSource = gst_element_factory_make ("videotestsrc", "videoSource");
			g_object_set (G_OBJECT (videoSource), "pattern",GST_VIDEO_TEST_SRC_BLUE ,NULL);
			textoverlay = gst_element_factory_make("textoverlay", "textoverlay" );
			g_object_set (G_OBJECT (textoverlay), "valignment",GST_BASE_TEXT_OVERLAY_VALIGN_TOP,NULL);
			g_object_set (G_OBJECT (textoverlay), "halignment",GST_BASE_TEXT_OVERLAY_HALIGN_LEFT,NULL);
			g_object_set (G_OBJECT (textoverlay), "font-desc","Sans, 30",NULL);
			g_object_set (G_OBJECT (textoverlay), "text",pText,NULL);
			videoflip = gst_element_factory_make("videoflip", "videoflip");
			g_object_set (G_OBJECT (videoflip), "method",GST_VIDEO_FLIP_METHOD_90L ,NULL);
			videosink = gst_element_factory_make ("nxvideosink", "nxvideosink");
			gst_bin_add_many (GST_BIN (videopipeline),videoSource,textoverlay, videoflip, videosink, NULL);

			caps = gst_caps_new_simple ("video/x-raw",
					//	"framerate",GST_TYPE_FRACTION, 30, 1,
					"width", G_TYPE_INT, 800,
					"height", G_TYPE_INT, 480,
					NULL);
			if (link_elements_with_filter (videoSource, textoverlay, caps ) != TRUE)
				error = true;

			if (gst_element_link (textoverlay , videoflip) != TRUE)
				error = true;
			if (gst_element_link (videoflip , videosink) != TRUE)
				error = true;

			ret = gst_element_set_state (videopipeline, GST_STATE_PLAYING);

			if (ret == GST_STATE_CHANGE_FAILURE) {
				g_printerr ("Unable to set the video pipeline to the playing state.\n");
				error= true;
			}
		}
		else
			updateText (pText); // update text if needed

		break;

	case VIDEOTASK_STREAM:
		if (actualTask != VIDEOTASK_STREAM) { // do nothing if already showing
			stopVideo();
			error = false;
			char str[30];
			char pipelinedescriptor[200];
//			const gchar * pipelinedescriptor = str2;

			int len;
			len= sprintf( str," host =%s ", HOST );

			if ( myFloorID == BASE_FLOOR )

				sprintf( str+len,"port= %d", VIDEOPORT1);
			else
				 sprintf( str+len,"port=%d", VIDEOPORT2);

		//	sprintf( pipelinedescriptor,"gst-launch-1.0 v4l2src device=/dev/video0 ! videoconvert ! videoscale ! video/x-raw,width=800,height=600 ! jpegenc ! rtpjpegpay ! udpsink");
			sprintf( pipelinedescriptor,"gst-launch-1.0 v4l2src ! videoconvert ! videoscale ! video/x-raw,width=800,height=600 ! jpegenc ! rtpjpegpay ! udpsink");
			strcat (pipelinedescriptor, str);
			videopipeline = gst_parse_launch(pipelinedescriptor,NULL);

			ret = gst_element_set_state (videopipeline, GST_STATE_PLAYING);
			if (ret == GST_STATE_CHANGE_FAILURE) {
				g_printerr ("Unable to set the pipeline to the playing state.\n");
				error = true;
			}
			break;



			if (cameraCard == -1 ) {		 // if no camera use testpicture
				videoSource = gst_element_factory_make ("videotestsrc", "videoSource");
				jpegenc = gst_element_factory_make ("jpegenc", "jpegenc");
			}
			else {
				videoSource = gst_element_factory_make ("v4l2src", "videoSource");
				g_object_set (videoSource, "device","/dev/video0",NULL);
			}

			videopayloader = gst_element_factory_make ("rtpjpegpay", "jpegpayloader");
			udpSink = gst_element_factory_make ("udpsink", "udpsink");
			image_sink = gst_element_factory_make("fakesink", "image_sink");

			//			textoverlay = gst_element_factory_make("textoverlay", "textoverlay" );  removed meoryleak
			//			g_object_set (G_OBJECT (textoverlay), "valignment",GST_BASE_TEXT_OVERLAY_VALIGN_TOP,NULL);
			//			g_object_set (G_OBJECT (textoverlay), "halignment",GST_BASE_TEXT_OVERLAY_HALIGN_LEFT,NULL);
			//			g_object_set (G_OBJECT (textoverlay), "font-desc","Sans, 30",NULL);
			//			g_object_set (G_OBJECT (textoverlay), "text",pText,NULL);
			//			strcpy ( textBuffer, pText);

#ifndef NOSCREEN
			jpegdec =  gst_element_factory_make ("jpegdec", "jpegdec");
			videosink =  gst_element_factory_make ("nxvideosink", "videosink");
			videoconvert =  gst_element_factory_make ("videoconvert", "videovideoconvert");
			videoflip = gst_element_factory_make("videoflip","videoflip");
			g_object_set (videoflip,"method", 3,NULL);
			tee= gst_element_factory_make ("tee", "tee");
			g_object_set (tee, "name","t",NULL);
			udpQueue= gst_element_factory_make ("queue", "udpQueue");
			videoQueue= gst_element_factory_make ("queue", "videoQueue");
#endif

			if ( myFloorID == BASE_FLOOR )
				g_object_set (udpSink, "port", VIDEOPORT1, NULL);
			else
				g_object_set (udpSink, "port", VIDEOPORT2, NULL);

			g_object_set (udpSink, "host", HOST ,NULL);

			videopipeline = gst_pipeline_new ("videopipeline");

#ifndef NOSCREEN
			if (!videopipeline || !videoSource || !videopayloader || !udpSink || !tee || !udpQueue || !videoQueue || !videosink || !jpegdec || !videoflip || !videoconvert) {
#else
				if (!videopipeline || !videoSource || !videopayloader || !udpSink ) {
#endif
					g_printerr ("VideoThread Not all elements could be created.\n");
					error = true;
				}
				if (!error) {
					/* Build the pipeline */
					//				gst_bin_add_many (GST_BIN (videopipeline),videoSource,tee, udpQueue, videoQueue, videopayloader, textoverlay, udpSink, jpegdec , videoconvert, videoflip, videosink,  NULL);
#ifndef NOSCREEN
					gst_bin_add_many (GST_BIN (videopipeline),videoSource,tee, udpQueue, videoQueue, videopayloader, udpSink, jpegdec , videoconvert, videoflip, videosink,  NULL);
#else
					gst_bin_add_many (GST_BIN (videopipeline),videoSource, videopayloader, udpSink,  NULL);
#endif
					if (cameraCard == -1 ) {  // testimage
						g_print ("testimage used.\n");
						gst_bin_add (GST_BIN (videopipeline),jpegenc);
						caps = gst_caps_new_simple ("video/x-raw",
								//	"framerate",GST_TYPE_FRACTION, 30, 1,
								"width", G_TYPE_INT, STREAMWIDTH,
								"height", G_TYPE_INT, STREAMHEIGHT,
								NULL);		error = false;

						if (link_elements_with_filter (videoSource, jpegenc, caps ) != TRUE) {
							g_printerr ("jpegenc elements could not be linked.\n");
							error = true;
						}
#ifdef NOSCREEN
						if (gst_element_link (jpegenc , videopayloader) != TRUE) {
							g_printerr ("jpegenc and videopayloader could not be linked.\n");
							error = true;
						}
						if (gst_element_link (videopayloader, udpSink  ) != TRUE) {
							error = true;
						}
#else
						if (gst_element_link (jpegenc , tee) != TRUE) {
							g_printerr ("jpegenc and tee could not be linked.\n");
							error = true;
						}
#endif
					}
					else {
						g_print ("camera used.\n");
						caps = gst_caps_new_simple ("image/jpeg",
								"framerate",GST_TYPE_FRACTION, 30, 1,
								"width", G_TYPE_INT, CAMERAWIDTH,
								"height", G_TYPE_INT, CAMERAHEIGHT,
								NULL);
#ifdef NOSCREEN


						if (link_elements_with_filter (videoSource, videopayloader, caps ) != TRUE) {
				//		if (link_elements_with_filter (videoSource, image_sink, caps ) != TRUE) {

							error = true;
						}

//					caps = gst_caps_new_simple("image/jpeg", //   "video/x-raw",
//							"width",G_TYPE_INT, STREAMWIDTH,
//							"height", G_TYPE_INT, STREAMHEIGHT,
//					//		"framerate", GST_TYPE_FRACTION, 30, 1,
//							NULL);
//					// "bpp", G_TYPE_INT, 24,
//					// "depth", G_TYPE_INT, 24,
//					//	if(!gst_element_link_many( videoflip, image_filter, NULL)) return FALSE;
//					//	if(!gst_element_link_filtered(image_filter, image_sink, caps)) return FALSE;
//					if (!gst_element_link_filtered(image_sink ,videopayloader, caps))
//						error = true;

					if (gst_element_link (videopayloader, udpSink  ) != TRUE)
						error = true;

#else
						if (link_elements_with_filter (videoSource, tee, caps ) != TRUE) {
							error = true;
						}
#endif

					}
					/* Manually link the Tee, which has "Request" pads */
					// make pads tee
#ifndef NOSCREEN

					tee_udp_pad = gst_element_get_request_pad (tee, "src_%u");
					//	g_print ("Obtained request pad %s for udp branch.\n", gst_pad_get_name (tee_udp_pad));
					tee_video_pad = gst_element_get_request_pad (tee, "src_%u");
					//		g_print ("Obtained request pad %s for video branch.\n", gst_pad_get_name (tee_video_pad));
					tee_app_pad = gst_element_get_request_pad (tee, "src_%u");
					//	g_print ("Obtained request pad %s for app branch.\n", gst_pad_get_name (tee_app_pad));

					queue_udp_pad = gst_element_get_static_pad (udpQueue, "sink");
					queue_video_pad = gst_element_get_static_pad (videoQueue, "sink");

					if (gst_pad_link (tee_udp_pad, queue_udp_pad) != GST_PAD_LINK_OK ||
							gst_pad_link (tee_video_pad, queue_video_pad) != GST_PAD_LINK_OK ){
						g_printerr ("Tee could not be linked\n");
						error = true;
					}
					// udp path
					if (gst_element_link (udpQueue, videopayloader  ) != TRUE) {
						error = true;
					}
					if (gst_element_link (videopayloader, udpSink  ) != TRUE) {
						error = true;
					}
					// display path
					if (gst_element_link (videoQueue, jpegdec  ) != TRUE) {
						error = true;
					}

					if (gst_element_link (jpegdec, videoconvert ) != TRUE) {
						error = true;
					}

					//				if (gst_element_link (videoconvert, textoverlay ) != TRUE)
					//					error = true;
					//		if (gst_element_link (textoverlay , videoflip) != TRUE)
					//			error = true;
					if (gst_element_link (videoconvert, videoflip ) != TRUE)
						error = true;


					if (gst_element_link (videoflip , videosink) != TRUE)
						error = true;
#endif
				}

                                                                                                   				if ( !error ) {
					/* Start playing */
					ret = gst_element_set_state (videopipeline, GST_STATE_PLAYING);
					if (ret == GST_STATE_CHANGE_FAILURE) {
						g_printerr ("Unable to set the pipeline to the playing state.\n");
						error = true;
					}
				}
				if (error)
					print("error video\n");
				else
					print("video started\n");
			}
			break;

	case VIDEOTASK_SHOWCAMERA:
		if (actualTask != VIDEOTASK_SHOWCAMERA) {
			stopVideo();
			error = false;

			videopipeline = gst_parse_launch("v4l2src ! videoconvert ! videoscale ! video/x-raw,width=800,height=600 !videoflip method=3 ! nxvideosink", NULL);
			ret = gst_element_set_state (videopipeline, GST_STATE_PLAYING);

			break;

//			videoSource = gst_element_factory_make ("v4l2src", "videoSource");
//			g_object_set (videoSource, "device","/dev/video0",NULL);
//			videoflip = gst_element_factory_make("videoflip", "videoflip");
//			g_object_set (G_OBJECT (videoflip), "method",GST_VIDEO_FLIP_METHOD_90L ,NULL);
//			videosink = gst_element_factory_make ("nxvideosink", "nxvideosink");
//			jpegdec =  gst_element_factory_make ("jpegdec", "jpegdec");
//			videoconvert =  gst_element_factory_make ("videoconvert", "videovideoconvert");
//
//			videopipeline = gst_pipeline_new ("videopipeline");
//
//			image_filter = gst_element_factory_make("ffmpegcolorspace", "image_filter");
//			/* A dummy sink for the image stream. Goes to bitheaven */
//			image_sink = gst_element_factory_make("fakesink", "image_sink");
//
//
//			gst_bin_add_many (GST_BIN (videopipeline),videoSource, jpegdec,videoconvert,videoflip, videosink, NULL);
//
//			caps = gst_caps_new_simple ("image/jpeg",
//					"framerate",GST_TYPE_FRACTION, 30, 1,
//					"width", G_TYPE_INT, CAMERAWIDTH,
//					"height", G_TYPE_INT, CAMERAHEIGHT,
//			NULL);
//
//			if (link_elements_with_filter (videoSource, jpegdec, caps ) != TRUE)
//					error = true;
//
//			if (gst_element_link (jpegdec, videoconvert ) != TRUE)
//				error = true;
//
//			if (gst_element_link (videoconvert, videoflip ) != TRUE)
//				error = true;



//			caps = gst_caps_new_simple("video/x-raw",
//			        "width", G_TYPE_INT, 640,
//			        "height", G_TYPE_INT, 480,
//			        "framerate", GST_TYPE_FRACTION, 30, 1,
//			        NULL);
//	       // "bpp", G_TYPE_INT, 24,
//	       // "depth", G_TYPE_INT, 24,
//		//	if(!gst_element_link_many( videoflip, image_filter, NULL)) return FALSE;
//		//	if(!gst_element_link_filtered(image_filter, image_sink, caps)) return FALSE;
//			if(!gst_element_link_filtered(videoflip, videosink, caps))
//				error = true;
//
//			if (gst_element_link (videoflip , videosink) != TRUE)
//				error = true;
//
//			ret = gst_element_set_state (videopipeline, GST_STATE_PLAYING);
//
//			if (ret == GST_STATE_CHANGE_FAILURE) {
//				g_printerr ("Unable to set the video pipeline to the playing state.\n");
//				error= true;
//			}

		}
		break;
		}


		actualTask = task;
		return error;
	}

	bool videoIsStopped ( void) {

		GstBus *bus;
		GstMessage *msg;
		bool stopped = false;
		if ( videopipeline != NULL ){

			bus = gst_element_get_bus (videopipeline);
			msg = gst_bus_pop (bus );
			gst_object_unref (bus);
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
		else
			stopped = true;
		return stopped;
	}

	bool setVideoText ( char * newText) {
		bool error = false;
		GstStateChangeReturn ret;
		if (actualTask == VIDEOTASK_SHOWMESSAGE ){
			gst_element_set_state (videopipeline, GST_STATE_NULL);
			g_object_set (G_OBJECT (textoverlay), "text",newText,NULL);
			ret = gst_element_set_state (videopipeline, GST_STATE_PLAYING);
			if (ret == GST_STATE_CHANGE_FAILURE) {
				g_printerr ("Unable to set the video pipeline to the playing state.\n");
				error = true;
			}
		}
		else
			error = true;

		return error;
	}

	streamerTask_t getVideoTask(){
		return actualTask;
	}















	//
	//
	//void* videoThread(void* args)
	//{
	//	GstElement *videopipeline, *videoSource, *videopayloader, *udpSink;
	//	GstElement *jpegenc , *jpegdec, *videoconvert ; // used for testscreen
	//	GstElement *tee , *udpQueue, *videoQueue, *textoverlay, *videoflip,*videosink ;
	//	GstPad *tee_udp_pad, *tee_video_pad, *tee_app_pad;
	//	GstPad *queue_udp_pad, *queue_video_pad;
	//	GstBus *bus;
	//	GstMessage *msg;
	//	GstStateChangeReturn ret;
	//	bool error = false;
	//	GstCaps *caps;
	//	GstState * gstState;
	//	threadStatus_t  * pThreadStatus = args;
	//
	//	char * pText =  (char *) pThreadStatus->info;
	//	char textBuffer[MESSAGEBUFFERSIZE];
	//
	//	int x = 0;
	//	runState_t runState = RUNSTATE_RUN;
	//
	//	pThreadStatus->mustStop = false;
	//
	//	while(1) {
	//
	//		switch (pThreadStatus->task){
	//		case VIDEOTASK_IDLE: // nothing to do , wait until task changes
	//			usleep(10000);
	//			break;
	//
	//		case VIDEOTASK_SHOWMESSAGE:
	//			error = false;
	//			print("messageThread started\n");
	//
	//			if ( strlen (pText ) < sizeof (textBuffer))
	//				strcpy( textBuffer, pText);
	//			else {
	//				strncpy( textBuffer, pText, sizeof (textBuffer-1));
	//				textBuffer[sizeof (textBuffer)-1] = 0;
	//			}
	//			videopipeline = gst_pipeline_new ("messageVideoPipeline");
	//			videoSource = gst_element_factory_make ("videotestsrc", "videoSource");
	//			g_object_set (G_OBJECT (videoSource), "pattern",GST_VIDEO_TEST_SRC_BLUE ,NULL);
	//			textoverlay = gst_element_factory_make("textoverlay", "textoverlay" );
	//			g_object_set (G_OBJECT (textoverlay), "valignment",GST_BASE_TEXT_OVERLAY_VALIGN_TOP,NULL);
	//			g_object_set (G_OBJECT (textoverlay), "halignment",GST_BASE_TEXT_OVERLAY_HALIGN_LEFT,NULL);
	//			g_object_set (G_OBJECT (textoverlay), "font-desc","Sans, 30",NULL);
	//			g_object_set (G_OBJECT (textoverlay), "text",pText,NULL);
	//			videoflip = gst_element_factory_make("videoflip", "videoflip");
	//			g_object_set (G_OBJECT (videoflip), "method",GST_VIDEO_FLIP_METHOD_90L ,NULL);
	//			videosink = gst_element_factory_make ("nxvideosink", "nxvideosink");
	//			gst_bin_add_many (GST_BIN (videopipeline),videoSource,textoverlay, videoflip, videosink, NULL);
	//
	//			caps = gst_caps_new_simple ("video/x-raw",
	//					//	"framerate",GST_TYPE_FRACTION, 30, 1,
	//					"width", G_TYPE_INT, 800,
	//					"height", G_TYPE_INT, 480,
	//					NULL);
	//			if (link_elements_with_filter (videoSource, textoverlay, caps ) != TRUE)
	//				error = true;
	//
	//
	//			if (gst_element_link (textoverlay , videoflip) != TRUE)
	//				error = true;
	//			if (gst_element_link (videoflip , videosink) != TRUE)
	//				error = true;
	//
	//		//	usleep(1000);  // delay, else segm fault?
	//
	//			ret = gst_element_set_state (videopipeline, GST_STATE_PLAYING);
	//
	//			if (ret == GST_STATE_CHANGE_FAILURE) {
	//				g_printerr ("Unable to set the video pipeline to the playing state.\n");
	//				error= true;
	//			}
	//			else
	//				pThreadStatus->run = true;
	//
	//			while (!error ) {
	//				usleep(10* 1000);
	//				pThreadStatus->runCntr++;
	////				if ( strcmp (pText, textBuffer) != 0 ){ // text changed ?
	////					gst_element_set_state (videopipeline, GST_STATE_NULL); // error
	////					g_object_set (G_OBJECT (textoverlay), "text",pText,NULL);
	////					ret = gst_element_set_state (videopipeline, GST_STATE_PLAYING);
	////					if (ret == GST_STATE_CHANGE_FAILURE) {
	////						g_printerr ("Unable to set the video pipeline to the playing state.\n");
	////						error = true;
	////					}
	////					if ( strlen (pText ) < sizeof (textBuffer))
	////						strcpy( textBuffer, pText);
	////					else {
	////						strncpy( textBuffer, pText, sizeof (textBuffer-1));
	////						textBuffer[sizeof (textBuffer)-1] = 0;
	////					}
	////				}
	//				//		bus = gst_element_get_bus (videopipeline);
	//				//		msg = gst_bus_pop (bus );
	//				//		/* Parse message */
	//				//		if (msg != NULL) {
	//				//			GError *err;
	//				//			gchar *debug_info;
	//				//
	//				//			switch (GST_MESSAGE_TYPE (msg)) {
	//				//			case GST_MESSAGE_ERROR:
	//				//				gst_message_parse_error (msg, &err, &debug_info);
	//				//				g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
	//				//				g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
	//				//				g_clear_error (&err);
	//				//				g_free (debug_info);
	//				//				error = true;
	//				//				break;
	//				//			case GST_MESSAGE_EOS:
	//				//				g_print ("End-Of-Stream reached.\n");
	//				//				error = true;
	//				//				break;
	//				//			default:
	//				//				break;
	//				//			}
	//				//		}
	//				//		gst_message_unref (msg);
	//				//		gst_object_unref (bus);
	//
	//				if ( pThreadStatus->mustStop){
	//					error = true;
	//				}
	//			}
	//			print("messageThread stopped\n");
	//			/* Free resources */
	//			gst_element_set_state (videopipeline, GST_STATE_NULL);
	//			gst_object_unref(videopipeline);
	//			pThreadStatus->run = false;
	//			pThreadStatus->mustStop = false;
	//			pThreadStatus->task = VIDEOTASK_IDLE;
	//			break;
	//
	//		case VIDEOTASK_STREAM:
	//			error = false;
	//			pThreadStatus->mustStop = false;
	//			 if (pThreadStatus->microCardNo == -1 ) {		 // if no camera use testpicture
	//				videoSource = gst_element_factory_make ("videotestsrc", "videoSource");
	//				jpegenc = gst_element_factory_make ("jpegenc", "jpegenc");
	//			}
	//			else {
	//				videoSource = gst_element_factory_make ("v4l2src", "videoSource");
	//				g_object_set (videoSource, "device","/dev/video0",NULL);
	//			}
	//
	//			videopayloader = gst_element_factory_make ("rtpjpegpay", "jpegpayloader");
	//			udpSink = gst_element_factory_make ("udpsink", "udpsink");
	//			videosink =  gst_element_factory_make ("nxvideosink", "videosink");
	//			jpegdec =  gst_element_factory_make ("jpegdec", "jpegdec");typedef enum VIDEOTASK { VIDEOTASK_SHOWMESSAGE, VIDEOTASK_STREAM } videoTask_t;
	//			videoconvert =  gst_element_factory_make ("videoconvert", "videovideoconvert");
	//			videoflip = gst_element_factory_make("videoflip","videoflip");
	//			g_object_set (videoflip,"method", 3,NULL);
	//
	////			textoverlay = gst_element_factory_make("textoverlay", "textoverlay" );  removed meoryleak
	////			g_object_set (G_OBJECT (textoverlay), "valignment",GST_BASE_TEXT_OVERLAY_VALIGN_TOP,NULL);
	////			g_object_set (G_OBJECT (textoverlay), "halignment",GST_BASE_TEXT_OVERLAY_HALIGN_LEFT,NULL);
	////			g_object_set (G_OBJECT (textoverlay), "font-desc","Sans, 30",NULL);
	////			g_object_set (G_OBJECT (textoverlay), "text",pText,NULL);
	////			strcpy ( textBuffer, pText);
	//
	//			tee= gst_element_factory_make ("tee", "tee");
	//			g_object_set (tee, "name","t",NULL);
	//
	//			udpQueue= gst_element_factory_make ("queue", "udpQueue");
	//			videoQueue= gst_element_factory_make ("queue", "videoQueue");
	//
	//
	//			if ( myFloorID == BASE_FLOOR )
	//				g_object_set (udpSink, "port", VIDEOPORT1, NULL);
	//			else
	//				g_object_set (udpSink, "port", VIDEOPORT2, NULL);
	//
	//			g_object_set (udpSink, "host", HOST ,NULL);
	//
	//			videopipeline = gst_pipeline_new ("videopipeline");
	//
	//			if (!videopipeline || !videoSource || !videopayloader || !udpSink || !tee || !udpQueue || !videoQueue || !videosink || !jpegdec || !videoflip || !videoconvert) {
	//				g_printerr ("VideoThread Not all elements could be created.\n");
	//				error = true;
	//			}
	//			else
	//				pThreadStatus->run = true;
	//
	//			if (!error) {
	//				/* Build the pipeline */
	////				gst_bin_add_many (GST_BIN (videopipeline),videoSource,tee, udpQueue, videoQueue, videopayloader, textoverlay, udpSink, jpegdec , videoconvert, videoflip, videosink,  NULL);
	//				gst_bin_add_many (GST_BIN (videopipeline),videoSource,tee, udpQueue, videoQueue, videopayloader, udpSink, jpegdec , videoconvert, videoflip, videosink,  NULL);
	//
	//				if (pThreadStatus->microCardNo == -1 ) {  // testimage
	//					g_print ("testimage used.\n");
	//					gst_bin_add (GST_BIN (videopipeline),jpegenc);
	//					caps = gst_caps_new_simple ("video/x-raw",
	//							//	"framerate",GST_TYPE_FRACTION, 30, 1,
	//							"width", G_TYPE_INT, 800,
	//							"height", G_TYPE_INT, 480,
	//							NULL);		error = false;
	//
	//					if (link_elements_with_filter (videoSource, jpegenc, caps ) != TRUE) {
	//						g_printerr ("jpegenc elements could not be linked.\n");
	//						error = true;
	//					}
	//					//		gst_caps_unref (caps);
	//					if (gst_element_link (jpegenc , tee) != TRUE) {
	//						g_printerr ("jpegenc and videopayloader could not be linked.\n");
	//						error = true;
	//					}
	//				}
	//				else {
	//					g_print ("camera used.\n");
	//					caps = gst_caps_new_simple ("image/jpeg",
	//							"framerate",GST_TYPE_FRACTION, 30, 1,
	//							"width", G_TYPE_INT, 800,
	//							"height", G_TYPE_INT, 600,
	//							NULL);
	//#ifdef NOSCREEN
	//					if (link_elements_with_filter (videoSource, videopayloader, caps ) != TRUE) {
	//						error = true;
	//					}
	//
	//					if (gst_element_link (videopayloader, udpSink  ) != TRUE) {
	//						error = true;
	//					}
	//#else
	//					if (link_elements_with_filter (videoSource, tee, caps ) != TRUE) {
	//						error = true;
	//					}
	//#endif
	//					//	gst_caps_unref (caps);
	//				}
	//				/* Manually link the Tee, which has "Request" pads */
	//				// make pads tee
	//#ifndef NOSCREEN
	//
	//				tee_udp_pad = gst_element_get_request_pad (tee, "src_%u");
	//				//	g_print ("Obtained request pad %s for udp branch.\n", gst_pad_get_name (tee_udp_pad));
	//				tee_video_pad = gst_element_get_request_pad (tee, "src_%u");
	//				//		g_print ("Obtained request pad %s for video branch.\n", gst_pad_get_name (tee_video_pad));
	//				tee_app_pad = gst_element_get_request_pad (tee, "src_%u");
	//				//	g_print ("Obtained request pad %s for app branch.\n", gst_pad_get_name (tee_app_pad));
	//
	//				queue_udp_pad = gst_element_get_static_pad (udpQueue, "sink");
	//				queue_video_pad = gst_element_get_static_pad (videoQueue, "sink");
	//
	//				if (gst_pad_link (tee_udp_pad, queue_udp_pad) != GST_PAD_LINK_OK ||
	//						gst_pad_link (tee_video_pad, queue_video_pad) != GST_PAD_LINK_OK ){
	//					g_printerr ("Tee could not be linked\n");
	//					error = true;
	//				}
	//				// udp path
	//				if (gst_element_link (udpQueue, videopayloader  ) != TRUE) {
	//					error = true;
	//				}
	//				if (gst_element_link (videopayloader, udpSink  ) != TRUE) {
	//					error = true;
	//				}
	//				// display path
	//				if (gst_element_link (videoQueue, jpegdec  ) != TRUE) {
	//					error = true;
	//				}
	//
	//				if (gst_element_link (jpegdec, videoconvert ) != TRUE) {
	//					error = true;
	//				}
	//
	////				if (gst_element_link (videoconvert, textoverlay ) != TRUE)
	////					error = true;
	////		if (gst_element_link (textoverlay , videoflip) != TRUE)
	////			error = true;
	//				if (gst_element_link (videoconvert, videoflip ) != TRUE)
	//					error = true;
	//
	//
	//				if (gst_element_link (videoflip , videosink) != TRUE)
	//					error = true;
	//#endif
	//			}
	//
	//			if ( !error ) {
	//				/* Start playing */
	//				ret = gst_element_set_state (videopipeline, GST_STATE_PLAYING);
	//				if (ret == GST_STATE_CHANGE_FAILURE) {
	//					g_printerr ("Unable to set the pipeline to the playing state.\n");
	//					error = true;
	//				}
	//			}
	//			while (!error ) {
	//				usleep(10 * 1000);
	//				pThreadStatus->runCntr++;
	////				bus = gst_element_get_bus (videopipeline);
	////				msg = gst_bus_pop (bus);
	////				gst_object_unref (bus);
	////				/* Parse message */
	////				if (msg != NULL) {
	////					GError *err;
	////					gchar *debug_info;
	////
	////					switch (GST_MESSAGE_TYPE (msg)) {
	////					case GST_MESSAGE_ERROR:
	////						gst_message_parse_error (msg, &err, &debug_info);
	////						g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
	////						g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
	////						g_clear_error (&err);
	////						g_free (debug_info);
	////						error = true;
	////						break;
	////					case GST_MESSAGE_EOS:
	////						g_print ("End-Of-Stream reached.\n");
	////						error = true;
	////						break;
	////						//			default:
	////						//				/* We should not reach here because we only asked for ERRORs and EOS */
	////						//				//g_printerr ("Unexpected message received.\n");
	////						//				break;
	////					}
	////					gst_message_unref (msg);
	////				}
	//				if ( pThreadStatus->mustStop){
	//					error = true;
	//				}
	//				switch (runState) {
	//				case RUNSTATE_RUN:
	//					if (pThreadStatus->pause){
	//						gst_element_set_state (videopipeline, GST_STATE_NULL);
	//						print ("video Paused\n");
	//						runState = RUNSTATE_PAUSE;
	//					}
	//					break;
	//
	//				case RUNSTATE_PAUSE:
	//					if (!pThreadStatus->pause){
	//						gst_element_set_state (videopipeline, GST_STATE_PLAYING);
	//						runState = RUNSTATE_RUN;
	//						print ("video running\n");
	//					}
	//					break;
	//				}
	//
	////				if ( strcmp (textBuffer, pText) != 0  ) { //  text changed
	////					strcpy ( textBuffer, pText);
	////					gst_element_set_state (videopipeline, GST_STATE_NULL);
	////					g_object_set (G_OBJECT (textoverlay), "text",pText,NULL);  //update screen
	////					gst_element_set_state (videopipeline, GST_STATE_PLAYING);
	////				}
	//			}
	//			gst_element_set_state (videopipeline, GST_STATE_NULL);
	//			usleep(100000);
	//			gst_object_unref(videopipeline);
	//			pThreadStatus->run = false;
	//			pThreadStatus->mustStop = false;
	//			pThreadStatus->task = VIDEOTASK_IDLE;
	//			runState = RUNSTATE_RUN;
	//			print ( "video stopped\n");
	//			break;
	//		} // end swicht task
	//	}
	//}
	//
	//
	//
	//// puts a message on screen
	//
	//void* messageThread(void* args){
	//
	//	GstElement *videopipeline, *videoSource,*videoconvert, *videosink , *videoflip, *textoverlay;
	//	GstCaps *caps;
	//	GstBus *bus;
	//	GstMessage *msg;
	//	bool error = false;
	//	GstStateChangeReturn ret;
	//	threadStatus_t  * pThreadStatus = args;
	//	char * pText =  (char *) pThreadStatus->info;
	//	char textBuffer[MESSAGEBUFFERSIZE];
	//
	//
	//	print("messageThread started\n");
	//
	//	if ( strlen (pText ) < sizeof (textBuffer))
	//		strcpy( textBuffer, pText);
	//	else {
	//		strncpy( textBuffer, pText, sizeof (textBuffer-1));
	//		textBuffer[sizeof (textBuffer)-1] = 0;
	//	}
	//
	//	pThreadStatus->mustStop = false;
	//	pThreadStatus->run = true;
	//
	//	videopipeline = gst_pipeline_new ("messageVideoPipeline");
	//
	//	videoSource = gst_element_factory_make ("videotestsrc", "videoSource");
	//
	//	g_object_set (G_OBJECT (videoSource), "pattern",GST_VIDEO_TEST_SRC_BLUE ,NULL);
	//
	//	textoverlay = gst_element_factory_make("textoverlay", "textoverlay" );
	//	g_object_set (G_OBJECT (textoverlay), "valignment",GST_BASE_TEXT_OVERLAY_VALIGN_TOP,NULL);
	//	g_object_set (G_OBJECT (textoverlay), "halignment",GST_BASE_TEXT_OVERLAY_HALIGN_LEFT,NULL);
	//
	//	g_object_set (G_OBJECT (textoverlay), "font-desc","Sans, 30",NULL);
	//	g_object_set (G_OBJECT (textoverlay), "text",pText,NULL);
	//
	//	videoflip = gst_element_factory_make("videoflip", "videoflip");
	//	g_object_set (G_OBJECT (videoflip), "method",GST_VIDEO_FLIP_METHOD_90L ,NULL);
	//	videosink = gst_element_factory_make ("nxvideosink", "nxvideosink");
	//	gst_bin_add_many (GST_BIN (videopipeline),videoSource,textoverlay, videoflip, videosink, NULL);
	//
	//	caps = gst_caps_new_simple ("video/x-raw",
	//			//	"framerate",GST_TYPE_FRACTION, 30, 1,
	//			"width", G_TYPE_INT, 800,
	//			"height", G_TYPE_INT, 480,
	//			NULL);
	//	if (link_elements_with_filter (videoSource, textoverlay, caps ) != TRUE)
	//		error = true;
	//
	//
	//	if (gst_element_link (textoverlay , videoflip) != TRUE)
	//		error = true;
	//	if (gst_element_link (videoflip , videosink) != TRUE)
	//		error = true;
	//
	//	usleep(1000);  // delay, else segm fault?
	//
	//	ret = gst_element_set_state (videopipeline, GST_STATE_PLAYING);
	//
	//
	//	if (ret == GST_STATE_CHANGE_FAILURE) {
	//		g_printerr ("Unable to set the video pipeline to the playing state.\n");
	//		error= true;
	//	}
	//
	//	while (!error ) {
	//		usleep(100000);
	//		if ( strcmp (pText, textBuffer) != 0 ){ // text changed ?
	//			gst_element_set_state (videopipeline, GST_STATE_NULL); // error
	//			g_object_set (G_OBJECT (textoverlay), "text",pText,NULL);
	//			ret = gst_element_set_state (videopipeline, GST_STATE_PLAYING);
	//			if (ret == GST_STATE_CHANGE_FAILURE) {
	//				g_printerr ("Unable to set the video pipeline to the playing state.\n");
	//				error = true;
	//			}
	//			if ( strlen (pText ) < sizeof (textBuffer))
	//				strcpy( textBuffer, pText);
	//			else {
	//				strncpy( textBuffer, pText, sizeof (textBuffer-1));
	//				textBuffer[sizeof (textBuffer)-1] = 0;
	//			}
	//		}
	//		//		bus = gst_element_get_bus (videopipeline);
	//		//		msg = gst_bus_pop (bus );
	//		//		/* Parse message */
	//		//		if (msg != NULL) {
	//		//			GError *err;
	//		//			gchar *debug_info;
	//		//
	//		//			switch (GST_MESSAGE_TYPE (msg)) {
	//		//			case GST_MESSAGE_ERROR:
	//		//				gst_message_parse_error (msg, &err, &debug_info);
	//		//				g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
	//		//				g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
	//		//				g_clear_error (&err);
	//		//				g_free (debug_info);
	//		//				error = true;
	//		//				break;
	//		//			case GST_MESSAGE_EOS:
	//		//				g_print ("End-Of-Stream reached.\n");
	//		//				error = true;
	//		//				break;
	//		//			default:
	//		//				break;
	//		//			}
	//		//		}
	//		//		gst_message_unref (msg);
	//		//		gst_object_unref (bus);
	//
	//		if ( pThreadStatus->mustStop){
	//			error = true;
	//		}
	//
	//	}
	//
	//	print("messageThread stopped\n");
	//	/* Free resources */
	//	gst_element_set_state (videopipeline, GST_STATE_NULL);
	//	gst_object_unref(videopipeline);
	//	pThreadStatus->run = false;
	//	pthread_exit(args);
	//	return (void *) NULL;
	//}
	//
	//void* videoThreadxx(void* args)
	//{
	//	GstElement *videopipeline, *videoSource, *videopayloader, *udpsink;
	//	GstElement *jpegenc; // used for testscreen
	//
	//	GstBus *bus;
	//	GstMessage *msg;
	//	GstStateChangeReturn ret;
	//	bool error = false;
	//	threadStatus_t  * pThreadStatus = args;
	//	pThreadStatus->mustStop = false;
	//	pThreadStatus->run = true;
	//
	//	print("videoThread started\n");
	//
	//	if (pThreadStatus->microCardNo == -1 ) {		 // if no camera use testpicture
	//		videoSource = gst_element_factory_make ("videotestsrc", "videoSource");
	//		jpegenc = gst_element_factory_make ("jpegenc", "jpegenc");
	//	}
	//	else {
	//		videoSource = gst_element_factory_make ("v4l2src", "videoSource");
	//		g_object_set (videoSource, "device","/dev/video0",NULL);
	//	}
	//
	//	videopayloader = gst_element_factory_make ("rtpjpegpay", "jpegpayloader");
	//	udpsink = gst_element_factory_make ("udpsink", "udpsink");
	//	if ( myFloorID == BASE_FLOOR )
	//		g_object_set (udpsink, "port", VIDEOPORT1, NULL);
	//	else
	//		g_object_set (udpsink, "port", VIDEOPORT2, NULL);
	//
	//	g_object_set (udpsink, "host", HOST ,NULL);
	//
	//	videopipeline = gst_pipeline_new ("videopipeline");
	//
	//	if (!videopipeline || !videoSource || !videopayloader || !udpsink) {
	//		g_printerr ("Not all elements could be created.\n");
	//		error = true;
	//	}
	//	if (!error) {
	//		/* Build the pipeline */
	//		gst_bin_add_many (GST_BIN (videopipeline),videoSource,videopayloader, udpsink, NULL);
	//
	//		GstCaps *caps;
	//		if (pThreadStatus->microCardNo == -1 ) {		 // if no camera use testpicture
	//			g_print ("testimage used.\n");
	//			gst_bin_add (GST_BIN (videopipeline),jpegenc);
	//			caps = gst_caps_new_simple ("video/x-raw",
	//					"framerate",GST_TYPE_FRACTION, 20, 1,
	//					"width", G_TYPE_INT, 800,
	//					"height", G_TYPE_INT, 600,
	//					NULL);
	//			if (link_elements_with_filter (videoSource, jpegenc, caps ) != TRUE) {
	//				g_printerr ("jpegenc elements could not be linked.\n");
	//				error = true;
	//			}
	//			if (gst_element_link (jpegenc , videopayloader) != TRUE) {
	//				g_printerr ("jpegenc and videopayloader could not be linked.\n");
	//				error = true;
	//			}
	//		}
	//		else {
	//			g_print ("camera used.\n");
	//			caps = gst_caps_new_simple ("image/jpeg",
	//					"framerate",GST_TYPE_FRACTION, 20, 1,
	//					"width", G_TYPE_INT, 800,
	//					"height", G_TYPE_INT, 600,
	//					NULL);
	//			if (link_elements_with_filter (videoSource, videopayloader, caps ) != TRUE) {
	//				g_printerr ("Video elements could not be linked.\n");
	//				error = true;
	//			}
	//		}
	//		if (gst_element_link (videopayloader, udpsink  ) != TRUE) {
	//			g_printerr ("Elements could not be linked.\n");
	//			error = true;
	//		}
	//	}
	//
	//	if ( !error ) {
	//		/* Start playing */
	//		ret = gst_element_set_state (videopipeline, GST_STATE_PLAYING);
	//		if (ret == GST_STATE_CHANGE_FAILURE) {
	//			g_printerr ("Unable to set the pipeline to the playing state.\n");
	//			//	gst_object_unref (videopipeline);
	//			//	return -1;
	//		}
	//		/* Wait until error or EOS */
	//	}
	//	while (!error ) {
	//		usleep( 100000);
	//
	//		//		bus = gst_element_get_bus (videopipeline);
	//		//		//	msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
	//		//		msg = gst_bus_pop();
	//		//		gst_object_unref (bus);
	//		//		/* Parse message */
	//		//		if (msg != NULL) {
	//		//			GError *err;
	//		//			gchar *debug_info;
	//		//
	//		//			switch (GST_MESSAGE_TYPE (msg)) {
	//		//			case GST_MESSAGE_ERROR:
	//		//				gst_message_parse_error (msg, &err, &debug_info);
	//		//				g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
	//		//				g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
	//		//				g_clear_error (&err);
	//		//				g_free (debug_info);
	//		//				error = true;
	//		//				break;
	//		//			case GST_MESSAGE_EOS:
	//		//				g_print ("End-Of-Stream reached.\n");
	//		//				break;
	//		//			default:
	//		//				/* We should not reach here because we only asked for ERRORs and EOS */
	//		//				g_printerr ("Unexpected message received.\n");
	//		//				break;
	//		//			}
	//		//			gst_message_unref (msg);
	//		//		}
	//		if ( pThreadStatus->mustStop){
	//			error = true;
	//		}
	//	}
	//	/* Free resources */
	//
	//	print("videoThread stopped\n");
	//	gst_element_set_state (videopipeline, GST_STATE_NULL);
	//	gst_object_unref(videopipeline);
	//	pThreadStatus->run = false;
	//	pthread_exit(args);
	//	return (void *) NULL;
	//}
	//
	//
	//
	//
	//
	//
