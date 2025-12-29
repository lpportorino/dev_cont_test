/**
 * @file gst_pipeline.c
 * @brief GStreamer pipeline implementation for video encoding
 */

#include "gst_pipeline.h"
#include <stdio.h>
#include <string.h>

static gboolean
bus_callback (GstBus *bus, GstMessage *message, gpointer data)
{
  (void)bus; // Unused parameter
  gst_pipeline_t *pipeline = (gst_pipeline_t *)data;

  switch (GST_MESSAGE_TYPE (message))
    {
    case GST_MESSAGE_ERROR:
      {
        GError *err;
        gchar *debug;
        gst_message_parse_error (message, &err, &debug);
        g_printerr ("[GST_PIPELINE] Error: %s\n", err->message);
        g_error_free (err);
        g_free (debug);
        if (pipeline->loop)
          {
            g_main_loop_quit (pipeline->loop);
          }
        break;
      }
    case GST_MESSAGE_EOS:
      g_print ("[GST_PIPELINE] End of stream\n");
      if (pipeline->loop)
        {
          g_main_loop_quit (pipeline->loop);
        }
      break;
    case GST_MESSAGE_STATE_CHANGED:
      if (GST_MESSAGE_SRC (message) == GST_OBJECT (pipeline->pipeline))
        {
          GstState old_state, new_state;
          gst_message_parse_state_changed (message, &old_state, &new_state,
                                            NULL);
          g_print ("[GST_PIPELINE] State changed: %s -> %s\n",
                   gst_element_state_get_name (old_state),
                   gst_element_state_get_name (new_state));
        }
      break;
    default:
      break;
    }

  return TRUE;
}

