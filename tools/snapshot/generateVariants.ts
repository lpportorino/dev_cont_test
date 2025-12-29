#!/usr/bin/env ts-node
/**
 * Generate nav ball test variants with different compass orientations
 */

import * as fs from 'fs';
import * as path from 'path';
import { execSync } from 'child_process';

const PROJECT_ROOT = path.resolve(__dirname, '../..');
const VARIANTS_DIR = path.join(PROJECT_ROOT, 'snapshot/variants');
const JSON_PATH = path.join(PROJECT_ROOT, 'test/proto_snapshot.json');
const VARIANT_JSON_PATH = path.join(PROJECT_ROOT, 'test/variant_temp.json');
const VARIANT_BIN_PATH = path.join(PROJECT_ROOT, 'test/variant_temp.bin');

// Test configurations: [name, azimuth, elevation, bank]
const TEST_CONFIGS: [string, number, number, number][] = [
  ['level_south', 180, 0, 0],
  ['level_north', 0, 0, 0],
  ['level_east', 90, 0, 0],
  ['level_west', 270, 0, 0],
  ['pitch_up_30', 180, 30, 0],
  ['pitch_down_30', 180, -30, 0],
  ['roll_right_45', 180, 0, 45],
  ['roll_left_45', 180, 0, -45],
  ['complex_1', 45, 20, 15],
  ['complex_2', 315, -25, -30],
];

// Filtering modes to test
const FILTER_MODES = [
  { name: 'fixed', useFloat: 0, suffix: '_fixed' },
  { name: 'float', useFloat: 1, suffix: '_float' },
];

async function main() {
  console.log('==========================================');
  console.log('  Nav Ball Test Variant Generator');
  console.log('==========================================');
  console.log('');

  // Ensure output directory exists
  if (!fs.existsSync(VARIANTS_DIR)) {
    fs.mkdirSync(VARIANTS_DIR, { recursive: true });
  }

  // Load base JSON
  if (!fs.existsSync(JSON_PATH)) {
    console.error('❌ test/proto_snapshot.json not found');
    process.exit(1);
  }

  const baseJson = JSON.parse(fs.readFileSync(JSON_PATH, 'utf-8'));

  for (const [name, azimuth, elevation, bank] of TEST_CONFIGS) {
    console.log(`Generating: ${name} (az=${azimuth}°, el=${elevation}°, bank=${bank}°)`);

    // Create modified JSON
    const variantJson = JSON.parse(JSON.stringify(baseJson));
    variantJson.compass.azimuth = azimuth;
    variantJson.compass.elevation = elevation;
    variantJson.compass.bank = bank;
    variantJson.compass.heading = azimuth;
    variantJson.compass.pitch = elevation;
    variantJson.compass.roll = bank;

    // Write variant JSON
    fs.writeFileSync(VARIANT_JSON_PATH, JSON.stringify(variantJson, null, 2));

    // Convert to binary protobuf
    execSync(
      `npm run json2proto -- ${VARIANT_JSON_PATH} ${VARIANT_BIN_PATH}`,
      { cwd: __dirname, stdio: 'pipe' }
    );

    // Copy binary to standard location for test harness
    fs.copyFileSync(VARIANT_BIN_PATH, path.join(PROJECT_ROOT, 'test/proto_snapshot.bin'));

    // Run test harness
    execSync(
      `./test/osd_test build/osd.wasm`,
      { cwd: PROJECT_ROOT, stdio: 'pipe' }
    );

    // Copy output with descriptive name
    fs.copyFileSync(
      path.join(PROJECT_ROOT, 'snapshot/osd_render.png'),
      path.join(VARIANTS_DIR, `${name}.png`)
    );

    console.log(`  ✓ Saved: snapshot/variants/${name}.png`);
  }

  // Restore original proto
  execSync(
    `npm run json2proto -- ${JSON_PATH} ${path.join(PROJECT_ROOT, 'test/proto_snapshot.bin')}`,
    { cwd: __dirname, stdio: 'pipe' }
  );

  // Cleanup temp files
  if (fs.existsSync(VARIANT_JSON_PATH)) fs.unlinkSync(VARIANT_JSON_PATH);
  if (fs.existsSync(VARIANT_BIN_PATH)) fs.unlinkSync(VARIANT_BIN_PATH);

  console.log('');
  console.log('==========================================');
  console.log(`✅ Generated ${TEST_CONFIGS.length} test variants`);
  console.log('==========================================');
  console.log('');
  console.log('Output directory: snapshot/variants/');
  console.log('');

  // List generated files
  const files = fs.readdirSync(VARIANTS_DIR).filter(f => f.endsWith('.png'));
  files.forEach(file => {
    const stats = fs.statSync(path.join(VARIANTS_DIR, file));
    const sizeKB = (stats.size / 1024).toFixed(1);
    console.log(`  ${file.padEnd(25)} ${sizeKB.padStart(6)} KB`);
  });
  console.log('');
}

main().catch(err => {
  console.error('Error:', err.message);
  process.exit(1);
});
