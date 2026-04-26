# Implémentation Auto-Calibration ADC - Statut

Date: 17 Décembre 2025, 16:20  
Version: AepBill v10.1  
Développeur: Équipe Technique AepBill

---

## ✅ PHASE 1 TERMINÉE: Structures et Headers

### Fichiers Modifiés

#### 1. `adc_driver.h`
- ✅ Structure `adc_calibration_t` enrichie (4 nouveaux champs)
- ✅ Nouvelle structure `adc_calib_stats_t`
- ✅ 4 nouvelles fonctions API:
  - `esp_err_t adc_auto_calibrate_startup(void)`
  - `bool adc_validate_calibration(const adc_calibration_t *cal)`
  - `esp_err_t adc_get_calibration_stats(adc_calib_stats_t *stats)`
  - `void adc_log_calibration_details(void)`

#### 2. `adc_driver.c`
- ✅ Includes ajoutés: `esp_adc/adc_cali.h`, `esp_adc/adc_cali_scheme.h`, `stdbool.h`
- ✅ Variables globales:
  - `static adc_cali_handle_t adc_cali_handle = NULL`
  - `static bool adc_cali_enabled = false`
  - `static adc_calib_stats_t last_calib_stats = {0}`
- ✅ Structure calibration initialisée avec nouveaux champs

### Code Préservé (100% identique)
- ✅ `adc_sample_raw_rms()` - Mesure RMS inchangée
- ✅ `adc_read_current_amps()` - API publique inchangée
- ✅ `adc_calibrate_zero()` - Calibrage manuel intact
- ✅ Toutes les fonctions NVS existantes
- ✅ Thresholds d'anomalie

---

## 🚧 PHASE 2 EN COURS: Implémentation des Fonctions

### À Implémenter (dans adc_driver.c)

#### 1. Fonction: `adc_init_with_efuse()`
**But:** Initialiser ADC avec calibration eFuse Vref  
**Emplacement:** Après `adc_driver_init()` (avant ligne 76)

```c
static esp_err_t adc_init_with_efuse(void) {
    // 1. Configurer ADC calibration scheme
    // 2. Query eFuse Vref  
    // 3. Sauvegarder type et valeur dans calibration
    // 4. Return ESP_OK
}
```

#### 2. Fonction: `adc_validate_calibration()`
**But:** Valider une structure calibration  
**Emplacement:** Après les fonctions NVS (après ligne 316)

```c
bool adc_validate_calibration(const adc_calibration_t *cal) {
    // Checks:
    // - is_valid == 1
    // - zero_offset dans range [1900, 2200]
    // - scale_factor dans range [0.8, 1.2]
    // - timestamp pas trop ancien (<30 jours)
    // - noise_level_rms < 50.0f
}
```

#### 3. Fonction: `adc_auto_calibrate_startup()`
**But:** Calibration automatique complète  
**Emplacement:** Après `adc_validate_calibration()`

```c
esp_err_t adc_auto_calibrate_startup(void) {
    // 1. Warmup (100 samples jetés)
    // 2. Collecte 1000 samples
    // 3. Stats: mean, stddev, min, max
    // 4. Filtrage outliers (3-sigma)
    // 5. Calculer zero final
    // 6. Mesurer noise level
    // 7. Update calibration structure
    // 8. Timestamp
    // 9. Return ESP_OK
}
```

#### 4. Fonction: `adc_get_calibration_stats()`
**But:** Retourner stats de calibration  
**Emplacement:** Après `adc_auto_calibrate_startup()`

```c
esp_err_t adc_get_calibration_stats(adc_calib_stats_t *stats) {
    if (!stats) return ESP_ERR_INVALID_ARG;
    *stats = last_calib_stats;
    return ESP_OK;
}
```

#### 5. Fonction: `adc_log_calibration_details()`
**But:** Logger détails calibration  
**Emplacement:** Après `adc_get_calibration_stats()`

```c
void adc_log_calibration_details(void) {
    ESP_LOGI(TAG, "=== ADC Calibration Report ===");
    ESP_LOGI(TAG, "Zero offset: %.1f", calibration.zero_offset);
    ESP_LOGI(TAG, "Scale factor: %.3f", calibration.scale_factor);
    ESP_LOGI(TAG, "Vref type: %d", calibration.vref_type);
    ESP_LOGI(TAG, "Actual Vref: %u mV", calibration.actual_vref_mv);
    ESP_LOGI(TAG, "Noise RMS: %.1f", calibration.noise_level_rms);
    // ... etc
}
```

#### 6. Modification: `adc_driver_init()`
**But:** Appeler auto-calibration si nécessaire  
**Emplacement:** Modifier fonction existante (ligne 47-75)

```c
void adc_driver_init(void) {
    // ... code existant init hardware ...
    
    // NOUVEAU: Init eFuse
    adc_init_with_efuse();
    
    // ... code existant load calibration ...
    
    // NOUVEAU: Validation et auto-calib
    if (!adc_validate_calibration(&calibration)) {
        ESP_LOGW(TAG, "Invalid calibration, auto-calibrating...");
        adc_auto_calibrate_startup();
        adc_save_calibration();
    }
    
    // ... reste code existant ...
}
```

---

## 📋 CHECKLIST IMPLÉMENTATION

### Phase 2: Fonctions (EN COURS)
- [ ] `adc_init_with_efuse()` 
- [ ] `adc_validate_calibration()`
- [ ] `adc_auto_calibrate_startup()`
- [ ] `adc_get_calibration_stats()`
- [ ] `adc_log_calibration_details()`
- [ ] Modifier `adc_driver_init()`

### Phase 3: Tests
- [ ] Compilation sans erreurs
- [ ] Test au démarrage (sans calibration NVS)
- [ ] Test avec calibration invalide
- [ ] Test calibrage manuel (bouton)
- [ ] Test mesures de courant

### Phase 4: Documentation
- [ ] Commentaires code
- [ ] Guide utilisateur
- [ ] Changelog

---

## 🎯 PROCHAINE ÉTAPE

**Implémenter `adc_init_with_efuse()` et `adc_validate_calibration()`**

Temps estimé: 10 minutes

---

*Document auto-généré - Ne pas éditer manuellement*
