# Interface Web Couleurs - Code à Ajouter

**Fichier:** `main/webserver/http_server.c`

---

## 1. Variables CSS (à ajouter dans la section <style>)

```css
/* === Palette de Couleurs Dynamiques === */
:root {
    /* Date & Heure */
    --color-date: #10B981;           /* Vert émeraude */
    --color-time: #3B82F6;           /* Bleu vif */
    
    /* Status Relais */
    --color-status-on: #22C55E;      /* Vert */
    --color-status-off: #EF4444;     /* Rouge */
    
    /* Courant */
    --color-current-normal: #F59E0B; /* Ambre/Orange */
    --color-current-low: #6B7280;    /* Gris */
    --color-current-high: #DC2626;   /* Rouge foncé */
    
    /* Alarme */
    --color-alarm-none: #9CA3AF;     /* Gris clair */
    --color-alarm-pending: #8B5CF6;  /* Violet */
    --color-alarm-soon: #F97316;     /* Orange vif */
}

/* === Animations === */
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

---

## 2. Styles pour Éléments Spécifiques

```css
/* Date & Heure (dans la card DateTime) */
.date-value {
    color: var(--color-date);
    font-weight: 600;
    font-size: 1.1rem;
}

.time-value {
    color: var(--color-time);
    font-weight: 700;
    font-size: 1.3rem;
}

/* Courant */
#currentValue {
    font-weight: 700;
    font-size: 1.4rem;
    transition: color 0.3s ease;
}

/* Alarme */
#nextAlarmValue {
    font-weight: 600;
    transition: color 0.3s ease, transform 0.3s ease;
}
```

---

## 3. JavaScript Dynamique (à ajouter dans updateData())

```javascript
function updateColors(data) {
    // === COURANT ===
    const currentValue = parseFloat(data.current) || 0;
    const currentElem = document.getElementById('currentValue');
    
    if (currentElem) {
        if (currentValue < 0.05) {
            // Courant très bas (presque 0)
            currentElem.style.color = 'var(--color-current-low)';
            currentElem.style.animation = 'none';
        } else if (currentValue > 3.0) {
            // Courant élevé (>3A) - ALERTE
            currentElem.style.color = 'var(--color-current-high)';
            currentElem.style.animation = 'pulse-red 1s infinite';
        } else {
            // Courant normal
            currentElem.style.color = 'var(--color-current-normal)';
            currentElem.style.animation = 'none';
        }
    }
    
    // === STATUS ON/OFF ===
    const statusBadge = document.querySelector('.status-card');
    if (statusBadge && data.relay_on !== undefined) {
        if (data.relay_on) {
            statusBadge.style.background = 'linear-gradient(135deg, #10B981 0%, #059669 100%)';
            statusBadge.style.animation = 'pulse-green 2s infinite';
        } else {
            statusBadge.style.background = 'linear-gradient(135deg, #EF4444 0%, #DC2626 100%)';
            statusBadge.style.animation = 'none';
        }
    }
    
    // === PROCHAINE ALARME ===
    const alarmElem = document.getElementById('nextAlarmValue');
    const alarmText = data.next_alarm || 'None';
    
    if (alarmElem) {
        if (alarmText === 'None' || alarmText === 'none' || !alarmText) {
            // Pas d'alarme
            alarmElem.style.color = 'var(--color-alarm-none)';
            alarmElem.style.fontStyle = 'italic';
            alarmElem.style.animation = 'none';
        } else {
            alarmElem.style.fontStyle = 'normal';
            
            // Calculer si alarme proche (<30 min)
            try {
                const now = new Date();
                const [hours, minutes] = alarmText.split(':').map(Number);
                
                if (!isNaN(hours) && !isNaN(minutes)) {
                    const alarmDate = new Date(now);
                    alarmDate.setHours(hours, minutes, 0, 0);
                    
                    // Si alarme est pour demain (heure passée aujourd'hui)
                    if (alarmDate < now) {
                        alarmDate.setDate(alarmDate.getDate() + 1);
                    }
                    
                    const diffMinutes = (alarmDate - now) / 1000 / 60;
                    
                    if (diffMinutes > 0 && diffMinutes < 30) {
                        // Alarme dans moins de 30 min - URGENT
                        alarmElem.style.color = 'var(--color-alarm-soon)';
                        alarmElem.style.animation = 'pulse-orange 2s infinite';
                    } else {
                        // Alarme programmée (>30 min)
                        alarmElem.style.color = 'var(--color-alarm-pending)';
                        alarmElem.style.animation = 'none';
                    }
                } else {
                    // Format invalide - couleur par défaut
                    alarmElem.style.color = 'var(--color-alarm-pending)';
                    alarmElem.style.animation = 'none';
                }
            } catch (e) {
                // Erreur parsing - couleur par défaut
                alarmElem.style.color = 'var(--color-alarm-pending)';
                alarmElem.style.animation = 'none';
            }
        }
    }
}

// Appeler updateColors() après avoir mis à jour le contenu
// Dans la fonction updateData existante, ajouter après les updates:
updateColors(data);
```

---

## 4. Modifications HTML (IDs à ajouter)

### Date/Heure
```html
<!-- Avant -->
<div>2025-12-17</div>
<div>17:56:12</div>

<!-- Après -->
<div class="date-value" id="dateValue">2025-12-17</div>
<div class="time-value" id="timeValue">17:56:12</div>
```

### Courant
```html
<!-- Avant -->
<div class="value">A 0.43</div>

<!-- Après -->
<div class="value">
    A <span id="currentValue">0.43</span>
</div>
```

### Alarme
```html
<!-- Avant -->
<div class="value">None</div>

<!-- Après -->
<div class="value">
    <span id="nextAlarmValue">None</span>
</div>
```

---

## Résumé des Changements

1. **CSS:** +40 lignes (variables + animations)
2. **JavaScript:** +80 lignes (fonction updateColors)
3. **HTML:** Ajout IDs sur 4 éléments
4. **Impact:** +3-4 KB firmware

---

*Code prêt pour intégration dans http_server.c*
