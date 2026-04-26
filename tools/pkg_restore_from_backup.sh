#!/bin/bash
# Script to repackage AepBill releases from a Full Backup
# Usage: ./repackage_from_backup.sh [path_to_backup.zip]

if [ -z "$1" ]; then
    echo "Finding latest backup..."
    BACKUP_FILE=$(ls -t AepBill_FULL_BACKUP_*.zip | head -n 1)
else
    BACKUP_FILE="$1"
fi

if [ -z "$BACKUP_FILE" ]; then
    echo "Error: No backup file found or specified."
    exit 1
fi

echo "Using Backup: $BACKUP_FILE"

# Since we are on Windows/Git Bash or Linux, we can use the python script for robustness
# or use native zip/unzip if preferred. Using Python ensures cross-platform paths.

if command -v python3 &>/dev/null; then
    python3 tools/pkg_restore_from_backup.py "$BACKUP_FILE"
elif command -v python &>/dev/null; then
    python tools/pkg_restore_from_backup.py "$BACKUP_FILE"
else
    echo "Error: Python not found. Please install Python to use this tool."
    exit 1
fi