gst_pipeline_t *
gst_pipeline_create (uint32_t width,
                      uint32_t height,
                      uint32_t fps,
                      uint32_t num_frames,
                      const char *output_file)
{
  g_print ("[GST_PIPELINE] Creating pipeline\n");
  g_print ("[GST_PIPELINE]   Resolution: %ux%u\n", width, height);
  g_print ("[GST_PIPELINE]   FPS: %u\n", fps);
  g_print ("[GST_PIPELINE]   Frames: %u\n", num_frames);
  g_print ("[GST_PIPELINE]   Output: %s\n", output_file);

  gst_pipeline_t *ctx = g_malloc0 (sizeof (gst_pipeline_t));
  if (!ctx)
    {
      g_printerr ("[GST_PIPELINE] Failed to allocate context\n");
      return NULL;
    }

  ctx->width = width;
  ctx->height = height;
  ctx->fps = fps;
  ctx->output_file = g_strdup (output_file);
  ctx->frame_count = 0;

  // Initialize GStreamer
  gst_init (NULL, NULL);

  // Create pipeline
  ctx->pipeline = gst_pipeline_new ("video-encoder");

  // Create elements
  ctx->videotestsrc = gst_element_factory_make ("videotestsrc", "background");
  ctx->capsfilter = gst_element_factory_make ("capsfilter", "capsfilter");
  ctx->appsrc = gst_element_factory_make ("appsrc", "source");
  ctx->mixer = gst_element_factory_make ("compositor", "mixer");
  ctx->videoconvert
    = gst_element_factory_make ("videoconvert", "videoconvert");
  ctx->videoscale = gst_element_factory_make ("videoscale", "videoscale");
  ctx->encoder = gst_element_factory_make ("x264enc", "encoder");
  ctx->muxer = gst_element_factory_make ("mp4mux", "muxer");
  ctx->filesink = gst_element_factory_make ("filesink", "filesink");

  if (!ctx->videotestsrc || !ctx->capsfilter || !ctx->appsrc || !ctx->mixer
      || !ctx->videoconvert || !ctx->videoscale || !ctx->encoder
      || !ctx->muxer || !ctx->filesink)
    {
      g_printerr ("[GST_PIPELINE] Failed to create elements\n");
      gst_pipeline_destroy (ctx);
      return NULL;
    }

  // Configure videotestsrc (noise/static background at full resolution)
  // Set num-buffers so it auto-sends EOS after exact frame count
  g_object_set (G_OBJECT (ctx->videotestsrc),
                "pattern", 1,             // 1 = snow (static noise)
                "is-live", FALSE,
                "num-buffers", num_frames, // Auto-EOS after this many frames
                NULL);

  // Configure capsfilter to force videotestsrc to output at full resolution
  GstCaps *bg_caps = gst_caps_new_simple (
    "video/x-raw", "width", G_TYPE_INT, width, "height", G_TYPE_INT, height,
    "framerate", GST_TYPE_FRACTION, fps, 1, NULL);
  g_object_set (G_OBJECT (ctx->capsfilter), "caps", bg_caps, NULL);
  gst_caps_unref (bg_caps);

  // Configure appsrc
  // Our framebuffer format: uint32_t 0xAABBGGRR = [RR, GG, BB, AA] in memory (little-endian)
  // This is RGBA byte order (standard format for WebGL2 and GStreamer)
  GstCaps *caps = gst_caps_new_simple (
    "video/x-raw", "format", G_TYPE_STRING, "RGBA", "width", G_TYPE_INT,
    width, "height", G_TYPE_INT, height, "framerate", GST_TYPE_FRACTION, fps,
    1, NULL);

  g_object_set (G_OBJECT (ctx->appsrc), "caps", caps, "format",
                GST_FORMAT_TIME, "is-live", FALSE, "block", FALSE, NULL);

  gst_caps_unref (caps);

  // Configure encoder for extremely high quality (noise requires very high bitrate)
  // Using very high constant bitrate for near-lossless quality
  g_object_set (G_OBJECT (ctx->encoder),
                "pass", 17,               // cbr (constant bitrate)
                "bitrate", 150000,        // 150 Mbps (near-lossless for noise)
                "speed-preset", 6,        // medium (balance speed/compression)
                "tune", 0,                // none (no special tuning)
                "key-int-max", fps * 2,   // keyframe every 2 seconds
                "bframes", 0,             // disable B-frames for max quality
                "threads", 4,             // multi-threaded encoding
                NULL);

  // Configure filesink
  g_object_set (G_OBJECT (ctx->filesink), "location", output_file, NULL);

  // Add elements to pipeline
  gst_bin_add_many (GST_BIN (ctx->pipeline), ctx->videotestsrc,
                    ctx->capsfilter, ctx->appsrc, ctx->mixer,
                    ctx->videoconvert, ctx->videoscale, ctx->encoder,
                    ctx->muxer, ctx->filesink, NULL);

  // Link pipeline:
  // videotestsrc → capsfilter → mixer sink_0 (background)
  // appsrc → mixer sink_1 (overlay with alpha)
  // mixer → videoconvert → videoscale → encoder → muxer → filesink

  // Link background: videotestsrc → capsfilter → mixer sink pad 0
  if (!gst_element_link (ctx->videotestsrc, ctx->capsfilter))
    {
      g_printerr (
        "[GST_PIPELINE] Failed to link videotestsrc to capsfilter\n");
      gst_pipeline_destroy (ctx);
      return NULL;
    }

  GstPad *mixer_sink0 = gst_element_request_pad_simple (ctx->mixer, "sink_0");
  GstPad *capsfilter_src = gst_element_get_static_pad (ctx->capsfilter, "src");
  if (gst_pad_link (capsfilter_src, mixer_sink0) != GST_PAD_LINK_OK)
    {
      g_printerr ("[GST_PIPELINE] Failed to link capsfilter to mixer\n");
      gst_object_unref (capsfilter_src);
      gst_object_unref (mixer_sink0);
      gst_pipeline_destroy (ctx);
      return NULL;
    }
  gst_object_unref (capsfilter_src);
  gst_object_unref (mixer_sink0);

  // Link overlay: appsrc → mixer sink pad 1 (with alpha blending)
  GstPad *mixer_sink1 = gst_element_request_pad_simple (ctx->mixer, "sink_1");
  g_object_set (mixer_sink1, "alpha", 1.0, NULL); // Full alpha from source
  GstPad *appsrc_src = gst_element_get_static_pad (ctx->appsrc, "src");
  if (gst_pad_link (appsrc_src, mixer_sink1) != GST_PAD_LINK_OK)
    {
      g_printerr ("[GST_PIPELINE] Failed to link appsrc to mixer\n");
      gst_object_unref (appsrc_src);
      gst_object_unref (mixer_sink1);
      gst_pipeline_destroy (ctx);
      return NULL;
    }
  gst_object_unref (appsrc_src);
  gst_object_unref (mixer_sink1);

  // Link post-mixer: mixer → videoconvert → videoscale → encoder → muxer → filesink
  // Force I420 format before encoding (ensures standard H.264 profile, not 4:4:4)
  GstCaps *i420_caps = gst_caps_new_simple (
    "video/x-raw", "format", G_TYPE_STRING, "I420", NULL);

  if (!gst_element_link_many (ctx->mixer, ctx->videoconvert, ctx->videoscale, NULL))
    {
      g_printerr ("[GST_PIPELINE] Failed to link mixer → videoconvert → videoscale\n");
      gst_caps_unref (i420_caps);
      gst_pipeline_destroy (ctx);
      return NULL;
    }

  if (!gst_element_link_filtered (ctx->videoscale, ctx->encoder, i420_caps))
    {
      g_printerr ("[GST_PIPELINE] Failed to link videoscale → encoder with I420 filter\n");
      gst_caps_unref (i420_caps);
      gst_pipeline_destroy (ctx);
      return NULL;
    }
  gst_caps_unref (i420_caps);

  if (!gst_element_link_many (ctx->encoder, ctx->muxer, ctx->filesink, NULL))
    {
      g_printerr ("[GST_PIPELINE] Failed to link encoder → muxer → filesink\n");
      gst_pipeline_destroy (ctx);
      return NULL;
    }

  // Setup bus watch
  GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (ctx->pipeline));
  gst_bus_add_watch (bus, bus_callback, ctx);
  gst_object_unref (bus);

  g_print ("[GST_PIPELINE] Pipeline created successfully\n");
  return ctx;
}

