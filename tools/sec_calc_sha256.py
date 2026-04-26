#!/usr/bin/env python3
"""
SHA256 Generator for ESP32 Firmware
Generates SHA256 checksum for OTA validation
"""

import hashlib
import sys
import os

def generate_sha256(firmware_path):
    """Generate SHA256 hash of firmware binary"""
    sha256_hash = hashlib.sha256()
    
    print(f"Calculating SHA256 for: {firmware_path}")
    
    file_size = os.path.getsize(firmware_path)
    print(f"Firmware size: {file_size} bytes ({file_size/1024:.2f} KB)")
    
    with open(firmware_path, 'rb') as f:
        # Read in chunks for large files
        bytes_read = 0
        for byte_block in iter(lambda: f.read(4096), b""):
            sha256_hash.update(byte_block)
            bytes_read += len(byte_block)
    
    return sha256_hash.hexdigest()

def main():
    if len(sys.argv) != 2:
        print("Usage: sec_calc_sha256.py <firmware.bin>")
        print("Example: sec_calc_sha256.py build/aep_bill_code_x.bin")
        sys.exit(1)
    
    firmware = sys.argv[1]
    
    if not os.path.exists(firmware):
        print(f"Error: Firmware file not found: {firmware}")
        sys.exit(1)
    
    # Generate checksum
    checksum = generate_sha256(firmware)
    
    # Write to .sha256 file
    sha_file = firmware + ".sha256"
    with open(sha_file, 'w') as f:
        f.write(checksum + "\n")
    
    print(f"\n{'='*70}")
    print(f"SHA256: {checksum}")
    print(f"{'='*70}")
    print(f"Checksum saved to: {sha_file}")
    print(f"\nCopy this hash when uploading firmware via OTA!")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
