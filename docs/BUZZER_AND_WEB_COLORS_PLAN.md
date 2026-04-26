# Plan Professionnel: Amélioration Buzzer & Interface Web (Couleurs)
**AepBill v10 - Système d'Alarme Avancé**

Date: 17 Décembre 2025, 18:05

---

## 📋 ANALYSE EXISTANTE

### ✅ Buzzer Déjà Implémenté
- **GPIO:** GPIO25 (BUZZER_PIN)
- **Fonction actuelle:** ON/OFF simple
- **Localisation:** `main/drivers/gpio_driver.c`
- **Utilisation:** Détection d'anomalie (5 min max)

### ⚠️ Limites Actuelles du Buzzer
1. **Pas d'alarme au démarrage** si anomalie détectée
2. **Pas de pattern intermittent** (seulement continu)
3. **Pas de différenciation** entre alarme urgente et avertissement

---

## 🔍 DATASHEET ESP32-WROOM-32 (ESP32-S)

### GPIO Utilisés (À NE PAS TOUCHER)
| GPIO | Fonction | Strapping Pin | Usage AepBill |
|------|----------|---------------|---------------|
| 0    | BOOT     | ✅ Oui        | - |
| 2    | DOWNLOAD | ✅ Oui        | - |
| 4    | -        | -             | - |
| 5    | -        | ✅ Oui        | - |
| 12   | Flash V  | ✅ Oui        | - |
| 15   | BOOT     | ✅ Oui        | - |
| 16   | RELAY    | -             | **Relais** |
| 21   | I2C SDA  | -             | **I2C (LCD/RTC)** |
| 22   | I2C SCL  | -             | **I2C (LCD/RTC)** |
| 23   | BUTTON   | -             | **Bouton Reset** |
| 25   | DAC1     | -             | **BUZZER** ✅ |
| 34   | ADC      | Input only    | **Capteur courant** |

### GPIO Recommandés Disponibles (Si besoin)
- **GPIO 26** (DAC2) - Alternative buzzer  
- **GPIO 27** - Usage général
- **GPIO 32** (ADC1_CH4) - Usage général
- **GPIO 33** (ADC1_CH5) - Usage général

### ⚠️ GPIO à ÉVITER
- **GPIO 6-11:** Flash interne (NEVER use!)
- **GPIO 1/3:** UART0 (Console)
- **GPIO 0,2,5,12,15:** Strapping pins (boot mode)

### 📌 Boutons Physiques
1. **Factory Reset:** GPIO 23 (RESET_PIN) - **Déjà implémenté**
2. **Reboot:** Button EN (Enable) - Hardware direct

**Conclusion:** GPIO 25 est **PARFAIT** pour le buzzer!

---

## 🎵 SOLUTION PROFESSIONNELLE: Buzzer Intelligent

### Architecture Proposée

```
┌─────────────────────────────────────────────────┐
│         BUZZER STATE MACHINE                    │
├─────────────────────────────────────────────────┤
│                                                 │
│  État 1: IDLE (Normal)                          │
│    - Buzzer OFF                                 │
│    - Monitoring anomalies                       │
│                                                 │
│  État 2: ALARM_STARTUP (Si anomalie au boot)    │
│    - Ton continu FORT (2000 Hz)                │
│    - Durée: 5 secondes                          │
│    - Transition → ALARM_INTERMITTENT            │
│                                                 │
│  État 3: ALARM_INTERMITTENT (Anomalie runtime)  │
│    - Pattern: BEEP-pause-BEEP-pause            │
│    - Fréquence: 2000 Hz                         │
│    - Cycle: 500ms ON, 1500ms OFF (2s total)    │
│    - Durée max: 5 minutes                       │
│    - Condition arrêt: Anomalie résolue          │
│                                                 │
│  État 4: TIMEOUT                                │
│    - Buzzer OFF après 5 min                     │
│    - Flag "timeout_reached"                     │
│                                                 │
└─────────────────────────────────────────────────┘
```

### Fonctions à Implémenter

#### 1. `buzzer_set_pattern()` - Nouveau
```c
typedef enum {
    BUZZER_PATTERN_OFF = 0,
    BUZZER_PATTERN_CONTINUOUS,      // Continu (alarme startup)
    BUZZER_PATTERN_FAST_BEEP,       // Rapide (100ms ON, 100ms OFF)
    BUZZER_PATTERN_SLOW_BEEP,       // Lent (500ms ON, 1500ms OFF)
} buzzer_pattern_t;

void buzzer_set_pattern(buzzer_pattern_t pattern);
```

