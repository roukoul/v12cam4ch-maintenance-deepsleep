# Plan Professionnel: Calibrage Automatique ADC/ACS712
**AepBill v10 - Système de Mesure de Courant Haute Précision**

Date: 17 Décembre 2025
Auteur: Équipe Technique AepBill

---

## 📋 ANALYSE DU SYSTÈME ACTUEL

### Hardware
- **Capteur**: ACS712 (Hall Effect Current Sensor)
- **Interface**: ESP32 ADC1 (12-bit, 0-3.3V)
- **Atténuation**: 12dB (0-3.3V range)
- **Sensibilité ACS712**: 
  - 185 mV/A (±5A)
  - 100 mV/A (±20A)
  - 66 mV/A (±30A)

### Logiciel Actuel (adc_driver.c)
✅ **Points forts identifiés:**
1. Calibrage zéro manuel fonctionnel (`adc_calibrate_zero()`)
2. Sauvegarde NVS des paramètres de calibration
3. Mesure RMS sur 200ms (10 cycles @ 50Hz)
4. Filtre EMA (α=0.3) pour réduction du bruit
5. Seuils d'anomalie configurables
6. Validation des valeurs chargées depuis NVS

❌ **Limitations actuelles:**
1. **Pas de calibration automatique au démarrage**
2. **Pas d'utilisation de l'eFuse Vref** (ESP32)
3. **Pas de compensation de température**
4. **Pas de validation périodique** du calibrage
5. **Calibration scale_factor** pas exploitée

---

## 🎯 OBJECTIFS DU PROJET

### Exigences Fonctionnelles
1. **Calibrage automatique au démarrage** de l'ESP32
2. **Maintien du calibrage manuel** via bouton (inchangé)
3. **Précision améliorée** grâce à:
   - Utilisation de l'eFuse Vref ESP32
   - Calibration multi-points
   - Compensation de drift
4. **Robustesse industrielle**:
   - Validation systématique
   - Fallback en cas d'échec
   - Logs détaillés

### Exigences Non-Fonctionnelles
- Temps de démarrage < 2 secondes (calibrage compris)
- Précision absolue: ±2% (±1% cible)
- Stabilité thermique: ±0.5% sur -10°C à +60°C
- Durabilité calibration: 10,000 cycles

---

## 📊 ÉTUDE DES DATASHEETS

### ESP32 ADC (Espressif Datasheet)

#### Caractéristiques ADC
```
Résolution: 12-bit (0-4095)
Vref interne: 1100 mV (nominal)
Vref range: 1000-1200 mV (variation inter-chip)
Atténuation 12dB: 0-3300 mV
Non-linéarité: ±2 LSB (typ), ±6 LSB (max)
```

#### eFuse Vref (BLOCK0)
- **Chips concernés**: ESP32-D0WD/D0WDQ6 (≥ semaine 1/2018)
- **Précision eFuse**: ±10 mV typical
- **Utilisation recommandée**: `esp_adc_cal_characterize()`
- **Avantages**:
  - Vref calibré en usine pour chaque chip
  - Amélioration précision de ~5-10%
  - Pas de coût en runtime

#### Calibration API (esp_adc_cal)
```c
esp_adc_cal_characteristics_t characteristics;
esp_adc_cal_value_t val_type = esp_adc_cal_characterize(
    ADC_UNIT_1,
    ADC_ATTEN_DB_12,
    ADC_WIDTH_BIT_12,
    DEFAULT_VREF,  // 1100 mV si pas d'eFuse
    &characteristics
);

// val_type retourne:
// - ESP_ADC_CAL_VAL_EFUSE_VREF: eFuse présent
// - ESP_ADC_CAL_VAL_EFUSE_TP: Two-Point calibration
// - ESP_ADC_CAL_VAL_DEFAULT_VREF: Utilise DEFAULT_VREF
```

#### Recommandations Espressif
1. **Multisampling**: 64-256 samples pour réduire le bruit
2. **Bypass capacitor**: 100nF sur entrée ADC
3. **Éviter ADC2** si WiFi actif (utiliser ADC1)
4. **Calibration**: Toujours utiliser `esp_adc_cal`

### ACS712 (Allegro Datasheet)

