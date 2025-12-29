#!/usr/bin/env python3
"""Modify compass values in proto snapshot JSON"""

import json
import sys

if len(sys.argv) != 6:
    print("Usage: modify_compass.py <input.json> <output.json> <azimuth> <elevation> <bank>")
    sys.exit(1)

input_file = sys.argv[1]
output_file = sys.argv[2]
azimuth = float(sys.argv[3])
elevation = float(sys.argv[4])
bank = float(sys.argv[5])

with open(input_file, 'r') as f:
    data = json.load(f)

# Update compass fields
data['compass']['azimuth'] = azimuth
data['compass']['elevation'] = elevation
data['compass']['bank'] = bank
data['compass']['heading'] = azimuth
data['compass']['pitch'] = elevation
data['compass']['roll'] = bank

with open(output_file, 'w') as f:
    json.dump(data, f, indent=2)

print(f"Updated compass: az={azimuth}°, el={elevation}°, bank={bank}°")
