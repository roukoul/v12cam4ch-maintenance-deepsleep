#ifndef LCD_I2C_DRIVER_H
#define LCD_I2C_DRIVER_H

#include "i2c_manager.h"

// LCD Commands
#define LCD_CMD_CLEAR_DISPLAY 0x01
#define LCD_CMD_RETURN_HOME 0x02
#define LCD_CMD_ENTRY_MODE 0x06
#define LCD_CMD_DISPLAY_ON 0x0C
#define LCD_CMD_FUNCTION 0x28 // 4-bit mode, 2 lines, 5x8 font

/**
 * @brief Initialize LCD Logic
 *
 * Does NOT install I2C driver.
 * Checks for device at 0x27.
 *
 * @return ESP_OK if found and init success
 */
esp_err_t lcd_init(void);

/**
 * @brief Clear display
 */
void lcd_clear(void);

/**
 * @brief Set cursor position
 * @param row 0 or 1
 * @param col 0 to 15
 */
void lcd_set_cursor(uint8_t row, uint8_t col);

/**
 * @brief Print string
 */
void lcd_print(const char *str);

/**
 * @brief Control backlight
 */
void lcd_backlight(bool on);

/**
 * @brief Update full display status (Dashboard)
 * @param status_line 16-char string for the second line
 */
void lcd_update_display(int day, int mon, int hour, int min, int sec,
                        const char *status_line, bool wifi_synced);

/**
 * @brief Display IP address for 10 seconds at startup
 * @param ip_str IP address string (e.g. "192.168.1.100")
 * @param is_ap true if Access Point mode, false if WiFi Station mode
 */
void lcd_show_ip_address(const char *ip_str, bool is_ap);

#endif // LCD_I2C_DRIVER_H
