# Android App Architecture Plan for AepBill

## Feasibility Analysis
**YES, it is 100% possible** to create an Android application that replicates the functionality of the AepBill web interface point-by-point.

The existing ESP32 firmware is already well-suited for this because it provides JSON APIs:
*   **Status & Monitoring**: The `/status` endpoint returns JSON (`relay`, `current`, `anomaly_code`, `next_alarm`). The App can poll this easily.
*   **Schedule Management**: The `/downloadSchedule` and `/uploadSchedule` endpoints use JSON. This is perfect for an App to load the schedule, let you edit it with a native UI, and push it back.
*   **Commands**: Relay switching, Restart, etc., are simple HTTP calls.

## Proposed App Features (Point-by-Point)

### 1. Dashboard (Home Screen)
*   **Visuals**: Native Android UI (Material Design) matching the web gradients.
*   **Real-time Data**: Polls ESP32 every 2-3 seconds.
    *   Displays Server Time, Next Alarm, Relay State (Toggle Switch).
    *   Shows Current (Amps) and Anomaly Badges (Red alerts for Overload/Leak).
    *   **Advantage**: Smoother animation, no page reloads.

### 2. Schedule Manager (Advanced)
*   Instead of HTML tables, use **Native Android Pickers** (TimePicker, Dropdowns).
*   **Tabs** for Days (Monday-Sunday).
*   **Validation**: The App can validate overlapping times *before* sending to ESP32.
*   **Sync**:
    *   *Load*: Downloads `alarmes.json` from `/downloadSchedule`.
    *   *Save*: Uploads JSON to `/uploadSchedule`.

### 3. Settings & Configuration
*   **WiFi Config**: Native form to POST to `/wifiConfig`.
*   **Calibration**: Inputs for Current Sensor calibration.
*   **Timezone**: Dropdown selection.

### 4. Admin & Security
*   **Login Screen**: Native login screen that authenticates with `/login`.
*   **Auto-Discovery**: The App could potentially scan the WiFi network to find the AepBill IP address automatically (feature to add).
*   **Biometrics**: Could save the admin password securely and unlock with Fingerprint/FaceID.

## Technical Stack
*   **Language**: Kotlin (Modern Android standard) or Flutter (Cross-platform).
*   **Networking**: Retrofit (for HTTP API calls).
*   **Serialization**: GSON or Moshi (for parsing ESP32 JSON).

## Roadmap if you proceed
1.  **Define API Interface**: Map all ESP32 URLs to App functions.
2.  **UI Design**: Create layouts for Dashboard and Alarm Lists.
3.  **Implementation**: Code the Logic and Networking.
4.  **Testing**: Verify against the real ESP32.
