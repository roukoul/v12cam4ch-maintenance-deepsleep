// ====================================================================
// Projet : Boitier Controleur de Cloche AepBill
// Configuration : ESP32 + Relais 5V + RTC DS3231 + LCD 1602 I2C + Bouton
// ====================================================================

$fn = 60;

// Dimensions globales
box_w = 140;       // Largeur (X)
box_d = 100;       // Profondeur (Y)
base_h = 32;       // Hauteur cuve (Z)
lid_h = 7;         // Epaisseur couvercle
wall = 4;          // Epaisseur parois
corner_r = 6;      // Rayon des coins

// Vis de fermeture M3
screw_x = box_w - 14;
screw_y = box_d - 14;

// Type de sortie pour les 2 fils de la cloche :
// "round_hole" = Trou rond 8.5mm pour presse-etoupe
// "u_slot"     = Encoche en U usinable directement en 3 axes par le dessus
bell_exit_type = "both"; 

module r_rect(w, d, h, r) {
    hull() {
        translate([r, r, 0]) cylinder(h=h, r=r);
        translate([w-r, r, 0]) cylinder(h=h, r=r);
        translate([r, d-r, 0]) cylinder(h=h, r=r);
        translate([w-r, d-r, 0]) cylinder(h=h, r=r);
    }
}

// --------------------------------------------------------------------
// 1. COUVERCLE SUPERIEUR (FACE AVANT)
// --------------------------------------------------------------------
module couvercle_AepBill() {
    difference() {
        r_rect(box_w, box_d, lid_h, corner_r);
        
        // 1. Gravure Logo "AepBill" (profondeur 0.7mm)
        translate([box_w/2, box_d - 15, lid_h - 0.7]) {
            linear_extrude(height = 1) {
                text("AepBill", size = 10, font = "Arial:style=Bold", halign = "center", valign = "center");
            }
        }
        translate([box_w/2, box_d - 22, lid_h - 0.5]) {
            linear_extrude(height = 1) {
                text("BELL CONTROLLER", size = 3.5, font = "Arial:style=Bold", halign = "center", valign = "center", spacing=1.2);
            }
        }

        // 2. Decoupe Fenetre LCD 1602 (71.5mm x 24.5mm)
        lcd_x = box_w/2 - 12;
        lcd_y = 50;
        translate([lcd_x - 71.5/2, lcd_y - 24.5/2, -1]) {
            cube([71.5, 24.5, lid_h + 2]);
        }
        // 4 trous fixation LCD M3 (entraxe 75 x 31mm)
        for (dx = [-75/2, 75/2]) {
            for (dy = [-31/2, 31/2]) {
                translate([lcd_x + dx, lcd_y + dy, -1]) cylinder(h=lid_h+2, d=3.2);
            }
        }

        // 3. Trou Bouton Poussoir Manuel (Diametre 12.2mm)
        btn_x = box_w - 25;
        btn_y = 48;
        translate([btn_x, btn_y, -1]) cylinder(h=lid_h+2, d=12.2);

        // 4. Trou Bouton Reset / Reboot (Diametre 4mm)
        translate([btn_x, 22, -1]) cylinder(h=lid_h+2, d=4.0);

        // 5. Grille Acoustique Buzzer (Rosace 5 trous de 2mm)
        buz_x = 30;
        buz_y = 20;
        translate([buz_x, buz_y, -1]) cylinder(h=lid_h+2, d=2.2);
        translate([buz_x + 3.5, buz_y, -1]) cylinder(h=lid_h+2, d=2.0);
        translate([buz_x - 3.5, buz_y, -1]) cylinder(h=lid_h+2, d=2.0);
        translate([buz_x, buz_y + 3.5, -1]) cylinder(h=lid_h+2, d=2.0);
        translate([buz_x, buz_y - 3.5, -1]) cylinder(h=lid_h+2, d=2.0);

        // 6. 4 Trous vis de fixation boitier M3 fraisees
        for(x = [-screw_x/2, screw_x/2]) {
            for(y = [-screw_y/2, screw_y/2]) {
                translate([box_w/2 + x, box_d/2 + y, -1]) {
                    cylinder(h=lid_h+2, d=3.4);
                    translate([0, 0, lid_h - 2]) cylinder(h=3.5, d1=3.4, d2=6.8);
                }
            }
        }
    }
}

// --------------------------------------------------------------------
// 2. CUVE INFERIEURE (BASE AVEC LOGEMENTS COMPOSANTS)
// --------------------------------------------------------------------
module base_AepBill() {
    difference() {
        r_rect(box_w, box_d, base_h, corner_r);
        
        // Poche principale interieure
        translate([wall, wall, wall]) {
            r_rect(box_w - 2*wall, box_d - 2*wall, base_h, corner_r - 2);
        }
        
        // ------------------------------------------------------------
        // SORTIE DES 2 FILS DE LA CLOCHE (RELAIS) - FLANC DROIT
        // ------------------------------------------------------------
        // A) Trou cylindrique pour presse-etoupe PG7 ou passe-fil (diametre 8.5 mm)
        translate([box_w - wall - 1, 35, 14]) {
            rotate([0, 90, 0]) cylinder(h=wall + 2, d=8.5);
        }
        
        // B) Encoche en U superieure (facilite l'usinage 3 axes direct par le dessus)
        translate([box_w - wall - 1, 35 - 3.5, base_h - 8]) {
            cube([wall + 2, 7, 9]);
        }

        // ------------------------------------------------------------
        // ENTREE ALIMENTATION 5V - FLANC GAUCHE
        // ------------------------------------------------------------
        translate([-1, 35, 14]) {
            rotate([0, 90, 0]) cylinder(h=wall + 2, d=8.5);
        }

        // 4 Trous taraudes M3 de fermeture
        for(x = [-screw_x/2, screw_x/2]) {
            for(y = [-screw_y/2, screw_y/2]) {
                translate([box_w/2 + x, box_d/2 + y, base_h - 12]) {
                    cylinder(h=13, d=2.9);
                }
            }
        }
    }
    
    // Plots de fixation internes pour visser la carte Relais 5V
    translate([box_w - 45, 20, wall]) cylinder(h=4, d=6);
    translate([box_w - 15, 20, wall]) cylinder(h=4, d=6);
}

// Visualisation
translate([0, 0, 0]) couvercle_AepBill();
translate([box_w + 15, 0, 0]) base_AepBill();
