import time
import requests
import os
import urllib3

# Disable insecure request warnings
urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

BASE_URL = "https://192.168.100.209"

def clear_screen():
    os.system('cls' if os.name == 'nt' else 'clear')

def get_status():
    try:
        response = requests.get(f"{BASE_URL}/status", verify=False, timeout=2)
        if response.status_code == 200:
            return response.json()
    except Exception as e:
        return None
    return None

def print_dashboard(data):
    clear_screen()
    print("=" * 50)
    print(f"   AEPBILL MONITORING SYSTEM v9.1   ")
    print("=" * 50)
    
    if not data:
        print("\n   ⚠️  CONNECTION LOST  ⚠️")
        print(f"   Connecting to {BASE_URL}...\n")
        print("=" * 50)
        return

    # OTA Mode Warning
    if data.get("ota_mode", False):
        print("\n" * 2)
        print("   !!!  SYSTEM UPDATING (OTA)  !!!")
        print("   PLEASE DO NOT POWER OFF")
        print("\n" * 2)
        return

    # Main Status
    print(f"\n   🕒 Time:       {data.get('time', '--:--')}")
    print(f"   🔌 Relay:      {'ON' if data.get('relay') == 1 else 'OFF'}")
    print(f"   ⏰ Next Alarm: {data.get('next_alarm', 'None')}")
    
    # Current
    current = data.get('current', 0.0)
    print("\n   ⚡ Current Load:")
    print(f"      {current:.3f} A")
    
    # Bar Chart ASCII
    bar_len = int(current * 10)  # 1A = 10 chars
    bar = "█" * bar_len
    print(f"      [{bar:<40}]")

    # Anomaly
    code = data.get('anomaly_code', 0)
    if code > 0:
        print(f"\n   ⚠️  ANOMALY DETECTED: Code {code}")
        
    print("\n" + "=" * 50)
    print("   Press Ctrl+C to Exit")

def main():
    print("Starting Terminal Dashboard...")
    while True:
        data = get_status()
        print_dashboard(data)
        time.sleep(1)

if __name__ == "__main__":
    main()
