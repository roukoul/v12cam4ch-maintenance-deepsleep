#!/usr/bin/env python3
import os
import sys
import zipfile
import glob
import shutil

def find_backup_zip(directory):
    # Find latest AepBill_FULL_BACKUP_*.zip
    files = glob.glob(os.path.join(directory, "AepBill_FULL_BACKUP_*.zip"))
    files.sort(key=os.path.getmtime, reverse=True)
    return files[0] if files else None

def repackage(backup_zip_path):
    print(f"📦 Repackaging from source: {os.path.basename(backup_zip_path)}")
    
    base_dir = os.path.dirname(os.path.abspath(backup_zip_path))
    extract_dir = os.path.join(base_dir, "temp_repack_extract")
    
    # 1. Extract everything
    if os.path.exists(extract_dir):
        shutil.rmtree(extract_dir)
    os.makedirs(extract_dir)
    
    with zipfile.ZipFile(backup_zip_path, 'r') as zip_ref:
        zip_ref.extractall(extract_dir)

    timestamp = "REPACK_" + os.path.basename(backup_zip_path).split('_')[-1].replace('.zip', '')
    
    # 2. Create Client Package
    client_zip = os.path.join(base_dir, f"AepBill_CLIENT_{timestamp}.zip")
    print(f"  -> Creating {os.path.basename(client_zip)}")
    with zipfile.ZipFile(client_zip, 'w', zipfile.ZIP_DEFLATED) as zf:
        # Manual
        zf.write(os.path.join(extract_dir, 'docs/MANUAL_UTILISATEUR.md'), 'MANUAL_UTILISATEUR.md')
        # Binaries (Firmware is in build/ or firmware/ depending on how pure backup was)
        # In our generator, we kept build root binaries
        bin_dir = os.path.join(extract_dir, 'build')
        zf.write(os.path.join(bin_dir, 'aep_bill_code_x.bin'), 'firmware/update_firmware.bin')
        zf.write(os.path.join(bin_dir, 'bootloader', 'bootloader.bin'), 'firmware/bootloader.bin')
        zf.write(os.path.join(bin_dir, 'partition_table', 'partition-table.bin'), 'firmware/partition-table.bin')
        
    
    # 3. Create Evaluation Package
    eval_zip = os.path.join(base_dir, f"AepBill_EVALUATION_{timestamp}.zip")
    print(f"  -> Creating {os.path.basename(eval_zip)}")
    with zipfile.ZipFile(eval_zip, 'w', zipfile.ZIP_DEFLATED) as zf:
        # Docs
        zf.write(os.path.join(extract_dir, 'docs/TECHNICAL_REPORT_2025.md'), 'docs/TECHNICAL_REPORT_2025.md')
        zf.write(os.path.join(extract_dir, 'docs/SECURITY_ROADMAP.md'), 'docs/SECURITY_ROADMAP.md')
        # Source (Add all 'main' and 'components')
        for root, dirs, files in os.walk(os.path.join(extract_dir, 'main')):
            for file in files:
                fpath = os.path.join(root, file)
                arcname = os.path.relpath(fpath, extract_dir) # src/main/...
                zf.write(fpath, arcname)
        # ELF/MAP
        zf.write(os.path.join(extract_dir, 'build/aep_bill_code_x.elf'), 'firmware/aep_bill.elf')
        zf.write(os.path.join(extract_dir, 'build/aep_bill_code_x.map'), 'firmware/aep_bill.map')

    # Cleanup
    shutil.rmtree(extract_dir)
    print("✅ Done.")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        target = sys.argv[1]
    else:
        # Try to find one in current dir
        target = find_backup_zip(os.getcwd())
    
    if target and os.path.exists(target):
        repackage(target)
    else:
        print("❌ No backup zip found or provided.")