bool
gst_pipeline_start (gst_pipeline_t *pipeline)
{
  if (!pipeline)
    return false;

  g_print ("[GST_PIPELINE] Starting pipeline\n");

  g_print ("[GST_PIPELINE] Setting state to PLAYING...\n");
  GstStateChangeReturn ret
    = gst_element_set_state (pipeline->pipeline, GST_STATE_PLAYING);
  g_print ("[GST_PIPELINE] State change result: %d\n", ret);

  if (ret == GST_STATE_CHANGE_FAILURE)
    {
      g_printerr ("[GST_PIPELINE] Failed to start pipeline\n");
      return false;
    }

  // Wait for state change
  g_print ("[GST_PIPELINE] Waiting for PLAYING state...\n");
  ret = gst_element_get_state (pipeline->pipeline, NULL, NULL,
                                5 * GST_SECOND); // 5 second timeout instead of infinite
  g_print ("[GST_PIPELINE] Get state result: %d\n", ret);

  if (ret == GST_STATE_CHANGE_FAILURE)
    {
      g_printerr ("[GST_PIPELINE] Failed to reach PLAYING state\n");
      return false;
    }

  if (ret == GST_STATE_CHANGE_ASYNC)
    {
      g_print ("[GST_PIPELINE] State change still async after timeout, continuing anyway\n");
    }

  pipeline->is_playing = true;
  g_print ("[GST_PIPELINE] Pipeline is playing\n");
  return true;
}

bool
gst_pipeline_push_frame (gst_pipeline_t *pipeline,
                          const uint8_t *rgba_data,
                          uint64_t timestamp)
{
  if (!pipeline || !rgba_data)
    return false;

  size_t frame_size = pipeline->width * pipeline->height * 4; // RGBA

  // Create GStreamer buffer
  GstBuffer *buffer = gst_buffer_new_allocate (NULL, frame_size, NULL);
  if (!buffer)
    {
      g_printerr ("[GST_PIPELINE] Failed to allocate buffer\n");
      return false;
    }

  // Copy RGBA data to buffer
  GstMapInfo map;
  if (!gst_buffer_map (buffer, &map, GST_MAP_WRITE))
    {
      g_printerr ("[GST_PIPELINE] Failed to map buffer\n");
      gst_buffer_unref (buffer);
      return false;
    }

  memcpy (map.data, rgba_data, frame_size);
  gst_buffer_unmap (buffer, &map);

  // Set buffer timestamp
  GST_BUFFER_PTS (buffer) = timestamp;
  GST_BUFFER_DURATION (buffer)
    = gst_util_uint64_scale_int (1, GST_SECOND, pipeline->fps);

  // Push buffer to appsrc
  GstFlowReturn ret;
  g_signal_emit_by_name (pipeline->appsrc, "push-buffer", buffer, &ret);
  gst_buffer_unref (buffer);

  if (ret != GST_FLOW_OK)
    {
      g_printerr ("[GST_PIPELINE] Failed to push buffer: %d\n", ret);
      return false;
    }

  pipeline->frame_count++;
  if (pipeline->frame_count % 30 == 0)
    {
      g_print ("[GST_PIPELINE] Pushed %lu frames\n", pipeline->frame_count);
    }

  return true;
}

void
gst_pipeline_finish (gst_pipeline_t *pipeline)
{
  if (!pipeline)
    return;

  g_print ("[GST_PIPELINE] Finishing pipeline (total frames: %lu)\n",
           pipeline->frame_count);

  // Send EOS on appsrc (videotestsrc already sent EOS via num-buffers)
  GstFlowReturn eos_ret;
  g_signal_emit_by_name (pipeline->appsrc, "end-of-stream", &eos_ret);

  // Wait for EOS to propagate through pipeline (no main loop needed)
  // The pipeline will naturally reach PAUSED state when all sources send EOS
  g_print ("[GST_PIPELINE] Waiting for pipeline to complete...\n");
  GstBus *bus = gst_element_get_bus (pipeline->pipeline);
  GstMessage *msg = gst_bus_timed_pop_filtered (
    bus, GST_CLOCK_TIME_NONE,
    (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));

  if (msg != NULL)
    {
      if (GST_MESSAGE_TYPE (msg) == GST_MESSAGE_ERROR)
        {
          GError *err;
          gchar *debug;
          gst_message_parse_error (msg, &err, &debug);
          g_printerr ("[GST_PIPELINE] Error during finish: %s\n",
                      err->message);
          g_error_free (err);
          g_free (debug);
        }
      else
        {
          g_print ("[GST_PIPELINE] EOS received\n");
        }
      gst_message_unref (msg);
    }
  gst_object_unref (bus);

  g_print ("[GST_PIPELINE] Pipeline finished\n");
}

void
gst_pipeline_destroy (gst_pipeline_t *pipeline)
{
  if (!pipeline)
    return;

  g_print ("[GST_PIPELINE] Destroying pipeline\n");

  if (pipeline->pipeline)
    {
      gst_element_set_state (pipeline->pipeline, GST_STATE_NULL);
      gst_object_unref (pipeline->pipeline);
    }

  g_free ((gpointer)pipeline->output_file);
  g_free (pipeline);
}
