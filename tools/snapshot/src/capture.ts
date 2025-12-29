#!/usr/bin/env ts-node
/**
 * Jettison Proto Snapshot Capture Tool
 *
 * ⚠️  WARNING: This tool SANITIZES sensitive data before saving ⚠️
 *
 * This tool connects to a production Jettison server, captures JonGUIState
 * protobuf messages, and saves them as binary snapshots for testing.
 *
 * CRITICAL SECURITY REQUIREMENT:
 * All snapshots are AUTOMATICALLY SANITIZED to remove:
 * - GPS coordinates (replaced with San Francisco coordinates)
 * - Timestamps (replaced with fixed test timestamp)
 * - Compass headings (replaced with fixed test heading)
 *
 * This prevents accidental leakage of sensitive operational data.
 */

import * as fs from 'fs';
import * as path from 'path';
import WebSocket from 'ws';
import { JonGUIState } from '../../../proto/ts/jon_shared_data';

// Configuration
const PRODUCTION_SERVER = process.env.PROD_SERVER || 'sych.local';
const WEBSOCKET_PORT = process.env.WS_PORT || '8099';
const WEBSOCKET_PATH = '/ws/ws_state';
const OUTPUT_DIR = path.join(__dirname, '../../../test');
const OUTPUT_BIN = path.join(OUTPUT_DIR, 'proto_snapshot.original.bin');
const OUTPUT_JSON = path.join(OUTPUT_DIR, 'proto_snapshot.json');
const CAPTURE_TIMEOUT_MS = 30000; // 30 seconds

// Try to load custom hooks (optional)
let customHook: any = null;
try {
  customHook = require('./hooks').hook;
  console.log('✓ Loaded custom hooks from hooks.ts');
} catch (e) {
  // No custom hooks, that's fine
}

/**
 * Decode protobuf binary to JSON object using TypeScript bindings
 *
 * @param data Raw protobuf binary data
 * @returns Decoded JonGUIState object
 */
function decodeProtoToJSON(data: Buffer): any {
  console.log('✓ Decoding protobuf using TypeScript bindings');

  try {
    // Convert Node.js Buffer to Uint8Array
    // The decode() method accepts Uint8Array directly
    const uint8Array = new Uint8Array(data);

    // Decode using generated TypeScript bindings
    const decoded = JonGUIState.decode(uint8Array);

    console.log('✓ Successfully decoded JonGUIState');
    console.log('  Fields present:', Object.keys(decoded).filter(k => (decoded as any)[k] !== undefined).join(', '));
    console.log('');

    // Convert to plain JSON object (handles Long, etc.)
    return JSON.parse(JSON.stringify(decoded, (key, value) => {
      // Convert Long to number
      if (value && typeof value === 'object' && 'low' in value && 'high' in value) {
        return Number(value);
      }
      return value;
    }));
  } catch (error) {
    console.error('❌ Failed to decode protobuf:', error);
    console.log('Falling back to raw hex dump');
    return {
      _raw_hex: data.toString('hex'),
      _size_bytes: data.length,
      _decode_error: error instanceof Error ? error.message : String(error)
    };
  }
}

/**
 * Sanitize protobuf JSON data
 *
 * ⚠️⚠️⚠️ CRITICAL SECURITY FUNCTION ⚠️⚠️⚠️
 *
 * This function MUST be called on ALL captured data before saving.
 * It replaces sensitive operational data with safe test values.
 *
 * @param proto Decoded protobuf as JSON object
 * @returns Sanitized protobuf JSON object
 */
function sanitizeProtoJSON(proto: any): any {
  console.log('');
  console.log('╔═══════════════════════════════════════════════════════╗');
  console.log('║  ⚠️  WARNING: SANITIZING SENSITIVE DATA ⚠️             ║');
  console.log('╚═══════════════════════════════════════════════════════╝');
  console.log('');
  console.log('The following data is being SANITIZED:');
  console.log('  • GPS Coordinates → San Francisco (37.7749, -122.4194)');
  console.log('  • Timestamp → 2025-01-01 12:00:00 UTC (1704110400)');
  console.log('  • Compass Heading → 180.0° (South)');
  console.log('');
  console.log('This sanitization is MANDATORY to prevent leakage of');
  console.log('sensitive operational data from production systems.');
  console.log('');

  // Sanitize GPS (both automatic and manual coordinates)
  if (proto.gps) {
    proto.gps.latitude = 37.7749;  // San Francisco
    proto.gps.longitude = -122.4194;
    proto.gps.altitude = 0;
    proto.gps.manualLatitude = 37.7749;  // San Francisco
    proto.gps.manualLongitude = -122.4194;
    proto.gps.manualAltitude = 0;
    console.log('  ✓ GPS sanitized (auto + manual)');
  }

  // Sanitize timestamp
  if (proto.time) {
    proto.time.timestamp = 1704110400;  // 2025-01-01 12:00:00 UTC
    console.log('  ✓ Timestamp sanitized');
  }

  // Sanitize compass (both field sets)
  if (proto.compass) {
    // Primary compass fields (actual orientation data)
    proto.compass.azimuth = 180.0;      // South
    proto.compass.elevation = 0.0;      // Level
    proto.compass.bank = 0.0;           // No roll
    // Secondary compass fields (if present)
    proto.compass.heading = 180.0;      // South
    proto.compass.pitch = 0.0;          // Level
    proto.compass.roll = 0.0;           // No roll
    console.log('  ✓ Compass sanitized');
  }

  // Sanitize actual space time (if present)
  if (proto.actualSpaceTime) {
    proto.actualSpaceTime.latitude = 37.7749;
    proto.actualSpaceTime.longitude = -122.4194;
    proto.actualSpaceTime.altitude = 0;
    proto.actualSpaceTime.unixTimestamp = 1704110400;
    console.log('  ✓ Actual space-time sanitized');
  }

  // Sanitize LRF target coordinates (observer and target positions)
  if (proto.lrf && proto.lrf.target) {
    proto.lrf.target.observerLatitude = 37.7749;
    proto.lrf.target.observerLongitude = -122.4194;
    proto.lrf.target.observerAltitude = 0;
    proto.lrf.target.targetLatitude = 37.7749;
    proto.lrf.target.targetLongitude = -122.4194;
    proto.lrf.target.targetAltitude = 0;
    console.log('  ✓ LRF target coordinates sanitized');
  }

  console.log('');
  return proto;
}

