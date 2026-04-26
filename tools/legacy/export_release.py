#!/usr/bin/env python3
import os
import shutil
import datetime
import zipfile

def export_release():
    print("📦 Creating Release Package...")
    
    # Paths
    project_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    build_dir = os.path.join(project_dir, 'build')
    
    # Timestamped archive name
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    version = "v7.2.0-Production-Secure"
    archive_name = f"AepBill_{version}_{timestamp}.zip"
    archive_path = os.path.join(project_dir, archive_name)

    # Files to include
    files_to_pack = [
        # Release Notes (from artifacts, assumed copied to root or we point to it)
        # Binaries
        (os.path.join(build_dir, 'aep_bill_code_x.bin'), 'firmware/aep_bill_code_x.bin'),
        (os.path.join(build_dir, 'bootloader', 'bootloader.bin'), 'firmware/bootloader.bin'),
        (os.path.join(build_dir, 'partition_table', 'partition-table.bin'), 'firmware/partition-table.bin'),
        (os.path.join(build_dir, 'ota_data_initial.bin'), 'firmware/ota_data_initial.bin'),
        # Debug symbols
        (os.path.join(build_dir, 'aep_bill_code_x.elf'), 'debug/aep_bill_code_x.elf'),
        (os.path.join(build_dir, 'aep_bill_code_x.map'), 'debug/aep_bill_code_x.map'),
        # Config
        (os.path.join(project_dir, 'sdkconfig'), 'sdkconfig_reference'),
    ]

    # Create Zip
    with zipfile.ZipFile(archive_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
        for src, arcname in files_to_pack:
            if os.path.exists(src):
                print(f"  + Adding {os.path.basename(src)}")
                zipf.write(src, arcname)
            else:
                print(f"  ! Warning: Missing {src}")
        
        # Add a README to the zip
        zipf.writestr("README.txt", f"""AepBill Firmware Release {version}
Generated on: {datetime.datetime.now()}

This package contains the production firmware for AepBill.

Installation via esptool:
esptool.py -p COMx -b 460800 --before default_reset --after hard_reset --chip esp32  write_flash --flash_mode dio --flash_size detect --flash_freq 40m 0x1000 firmware/bootloader.bin 0x8000 firmware/partition-table.bin 0x10000 firmware/ota_data_initial.bin 0x20000 firmware/aep_bill_code_x.bin
""")

    print(f"\n✅ Archive Created: {archive_path}")

if __name__ == "__main__":
    export_release()
