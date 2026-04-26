#!/usr/bin/env python3
"""
Script to upload firmware to ESP32 via OTA (HTTP)
Usage: python ota_upload.py
"""

import requests
import sys
import os

# Configuration
ESP32_IP = "192.168.100.209"
OTA_URL = f"http://{ESP32_IP}/ota_update"
FIRMWARE_PATH = r"c:\esp32\AepBill_v10\build\aep_bill_code_x.bin"

# Default credentials (change if needed)
USERNAME = "aep"
PASSWORD = "aep2025"

def upload_firmware():
    """Upload firmware to ESP32 via OTA"""
    
    if not os.path.exists(FIRMWARE_PATH):
        print(f"❌ Error: Firmware file not found: {FIRMWARE_PATH}")
        return False
    
    file_size = os.path.getsize(FIRMWARE_PATH)
    print(f"📦 Firmware file: {FIRMWARE_PATH}")
    print(f"📊 File size: {file_size:,} bytes ({file_size/1024:.1f} KB)")
    print(f"🌐 Target: {ESP32_IP}")
    print(f"🔄 Uploading...")
    
    try:
        with open(FIRMWARE_PATH, 'rb') as f:
            files = {'file': ('firmware.bin', f, 'application/octet-stream')}
            
            # Try with authentication
            auth = (USERNAME, PASSWORD)
            
            response = requests.post(
                OTA_URL, 
                files=files,
                auth=auth,
                timeout=120  # 2 minutes timeout
            )
            
            if response.status_code == 200:
                print("✅ Upload successful!")
                print(f"📝 Response: {response.text}")
                print("\n⚠️  ESP32 is rebooting... Wait 10 seconds before reconnecting.")
                return True
            else:
                print(f"❌ Upload failed! Status code: {response.status_code}")
                print(f"📝 Response: {response.text}")
                return False
                
    except requests.exceptions.Timeout:
        print("⏱️  Timeout - This is NORMAL if ESP32 rebooted during upload")
        print("✅ Upload likely successful - Check ESP32 monitor")
        return True
    except requests.exceptions.ConnectionError:
        print("❌ Connection error - Is ESP32 online?")
        return False
    except Exception as e:
        print(f"❌ Error: {e}")
        return False

if __name__ == "__main__":
    print("=" * 60)
    print("  ESP32 AepBill OTA Firmware Updater")
    print("=" * 60)
    print()
    
    success = upload_firmware()
    
    print()
    print("=" * 60)
    sys.exit(0 if success else 1)