#### 2. `buzzer_task()` - FreeRTOS Task
- Gère les patterns PWM
- Non-bloquant
- Indépendant du main loop

#### 3. `buzzer_alarm_startup()` - Alarme démarrage
```c
void buzzer_alarm_startup(void) {
    // Son continu pendant 5 secondes
    // Puis passage en mode intermittent
}
```

### Implémentation PWM (Son)

**Utilisation LEDC (LED Controller) pour générer des tons:**
```c
// Configuration LEDC pour buzzer
ledc_timer_config_t ledc_timer = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .timer_num = LEDC_TIMER_0,
    .duty_resolution = LEDC_TIMER_10_BIT,
    .freq_hz = 2000,  // 2 kHz (ton alarme)
    .clk_cfg = LEDC_AUTO_CLK
};

ledc_channel_config_t ledc_channel = {
    .channel = LEDC_CHANNEL_0,
    .duty = 0,
    .gpio_num = BUZZER_PIN,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .hpoint = 0,
    .timer_sel = LEDC_TIMER_0
};
```

**Avantages PWM:**
- Contrôle de la **fréquence** (pitch)
- Contrôle du **volume** (duty cycle)
- Son plus professionnel qu'ON/OFF basique

---

## 🎨 SOLUTION INTERFACE WEB: Couleurs Dynamiques