#### Spécifications Électriques
```
Supply Voltage: 5V ±10%
Output voltage @ 0A: Vcc/2 (2.5V nominal)
Output noise: 21 mV RMS (typ)
Total output error @ 25°C: 1.5% max
Temperature drift: ±1.5% typical (-40°C to +85°C)
Bandwidth: 80 kHz (typ)
Response time: 5 µs (typ)
Internal resistance: 1.2 mΩ
Isolation: 2.1 kVRMS
```

#### Sensitivités
| Modèle | Range | Sensibilité | Output @ 0A |
|--------|-------|-------------|-------------|
| ACS712-05B | ±5A | 185 mV/A | 2.5V ±0.125V |
| ACS712-20A | ±20A | 100 mV/A | 2.5V ±0.100V |
| ACS712-30A | ±30A | 66 mV/A | 2.5V ±0.075V |

#### Sources d'Erreur
1. **Offset voltage @ 0A**: ±100-125 mV (variation inter-chip)
2. **Temperature drift**: ~1.5% sur plage de température
3. **Supply voltage dependency**: Ratiométrique (Vout ∝ Vcc)
4. **Magnetic interference**: Sensible aux champs magnétiques externes

#### Technique de Compensation (Datasheet)
- **Chopper-stabilized BiCMOS Hall IC**: Réduit le drift d'offset
- **Internal temperature compensation**: Circuit intégré
- **Filter pin**: Capacitor externe pour réduire le bruit

---

## 🔬 ANALYSE DES SOURCES D'ERREUR

### 1. Erreurs ADC ESP32
| Source | Magnitude | Solution |
|--------|-----------|----------|
| Vref variation | ±9% (1000-1200mV) | eFuse Vref |
| Non-linéarité | ±2 LSB (0.05%) | Calibration curve |
| Quantization noise | ±0.5 LSB | Multisampling |
| Offset error | ±10 mV | Zero calibration |

### 2. Erreurs ACS712
| Source | Magnitude | Solution |
|--------|-----------|----------|
| Zero offset | ±125 mV | Zero calibration |
| Scale error | ±1.5% | Multi-point calibration |
| Temp. drift | ±1.5% | Periodic recalibration |
| Supply noise | Variable | Bypass cap + filtering |

### 3. Erreurs Système
| Source | Magnitude | Solution |
|--------|-----------|----------|
| Vcc instability | ±5% | Regulated PSU |
| EMI/RFI | Variable | Shielding + filtering |
| Aging | <0.1%/year | Annual recalibration |
| Connection resistance | <0.1% | Proper wiring |

---

## 🎯 SOLUTION PROFESSIONNELLE PROPOSÉE

### Architecture Globale

