/**
 * Example Hook for Field Overrides
 *
 * Copy this file to `hooks.ts` and customize the field overrides.
 * The hook function receives the decoded JSON protobuf and can modify any fields.
 *
 * This is useful for:
 * - Testing with specific values
 * - Anonymizing sensitive data
 * - Creating test fixtures with known states
 */

export interface ProtoHook {
  /**
   * Called after sanitization, before saving to JSON
   * @param proto The decoded protobuf as JSON object
   * @returns Modified protobuf object
   */
  transformProto(proto: any): any;
}

/**
 * Example hook implementation
 */
export const hook: ProtoHook = {
  transformProto(proto: any): any {
    console.log('🔧 Applying custom field overrides from hooks.ts');

    // Example: Override GPS coordinates to a specific test location
    if (proto.gps) {
      proto.gps.latitude = 40.7128;  // New York City
      proto.gps.longitude = -74.0060;
      console.log('  • GPS → New York City');
    }

    // Example: Set compass to due north
    if (proto.compass) {
      proto.compass.heading = 0.0;  // North
      console.log('  • Compass → 0° (North)');
    }

    // Example: Set specific timestamp
    if (proto.time) {
      proto.time.timestamp = 1704110400;  // 2025-01-01 12:00:00 UTC
      console.log('  • Timestamp → 2025-01-01 12:00:00 UTC');
    }

    // Example: Override rotary state
    if (proto.rotary) {
      proto.rotary.is_moving = false;
      proto.rotary.speed_azimuth = 0;
      proto.rotary.speed_elevation = 0;
      console.log('  • Rotary → Stopped');
    }

    // Example: Set crosshair offset
    if (proto.camera_alignment) {
      proto.camera_alignment.day_cross_hair_offset_hor = 10;
      proto.camera_alignment.day_cross_hair_offset_ver = -5;
      console.log('  • Crosshair offset → (10, -5)');
    }

    return proto;
  }
};
