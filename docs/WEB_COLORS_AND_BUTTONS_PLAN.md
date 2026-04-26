# Plan Complet: Interface Web Couleurs + Boutons Physiques

Date: 17 Décembre 2025, 20:30
Version: AepBill v10.3
Status: 🚧 **EN COURS**

---

## 🎨 PARTIE 1: INTERFACE WEB COULEURS DYNAMIQUES

### Objectif
Améliorer l'interface web avec des couleurs distinctes et animations pour:
- **Date** (Vert émeraude)
- **Heure** (Bleu vif)
- **Status ON/OFF** (Vert/Rouge avec animations)
- **Courant** (Orange normal, Rouge si >3A avec pulse)
- **Alarme** (Gris si none, Violet si set, Orange si imminent <30min)

### Architecture
Modifier `main/webserver/http_server.c` (HTML/CSS/JS embarqué)

### Implémentation

#### 1. Palette CSS (Variables)
```css
:root {
    /* Date & Heure */
    --color-date: #10B981;           /* Vert émeraude */
    --color-time: #3B82F6;           /* Bleu vif */
    
    /* Status */
    --color-status-on: #22C55E;      /* Vert */
    --color-status-off: #EF4444;     /* Rouge */
    
    /* Courant */
    --color-current-normal: #F59E0B; /* Ambre */
    --color-current-low: #6B7280;    /* Gris */
    --color-current-high: #DC2626;   /* Rouge foncé */
    
    /* Alarme */
    --color-alarm-none: #9CA3AF;     /* Gris clair */
    --color-alarm-pending: #8B5CF6;  /* Violet */
    --color-alarm-soon: #F97316;     /* Orange vif */
}

/* Animations */
@keyframes pulse-green {
    0%, 100% { opacity: 1; }
    50% { opacity: 0.8; }
}

@keyframes pulse-red {
    0%, 100% { opacity: 1; }
    50% { opacity: 0.6; }
}

@keyframes pulse-orange {
    0%, 100% { opacity: 1; transform: scale(1); }
    50% { opacity: 0.8; transform: scale(1.05); }
}
```

#### 2. Styles Date/Heure
```css
.date {
    color: var(--color-date);
    font-weight: 600;
    font-size: 1.1rem;
}

.time {
    color: var(--color-time);
    font-weight: 700;
    font-size: 1.3rem;
}
```

#### 3. JavaScript Dynamique
```javascript
function updateStatusColors(data) {
    // Status ON/OFF
    const statusBadge = document.getElementById('statusBadge');
    if (data.relay_on) {
        statusBadge.style.background = 'linear-gradient(135deg, #10B981 0%, #059669 100%)';
        statusBadge.style.animation = 'pulse-green 2s infinite';
    } else {
        statusBadge.style.background = 'linear-gradient(135deg, #EF4444 0%, #DC2626 100%)';
        statusBadge.style.animation = 'none';
    }
    
    // Courant
    const currentValue = parseFloat(data.current);
    const currentElem = document.getElementById('currentValue');
    if (currentValue < 0.05) {
        currentElem.style.color = 'var(--color-current-low)';
        currentElem.style.animation = 'none';
    } else if (currentValue > 3.0) {
        currentElem.style.color = 'var(--color-current-high)';
        currentElem.style.animation = 'pulse-red 1s infinite';
    } else {
        currentElem.style.color = 'var(--color-current-normal)';
        currentElem.style.animation = 'none';
    }
    
    // Alarme
    const alarmElem = document.getElementById('nextAlarm');
    const alarmText = data.next_alarm || 'None';
    
    if (alarmText === 'None' || alarmText === 'none') {
        alarmElem.style.color = 'var(--color-alarm-none)';
        alarmElem.style.fontStyle = 'italic';
        alarmElem.style.animation = 'none';
    } else {
        // Calculer si alarme proche (<30 min)
        const now = new Date();
        const [hours, minutes] = alarmText.split(':').map(Number);
        const alarmDate = new Date(now);
        alarmDate.setHours(hours, minutes, 0, 0);
        
        const diffMinutes = (alarmDate - now) / 1000 / 60;
        
        if (diffMinutes > 0 && diffMinutes < 30) {
            alarmElem.style.color = 'var(--color-alarm-soon)';
            alarmElem.style.animation = 'pulse-orange 2s infinite';
        } else {
            alarmElem.style.color = 'var(--color-alarm-pending)';
            alarmElem.style.animation = 'none';
        }
    }
}
```

### Fichiers à Modifier
- `main/webserver/http_server.c` (HTML/CSS/JS embarqué dans INDEX_HTML)

---

## 🔘 PARTIE 2: BOUTONS PHYSIQUES

### État Actuel

#### ✅ Existant
1. **Bouton EN (Enable)** - Hardware direct → Reboot ESP32
2. **GPIO 23 (RESET_PIN)** - Détecté mais non utilisé dans main loop
3. **Factory Reset Web** - Endpoint `/factoryReset` fonctionnel

