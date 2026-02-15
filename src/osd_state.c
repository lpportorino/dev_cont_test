// OSD State Accessors Implementation
// Extracts data from protobuf state for widget consumption

#include "osd_state.h"

#include "core/osd_context.h"
#include "proto/jon_shared_data.pb.h"

#include <string.h>

// ════════════════════════════════════════════════════════════
// ORIENTATION DATA
// ════════════════════════════════════════════════════════════

bool
osd_state_get_orientation(const osd_state_t *state,
                          double *azimuth,
                          double *elevation,
                          double *bank)
{
  if (!state || !state->has_compass)
    return false;

  if (azimuth)
    *azimuth = state->compass.azimuth;
  if (elevation)
    *elevation = state->compass.elevation;
  if (bank)
    *bank = state->compass.bank;

  return true;
}

// ════════════════════════════════════════════════════════════
// SPEED DATA
// ════════════════════════════════════════════════════════════

bool
osd_state_get_speeds(const osd_state_t *state,
                     double *azimuth_speed,
                     double *elevation_speed,
                     bool *is_moving)
{
  if (!state || !state->has_rotary)
    return false;

  if (azimuth_speed)
    *azimuth_speed = state->rotary.azimuth_speed;
  if (elevation_speed)
    *elevation_speed = state->rotary.elevation_speed;
  if (is_moving)
    *is_moving = state->rotary.is_moving;

  return true;
}

// ════════════════════════════════════════════════════════════
// CROSSHAIR OFFSET
// ════════════════════════════════════════════════════════════

void
osd_state_get_crosshair_offset(const osd_state_t *state,
                               bool is_thermal_stream,
                               int *offset_x,
                               int *offset_y)
{
  // Default to center (no offset)
  int x = 0;
  int y = 0;

  if (state && state->has_rec_osd)
    {
      if (is_thermal_stream)
        {
          x = state->rec_osd.heat_crosshair_offset_horizontal;
          y = state->rec_osd.heat_crosshair_offset_vertical;
        }
      else
        {
          x = state->rec_osd.day_crosshair_offset_horizontal;
          y = state->rec_osd.day_crosshair_offset_vertical;
        }
    }

  if (offset_x)
    *offset_x = x;
  if (offset_y)
    *offset_y = y;
}

// ════════════════════════════════════════════════════════════
// TIME DATA
// ════════════════════════════════════════════════════════════

int64_t
osd_state_get_timestamp(const osd_state_t *state)
{
  if (!state || !state->has_time)
    return 0;

  return state->time.timestamp;
}

// ════════════════════════════════════════════════════════════
// GPS DATA
// ════════════════════════════════════════════════════════════

bool
osd_state_get_gps(const osd_state_t *state, osd_gps_position_t *pos)
{
  if (!pos)
    return false;

  // Initialize to invalid
  pos->valid     = false;
  pos->latitude  = 0.0;
  pos->longitude = 0.0;
  pos->altitude  = 0.0;
  pos->timestamp = 0;

  if (!state || !state->has_actual_space_time)
    return false;

  const ser_JonGuiDataActualSpaceTime *ast = &state->actual_space_time;

  pos->latitude  = ast->latitude;
  pos->longitude = ast->longitude;
  pos->altitude  = ast->altitude;
  pos->timestamp = ast->timestamp;
  pos->valid     = true;

  return true;
}

// ════════════════════════════════════════════════════════════
// STATE TIMING DATA
// ════════════════════════════════════════════════════════════

uint64_t
osd_state_get_monotonic_time_us(const osd_state_t *state)
{
  if (!state)
    return 0;

  return state->system_monotonic_time_us;
}

uint64_t
osd_state_get_frame_monotonic_day_us(const osd_state_t *state)
{
  if (!state)
    return 0;

  return state->frame_monotonic_day_us;
}

uint64_t
osd_state_get_frame_monotonic_heat_us(const osd_state_t *state)
{
  if (!state)
    return 0;

  return state->frame_monotonic_heat_us;
}

// ════════════════════════════════════════════════════════════
// ROI DATA
// ════════════════════════════════════════════════════════════

