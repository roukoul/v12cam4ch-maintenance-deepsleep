# Solution LCD I2C pour ESP32 - Documentation

## Problème rencontré
L'affichage LCD 16x2 via PCF8574 (I2C) affiche des caractères corrompus (charabia).

## Analyses effectuées

### 1. Validation matérielle ✅
- **LCD fonctionne avec Arduino** (LiquidCrystal_I2C library)  
- **Adresse I2C confirmée:** 0x27
- **Scan I2C ESP32:** Device détecté à 0x27 ✅
- **Conclusion:** Matériel OK, problème logiciel uniquement

### 2. Documentation ESP-IDF consultée
- **esp_rom_delay_us()** est approprié pour les délais HD44780 (microseconds)
- **Timing HD44780:** 37us par caractère, >1.52ms pour clear
- **I2C ESP-IDF:** Bibliothèque officielle esp-idf-lib disponible

## Solution recommandée: Utiliser esp-idf-lib

### Pourquoi esp-idf-lib ?
1. **Testé et validé** par la communauté ESP-IDF
2. **Maintenu activement** (UncleRus/esp-idf-lib)
3. **Support PCF8574** intégré  
4. **Compatible ESP-IDF 5.x**
5. **Exemples fonctionnels** fournis

### Composants nécessaires
- `hd44780` - Driver LCD HD44780
- `pcf8574` - Driver I/O expander PCF8574
- `i2cdev` - Helper I2C (esp-idf-lib)

### Installation

#### Méthode 1: IDF Component Manager (recommandée)
```yaml
# Dans idf_component.yml
dependencies:
  hd44780:
    version: "^1.0.0"
  pcf8574:
    version: "^1.0.0"
```

#### Méthode 2: Git submodule
```bash
cd components
git submodule add https://github.com/UncleRus/esp-idf-lib.git
```

### Intégration dans AepBill_v10

1. **Supprimer driver actuel** `lcd_i2c.c` (implementation défectueuse)

2. **Ajouter esp-idf-lib/components:**
   - `hd44780/`
   - `pcf8574/`
   - `i2cdev/`

3. **Créer wrapper `lcd_i2c_wrapper.c`:**

```c
#include "hd44780.h"
#include "pcf8574.h"

static hd44780_t lcd_dev;
static i2c_dev_t pcf8574_dev;

// Callback pour écriture via PCF8574
static esp_err_t lcd_write_cb(const hd44780_t *lcd, uint8_t data) {
    return pcf8574_port_write(&pcf8574_dev, data);
}

esp_err_t lcd_init(void) {
    // Init PCF8574
    pcf8574_init_desc(&pcf8574_dev, 0x27, I2C_NUM_0, 
                      GPIO_NUM_21, GPIO_NUM_22);
    
    // Init LCD via PCF8574
    lcd_dev.write_cb = lcd_write_cb;
    lcd_dev.font = HD44780_FONT_5X8;
    lcd_dev.lines = 2;
    lcd_dev.pins.rs = 0;  // P0
    lcd_dev.pins.e = 2;   // P2
    lcd_dev.pins.d4 = 4;  // P4
    lcd_dev.pins.d5 = 5;  // P5
    lcd_dev.pins.d6 = 6;  // P6
    lcd_dev.pins.d7 = 7;  // P7
    lcd_dev.pins.bl = 3;  // P3 (backlight)
    
    return hd44780_init(&lcd_dev);
}

void lcd_print(const char *str) {
    hd44780_puts(&lcd_dev, str);
}

void lcd_set_cursor(uint8_t row, uint8_t col) {
    hd44780_gotoxy(&lcd_dev, col, row);
}

void lcd_clear(void) {
    hd44780_clear(&lcd_dev);
}
```

4. **Avantages de cette approche:**
   - Code testé et validé
   - Pas de bugs de timing
   - Support PCF8574 natif
   - Documentation complète
   - Maintenance communautaire

### Références
- [esp-idf-lib GitHub](https://github.com/UncleRus/esp-idf-lib)
- [hd44780 component](https://github.com/UncleRus/esp-idf-lib/tree/master/components/hd44780)
- [pcf8574 component](https://github.com/UncleRus/esp-idf-lib/tree/master/components/pcf8574)
- [Example HD44780](https://github.com/UncleRus/esp-idf-lib/tree/master/examples/hd44780)

### Alternative: Copier implémentation Arduino EXACTE
Si esp-idf-lib ne peut pas être utilisé, copier l'implémentation exacte de:
- https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library

**Points critiques à copier:**
1. Séquence d'initialisation (delay 1000ms initial!)
2. `write_4bits()` + `pulse_enable()` 
3. `send(value, mode)` où mode = bit RS directement (pas 0/1)
4. Timing: EN high 1us, EN low 50us