### État Actuel (Image fournie)
- Date: Vert (#00ff00) ✅ OK
- Heure: Cyan (#00ffff) ✅ OK  
- Status OFF: Rouge (gradient) ✅ OK
- Courant: Noir (texte normal) ❌ À améliorer
- Alarme: Noir (texte normal) ❌ À améliorer

### Palette Professionnelle Proposée

```css
/* === VARIABLES CSS (Tokens de couleur) === */
:root {
    /* Date & Heure */
    --color-date: #10B981;           /* Vert émeraude */
    --color-time: #3B82F6;           /* Bleu vif */
    
    /* Status Relais */
    --color-status-on: #22C55E;      /* Vert clair */
    --color-status-off: #EF4444;     /* Rouge */
    
    /* Courant */
    --color-current-normal: #F59E0B; /* Ambre/Orange */
    --color-current-low: #6B7280;    /* Gris (si 0A) */
    --color-current-high: #DC2626;   /* Rouge foncé (surcharge) */
    
    /* Alarme */
    --color-alarm-none: #9CA3AF;     /* Gris clair */
    --color-alarm-pending: #8B5CF6;  /* Violet */
    --color-alarm-soon: #F97316;     /* Orange vif (<30 min) */
    
    /* Gradients */
    --gradient-on: linear-gradient(135deg, #10B981 0%, #059669 100%);
    --gradient-off: linear-gradient(135deg, #EF4444 0%, #DC2626 100%);
}
```

### Modification HTML/CSS

#### Fichier: `main/webserver/index_html.c`

**Date & Heure:**
```html
<!-- AVANT -->
<span class="date">{{DATE}}</span>
<span class="time">{{TIME}}</span>

<!-- APRÈS -->
<span class="date" style="color: var(--color-date); font-weight: 600;">{{DATE}}</span>
<span class="time" style="color: var(--color-time); font-weight: 700;">{{TIME}}</span>
```

**Status ON/OFF:**
```html
<!-- Déjà géré par gradient, améliorer: -->
<div class="status-badge" id="statusBadge">
    <!-- JS dynamique applique .status-on ou .status-off -->
</div>

<style>
.status-badge.status-on {
    background: var(--gradient-on);
    color: white;
    animation: pulse-green 2s infinite;
}

.status-badge.status-off {
    background: var(--gradient-off);
    color: white;
}

@keyframes pulse-green {
    0%, 100% { opacity: 1; }
    50% { opacity: 0.8; }
}
</style>
```

**Courant:**
```html
<!-- AVANT -->
<div class="value">A {{CURRENT}}</div>

<!-- APRÈS -->
<div class="value" id="currentValue" style="color: var(--color-current-normal); font-weight: 700; font-size: 1.5rem;">
    A <span id="currentNum">{{CURRENT}}</span>
</div>

<script>
function updateCurrentColor(current) {
    const elem = document.getElementById('currentValue');
    if (current < 0.05) {
        elem.style.color = 'var(--color-current-low)';
    } else if (current > 3.0) {
        elem.style.color = 'var(--color-current-high)';
        elem.style.animation = 'pulse-red 1s infinite';
    } else {
        elem.style.color = 'var(--color-current-normal)';
        elem.style.animation = 'none';
    }
}

@keyframes pulse-red {
    0%, 100% { opacity: 1; }
    50% { opacity: 0.6; }
}
</script>
```

**Prochaine Alarme:**
```html
<!-- AVANT -->
<div class="value">{{NEXT_ALARM}}</div>

<!-- APRÈS -->
<div class="value" id="nextAlarm" style="font-weight: 600;">
    <span id="alarmText">{{NEXT_ALARM}}</span>
</div>

<script>
function updateAlarmColor(alarmTime) {
    const elem = document.getElementById('nextAlarm');
    
    if (alarmTime === "None" || alarmTime === "none" || !alarmTime) {
        elem.style.color = 'var(--color-alarm-none)';
        elem.style.fontStyle = 'italic';
    } else {
        // Calculer si alarme proche (<30 min)
        const now = new Date();
        const [hours, minutes] = alarmTime.split(':').map(Number);
        const alarmDate = new Date(now);
        alarmDate.setHours(hours, minutes, 0, 0);
        
        const diff = (alarmDate - now) / 1000 / 60; // minutes
        
        if (diff < 30 && diff > 0) {
            elem.style.color = 'var(--color-alarm-soon)';
            elem.style.animation = 'pulse-orange 2s infinite';
        } else {
            elem.style.color = 'var(--color-alarm-pending)';
            elem.style.animation = 'none';
        }
    }
}

@keyframes pulse-orange {
    0%, 100% { opacity: 1; transform: scale(1); }
    50% { opacity: 0.8; transform: scale(1.05); }
}
</script>
```

---

## 📝 PLAN D'IMPLÉMENTATION

### Phase 1: Buzzer Intelligent (Priorité 1)
**Durée:** 1-2 heures

1. ✅ Vérifier GPIO (Déjà fait: GPIO 25 OK)
2. [ ] Créer `buzzer_pattern.c` avec state machine
3. [ ] Implémenter PWM via LEDC
4. [ ] Ajouter task FreeRTOS pour patterns
5. [ ] Modifier `main.c` pour alarme startup
6. [ ] Tester patterns

### Phase 2: Interface Web Couleurs (Priorité 2)
**Durée:** 30 min - 1 heure

1. [ ] Ajouter variables CSS dans `index_html.c`
2. [ ] Modifier styles date/heure
3. [ ] Améliorer styles courant (dynamique)
4. [ ] Améliorer styles alarme (dynamique)
5. [ ] Ajouter animations
6. [ ] Tester dans navigateur

### Phase 3: Tests & Validation
1. [ ] Test buzzer continu (startup alarm)
2. [ ] Test buzzer intermittent (runtime alarm)
3. [ ] Test timeout 5 minutes
4. [ ] Test interface couleurs
5. [ ] Test responsive (mobile/desktop)

---

## 🎯 RÉSUMÉ DES MODIFICATIONS

### Hardware
- ✅ **Aucune modification** (GPIO 25 parfait)
- ✅ Buzzer déjà connecté

### Firmware
- **Fichiers à créer:**
  - `main/drivers/buzzer_pattern.c/h`
- **Fichiers à modifier:**
  - `main/drivers/gpio_driver.c` (PWM LEDC)
  - `main/main.c` (alarme startup + task buzzer)
  
### Interface Web
- **Fichiers à modifier:**
  - `main/webserver/index_html.c` (ajout CSS + JS)

### Augmentation taille firmware estimée
- +5 KB (patterns buzzer)
- +2 KB (CSS/JS interface)
- **Total: ~7 KB** (négligeable)

---

## ✅ VALIDATION DATASHEET ESP32

### GPIO 25 (Buzzer actuel)
- ✅ **Safe:** Pas de strapping pin
- ✅ **DAC1:** Peut générer signaux analogiques (bonus!)
- ✅ **Compatible PWM:** Via LEDC
- ✅ **Pas de conflit:** Libre après boot

### Boutons Physiques
1. **EN (Enable)** - Reboot hardware ✅
2. **GPIO 23** - Factory reset logiciel ✅

**Conclusion:** Configuration actuelle **PARFAITE**, aucun changement GPIO nécessaire!

---

*Document de planification - Prêt pour implémentation*
