# 1. Charger l'environnement ESP-IDF
. $HOME\esp\v5.5.1\esp-idf\export.ps1

# 2. Se placer DANS le dossier du script (esp-cam-firmware)
Set-Location $PSScriptRoot

# 3. Configurer le projet
idf.py set-target esp32

# 3. Compiler
idf.py build

# 4. Flasher (Port COM6)
write-host "Flashing to COM6..."
idf.py -p COM6 flash monitor
