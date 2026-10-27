#include "i18n.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "nvs.h"

static const char *TAG = "I18N";

extern esp_err_t nvs_read_string(const char *key, char *value, size_t max_len);
extern esp_err_t nvs_write_string(const char *key, const char *value);

// Trilingual translation table: [STR_KEY][LANG_AR, LANG_FR, LANG_EN]
static const char* s_translations[STR_TOTAL_COUNT][LANG_MAX] = {
    // General
    [STR_APP_TITLE] = {
        "نظام التحكم اللاسلكي الاتوماتيكي",
        "Système de Contrôle Sans-Fil Automatique",
        "Automatic Wireless Control System"
    },
    [STR_APP_SUBTITLE] = {
        "إعداد و تجميع الاستاذ الدادسي احمد استاذ الفيزياء و الكيمياء ثانوية تنزرت التأهيلية مديرية تارودانت",
        "Conception & Réalisation : Prof. Eddadsi Ahmed (Physique-Chimie) - Lycée Qualifiant Tinzert, Taroudant",
        "Designed & Built by: Prof. Eddadsi Ahmed (Physics & Chemistry) - Tinzert High School, Taroudant"
    },
    [STR_BELL_SYSTEM] = {
        "جرس AepBill",
        "Sonnerie AepBill",
        "AepBill Bell System"
    },
    [STR_YES] = {
        "نعم",
        "Oui",
        "Yes"
    },
    [STR_CANCEL] = {
        "إلغاء",
        "Annuler",
        "Cancel"
    },
    [STR_CONFIRM] = {
        "تأكيد",
        "Confirmer",
        "Confirm"
    },
    [STR_SAVE] = {
        "حفظ",
        "Enregistrer",
        "Save"
    },
    [STR_SUCCESS] = {
        "نجاح",
        "Succès",
        "Success"
    },
    [STR_ERROR] = {
        "خطأ",
        "Erreur",
        "Error"
    },
    [STR_SAVED_SUCCESS] = {
        "تم الحفظ بنجاح",
        "Enregistré avec succès",
        "Saved successfully"
    },
    [STR_PLEASE_WAIT] = {
        "يرجى الانتظار...",
        "Veuillez patienter...",
        "Please wait..."
    },
    [STR_BACK_TO_DASHBOARD] = {
        "العودة إلى اللوحة الرئيسية",
        "Retour au tableau de bord",
        "Back to Dashboard"
    },
    [STR_RETURNING] = {
        "جاري العودة...",
        "Redirection en cours...",
        "Redirecting..."
    },

    // Login
    [STR_LOGIN_TITLE] = {
        "تسجيل الدخول",
        "Connexion Administrateur",
        "Admin Login"
    },
    [STR_PASSWORD] = {
        "كلمة المرور",
        "Mot de passe",
        "Password"
    },
    [STR_LOGIN_BTN] = {
        "دخول",
        "Se connecter",
        "Sign In"
    },
    [STR_WRONG_PASSWORD] = {
        "كلمة المرور خاطئة",
        "Mot de passe incorrect",
        "Incorrect Password"
    },
    [STR_CHECK_PASSWORD] = {
        "يرجى التحقق من كلمة المرور والمحاولة مرة أخرى",
        "Veuillez vérifier votre mot de passe et réessayer",
        "Please check your password and try again"
    },
    [STR_RETRY] = {
        "إعادة المحاولة",
        "Réessayer",
        "Try Again"
    },

    // Dashboard
    [STR_SYS_NORMAL] = {
        "✅ النظام يعمل بشكل طبيعي",
        "✅ Le système fonctionne normalement",
        "✅ System is operating normally"
    },
    [STR_SYS_RUNNING] = {
        "النظام شغّال",
        "Système En Marche",
        "System Running"
    },
    [STR_SYS_STOPPED] = {
        "النظام متوقف",
        "Système Arrêté",
        "System Stopped"
    },
    [STR_CURRENT_TIME] = {
        "الوقت الحالي",
        "Heure actuelle",
        "Current Time"
    },
    [STR_NEXT_BELL] = {
        "الجرس القادم",
        "Prochaine sonnerie",
        "Next Bell"
    },
    [STR_NONE] = {
        "لا يوجد",
        "Aucune",
        "None"
    },
    [STR_RELAY_BELL] = {
        "جرس S",
        "Sonnerie S (R1)",
        "Bell S (R1)"
    },
    [STR_RELAY_DEV_A] = {
        "جهاز A",
        "Appareil A (R2)",
        "Device A (R2)"
    },
    [STR_RELAY_DEV_B] = {
        "جهاز B",
        "Appareil B (R3)",
        "Device B (R3)"
    },
    [STR_RELAY_DEV_C] = {
        "جهاز C",
        "Appareil C (R4)",
        "Device C (R4)"
    },
    [STR_CAM_FEED] = {
        "🎥 بث المراقبة",
        "🎥 Flux Vidéo en Direct",
        "🎥 Live Camera Feed"
    },
    [STR_CAM_DASHBOARD] = {
        "🌐 لوحة تحكم الكاميرا",
        "🌐 Panneau Caméra",
        "🌐 Camera Dashboard"
    },
    [STR_CAM_ENTER_IP] = {
        "أدخل عنوان IP الكاميرا:",
        "Entrez l’adresse IP de la caméra :",
        "Enter Camera IP address:"
    },
    [STR_SET_TIME_TITLE] = {
        "⏰ ضبط الوقت والساعة",
        "⏰ Réglage de l’Horloge",
        "⏰ Clock & Time Settings"
    },
    [STR_SYNC_DEV_TIME] = {
        "🔄 مزامنة فورية مع وقت جهازي (الهاتف / الحاسوب)",
        "🔄 Synchroniser avec l’heure de mon appareil",
        "🔄 Sync with my device clock"
    },
    [STR_YEAR] = {
        "السنة",
        "Année",
        "Year"
    },
    [STR_MONTH] = {
        "الشهر",
        "Mois",
        "Month"
    },
    [STR_DAY] = {
        "اليوم",
        "Jour",
        "Day"
    },
    [STR_HOUR] = {
        "الساعة (HH)",
        "Heure (HH)",
        "Hour (HH)"
    },
    [STR_MINUTE] = {
        "الدقيقة (MM)",
        "Minute (MM)",
        "Minute (MM)"
    },
    [STR_SAVE_TIME] = {
        "💾 حفظ الوقت",
        "💾 Enregistrer l’Heure",
        "💾 Save Time"
    },

    // v12 Predictive Maintenance
    [STR_MAINT_TITLE] = {
        "🩺 الصيانة الوقائية بالذكاء الاصطناعي",
        "🩺 Maintenance Prédictive par IA",
        "🩺 AI Predictive Maintenance"
    },
    [STR_MAINT_CHECKING] = {
        "جاري فحص حالة النظام...",
        "Vérification de l’état du système...",
        "Checking system status..."
    },
    [STR_MAINT_RELAY_HEALTH] = {
        "صحة المُرحّل (الرولي): ",
        "Santé du relais : ",
        "Relay Health: "
    },
    [STR_MAINT_TEMP] = {
        " | الحرارة: ",
        " | Température : ",
        " | Temperature: "
    },
    [STR_MAINT_CALIBRATING] = {
        "⏳ جاري معايرة التقدير...",
        "⏳ Étalonnage de l’estimation en cours...",
        "⏳ Calibrating estimation..."
    },
    [STR_MAINT_CRITICAL_REPLACE] = {
        "🔴 حرج: استبدال فوري!",
        "🔴 Critique: Remplacement immédiat !",
        "🔴 Critical: Immediate replacement!"
    },
    [STR_MAINT_REMAINING_PREFIX] = {
        "📅 متبقي تقريباً: ",
        "📅 Restant approx. : ",
        "📅 Remaining approx.: "
    },
    [STR_MAINT_DAYS] = {
        " يوم",
        " jours",
        " days"
    },
    [STR_MAINT_DANGER_REPLACE] = {
        "⚠️ خطر: يجب الاستبدال!",
        "⚠️ Danger: Doit être remplacé !",
        "⚠️ Danger: Must be replaced!"
    },

    // v12 Energy & Sleep Management
    [STR_POWER_TITLE] = {
        "⚡ إدارة الطاقة (Eco-Energy)",
        "⚡ Gestion de l’Énergie (Eco-Energy)",
        "⚡ Energy Management (Eco-Energy)"
    },
    [STR_POWER_MODE] = {
        "وضع التشغيل:",
        "Mode de fonctionnement :",
        "Operating Mode:"
    },
    [STR_POWER_MODE_NORMAL] = {
        "عادي (Normal)",
        "Normal (Actif)",
        "Normal (Active)"
    },
    [STR_POWER_MODE_MODEM] = {
        "توفير (Modem Sleep)",
        "Économie (Modem Sleep)",
        "Power Saving (Modem Sleep)"
    },
    [STR_POWER_MODE_DEEP] = {
        "يومي (Deep Sleep)",
        "Quotidien (Deep Sleep)",
        "Daily (Deep Sleep)"
    },
    [STR_POWER_MODE_HOLIDAY] = {
        "عطلة (Holiday Mode)",
        "Vacances (Holiday Mode)",
        "Holiday Mode"
    },
    [STR_POWER_MODE_WEEKLY] = {
        "أسبوعي (Weekly Mode)",
        "Hebdomadaire (Weekly Mode)",
        "Weekly Mode"
    },
    [STR_POWER_DAILY_DESC] = {
        "حدد وقت النوم التلقائي (يوميا):",
        "Définir la plage de veille automatique (quotidienne) :",
        "Set automatic sleep schedule (daily):"
    },
    [STR_POWER_TO] = {
        "إلى",
        "à",
        "to"
    },
    [STR_POWER_FROM] = {
        "من:",
        "De :",
        "From:"
    },
    [STR_POWER_HOLIDAY_DESC] = {
        "حدد وقت وتاريخ العطلة (Holiday):",
        "Définir la période de vacance (Holiday) :",
        "Set holiday period (Holiday):"
    },
    [STR_POWER_WEEKLY_DESC] = {
        "حدد وقت النوم لكل يوم (من الأحد إلى السبت):",
        "Définir la veille pour chaque jour (Dimanche à Samedi) :",
        "Set sleep schedule for each day (Sunday to Saturday):"
    },
    [STR_POWER_SAVED_OK] = {
        "تم الحفظ بنجاح ✓",
        "Enregistré avec succès ✓",
        "Saved successfully ✓"
    },
    [STR_POWER_SAVE_BTN] = {
        "حفظ إعدادات النوم",
        "Enregistrer les paramètres de veille",
        "Save Sleep Settings"
    },
    [STR_DAY_SUN_SHORT] = {
        "الأحد (Sun)",
        "Dimanche (Dim)",
        "Sunday (Sun)"
    },
    [STR_DAY_MON_SHORT] = {
        "الإثنين (Mon)",
        "Lundi (Lun)",
        "Monday (Mon)"
    },
    [STR_DAY_TUE_SHORT] = {
        "الثلاثاء (Tue)",
        "Mardi (Mar)",
        "Tuesday (Tue)"
    },
    [STR_DAY_WED_SHORT] = {
        "الأربعاء (Wed)",
        "Mercredi (Mer)",
        "Wednesday (Wed)"
    },
    [STR_DAY_THU_SHORT] = {
        "الخميس (Thu)",
        "Jeudi (Jeu)",
        "Thursday (Thu)"
    },
    [STR_DAY_FRI_SHORT] = {
        "الجمعة (Fri)",
        "Vendredi (Ven)",
        "Friday (Fri)"
    },
    [STR_DAY_SAT_SHORT] = {
        "السبت (Sat)",
        "Samedi (Sam)",
        "Saturday (Sat)"
    },


    // Navigation Menu
    [STR_NAV_SETTINGS] = {
        "⏰ إعدادات الجرس",
        "⏰ Programmation Sonneries",
        "⏰ Bell Schedules"
    },
    [STR_NAV_WIFI] = {
        "📶 إعدادات الشبكة",
        "📶 Configuration Wi-Fi",
        "📶 Wi-Fi Settings"
    },
    [STR_NAV_TIMEZONE] = {
        "🌍 المنطقة الزمنية",
        "🌍 Fuseau Horaire",
        "🌍 Time Zone"
    },
    [STR_NAV_PASSWORD] = {
        "🔑 تغيير كلمة السر",
        "🔑 Changer Mot de Passe",
        "🔑 Change Password"
    },
    [STR_NAV_ABOUT] = {
        "ℹ️ حول الجهاز",
        "ℹ️ À Propos",
        "ℹ️ About System"
    },
    [STR_NAV_BACKUP] = {
        "💾 نسخة احتياطية",
        "💾 Sauvegarde & Restauration",
        "💾 Backup & Restore"
    },
    [STR_NAV_OTA] = {
        "🔄 تحديث OTA",
        "🔄 Mise à Jour OTA",
        "🔄 OTA Firmware Update"
    },
    [STR_NAV_LOGS] = {
        "📜 سجلات النظام",
        "📜 Journaux Système",
        "📜 System Logs"
    },
    [STR_NAV_RESTART] = {
        "🔄 إعادة التشغيل",
        "🔄 Redémarrer",
        "🔄 Restart"
    },
    [STR_NAV_FACTORY_RESET] = {
        "⚠️ ضبط المصنع",
        "⚠️ Réinitialisation Usine",
        "⚠️ Factory Reset"
    },
    [STR_NAV_LOGOUT] = {
        "🚪 تسجيل الخروج",
        "🚪 Déconnexion",
        "🚪 Logout"
    },

    // Settings / Schedule
    [STR_SCHEDULE_TITLE] = {
        "جدولة الجرس",
        "Planification des Sonneries",
        "Bell Schedule"
    },
    [STR_DATE_AND_TIME] = {
        "التاريخ والساعة:",
        "Date et heure :",
        "Date and time:"
    },
    [STR_ENABLE_DAY] = {
        "تفعيل:",
        "Activer :",
        "Enable:"
    },
    [STR_ALARM_NUM] = {
        "منبه",
        "Sonnerie",
        "Bell"
    },
    [STR_START_TIME] = {
        "البداية:",
        "Début :",
        "Start:"
    },
    [STR_END_TIME] = {
        "النهاية:",
        "Fin :",
        "End:"
    },
    [STR_END_DAY] = {
        "يوم النهاية:",
        "Jour de fin :",
        "End Day:"
    },
    [STR_SAME_DAY] = {
        "نفس اليوم",
        "Même jour",
        "Same Day"
    },
    [STR_NEXT_DAY] = {
        "اليوم التالي (+1)",
        "Jour suivant (+1)",
        "Next Day (+1)"
    },
    [STR_SELECT_OUTPUT] = {
        "الرنين:",
        "Sortie / Relais :",
        "Output / Relay:"
    },
    [STR_IMPORT_EXPORT] = {
        "📥 استيراد / تصدير",
        "📥 Importer / Exporter",
        "📥 Import / Export"
    },
    [STR_IMPORT_JSON] = {
        "📁 استيراد ملف JSON :",
        "📁 Importer fichier JSON :",
        "📁 Import JSON file:"
    },
    [STR_UPLOAD_SCHEDULE] = {
        "📤 تحميل الجدول (JSON)",
        "📤 Importer le planning (JSON)",
        "📤 Import Schedule (JSON)"
    },
    [STR_DOWNLOAD_SCHEDULE] = {
        "📥 تنزيل الجدول (JSON)",
        "📥 Télécharger le planning (JSON)",
        "📥 Download Schedule (JSON)"
    },
    [STR_SAVE_SETTINGS] = {
        "حفظ الإعدادات",
        "Enregistrer les modifications",
        "Save Settings"
    },
    [STR_SAVING] = {
        "جاري الحفظ...",
        "Enregistrement en cours...",
        "Saving..."
    },
    [STR_ALERT_END_AFTER_START] = {
        "⚠️ خطأ: على نفس اليوم، النهاية يجب أن تكون بعد البداية!",
        "⚠️ Erreur : Sur le même jour, l’heure de fin doit être après l’heure de début !",
        "⚠️ Error: On the same day, end time must be after start time!"
    },

    // Days of Week
    [STR_DAY_MONDAY] = {
        "الإثنين",
        "Lundi",
        "Monday"
    },
    [STR_DAY_TUESDAY] = {
        "الثلاثاء",
        "Mardi",
        "Tuesday"
    },
    [STR_DAY_WEDNESDAY] = {
        "الأربعاء",
        "Mercredi",
        "Wednesday"
    },
    [STR_DAY_THURSDAY] = {
        "الخميس",
        "Jeudi",
        "Thursday"
    },
    [STR_DAY_FRIDAY] = {
        "الجمعة",
        "Vendredi",
        "Friday"
    },
    [STR_DAY_SATURDAY] = {
        "السبت",
        "Samedi",
        "Saturday"
    },
    [STR_DAY_SUNDAY] = {
        "الأحد",
        "Dimanche",
        "Sunday"
    },

    // WiFi
    [STR_WIFI_TITLE] = {
        "إعدادات الشبكة",
        "Configuration Wi-Fi",
        "Wi-Fi Settings"
    },
    [STR_CURRENT_SSID] = {
        "SSID الحالي:",
        "SSID actuel :",
        "Current SSID:"
    },
    [STR_NETWORK_NAME] = {
        "اسم الشبكة (SSID):",
        "Nom du réseau (SSID) :",
        "Network Name (SSID):"
    },
    [STR_WIFI_WARN_RESTART] = {
        "⚠️ ستحتاج إلى إعادة التشغيل لتطبيق التغييرات",
        "⚠️ Un redémarrage est nécessaire pour appliquer les modifications",
        "⚠️ Restart required to apply changes"
    },
    [STR_WIFI_INVALID_INPUT] = {
        "SSID وكلمة المرور مطلوبان",
        "Le SSID et le mot de passe sont obligatoires",
        "SSID and password are required"
    },
    [STR_WIFI_SAVE_FAILED] = {
        "حدث خطأ أثناء حفظ إعدادات WiFi",
        "Erreur lors de l’enregistrement des paramètres Wi-Fi",
        "Error saving Wi-Fi settings"
    },
    [STR_WIFI_SAVED] = {
        "تم حفظ WiFi",
        "Wi-Fi enregistré",
        "Wi-Fi Saved"
    },
    [STR_WIFI_RESTART_PROMPT] = {
        "أعد تشغيل النظام لتطبيق التغييرات",
        "Redémarrez le système pour appliquer les modifications",
        "Restart system to apply changes"
    },
    [STR_RESTART_NOW] = {
        "إعادة التشغيل",
        "Redémarrer maintenant",
        "Restart Now"
    },
    [STR_LATER] = {
        "لاحقا",
        "Plus tard",
        "Later"
    },

    // Timezone
    [STR_TZ_TITLE] = {
        "إعدادات المنطقة الزمنية",
        "Réglages du Fuseau Horaire",
        "Time Zone Settings"
    },
    [STR_SELECT_TZ] = {
        "اختر المنطقة الزمنية:",
        "Sélectionnez le fuseau horaire :",
        "Select Time Zone:"
    },
    [STR_TZ_SAVED_TITLE] = {
        "تم حفظ المنطقة الزمنية",
        "Fuseau horaire enregistré",
        "Time Zone Saved"
    },
    [STR_TZ_APPLIED_IMMEDIATELY] = {
        "سيتم تطبيق التغييرات فورا",
        "Les modifications sont appliquées immédiatement",
        "Changes applied immediately"
    },

    // Current Config
    [STR_CURR_TITLE] = {
        "إعدادات مستشعر التيار",
        "Configuration Capteur de Courant",
        "Current Sensor Settings"
    },
    [STR_CURR_CURRENT_STATUS] = {
        "📊 الحالة الحالية:",
        "📊 État actuel :",
        "📊 Current Status:"
    },
    [STR_CURR_ZERO_POINT] = {
        "النقطة الصفرية:",
        "Point zéro :",
        "Zero Point:"
    },
    [STR_CURR_SCALE_FACTOR] = {
        "عامل التعديل:",
        "Facteur d’échelle :",
        "Scale Factor:"
    },
    [STR_CURR_CALIB_SAVED] = {
        "✅ محفوظ",
        "✅ Enregistré",
        "✅ Saved"
    },
    [STR_CURR_CALIB_DEFAULT] = {
        "⚠️ افتراضي",
        "⚠️ Par défaut",
        "⚠️ Default"
    },
    [STR_CURR_THRESHOLDS] = {
        "🎚️ العتبات",
        "🎚️ Seuils de Détection",
        "🎚️ Detection Thresholds"
    },
    [STR_CURR_MIN_LOAD] = {
        "الحد الأدنى للحمل (A):",
        "Charge minimale (A) :",
        "Minimum Load (A):"
    },
    [STR_CURR_ANOMALY] = {
        "عتبة التسرب (A):",
        "Seuil de fuite / anomalie (A) :",
        "Leakage / Anomaly threshold (A):"
    },
    [STR_CURR_MAX_CRITICAL] = {
        "الحد الأقصى الحرج (A):",
        "Seuil critique maximum (A) :",
        "Maximum Critical Threshold (A):"
    },

    // Password change
    [STR_PWD_TITLE] = {
        "تأكيد كلمة المرور",
        "Modification du Mot de Passe",
        "Change Admin Password"
    },
    [STR_OLD_PWD] = {
        "كلمة المرور الحالية:",
        "Mot de passe actuel :",
        "Current Password:"
    },
    [STR_NEW_PWD] = {
        "كلمة المرور الجديدة:",
        "Nouveau mot de passe :",
        "New Password:"
    },
    [STR_CONFIRM_PWD] = {
        "تأكيد كلمة المرور:",
        "Confirmer le mot de passe :",
        "Confirm Password:"
    },
    [STR_PWD_WRONG_OLD] = {
        "كلمة المرور الحالية غير صحيحة",
        "Le mot de passe actuel est incorrect",
        "Current password is incorrect"
    },
    [STR_PWD_MISMATCH] = {
        "كلمتا المرور الجديدتان يجب أن تكونا متطابقتين",
        "Les deux nouveaux mots de passe ne correspondent pas",
        "New passwords do not match"
    },
    [STR_PWD_CHANGED_OK] = {
        "تم حفظ كلمة المرور الجديدة بنجاح",
        "Le mot de passe a été modifié avec succès",
        "Password changed successfully"
    },

    // About
    [STR_ABOUT_TITLE] = {
        "حول نظام جرس المدرسة اللاسلكي",
        "À Propos du Système AepBill",
        "About AepBill Bell System"
    },
    [STR_ABOUT_SYSTEM] = {
        "النظام",
        "Système",
        "System"
    },
    [STR_ABOUT_VERSION] = {
        "الإصدار",
        "Version",
        "Version"
    },
    [STR_ABOUT_DEV] = {
        "المطور",
        "Développeur",
        "Developer"
    },
    [STR_ABOUT_DEV_NAME] = {
        "أ. الدادسي أحمد<br><small>(الفيزياء والكيمياء)</small>",
        "Prof. Eddadsi Ahmed<br><small>(Physique & Chimie)</small>",
        "Prof. Eddadsi Ahmed<br><small>(Physics & Chemistry)</small>"
    },
    [STR_ABOUT_INSTITUTION] = {
        "المؤسسة",
        "Établissement",
        "Institution"
    },
    [STR_ABOUT_INSTITUTION_NAME] = {
        "ثانوية تنزرت التأهيلية<br>تارودانت، المغرب",
        "Lycée Qualifiant Tinzert<br>Taroudant, Maroc",
        "Tinzert High School<br>Taroudant, Morocco"
    },
    [STR_ABOUT_RIGHTS] = {
        "الحقوق",
        "Droits d’auteur",
        "Copyright"
    },
    [STR_ABOUT_RIGHTS_TEXT] = {
        "© 2026 جميع الحقوق محفوظة<br>الدادسي أحمد",
        "© 2026 Tous droits réservés<br>Eddadsi Ahmed",
        "© 2026 All rights reserved<br>Eddadsi Ahmed"
    },

    // Backup & Restore
    [STR_BACKUP_TITLE] = {
        "💾 النسخ الاحتياطي والاستعادة",
        "💾 Sauvegarde & Restauration",
        "💾 Backup & Restore"
    },
    [STR_BACKUP_DOWNLOAD_TITLE] = {
        "📥 تحميل النسخة الاحتياطية",
        "📥 Télécharger la Sauvegarde",
        "📥 Download Backup"
    },
    [STR_BACKUP_DOWNLOAD_DESC] = {
        "تصدير الإعدادات الكاملة (المنطقة الزمنية، معايرة المستشعر) كملف JSON.",
        "Exportez votre configuration complète en JSON (fuseau horaire, étalonnage ADC).",
        "Export complete configuration as JSON (timezone, ADC calibration)."
    },
    [STR_BACKUP_DOWNLOAD_BTN] = {
        "📥 تنزيل الإعدادات",
        "📥 Télécharger Configuration",
        "📥 Download Configuration"
    },
    [STR_RESTORE_TITLE] = {
        "📤 استعادة الإعدادات",
        "📤 Restaurer la Configuration",
        "📤 Restore Configuration"
    },
    [STR_RESTORE_DESC] = {
        "⚠️ تحذير: سيعاد تشغيل النظام بعد الاستعادة.",
        "⚠️ Attention : Le système redémarrera après la restauration.",
        "⚠️ Warning: System will restart after restore."
    },
    [STR_RESTORE_BTN] = {
        "📤 استعادة",
        "📤 Restaurer",
        "📤 Restore"
    },

    // OTA
    [STR_OTA_TITLE] = {
        "تحديث نظام AepBill",
        "Mise à Jour du Système AepBill",
        "AepBill System Firmware Update"
    },
    [STR_OTA_HASH_LABEL] = {
        "تجزئة SHA256 للبرنامج:",
        "Empreinte SHA256 du binaire :",
        "Firmware SHA256 Hash:"
    },
    [STR_OTA_HASH_HINT] = {
        "احصل على التجزئة من ملف .bin.sha256",
        "Copiez la valeur depuis le fichier .bin.sha256",
        "Get hash value from .bin.sha256 file"
    },
    [STR_OTA_FILE_LABEL] = {
        "اختر ملف البرنامج (.bin):",
        "Sélectionnez le fichier firmware (.bin) :",
        "Select firmware binary (.bin):"
    },
    [STR_OTA_SUBMIT] = {
        "تحديث الآن",
        "Mettre à jour maintenant",
        "Update Now"
    },

    // System actions
    [STR_REBOOT_CONFIRM] = {
        "هل أنت متأكد أنك تريد إعادة تشغيل النظام؟",
        "Êtes-vous sûr de vouloir redémarrer le système ?",
        "Are you sure you want to restart the system?"
    },
    [STR_REBOOTING] = {
        "جاري إعادة التشغيل...",
        "Redémarrage en cours...",
        "Rebooting in progress..."
    },
    [STR_FACTORY_RESET_TITLE] = {
        "تأكيد ضبط المصنع",
        "Confirmation Réinitialisation d’Usine",
        "Factory Reset Confirmation"
    },
    [STR_FACTORY_RESET_WARN] = {
        "تحذير! سيتم حذف جميع الإعدادات والبيانات:<ul><li>جميع المواقيت المبرمجة</li><li>كلمة المرور</li><li>إعدادات الشبكة</li></ul>",
        "Attention ! Toutes les données seront supprimées :<ul><li>Toutes les programmations d’alarmes</li><li>Mot de passe administrateur</li><li>Paramètres réseau Wi-Fi</li></ul>",
        "Warning! All data and settings will be deleted:<ul><li>All scheduled bells</li><li>Admin password</li><li>Wi-Fi network settings</li></ul>"
    },
    [STR_FACTORY_RESET_CONFIRM_BTN] = {
        "نعم، حذف الكل",
        "Oui, Tout Réinitialiser",
        "Yes, Reset All"
    }
};