bool
osd_state_get_rois(const osd_state_t *state,
                   bool is_thermal_stream,
                   osd_roi_data_t *out)
{
  if (!out)
    return false;

  memset(out, 0, sizeof(*out));

  if (!state || !state->has_cv)
    return false;

  const ser_JonGuiDataCV *cv = &state->cv;

  if (is_thermal_stream)
    {
      if (cv->has_roi_focus_heat)
        {
          out->focus
            = (osd_roi_t){ cv->roi_focus_heat.x1, cv->roi_focus_heat.y1,
                           cv->roi_focus_heat.x2, cv->roi_focus_heat.y2, true };
        }
      if (cv->has_roi_track_heat)
        {
          out->track
            = (osd_roi_t){ cv->roi_track_heat.x1, cv->roi_track_heat.y1,
                           cv->roi_track_heat.x2, cv->roi_track_heat.y2, true };
        }
      if (cv->has_roi_zoom_heat)
        {
          out->zoom
            = (osd_roi_t){ cv->roi_zoom_heat.x1, cv->roi_zoom_heat.y1,
                           cv->roi_zoom_heat.x2, cv->roi_zoom_heat.y2, true };
        }
      if (cv->has_roi_fx_heat)
        {
          out->fx = (osd_roi_t){ cv->roi_fx_heat.x1, cv->roi_fx_heat.y1,
                                 cv->roi_fx_heat.x2, cv->roi_fx_heat.y2, true };
        }
    }
  else
    {
      if (cv->has_roi_focus_day)
        {
          out->focus
            = (osd_roi_t){ cv->roi_focus_day.x1, cv->roi_focus_day.y1,
                           cv->roi_focus_day.x2, cv->roi_focus_day.y2, true };
        }
      if (cv->has_roi_track_day)
        {
          out->track
            = (osd_roi_t){ cv->roi_track_day.x1, cv->roi_track_day.y1,
                           cv->roi_track_day.x2, cv->roi_track_day.y2, true };
        }
      if (cv->has_roi_zoom_day)
        {
          out->zoom
            = (osd_roi_t){ cv->roi_zoom_day.x1, cv->roi_zoom_day.y1,
                           cv->roi_zoom_day.x2, cv->roi_zoom_day.y2, true };
        }
      if (cv->has_roi_fx_day)
        {
          out->fx = (osd_roi_t){ cv->roi_fx_day.x1, cv->roi_fx_day.y1,
                                 cv->roi_fx_day.x2, cv->roi_fx_day.y2, true };
        }
    }

  out->valid = true;
  return true;
}

// ════════════════════════════════════════════════════════════
// CAMERA DAY DATA
// ════════════════════════════════════════════════════════════

bool
osd_state_get_camera_day(const osd_state_t *state, osd_camera_day_data_t *out)
{
  if (!out)
    return false;

  out->valid           = false;
  out->sensor_gain     = 0.0;
  out->iris_pos        = 0.0;
  out->focus_pos       = 0.0;
  out->zoom_pos        = 0.0;
  out->exposure        = 0.0;
  out->auto_gain       = false;
  out->auto_iris       = false;
  out->has_sensor_gain = false;
  out->has_exposure    = false;

  if (!state || !state->has_camera_day)
    return false;

  const ser_JonGuiDataCameraDay *cam = &state->camera_day;

  out->sensor_gain     = cam->sensor_gain;
  out->iris_pos        = cam->iris_pos;
  out->focus_pos       = cam->focus_pos;
  out->zoom_pos        = cam->zoom_pos;
  out->exposure        = cam->exposure;
  out->auto_gain       = cam->auto_gain;
  out->auto_iris       = cam->auto_iris;
  out->has_sensor_gain = cam->has_sensor_gain;
  out->has_exposure    = cam->has_exposure;
  out->valid           = true;

  return true;
}

// ════════════════════════════════════════════════════════════
// SHARPNESS DATA
// ════════════════════════════════════════════════════════════

bool
osd_state_get_sharpness(const osd_context_t *ctx, osd_sharpness_data_t *out)
{
  if (!out || !ctx)
    return false;

  out->valid        = false;
  out->global_score = 0.0f;
  out->grid_count   = 0;

  if (!ctx->cv_meta.sharpness_valid)
    return false;

  out->global_score = ctx->cv_meta.sharpness_level0;
  out->grid_count   = ctx->cv_meta.sharpness_level3_count;

  int copy_count = out->grid_count;
  if (copy_count > 64)
    copy_count = 64;
  for (int i = 0; i < copy_count; i++)
    {
      out->grid_8x8[i] = ctx->cv_meta.sharpness_level3[i];
    }

  out->valid = true;
  return true;
}

// ════════════════════════════════════════════════════════════
// DETECTION DATA
// ════════════════════════════════════════════════════════════

bool
osd_state_get_detections(const osd_context_t *ctx, osd_detections_data_t *out)
{
  if (!out || !ctx)
    return false;

  out->valid  = false;
  out->count  = 0;
  out->status = 0;

  if (!ctx->detections.valid)
    return false;

  out->status = ctx->detections.status;
  out->count  = ctx->detections.count;

  int copy_count = out->count;
  if (copy_count > 64)
    copy_count = 64;
  for (int i = 0; i < copy_count; i++)
    {
      out->items[i].x1         = ctx->detections.items[i].x1;
      out->items[i].y1         = ctx->detections.items[i].y1;
      out->items[i].x2         = ctx->detections.items[i].x2;
      out->items[i].y2         = ctx->detections.items[i].y2;
      out->items[i].confidence = ctx->detections.items[i].confidence;
      out->items[i].class_id   = ctx->detections.items[i].class_id;
    }

  out->valid = true;
  return true;
}