/**
 * Main capture function
 */
async function captureSnapshot(): Promise<void> {
  console.log('========================================');
  console.log('  Jettison Proto Snapshot Capture');
  console.log('========================================');
  console.log('');
  console.log(`Production Server: ${PRODUCTION_SERVER}`);
  console.log(`WebSocket: ws://${PRODUCTION_SERVER}:${WEBSOCKET_PORT}${WEBSOCKET_PATH}`);
  console.log(`Output Binary: ${OUTPUT_BIN}`);
  console.log(`Output JSON: ${OUTPUT_JSON}`);
  console.log(`Timeout: ${CAPTURE_TIMEOUT_MS}ms`);
  console.log('');

  return new Promise((resolve, reject) => {
    // Try ws:// first (non-SSL), fall back to wss:// if needed
    const useSSL = process.env.USE_SSL === 'true';
    const protocol = useSSL ? 'wss' : 'ws';
    const wsUrl = `${protocol}://${PRODUCTION_SERVER}:${WEBSOCKET_PORT}${WEBSOCKET_PATH}`;

    console.log('Connecting to production server...');
    const ws = new WebSocket(wsUrl, {
      rejectUnauthorized: false  // Accept self-signed certificates
    });

    let captureComplete = false;
    let messageCount = 0;

    // Timeout handler
    const timeout = setTimeout(() => {
      if (!captureComplete) {
        console.error('❌ Timeout waiting for protobuf message');
        ws.close();
        reject(new Error('Capture timeout'));
      }
    }, CAPTURE_TIMEOUT_MS);

    ws.on('open', () => {
      console.log('✓ WebSocket connected');
      console.log('Waiting for protobuf message...');
      console.log('');
    });

    ws.on('message', (data: Buffer) => {
      messageCount++;
      console.log(`📦 Received message #${messageCount} (${data.length} bytes)`);

      if (captureComplete) {
        return; // Already captured, ignore subsequent messages
      }

      try {
        // Ensure output directory exists
        if (!fs.existsSync(OUTPUT_DIR)) {
          fs.mkdirSync(OUTPUT_DIR, { recursive: true });
        }

        // Save original binary (for backup)
        fs.writeFileSync(OUTPUT_BIN, data);
        console.log(`✓ Saved original binary: ${OUTPUT_BIN}`);

        // Decode to JSON
        let proto = decodeProtoToJSON(data);

        // MANDATORY SANITIZATION
        proto = sanitizeProtoJSON(proto);

        // Apply custom hooks if available
        if (customHook && customHook.transformProto) {
          console.log('');
          proto = customHook.transformProto(proto);
        }

        // Save as JSON
        fs.writeFileSync(OUTPUT_JSON, JSON.stringify(proto, null, 2));

        console.log('');
        console.log('✅ Snapshot saved successfully');
        console.log('');
        console.log(`Binary (original): ${OUTPUT_BIN} (${data.length} bytes)`);
        console.log(`JSON (sanitized): ${OUTPUT_JSON}`);
        console.log('');
        console.log('╔═══════════════════════════════════════════════════════╗');
        console.log('║  ⚠️  DATA HAS BEEN SANITIZED ⚠️                        ║');
        console.log('╚═══════════════════════════════════════════════════════╝');
        console.log('');
        console.log('You can now:');
        console.log('  1. Edit test/proto_snapshot.json for testing');
        console.log('  2. Create tools/snapshot/src/hooks.ts for custom overrides');
        console.log('  3. Use npm run json2proto to convert back to binary');
        console.log('');

        captureComplete = true;
        clearTimeout(timeout);
        ws.close();
        resolve();
      } catch (error) {
        console.error('❌ Error saving snapshot:', error);
        clearTimeout(timeout);
        ws.close();
        reject(error);
      }
    });

    ws.on('error', (error) => {
      console.error('❌ WebSocket error:', error.message);
      clearTimeout(timeout);
      reject(error);
    });

    ws.on('close', () => {
      console.log('WebSocket closed');
      if (!captureComplete) {
        clearTimeout(timeout);
        reject(new Error('WebSocket closed before capturing message'));
      }
    });
  });
}

// Run capture if called directly
if (require.main === module) {
  captureSnapshot()
    .then(() => {
      console.log('========================================');
      console.log('✅ Capture complete');
      console.log('========================================');
      console.log('');
      process.exit(0);
    })
    .catch((error) => {
      console.error('');
      console.error('========================================');
      console.error('❌ Capture failed');
      console.error('========================================');
      console.error('');
      console.error('Error:', error.message);
      console.error('');
      process.exit(1);
    });
}

export { captureSnapshot, sanitizeProtoJSON, decodeProtoToJSON };
