import * as fs from 'fs';
import { JonGUIState } from '../../../proto/ts/jon_shared_data';
import { BinaryWriter } from '@bufbuild/protobuf/wire';

// Generate animated sequence of speed indicators
// Simulates rotary scanning left-right and up-down

const FRAMES = 120; // 2 seconds at 60fps
const OUTPUT_DIR = '../../test/animation';

// Create output directory
if (!fs.existsSync(OUTPUT_DIR)) {
  fs.mkdirSync(OUTPUT_DIR, { recursive: true });
}

// Load base snapshot
const baseJSON = JSON.parse(fs.readFileSync('../../test/proto_snapshot.json', 'utf-8'));

console.log('========================================');
console.log('  Animation Generator');
console.log('========================================');
console.log();
console.log(`Generating ${FRAMES} frames...`);
console.log();

// Starting position and time
let current_azimuth = baseJSON.rotary.azimuth;
let current_elevation = baseJSON.rotary.elevation;
let current_timestamp = baseJSON.time.timestamp; // Unix timestamp (seconds)

for (let frame = 0; frame < FRAMES; frame++) {
  // Calculate animation time (0 to 1)
  const t = frame / FRAMES;
  const angle = t * Math.PI * 4; // 2 full cycles

  // Animate azimuth speed (left-right): sine wave
  // Speed is normalized 0.0-1.0 per protobuf spec (not degrees!)
  const azimuth_speed = Math.sin(angle) * 0.8; // ±0.8 (80% max speed)

  // Animate elevation speed (up-down): cosine wave (90° phase shift)
  const elevation_speed = Math.cos(angle) * 0.6; // ±0.6 (60% max speed)

  // Convert normalized speed to degrees for realistic motion
  // Assume max speed = 10 deg/s at speed=1.0
  const MAX_DEG_PER_SEC = 10.0;
  const dt = 1/60; // 60fps = 1/60 second per frame
  current_azimuth += azimuth_speed * MAX_DEG_PER_SEC * dt;
  current_elevation += elevation_speed * MAX_DEG_PER_SEC * dt;

  // Wrap azimuth to 0-360
  while (current_azimuth < 0) current_azimuth += 360;
  while (current_azimuth >= 360) current_azimuth -= 360;

  // Clamp elevation to ±90
  current_elevation = Math.max(-90, Math.min(90, current_elevation));

  // Update rotary state (navball and compass will use these)
  baseJSON.rotary.azimuth = current_azimuth;
  baseJSON.rotary.elevation = current_elevation;
  baseJSON.rotary.azimuthSpeed = azimuth_speed;
  baseJSON.rotary.elevationSpeed = elevation_speed;
  baseJSON.rotary.isMoving = Math.abs(azimuth_speed) > 0.05 || Math.abs(elevation_speed) > 0.05;

  // Sync compass with rotary (for realistic navball orientation)
  baseJSON.compass.azimuth = current_azimuth;
  baseJSON.compass.elevation = current_elevation;

  // Update timestamp (advance by 1/60 second = one frame at 60fps)
  current_timestamp += dt;
  baseJSON.time.timestamp = Math.floor(current_timestamp);

  // Convert to protobuf
  const message = JonGUIState.fromJSON(baseJSON);
  const writer = new BinaryWriter();
  JonGUIState.encode(message, writer);
  const binary = writer.finish();

  // Save frame
  const frameNum = String(frame).padStart(4, '0');
  const framePath = `${OUTPUT_DIR}/frame_${frameNum}.bin`;
  fs.writeFileSync(framePath, binary);

  if (frame % 20 === 0) {
    console.log(`Frame ${frame}/${FRAMES}: az=${azimuth_speed.toFixed(2)}°/s, el=${elevation_speed.toFixed(2)}°/s`);
  }
}

console.log();
console.log('✅ Animation frames generated');
console.log(`Output: ${OUTPUT_DIR}/frame_*.bin`);
console.log();
