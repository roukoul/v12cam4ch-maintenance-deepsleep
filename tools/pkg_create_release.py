#!/usr/bin/env python3
import os
import shutil
import datetime
import zipfile

def create_archive(name, content_dict, version, output_dir):
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    archive_name = f"AepBill_{name}_{version}_{timestamp}.zip"
    archive_path = os.path.join(output_dir, archive_name)
    
    print(f"📦 Packaging '{name}' -> {archive_path}...")
    
    with zipfile.ZipFile(archive_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
        for arcname, source in content_dict.items():
            if os.path.exists(source):
                if os.path.isdir(source):
                    for root, dirs, files in os.walk(source):
                        for file in files:
                            file_path = os.path.join(root, file)
                            arcname_path = os.path.join(arcname, os.path.relpath(file_path, source))
                            zipf.write(file_path, arcname_path)
                else:
                    zipf.write(source, arcname)
            else:
                print(f"  ⚠️ Warning: Missing {source}")
    
    return archive_path

def main():
    import sys
    
    # Default version or from argument
    version = "v7.3.0"
    if len(sys.argv) > 1:
        version = sys.argv[1]

    project_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    build_dir = os.path.join(project_dir, 'build')
    brain_dir = os.path.join(os.path.dirname(os.path.dirname(project_dir)), 'brain', '987f1f3c-e675-4232-ab61-cd70384fc050')
    
    # Create releases directory
    releases_dir = os.path.join(project_dir, 'releases', version)
    if not os.path.exists(releases_dir):
        os.makedirs(releases_dir)
        print(f"📂 Created release directory: {releases_dir}")

    # 1. Package Evaluation (Technique)
    eval_content = {
        'docs/TECHNICAL_REPORT_2025.md': os.path.join(brain_dir, 'FINAL_REPORT_2025.md'),
        'docs/SECURITY_ROADMAP.md': os.path.join(brain_dir, 'ROADMAP_SECURITY_2026.md'),
        'docs/WPA3_Implementation.md': os.path.join(brain_dir, 'WPA3_IMPLEMENTATION_PLAN.md'),
        'src/main': os.path.join(project_dir, 'main'),
        'src/components': os.path.join(project_dir, 'components') if os.path.exists(os.path.join(project_dir, 'components')) else None,
        'src/sdkconfig': os.path.join(project_dir, 'sdkconfig'),
        'firmware/aep_bill.elf': os.path.join(build_dir, 'aep_bill_code_x.elf'),
        'firmware/aep_bill.map': os.path.join(build_dir, 'aep_bill_code_x.map'),
    }
    # Filter out None
    eval_content = {k: v for k, v in eval_content.items() if v}
    create_archive('EVALUATION_TECH', eval_content, version, releases_dir)

    # 2. Package Client (Commercial)
    client_content = {
        'MANUAL_UTILISATEUR.md': os.path.join(brain_dir, 'USER_MANUAL_v7.3.0.md'),
        'firmware/update_firmware.bin': os.path.join(build_dir, 'aep_bill_code_x.bin'),
        'firmware/bootloader.bin': os.path.join(build_dir, 'bootloader', 'bootloader.bin'),
        'firmware/partition-table.bin': os.path.join(build_dir, 'partition_table', 'partition-table.bin'),
        'README_INSTALL.txt': os.path.join(project_dir, 'README.md'),
    }
    create_archive('CLIENT_COMMERCIAL', client_content, version, releases_dir)

    # 3. Package Backup (Total)
    print("📦 Packaging 'FULL_BACKUP'...")
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_name = f"AepBill_FULL_BACKUP_{version}_{timestamp}.zip"
    backup_path = os.path.join(releases_dir, backup_name)
    
    with zipfile.ZipFile(backup_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
        # 1. Add Documentation
        docs_to_include = [
            ('USER_MANUAL_v7.3.0.md', 'docs/MANUAL_UTILISATEUR.md'),
            ('FINAL_REPORT_2025.md', 'docs/TECHNICAL_REPORT_2025.md'),
            ('ROADMAP_SECURITY_2026.md', 'docs/SECURITY_ROADMAP.md'),
            ('WPA3_IMPLEMENTATION_PLAN.md', 'docs/WPA3_Implementation.md'),
            ('FLASH_ENCRYPTION_PLAN.md', 'docs/FLASH_ENCRYPTION_PLAN.md'),
            ('flash_encryption_walkthrough.md', 'docs/FLASH_ENCRYPTION_WALKTHROUGH.md'),
            ('security_state_analysis.md', 'docs/SECURITY_STATE_ANALYSIS.md')
        ]
        for src_name, arc_name in docs_to_include:
            src_path = os.path.join(brain_dir, src_name)
            if os.path.exists(src_path):
                zipf.write(src_path, arc_name)

        # 2. Add Project Files
        for root, dirs, files in os.walk(project_dir):
            # Exclude build dir heavy content but keep binaries
            if 'build' in root and 'bootloader' not in root and 'partition_table' not in root:
                 pass 
            
            # Exclude .git and .gemini and releases folder itself to avoid recursion
            if '.git' in root or '.gemini' in root or 'releases' in root:
                continue

            for file in files:
                file_path = os.path.join(root, file)
                if 'build' in root:
                     if not file.endswith('.bin') and not file.endswith('.elf') and not file.endswith('.map') and not file.endswith('.csv'):
                         continue
                
                arcname = os.path.relpath(file_path, project_dir)
                zipf.write(file_path, arcname)
    
    print(f"✅ Full Backup Created: {backup_path}")

if __name__ == "__main__":
    main()
