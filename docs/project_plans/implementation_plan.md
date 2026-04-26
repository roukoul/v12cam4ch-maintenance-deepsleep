# UI & Content Update Plan

## Goal Description
Update the web interface aesthetics and content as requested by the user.
*   **Safety Priority**: ABSOLUTE. No changes to OTA logic (`update_post_handler`) or core system logic.
*   **Aesthetics**: Modernize font (Cairo/Tajawal), Green Clock, clean up About page.
*   **Content**: Update Author to "Eddadssi Ahmed", Version "v9.1", Year "2026".

## User Review Required
> [!IMPORTANT]
> **Safety Check**: This plan ONLY modifies static HTML strings and CSS within `http_server.c`.
> **No logic changes** will be made to `update_post_handler` or `wifi_config_post_handler`.

## Proposed Changes

### [Web Server]
#### [MODIFY] [http_server.c](file:///c:/esp32/AepBill_v9_1/main/webserver/http_server.c)

1.  **CSS Update (`HTML_STYLE`)**:
    *   **Font**: Import 'Cairo' (Google Fonts) for a modern, pretty look in Arabic/French.
    *   **Clock Color**: Add a specific rule for the time display (or the card containing it) to be green.
    *   **Reason**: "c'est visible" (it's visible) and "joli" (pretty).

2.  **About Page (`about_handler`)**:
    *   **Update Info**:
        *   Author: `Eddadssi Ahmed` -> `Prof. Eddadssi Ahmed (Physique-Chimie)`
        *   Location: `Lycée Qualifiant Tinzert, Taroudant, Maroc`
        *   Version: `v8.3` -> `v9.1`
        *   Copyright: `2026 - Droits réservés à Eddadssi Ahmed`
    *   **Remove Cards** (as per images):
        *   ❌ MAC Address Card
        *   ❌ Free Heap (الذاكرة الحرة) Card

3.  **Dashboard (`root_get_handler`)**:
    *   Target the "Time" card to apply the green style defined in CSS.

## Verification Plan

### Automated Tests
*   **Compile Test**: Ensure no syntax errors (missing semicolons in C strings).
*   **Security Script**: Re-run `tools/security_test.py` to confirm `/update` is still protected and accessible (logic untouched).

### Manual Verification (User)
1.  **Deploy OTA**: Flash the new firmware via the *existing* OTA.
2.  **Check About**: Confirm MAC/Heap cards are gone, text is correct.
3.  **Check Dashboard**: Confirm Clock is Green and Font is new.
4.  **Check Security**: Confirm `/update` page still loads and asks for Login.
