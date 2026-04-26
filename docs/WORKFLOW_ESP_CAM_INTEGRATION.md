# Workflow: ESP-CAM Integration & Watchdog Setup

This document outlines the step-by-step process to add a "Smart Security Camera" with "Watchdog Capability" to your AepBill system.

## Phase 1: ESP-CAM Firmware (The "Guardian")

We need to create a dedicated firmware for the ESP-CAM because it runs independently.

1.  **Create New Project**:
    *   Initialize a new ESP-IDF project (e.g., in a folder named `esp-cam-firmware`).
    *   **Configuration**:
        *   Enable PSRAM (Critical for Camera).
        *   Enable Camera Driver Support (OV2640).
        *   Enable SD Card in **1-Bit Mode** (Frees pins 4, 12, 13 for other uses).
2.  **Implement Features**:
    *   **Camera Server**: Standard MJPEG Streamer on port 80 or 81.
    *   **Motion Detection**: Compare standard resolution frames. If motion > threshold -> Save video to SD Card.
    *   **Watchdog Logic (The Heart)**:
        *   Listen for a "Heartbeat" pulse on **GPIO 12**.
        *   If no pulse is received for >10 Seconds (Primary ESP Frozen), drive **GPIO 33 (LED)** LOW for 100ms.
        *   *Why GPIO 33?* It connects to the Reset pin of the Primary ESP. When it goes LOW, the Primary ESP reboots!

## Phase 2: Primary ESP Updates (The "Monitored")

We need to tell the main AepBill system to "stay alive" and show the camera feed.

1.  **Heartbeat Transmitter**:
    *   In `main.c`, configure **GPIO 13** as an Output.
    *   Create a simple task that toggles this pin every 1 second.
    *   *Result*: As long as the Main ESP is running, it "beats" (pulses). If it freezes, the pulse stops.
2.  **Web Interface Integration**:
    *   Update the device's dashboard to include an `<img>` tag that points to the Camera's IP (e.g., `http://aepbill-cam.local/stream`).

## Phase 3: Wiring (The Physical Link)

**CRITICAL**: Ensure power is OFF before wiring!

| Primary ESP (Main Board) | ESP-CAM (Camera Board) | Function |
| :--- | :--- | :--- |
| **5V / VCC** | **5V** | Shared Power |
| **GND** | **GND** | Common Ground |
| **GPIO 13** | **GPIO 12** | Heartbeat Signal (Life Pulse) |
| **EN (Reset)** | **GPIO 33** | Reset Trigger (Watchdog) |

*   **Note**: GPIO 33 on the ESP-CAM is usually the internal Red LED. When the Watchdog triggers (resetting your system), the Red LED on the camera will flash!

## Phase 4: Verification (The Test)

1.  **Power Up**: Both boards should start.
2.  **Check Stream**: Open web browser to Camera IP -> See video.
3.  **Check Heartbeat**: Use a multimeter on GPIO 13/12 connectivity if unsure.
4.  **Simulate Failure (The Fun Part)**:
    *   Temporarily modify the Primary ESP code to STOP the heartbeat (comment out the toggle).
    *   Flash the Primary ESP.
    *   Wait ~10 seconds.
    *   **Observation**: The Primary ESP should suddenly REBOOT. This confirms the Watchdog is biting!

## Summary of Roles
*   **Primary ESP**: Runs the main logic (Relays, Alarms) and tells the Camera "I'm alive!" every second.
*   **ESP-CAM**: Records thieves (Motion Detect) and acts as a Doctor (Resets Primary ESP if it dies).