static const char* s_days[7][LANG_MAX] = {
    { "الإثنين", "Lundi", "Monday" },
    { "الثلاثاء", "Mardi", "Tuesday" },
    { "الأربعاء", "Mercredi", "Wednesday" },
    { "الخميس", "Jeudi", "Thursday" },
    { "الجمعة", "Vendredi", "Friday" },
    { "السبت", "Samedi", "Saturday" },
    { "الأحد", "Dimanche", "Sunday" }
};

app_lang_t i18n_get_req_lang(httpd_req_t *req) {
    if (!req) return LANG_AR;

    // 1. Check Query parameter: ?lang=fr or ?lang=en or ?lang=ar
    char query[64] = {0};
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        char val[16] = {0};
        if (httpd_query_key_value(query, "lang", val, sizeof(val)) == ESP_OK) {
            if (strcmp(val, "fr") == 0) return LANG_FR;
            if (strcmp(val, "en") == 0) return LANG_EN;
            if (strcmp(val, "ar") == 0) return LANG_AR;
        }
    }

    // 2. Check Cookie header: Cookie: lang=fr
    size_t cookie_len = httpd_req_get_hdr_value_len(req, "Cookie");
    if (cookie_len > 0 && cookie_len < 256) {
        char cookie_buf[256] = {0};
        if (httpd_req_get_hdr_value_str(req, "Cookie", cookie_buf, sizeof(cookie_buf)) == ESP_OK) {
            char *ptr = strstr(cookie_buf, "lang=");
            if (ptr) {
                ptr += 5;
                if (strncmp(ptr, "fr", 2) == 0) return LANG_FR;
                if (strncmp(ptr, "en", 2) == 0) return LANG_EN;
                if (strncmp(ptr, "ar", 2) == 0) return LANG_AR;
            }
        }
    }

    // 3. Check NVS default setting
    char stored_lang[16] = {0};
    if (nvs_read_string("app_lang", stored_lang, sizeof(stored_lang)) == ESP_OK) {
        if (strcmp(stored_lang, "fr") == 0) return LANG_FR;
        if (strcmp(stored_lang, "en") == 0) return LANG_EN;
        if (strcmp(stored_lang, "ar") == 0) return LANG_AR;
    }

    // Fallback: Default to Arabic
    return LANG_AR;
}