#### ❌ Manquant
- **Bouton physique factory reset** (appui long GPIO 23)
- **Intégration dans main loop** pour détecter appui

### Solution Proposée

#### Fonctionnement
```
GPIO 23 (RESET_PIN):
  - Appui COURT (< 3s): rien (pour éviter reset accidentel)
  - Appui LONG (> 5s): FACTORY RESET
      → Efface NVS (WiFi, alarmes, calibrations)
      → Buzzer 3 bips confirmation
      → Reboot automatique
```

#### Implémentation

**1. Dans `gpio_driver.h`:**
```c
/**
 * @brief Check if factory reset requested (long press >5s)
 * @return true if button held >5s
 */
bool is_factory_reset_requested(void);
```

**2. Dans `gpio_driver.c`:**
```c
bool is_factory_reset_requested(void) {
    static uint32_t press_start = 0;
    static bool was_pressed = false;
    
    bool is_pressed = (gpio_get_level(RESET_PIN) == 0);
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);
    
    if (is_pressed && !was_pressed) {
        // Début appui
        press_start = now;
        was_pressed = true;
    } else if (!is_pressed && was_pressed) {
        // Fin appui
        was_pressed = false;
        press_start = 0;
    } else if (is_pressed && was_pressed) {
        // Toujours appuyé - vérifier durée
        uint32_t duration = now - press_start;
        if (duration > 5000) { // 5 secondes
            return true;
        }
    }
    
    return false;
}
```

**3. Dans `main.c` (main loop):**
```c
// Check Factory Reset Button (GPIO 23, >5s press)
if (is_factory_reset_requested()) {
    ESP_LOGW(TAG, "🏭 FACTORY RESET REQUESTED (button held >5s)");
    
    // Bip confirmation (3x rapide)
    for (int i = 0; i < 3; i++) {
        buzzer_start(BUZZER_PATTERN_FAST_BEEP, 200);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
    
    ESP_LOGW(TAG, "Erasing all NVS data...");
    
    // Effacer TOUTES les données NVS
    esp_err_t err = nvs_flash_erase();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✅ NVS erased successfully");
    } else {
        ESP_LOGE(TAG, "❌ NVS erase failed: %s", esp_err_to_name(err));
    }
    
    // Attendre buzzer
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    ESP_LOGW(TAG, "🔄 Rebooting in 2 seconds...");
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    esp_restart();
}
```

### Sécurité
- **Appui > 5s** requis pour éviter reset accidentel
- **Buzzer 3 bips** pour feedback utilisateur
- **Logs clairs** dans console
- **Reboot auto** après reset

---

## 📋 PLAN D'IMPLÉMENTATION

### Phase 1: Boutons Physiques (PRIORITÉ 1)
**Durée:** 20 minutes

1. [ ] Modifier `gpio_driver.h` - Ajouter `is_factory_reset_requested()`
2. [ ] Implémenter dans `gpio_driver.c`
3. [ ] Intégrer dans `main.c` main loop
4. [ ] Tester appui long (>5s)
5. [ ] Valider effacement NVS + reboot

### Phase 2: Interface Web Couleurs (PRIORITÉ 2)
**Durée:** 30-40 minutes

1. [ ] Localiser INDEX_HTML dans `http_server.c`
2. [ ] Ajouter variables CSS (:root)
3. [ ] Modifier classes date/heure
4. [ ] Ajouter animations CSS
5. [ ] Implémenter JavaScript dynamique
6. [ ] Modifier endpoint `/status` si nécessaire
7. [ ] Tester sur navigateur (desktop + mobile)

### Phase 3: Tests & Validation
1. [ ] Test bouton factory reset (>5s)
2. [ ] Test couleurs web (tous states)
3. [ ] Test animations (ON/OFF, courant élevé, alarme proche)
4. [ ] Test responsive design

---

## ✅ CHECKLIST FINALE

### Boutons Physiques
- [ ] EN button (reboot) - ✅ Hardware natif
- [ ] GPIO 23 factory reset (>5s) - 🚧 À implémenter
- [ ] Feedback buzzer 3 bips - 🚧 À implémenter
- [ ] Effacement NVS complet - 🚧 À implémenter

### Interface Web
- [ ] CSS variables couleurs - 🚧 À implémenter
- [ ] Animations pulse/scale - 🚧 À implémenter
- [ ] JavaScript dynamique - 🚧 À implémenter
- [ ] Responsive design - ⚠️ À vérifier

---

## 🎯 ORDRE D'EXÉCUTION RECOMMANDÉ

1. **Boutons physiques d'abord** (critique pour utilisateur)
2. **Interface web ensuite** (amélioration UX)

---

*Plan d'implémentation - AepBill v10.3*