// ════════════════════════════════════════════════════════════
// CLIENT METADATA
// ════════════════════════════════════════════════════════════

bool
osd_state_get_client_metadata(const osd_context_t *ctx,
                              osd_client_metadata_t *metadata)
{
  // Check both pointers upfront before any dereference
  if (!metadata || !ctx)
    return false;

  // Initialize to invalid/zero
  metadata->valid              = false;
  metadata->canvas_width_px    = 0;
  metadata->canvas_height_px   = 0;
  metadata->device_pixel_ratio = 0.0f;
  metadata->osd_buffer_width   = 0;
  metadata->osd_buffer_height  = 0;

  // Proxy bounds
  metadata->video_proxy_ndc_x      = 0.0f;
  metadata->video_proxy_ndc_y      = 0.0f;
  metadata->video_proxy_ndc_width  = 0.0f;
  metadata->video_proxy_ndc_height = 0.0f;

  // Scale factor
  metadata->scale_factor = 0.0f;

  // Theme info
  metadata->is_sharp_mode   = false;
  metadata->theme_hue       = 0.0f;
  metadata->theme_chroma    = 0.0f;
  metadata->theme_lightness = 0.0f;

  if (!ctx->client_metadata.valid)
    return false;

  // Canvas info
  metadata->canvas_width_px    = ctx->client_metadata.canvas_width_px;
  metadata->canvas_height_px   = ctx->client_metadata.canvas_height_px;
  metadata->device_pixel_ratio = ctx->client_metadata.device_pixel_ratio;
  metadata->osd_buffer_width   = ctx->client_metadata.osd_buffer_width;
  metadata->osd_buffer_height  = ctx->client_metadata.osd_buffer_height;

  // Proxy bounds
  metadata->video_proxy_ndc_x     = ctx->client_metadata.video_proxy_ndc_x;
  metadata->video_proxy_ndc_y     = ctx->client_metadata.video_proxy_ndc_y;
  metadata->video_proxy_ndc_width = ctx->client_metadata.video_proxy_ndc_width;
  metadata->video_proxy_ndc_height
    = ctx->client_metadata.video_proxy_ndc_height;

  // Scale factor
  metadata->scale_factor = ctx->client_metadata.scale_factor;

  // Theme info
  metadata->is_sharp_mode   = ctx->client_metadata.is_sharp_mode;
  metadata->theme_hue       = ctx->client_metadata.theme_hue;
  metadata->theme_chroma    = ctx->client_metadata.theme_chroma;
  metadata->theme_lightness = ctx->client_metadata.theme_lightness;

  metadata->valid = true;

  return true;
}

// ════════════════════════════════════════════════════════════
// SAM TRACKING DATA
// ════════════════════════════════════════════════════════════

bool
osd_state_get_sam_tracking(const osd_context_t *ctx,
                           osd_sam_tracking_data_t *out)
{
  if (!out || !ctx)
    return false;

  out->valid            = false;
  out->status           = 0;
  out->state            = 0;
  out->bbox_x1          = 0.0f;
  out->bbox_y1          = 0.0f;
  out->bbox_x2          = 0.0f;
  out->bbox_y2          = 0.0f;
  out->centroid_x       = 0.0f;
  out->centroid_y       = 0.0f;
  out->confidence       = 0.0f;
  out->mask_width       = 0;
  out->mask_height      = 0;
  out->mask_pixels      = 0;
  out->kf_predicted_x   = 0.0f;
  out->kf_predicted_y   = 0.0f;
  out->lost_frame_count = 0;

  if (!ctx->sam_tracking.valid)
    return false;

  out->status           = ctx->sam_tracking.status;
  out->state            = ctx->sam_tracking.state;
  out->bbox_x1          = ctx->sam_tracking.bbox_x1;
  out->bbox_y1          = ctx->sam_tracking.bbox_y1;
  out->bbox_x2          = ctx->sam_tracking.bbox_x2;
  out->bbox_y2          = ctx->sam_tracking.bbox_y2;
  out->centroid_x       = ctx->sam_tracking.centroid_x;
  out->centroid_y       = ctx->sam_tracking.centroid_y;
  out->confidence       = ctx->sam_tracking.confidence;
  out->mask_width       = ctx->sam_tracking.mask_width;
  out->mask_height      = ctx->sam_tracking.mask_height;
  out->mask_pixels      = ctx->sam_tracking.mask_pixels;
  out->kf_predicted_x   = ctx->sam_tracking.kf_predicted_x;
  out->kf_predicted_y   = ctx->sam_tracking.kf_predicted_y;
  out->lost_frame_count = ctx->sam_tracking.lost_frame_count;

  out->valid = true;
  return true;
}