void i18n_set_default_lang(app_lang_t lang) {
    const char *code = i18n_code(lang);
    nvs_write_string("app_lang", code);
    ESP_LOGI(TAG, "Default language set to: %s", code);
}

const char* i18n_tr(app_str_t str_id, app_lang_t lang) {
    if (str_id < 0 || str_id >= STR_TOTAL_COUNT) return "";
    if (lang < 0 || lang >= LANG_MAX) lang = LANG_AR;
    const char *val = s_translations[str_id][lang];
    return val ? val : "";
}

const char* i18n_dir(app_lang_t lang) {
    return (lang == LANG_AR) ? "rtl" : "ltr";
}

const char* i18n_code(app_lang_t lang) {
    switch (lang) {
        case LANG_FR: return "fr";
        case LANG_EN: return "en";
        case LANG_AR:
        default: return "ar";
    }
}

const char* i18n_font(app_lang_t lang) {
    if (lang == LANG_AR) {
        return "'Cairo', 'Tajawal', sans-serif";
    } else {
        return "system-ui, -apple-system, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif";
    }
}

const char* i18n_day_name(int day_idx, app_lang_t lang) {
    if (day_idx < 0 || day_idx >= 7) return "";
    if (lang < 0 || lang >= LANG_MAX) lang = LANG_AR;
    return s_days[day_idx][lang];
}

void i18n_send_lang_bar(httpd_req_t *req, app_lang_t lang) {
    char bar[1024];
    snprintf(bar, sizeof(bar),
        "<div class='lang-bar'>"
        "  <button type='button' onclick=\"changeAppLang('ar')\" class='lang-btn%s'>🇸🇦 العربية</button>"
        "  <button type='button' onclick=\"changeAppLang('fr')\" class='lang-btn%s'>🇫🇷 Français</button>"
        "  <button type='button' onclick=\"changeAppLang('en')\" class='lang-btn%s'>🇬🇧 English</button>"
        "</div>"
        "<script>"
        "function changeAppLang(l){"
        "  document.cookie = 'lang=' + l + '; path=/; max-age=31536000';"
        "  fetch('/setLang?lang=' + l).finally(function(){ location.reload(); });"
        "}"
        "</script>",
        (lang == LANG_AR) ? " active" : "",
        (lang == LANG_FR) ? " active" : "",
        (lang == LANG_EN) ? " active" : "");
    httpd_resp_sendstr_chunk(req, bar);
}