```
┌─────────────────────────────────────────────────────────┐
│                   ESP32 STARTUP                         │
│  ┌─────────────────────────────────────────────────┐   │
│  │  1. NVS Init                                    │   │
│  │  2. ADC Init (avec eFuse Vref)                  │   │
│  │  3. Chargement calibration sauvegardée          │   │
│  │  4. VALIDATION calibration                      │   │
│  │  │  ├─ Valid? → Utiliser                        │   │
│  │  │  └─ Invalid? → AUTO-CALIBRATION              │   │
│  │  5. Mode normal opération                       │   │
│  └─────────────────────────────────────────────────┘   │
│                                                          │
│  ┌─────────────────────────────────────────────────┐   │
│  │         AUTO-CALIBRATION ROUTINE                │   │
│  │  ┌──────────────────────────────────────────┐  │   │
│  │  │ PHASE 1: Zero Calibration                │  │   │
│  │  │  - 1000 samples @ 1ms intervals          │  │   │
│  │  │  - Stats: mean, stddev, min, max         │  │   │
│  │  │  - Validation: stddev < threshold        │  │   │
│  │  │  - Retry: max 3 attempts                 │  │   │
│  │  └──────────────────────────────────────────┘  │   │
│  │  ┌──────────────────────────────────────────┐  │   │
│  │  │ PHASE 2: eFuse Vref Integration          │  │   │
│  │  │  - Query esp_adc_cal_characterize()      │  │   │
│  │  │  - Adjust ADC_VOLTAGE_REF if eFuse       │  │   │
│  │  │  - Log calibration type used             │  │   │
│  │  └──────────────────────────────────────────┘  │   │
│  │  ┌──────────────────────────────────────────┐  │   │
│  │  │ PHASE 3: Sanity Checks                   │  │   │
│  │  │  - Zero offset: 1900-2200 ADC counts     │  │   │
│  │  │  - Noise level: < 50 ADC counts RMS      │  │   │
│  │  │  - Stability test: 10 consecutive reads  │  │   │
│  │  └──────────────────────────────────────────┘  │   │
│  │  ┌──────────────────────────────────────────┐  │   │
│  │  │ PHASE 4: Sauvegarde NVS                  │  │   │
│  │  │  - Timestamp calibration                 │  │   │
│  │  │  - Mark as valid                         │  │   │
│  │  │  - Save to NVS                           │  │   │
│  │  └──────────────────────────────────────────┘  │   │
│  └─────────────────────────────────────────────────┘   │
│                                                          │
│  ┌─────────────────────────────────────────────────┐   │
│  │      CALIBRATION MANUELLE (BOUTON)              │   │
│  │  - Déclenchée par utilisateur                   │   │
│  │  - Identique à auto-calibration                 │   │
│  │  - Feedback LCD/LED                             │   │
│  │  - INTACT (code existant préservé)              │   │
│  └─────────────────────────────────────────────────┘   │
│                                                          │
│  ┌─────────────────────────────────────────────────┐   │
│  │    RECALIBRATION PÉRIODIQUE (Runtime)           │   │
│  │  - Toutes les 24h: validation drift             │   │
│  │  - Si drift > seuil: log warning                │   │
│  │  - Si drift > critique: auto-recalibration      │   │
│  └─────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

### Plan d'Implémentation

#### Phase 1: Amélioration ADC avec eFuse Vref
**Fichiers modifiés:** `adc_driver.c`, `adc_driver.h`

**Nouvelles fonctionnalités:**
```c
// 1. Structure enrichie
typedef struct {
    float zero_offset;           // Existant
    float scale_factor;          // Existant
    uint8_t is_valid;           // Existant
    uint32_t calibration_timestamp; // NOUVEAU
    esp_adc_cal_value_t vref_type;  // NOUVEAU (eFuse, TP, Default)
    uint32_t actual_vref_mv;     // NOUVEAU (Vref réel utilisé)
    float noise_level_rms;       // NOUVEAU (niveau de bruit mesuré)
} adc_calibration_t;

// 2. Calibration avancée avec eFuse
esp_err_t adc_auto_calibrate_startup(void);

// 3. Validation calibration
bool adc_validate_calibration(const adc_calibration_t *cal);

