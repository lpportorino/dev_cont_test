#!/usr/bin/env ts-node
/**
 * JSON to Protobuf Converter
 *
 * Converts a JSON snapshot back to binary protobuf format for use with WASM module.
 *
 * Usage:
 *   npm run json2proto [input.json] [output.bin]
 *
 * If no arguments provided:
 *   Input: test/proto_snapshot.json
 *   Output: test/proto_snapshot.bin
 */

import * as fs from 'fs';
import * as path from 'path';
import { JonGUIState } from '../../../proto/ts/jon_shared_data';
import { BinaryWriter } from '@bufbuild/protobuf/wire';
import Long from 'long';

const INPUT_JSON = process.argv[2] || path.join(__dirname, '../../../test/proto_snapshot.json');
const OUTPUT_BIN = process.argv[3] || path.join(__dirname, '../../../test/proto_snapshot.bin');

/**
 * Convert plain JSON numbers to Long objects for 64-bit integer fields
 * The protobuf encoder expects Long objects, not plain numbers
 *
 * Strategy: Only convert fields that are DEFINITELY 64-bit Longs:
 * - Fields ending with 'Us' (microsecond timestamps)
 * - Fields with "timestamp" or "Timestamp" that have large values (> 1 billion)
 * - UUID part fields (uuidPart1, uuidPart2, etc.)
 */
function convertLongFields(obj: any): any {
  if (obj === null || obj === undefined || typeof obj !== 'object') {
    return obj;
  }

  // Handle arrays
  if (Array.isArray(obj)) {
    return obj.map(item => convertLongFields(item));
  }

  // Convert known Long fields based on naming patterns and value ranges
  const result: any = {};
  for (const key in obj) {
    const value = obj[key];

    if (typeof value === 'number') {
      // Check if this field is likely a Long (64-bit integer)
      const isLongField =
        key.endsWith('Us') ||           // microsecond timestamps (systemMonotonicTimeUs)
        key.includes('uuidPart') ||     // UUID parts
        // Timestamp fields with large values (> 1 billion = year 2001+)
        // This avoids converting small IDs or counts
        ((key.includes('timestamp') || key.includes('Timestamp')) && Math.abs(value) > 1000000000);

      if (isLongField) {
        result[key] = Long.fromNumber(value);
      } else {
        result[key] = value;
      }
    }
    // Recursively convert nested objects
    else if (typeof value === 'object') {
      result[key] = convertLongFields(value);
    }
    // Keep other values as-is
    else {
      result[key] = value;
    }
  }

  return result;
}

async function jsonToProto(): Promise<void> {
  console.log('========================================');
  console.log('  JSON → Protobuf Converter');
  console.log('========================================');
  console.log('');
  console.log(`Input JSON: ${INPUT_JSON}`);
  console.log(`Output Binary: ${OUTPUT_BIN}`);
  console.log('');

  // Read JSON file
  console.log('Reading JSON file...');
  if (!fs.existsSync(INPUT_JSON)) {
    throw new Error(`Input file not found: ${INPUT_JSON}`);
  }

  const jsonData = fs.readFileSync(INPUT_JSON, 'utf-8');
  const jsonObject = JSON.parse(jsonData);

  console.log('✓ JSON parsed successfully');
  console.log('  Protocol Version:', jsonObject.protocolVersion);
  console.log('  System Monotonic Time:', jsonObject.systemMonotonicTimeUs);
  console.log('  Fields present:', Object.keys(jsonObject).filter(k => jsonObject[k] !== undefined).length);
  console.log('');

  // Convert JSON to JonGUIState message using fromJSON
  // This handles all type conversions (Long, enums, etc.) automatically
  console.log('Converting JSON to protobuf message...');
  const protoMessage = JonGUIState.fromJSON(jsonObject);
  console.log('✓ Message converted');
  console.log('');

  // Encode JSON to protobuf binary
  console.log('Encoding to protobuf binary...');
  try {
    // Create a BinaryWriter
    const writer = new BinaryWriter();

    // Encode the message using generated TypeScript bindings
    JonGUIState.encode(protoMessage, writer);

    // Get the resulting bytes
    const binaryData = writer.finish();

    console.log('✓ Successfully encoded JonGUIState');
    console.log('  Output size:', binaryData.length, 'bytes');
    console.log('');

    // Write to file
    const buffer = Buffer.from(binaryData);
    fs.writeFileSync(OUTPUT_BIN, buffer);

    console.log(`✓ Saved binary protobuf: ${OUTPUT_BIN}`);
    console.log('');

    // Verify by comparing with original if it exists
    const originalBin = path.join(path.dirname(OUTPUT_BIN), 'proto_snapshot.original.bin');
    if (fs.existsSync(originalBin)) {
      const originalSize = fs.statSync(originalBin).size;
      console.log('Comparison with original:');
      console.log(`  Original: ${originalSize} bytes`);
      console.log(`  New:      ${buffer.length} bytes`);
      console.log(`  Difference: ${buffer.length - originalSize} bytes`);
      console.log('');
    }

    console.log('========================================');
    console.log('✅ Conversion complete');
    console.log('========================================');
    console.log('');
  } catch (error) {
    console.error('❌ Failed to encode protobuf:', error);
    throw error;
  }
}

// Run if called directly
if (require.main === module) {
  jsonToProto()
    .then(() => process.exit(0))
    .catch((error) => {
      console.error('');
      console.error('========================================');
      console.error('❌ Conversion failed');
      console.error('========================================');
      console.error('');
      console.error('Error:', error.message);
      console.error('');
      process.exit(1);
    });
}

export { jsonToProto };
