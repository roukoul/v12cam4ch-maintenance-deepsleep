/**
 * @file lcd_diagnostic.md
 * @brief LCD I2C Troubleshooting Guide
 * 
 * PROBLEME: Caractères corrompus sur LCD 16x2 I2C
 * 
 * CAUSES POSSIBLES:
 * 
 * 1. ADRESSE I2C INCORRECTE
 *    - PCF8574: 0x27 (défaut actuel)
 *    - PCF8574A: 0x3F
 *    - Test: Scanner I2C affiche "Device Found at Address: 0x27"
 *    - Si vous voyez 0x3F au lieu de 0x27, changez I2C_ADDR_LCD dans i2c_manager.h
 * 
 * 2. MAPPING DES PINS INCORRECT
 *    - Configuration actuelle:
 *      P7 P6 P5 P4 | P3 P2 P1 P0
 *      D7 D6 D5 D4 | BL EN RW RS
 *    
 *    - Configuration alternative (certains modules):
 *      P7 P6 P5 P4 | P3 P2 P1 P0
 *      D7 D6 D5 D4 | RS RW EN BL
 *    
 *    - Si le mapping est différent, les bits EN, RS, RW doivent être modifiés
 * 
 * 3. BACKLIGHT INTERFÉRENCE
 *    - Bit actuellement utilisé: P3 (0x08)
 *    - Certains modules: P7 ou pas de backlight
 *    - Solution: Désactiver backlight (0x00) pour tester
 * 
 * 4. TIMING TROP RAPIDE/LENT
 *    - EN pulse: Actuellement 2us HIGH, 500us LOW
 *    - Certains LCD nécessitent plus de temps
 * 
 * 5. ROM CODE JAPONAISE
 *    - LCD HD44780 avec ROM Code A02 (japonais) au lieu de A00 (européen)
 *    - Même les caractères ASCII de base sont corrompus
 *    - PAS DE SOLUTION LOGICIELLE - Il faut changer de LCD
 * 
 * DIAGNOSTIC STEP-BY-STEP:
 * 
 * √ Étape 1: Vérifier l'adresse I2C
 *   - Regarder les logs: "LCD found at 0x27"
 *   - Scanner I2C: "Device Found at Address: 0x27"
 *   - Si différent, modifier I2C_ADDR_LCD
 * 
 * √ Étape 2: Tester sans backlight
 *   - Modifier: static uint8_t s_backlight_val = 0x00;
 *   - Compiler et flasher
 *   - Si écran noir mais caractères corrects: c'est le backlight
 * 
 * □ Étape 3: Tester mapping alternatif
 *   - Essayer RS=0x01, EN=0x04, RW=0x02 (actuel)
 *   - Essayer RS=0x02, EN=0x04, RW=0x01 (swappé)
 *   - Essayer aussi EN sur P3: EN=0x08
 * 
 * □ Étape 4: Augmenter timing
 *   - EN pulse HIGH: 10us au lieu de 2us
 *   - EN pulse LOW: 1000us (1ms) au lieu de 500us
 * 
 * □ Étape 5: Tester avec Arduino
 *   - Utiliser bibliothèque LiquidCrystal_I2C
 *   - Si ça marche sur Arduino, copier les paramètres
 * 
 * CODE ARDUINO DE RÉFÉRENCE:
 * ```cpp
 * #include <Wire.h>
 * #include <LiquidCrystal_I2C.h>
 * 
 * // Test différentes adresses
 * LiquidCrystal_I2C lcd(0x27, 16, 2); // ou 0x3F
 * 
 * void setup() {
 *   lcd.init();
 *   lcd.backlight();
 *   lcd.print("Test 0123456789");
 * }
 * ```
 * 
 * SOLUTIONS RAPIDES À TESTER:
 * 
 * A. Changer l'adresse à 0x3F:
 *    Dans i2c_manager.h: #define I2C_ADDR_LCD 0x3F
 * 
 * B. Inverser RS et RW:
 *    Modifier lcd_send_nibble() avec mode inversé
 * 
 * C. Module incompatible:
 *    Acheter un nouveau module LCD I2C avec PCF8574 standard
 */
