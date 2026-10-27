( ======================================================== )
( G-CODE TEST : GRAVURE DU LOGO AepBill                    )
( Outil : Fraise V-Bit 60 degres                           )
( Broche : 12000 RPM | Avance : 800 mm/min | Plongee : 250 )
( Origine Z0 : Surface superieure                          )
( ======================================================== )

G21 G90 G94 G17
G0 Z5.000
M3 S12000
G4 P2.0

( --- Debut Gravure Logo --- )
G0 X45.000 Y16.000
G1 Z-0.600 F250
G1 X49.000 Y6.000 F800
G1 X53.000 Y16.000
G0 Z2.000
G0 X47.000 Y11.000
G1 Z-0.600 F250
G1 X51.000 Y11.000 F800
G0 Z5.000

( --- Fin du programme --- )
G0 X0 Y0
M5
M30