// 4. Stats calibration
esp_err_t adc_get_calibration_stats(adc_calib_stats_t *stats);
```

#### Phase 2: Auto-Calibration au Démarrage
**Intégration dans `main.c`:**

```c
void app_main(void) {
    // ... init NVS, WiFi, etc ...
    
    // ADC Init avec auto-calibration
    adc_driver_init();  // Appelle maintenant auto-calibration
    
    // ... suite normal setup ...
}
```

**Logique dans `adc_driver_init()`:**
```c
void adc_driver_init(void) {
    // 1. Init hardware ADC
    // 2. Tenter chargement NVS
    // 3. SI calibration invalide OU trop ancienne:
    //       → adc_auto_calibrate_startup()
    // 4. SINON: utiliser calibration sauvegardée
    // 5. Log résultats détaillés
}
```

#### Phase 3: Validation et Fallback
**Critères de validation:**
```c
bool adc_validate_calibration(const adc_calibration_t *cal) {
    if (!cal || cal->is_valid != 1) return false;
    
    // Check 1: Zero offset dans plage raisonnable
    if (cal->zero_offset < 1900.0f || cal->zero_offset > 2200.0f)
        return false;
    
    // Check 2: Scale factor raisonnable
    if (cal->scale_factor < 0.8f || cal->scale_factor > 1.2f)
        return false;
    
    // Check 3: Pas trop ancien (30 jours max)
    uint32_t age_sec = (esp_timer_get_time() / 1000000) - cal->calibration_timestamp;
    if (age_sec > (30 * 24 * 3600))
        return false;
    
    // Check 4: Bruit acceptable
    if (cal->noise_level_rms > 50.0f)
        return false;
    
    return true;
}
```

#### Phase 4: Recalibration Périodique
**Task FreeRTOS dédiée:**
```c
void adc_monitoring_task(void *pvParameters) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(3600000)); // Toutes les heures
        
        // Vérifier drift du zero
        float current_zero = adc_measure_quick_zero();
        float drift = fabs(current_zero - calibration.zero_offset);
        
        if (drift > DRIFT_WARNING_THRESHOLD) {
            ESP_LOGW(TAG, "Zero drift detected: %.1f ADC counts", drift);
            
            if (drift > DRIFT_CRITICAL_THRESHOLD) {
                ESP_LOGE(TAG, "Critical drift! Auto-recalibrating...");
                adc_auto_calibrate_startup();
            }
        }
    }
}
```

---

## 📝 SPÉCIFICATIONS TECHNIQUES DÉTAILLÉES

### Algorithme de Zero Calibration Amélioré

```c
float adc_calibrate_zero_advanced(adc_calib_stats_t *stats) {
    const int SAMPLES = 1000;
    const int WARMUP_SAMPLES = 100;
    
    ESP_LOGI(TAG, "Advanced zero calibration starting...");
    
    // Warmup: jeter les N premiers échantillons
    for (int i = 0; i < WARMUP_SAMPLES; i++) {
        int dummy;
        adc_oneshot_read(adc_handle, CURRENT_SENSOR_CHANNEL, &dummy);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    // Collecte statistiques
    double sum = 0;
    double sum_sq = 0;
    int min_val = 4095;
    int max_val = 0;
    int raw_samples[SAMPLES];
    
    for (int i = 0; i < SAMPLES; i++) {
        int raw;
        adc_oneshot_read(adc_handle, CURRENT_SENSOR_CHANNEL, &raw);
        
        raw_samples[i] = raw;
        sum += raw;
        sum_sq += (raw * raw);
        
        if (raw < min_val) min_val = raw;
        if (raw > max_val) max_val = raw;
        
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    // Calculs statistiques
    double mean = sum / SAMPLES;
    double variance = (sum_sq / SAMPLES) - (mean * mean);
    double stddev = sqrt(variance);
    
    // Filtrage outliers (méthode 3-sigma)
    double filtered_sum = 0;
    int filtered_count = 0;
    double threshold = 3.0 * stddev;
    
    for (int i = 0; i < SAMPLES; i++) {
        if (fabs(raw_samples[i] - mean) < threshold) {
            filtered_sum += raw_samples[i];
            filtered_count++;
        }
    }
    
    float final_zero = (filtered_count > 0) ? 
        (filtered_sum / filtered_count) : mean;
    
    // Remplir stats si demandé
    if (stats) {
        stats->mean = mean;
        stats->stddev = stddev;
        stats->min = min_val;
        stats->max = max_val;
        stats->samples_count = SAMPLES;
        stats->outliers_rejected = SAMPLES - filtered_count;
    }
    
    ESP_LOGI(TAG, "Zero calib: mean=%.1f std=%.1f min=%d max=%d final=%.1f",
             mean, stddev, min_val, max_val, final_zero);
    
    // Validation
    if (stddev > 50.0) {
        ESP_LOGW(TAG, "High noise level! stddev=%.1f", stddev);
        return -1.0f; // Échec
    }
    
    return final_zero;
}
```

### Intégration eFuse Vref

```c
esp_err_t adc_init_with_efuse(void) {
    // Init oneshot ADC
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));
    
    // Configure channel
    adc_oneshot_chan_cfg_t chan_config = {
        .bit_width = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(
        adc_handle, CURRENT_SENSOR_CHANNEL, &chan_config));
    
    // Characterize avec eFuse
    esp_adc_cal_characteristics_t *characteristics = 
        malloc(sizeof(esp_adc_cal_characteristics_t));
    
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(
        ADC_UNIT_1,
        ADC_ATTEN_DB_12,
        ADC_WIDTH_BIT_12,
        DEFAULT_VREF,  // 1100 mV par défaut
        characteristics
    );
    
    // Sauvegarder le type et Vref réel
    calibration.vref_type = val_type;
    calibration.actual_vref_mv = characteristics->vref;
    
    // Log résultat
    switch (val_type) {
        case ESP_ADC_CAL_VAL_EFUSE_VREF:
            ESP_LOGI(TAG, "eFuse Vref available: %d mV", 
                     characteristics->vref);
            break;
        case ESP_ADC_CAL_VAL_EFUSE_TP:
            ESP_LOGI(TAG, "Two-Point calibration available: %d mV", 
                     characteristics->vref);
            break;
        default:
            ESP_LOGW(TAG, "Using default Vref: %d mV", 
                     characteristics->vref);
            break;
    }
    
    // Utiliser pour conversion
    // (à intégrer dans adc_sample_raw_rms)
    global_characteristics = characteristics;
    
    return ESP_OK;
}
```

### Conversion avec Calibration eFuse

```c
static float adc_sample_raw_rms_calibrated(void) {
    const int measurement_duration_ms = 200;
    const int sample_delay_us = 40;
    
    double sum_squares = 0;
    int count = 0;
    
    int64_t start_time = esp_timer_get_time();
    int64_t end_time = start_time + (measurement_duration_ms * 1000);
    
    while (esp_timer_get_time() < end_time) {
        int raw;
        esp_err_t ret = adc_oneshot_read(adc_handle, CURRENT_SENSOR_CHANNEL, &raw);
        
        if (ret != ESP_OK) {
            count++;
            esp_rom_delay_us(sample_delay_us);
            continue;
        }
        
        // NOUVEAU: Conversion calibrée
        uint32_t voltage_mv;
        esp_adc_cal_raw_to_voltage(raw, global_characteristics, &voltage_mv);
        
        // Calculer diff depuis zero (aussi en mV)
        uint32_t zero_mv;
        esp_adc_cal_raw_to_voltage((int)virtual_zero, global_characteristics, &zero_mv);
        
        float diff_mv = (float)voltage_mv - (float)zero_mv;
        
        // Noise gate
        if (fabs(diff_mv) < NOISE_GATE_MV) {
            diff_mv = 0;
        }
        
        sum_squares += (double)(diff_mv * diff_mv);
        count++;
        
        esp_rom_delay_us(sample_delay_us);
    }
    
    if (count == 0) return 0.0f;
    
    // RMS en mV
    double rms_mv = sqrt(sum_squares / (double)count);
    
    // Conversion en Ampères (ACS712 sensitivity)
    float amps_rms = (float)(rms_mv / ADC_SENSITIVITY);
    
    // Appliquer scale factor
    if (calibration.is_valid == 1) {
        amps_rms *= calibration.scale_factor;
    }
    
    return amps_rms;
}
```

---

## 🔍 TESTS ET VALIDATION

### Tests Unitaires Requis

```c
// Test 1: Zero calibration repeatability
void test_zero_calib_repeatability(void) {
    float zeros[10];
    for (int i = 0; i < 10; i++) {
        zeros[i] = adc_calibrate_zero_advanced(NULL);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Calculer écart-type
    float mean = 0;
    for (int i = 0; i < 10; i++) mean += zeros[i];
    mean /= 10;
    
    float variance = 0;
    for (int i = 0; i < 10; i++) {
        float diff = zeros[i] - mean;
        variance += diff * diff;
    }
    float stddev = sqrt(variance / 10);
    
    ESP_LOGI(TAG, "Repeatability test: mean=%.1f stddev=%.1f", mean, stddev);
    assert(stddev < 10.0f); // < 10 ADC counts acceptable
}

// Test 2: Validation load/save NVS
void test_nvs_persistence(void) {
    adc_calibration_t original = calibration;
    
    // Save
    esp_err_t err = adc_save_calibration();
    assert(err == ESP_OK);
    
    // Modify
    calibration.zero_offset = 9999.0f;
    
    // Reload
    err = adc_load_calibration();
    assert(err == ESP_OK);
    assert(fabs(calibration.zero_offset - original.zero_offset) < 0.1f);
    
    ESP_LOGI(TAG, "NVS persistence test: PASS");
}

// Test 3: eFuse Vref detection
void test_efuse_vref(void) {
    esp_adc_cal_characteristics_t chars;
    esp_adc_cal_value_t type = esp_adc_cal_characterize(
        ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, &chars);
    
    ESP_LOGI(TAG, "eFuse test: type=%d vref=%d", type, chars.vref);
    assert(chars.vref >= 1000 && chars.vref <= 1200);
}
```

### Tests d'Intégration

```c
// Test complet startup avec auto-calibration
void test_full_startup_sequence(void) {
    ESP_LOGI(TAG, "=== Full startup test ===");
    
    // 1. Effacer NVS pour simuler premier boot
    nvs_flash_erase();
    nvs_flash_init();
    
    // 2. Init ADC (devrait auto-calibrer)
    int64_t start = esp_timer_get_time();
    adc_driver_init();
    int64_t elapsed = esp_timer_get_time() - start;
    
    ESP_LOGI(TAG, "Startup time: %lld ms", elapsed / 1000);
    assert(elapsed < 2000000); // < 2 secondes
    
    // 3. Vérifier que calibration est valide
    const adc_calibration_t *cal = adc_get_calibration();
    assert(cal->is_valid == 1);
    assert(cal->zero_offset >= 1900.0f && cal->zero_offset <= 2200.0f);
    
    // 4. Faire une mesure
    float current = adc_read_current_amps();
    ESP_LOGI(TAG, "Current reading: %.3f A", current);
    
    ESP_LOGI(TAG, "=== Startup test PASS ===");
}
```

---

## 📈 MÉTRIQUES DE PERFORMANCE

### Critères de Succès

| Métrique | Avant | Après (Cible) |
|----------|-------|---------------|
| Précision absolue | ±5% | ±1-2% |
| Répétabilité | ±3% | ±0.5% |
| Drift thermique | ±2%/°C | ±0.5%/°C |
| Temps calibration | ~500ms | ~1000ms |
| Temps startup | N/A | <2s |
| Durée validité calib | Indéfinie | 30 jours |

### Logging et Monitoring

```c
// Log complet calibration
void adc_log_calibration_details(void) {
    const adc_calibration_t *cal = adc_get_calibration();
    
    ESP_LOGI(TAG, "=== ADC Calibration Report ===");
    ESP_LOGI(TAG, "Zero offset: %.1f ADC counts", cal->zero_offset);
    ESP_LOGI(TAG, "Scale factor: %.3f", cal->scale_factor);
    ESP_LOGI(TAG, "Valid: %s", cal->is_valid ? "YES" : "NO");
    ESP_LOGI(TAG, "Vref type: %d (0=efuse, 1=tp, 2=default)", cal->vref_type);
    ESP_LOGI(TAG, "Actual Vref: %u mV", cal->actual_vref_mv);
    ESP_LOGI(TAG, "Noise RMS: %.1f ADC counts", cal->noise_level_rms);
    ESP_LOGI(TAG, "Timestamp: %u s", cal->calibration_timestamp);
    
    uint32_t age = (esp_timer_get_time() / 1000000) - cal->calibration_timestamp;
    ESP_LOGI(TAG, "Age: %u hours", age / 3600);
    ESP_LOGI(TAG, "=============================");
}
```

---

## 🚀 PLAN DE DÉPLOIEMENT

### Phase 1: Développement (Semaine 1)
- [ ] Implémenter eFuse Vref dans `adc_driver.c`
- [ ] Créer `adc_auto_calibrate_startup()`
- [ ] Enrichir structure `adc_calibration_t`
- [ ] Tests unitaires

### Phase 2: Intégration (Semaine 2)
- [ ] Intégrer dans `main.c` au startup
- [ ] Ajouter task de monitoring périodique
- [ ] Tests d'intégration
- [ ] Documentation code

### Phase 3: Validation (Semaine 3)
- [ ] Tests en conditions réelles
- [ ] Mesure précision vs. référence
- [ ] Test dérive thermique
- [ ] Ajustements finaux

### Phase 4: Production (Semaine 4)
- [ ] Code review
- [ ] Optimisation performance
- [ ] Documentation utilisateur
- [ ] Release v10.1

---

## 📚 RÉFÉRENCES

### Datasheets
1. **ESP32 Technical Reference Manual** - Espressif Systems
2. **ESP-IDF ADC Calibration API Guide** - Espressif
3. **ACS712 Datasheet** - Allegro MicroSystems

### Standards
- IEC 61000-4-30: Power quality measurement methods
- IEC 62053: Electricity metering equipment

### Code Références
- ESP-IDF examples: `peripherals/adc/oneshot_read`
- ESP-IDF examples: `peripherals/adc/continuous_read`

---

**FIN DU PLAN PROFESSIONNEL**

*Ce document constitue la spécification technique complète pour l'implémentation du calibrage automatique ADC haute précision dans AepBill v10.*
