/**
 * @file gst_pipeline.h
 * @brief GStreamer pipeline for video encoding
 *
 * Creates appsrc → encoder → filesink pipeline for OSD testing.
 * Based on jettison appsrc_osd architecture.
 */

#ifndef GST_PIPELINE_H
#define GST_PIPELINE_H

#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <stdbool.h>
#include <stdint.h>

// Video pipeline context
typedef struct
{
  // GStreamer components
  GstElement *pipeline;
  GstElement *videotestsrc; // Noise/static background
  GstElement *capsfilter;   // Force videotestsrc to full resolution
  GstElement *appsrc;       // OSD overlay
  GstElement *mixer;        // Compositor (videomixer or compositor)
  GstElement *videoconvert;
  GstElement *videoscale;
  GstElement *encoder;
  GstElement *muxer;
  GstElement *filesink;

  // Video parameters
  uint32_t width;
  uint32_t height;
  uint32_t fps;
  const char *output_file;

  // State
  GMainLoop *loop;
  bool is_playing;
  uint64_t frame_count;
} gst_pipeline_t;

/**
 * Create and initialize GStreamer pipeline
 *
 * Pipeline:
 *   videotestsrc (snow/static) → capsfilter → mixer sink_0
 *   appsrc (OSD overlay) → mixer sink_1
 *   mixer → videoconvert → videoscale → x264enc → mp4mux → filesink
 *
 * @param width Video width
 * @param height Video height
 * @param fps Frames per second
 * @param num_frames Total number of frames (videotestsrc will auto-EOS after this)
 * @param output_file Output MP4 file path
 * @return Initialized pipeline, or NULL on error
 */
gst_pipeline_t *gst_pipeline_create (uint32_t width,
                                      uint32_t height,
                                      uint32_t fps,
                                      uint32_t num_frames,
                                      const char *output_file);

/**
 * Start pipeline playback
 *
 * @param pipeline Pipeline context
 * @return true on success, false on error
 */
bool gst_pipeline_start (gst_pipeline_t *pipeline);

/**
 * Push RGBA frame to pipeline
 *
 * @param pipeline Pipeline context
 * @param rgba_data RGBA frame data (width × height × 4 bytes)
 * @param timestamp Frame timestamp in nanoseconds
 * @return true on success, false on error
 */
bool gst_pipeline_push_frame (gst_pipeline_t *pipeline,
                               const uint8_t *rgba_data,
                               uint64_t timestamp);

/**
 * Signal end-of-stream and wait for completion
 *
 * @param pipeline Pipeline context
 */
void gst_pipeline_finish (gst_pipeline_t *pipeline);

/**
 * Stop pipeline and cleanup resources
 *
 * @param pipeline Pipeline context
 */
void gst_pipeline_destroy (gst_pipeline_t *pipeline);

#endif // GST_PIPELINE_H
