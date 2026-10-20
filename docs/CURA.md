# Configurazione di Cura per il firmware stock

Tutti i valori qui sotto sono quelli **effettivamente compilati** nel firmware (verificati in
`Marlin/Configuration.h` e `Marlin/Configuration_adv.h`), non valori "di solito".

## 1. Connessione (Preferences → Communication)

| Campo | Valore |
| --- | --- |
| Porta | la COM del CH340 (stessa che usi per il flash) |
| **Baudrate** | **250000** (non 115200: il firmware stock di Longer è a 115200) |
| G-code flavor | **Marlin** |

## 2. Macchina (Printer → Machine)

| Campo | Valore |
| --- | --- |
| Larghezza / profondità (X/Y) | **220 x 220 mm** (nel firmware `X_BED_SIZE 222`) |
| Altezza (Z) | **250 mm** |
| Origine | front-left |
| G-code flavor | Marlin |

## 3. Estrusore (Printer → Extruder)

| Campo | Valore |
| --- | --- |
| **E-steps** | **92.6** - identico al firmware, quindi **non serve alcun `M92`** |
| Flow | 100% (per altri filamenti regola il Flow, non gli E-steps) |
| Diametro filamento / ugello | 1.75 / 0.4 |

## 4. Velocità (Printer → Speed)

| Campo | Valore | Nota |
| --- | --- | --- |
| Print speed | **max 250 mm/s** | il firmware limita X/Y a 300 mm/s; 150-200 mm/s è la zona comoda |
| Travel speed | max 250 mm/s | |
| Acceleration | **max 3000 mm/s²** | il firmware ha `DEFAULT_ACCELERATION 3000`; la "initial layer acceleration" puoi tenerla più bassa (500-1000) |
| Z hop | **0** | il limite Z è 5 mm/s: ogni mm di Z hop costa 0,2 s |

> I "jerk" / "scrolling speed" di Cura **non esistono in Marlin**: ignorali.

## 5. Printer Features

| Opzione | Stato | Motivo |
| --- | --- | --- |
| **Power loss recovery** | **ON** | `POWER_LOSS_RECOVERY` è attivo nel firmware ✅ |
| Filament change | OFF | serve il sensore di filamento (non presente sullo stock) |
| Use relative Z | OFF (Cura usa Z assoluto) | vedi nota sullo Z-offset qui sotto |

## 6. Start G-code

```gcode
M140 S{bedtemp}
M104 S{hotendtemp}
G28
```

Opzionale, per il Linear Advance (vedi §8):

```gcode
M900 K0.04
```

**Non aggiungere `G29`**: sulla LK4 Pro stock non c'è la sonda, il `G29` non fa nulla di utile
(la build Plus blocca comunque l'auto-level).

## 7. End G-code

```gcode
M104 S0
M140 S0
G91
G1 Z10 F300
G90
M84
```

## 8. Le funzioni del firmware da sfruttare

### Linear Advance (pressure advance)
- **Attivo nel firmware** (`LIN_ADVANCE`), con K di default **0.22** (il valore che Marlin indica
  per un estrusore direct drive con ~6.5 cm tra ingranaggio e heatbreak).
- **Regolabile dal display**: *Advanced Settings → Linear Advance*. Non serve toccare Cura.
- Se preferisci fissarlo dal PC: `M900 K0.04` (o il valore che trovi con il test della torre).
- Per tararlo: stampa una torre 20x20 mm con K crescenti (0, 0.02, 0.04, 0.06...) e scegli il
  valore in cui gli spigoli non presentano "orecchie" né increspature.

### Arc moves (G2/G3)
`ARC_SUPPORT` e' **attivo nel firmware**, quindi Marlin interpreta e corregge i comandi `G2`/`G3`.

**Cura pero' non emette archi**: verificato nella sorgente ufficiale di Cura (4.13 e 5.x) non
esiste alcuna impostazione per generare G2/G3 - Cura spezza ogni curva in segmenti lineari.
L'opzione "arc" che si vede in qualche guida e' di altri slicer.

Per sfruttare gli archi serve uno slicer che li emetta:
- **PrusaSlicer** o **OrcaSlicer** (supporto archi nativo, profili Marlin gia' disponibili)
- valori da usare per la LK4 Pro stock: volume **222 x 222 x 250 mm**, E-steps **92.6**,
  max feedrate X/Y **300 mm/s**, Z **5 mm/s**, accelerazione **3000 mm/s2**, Z origin front-left.

Con Cura non perdi nulla: `ARC_SUPPORT` resta semplicemente inerte (il firmware e' pronto, ma
l'input non arriva).

### Power loss recovery
Attivalo in Cura (§5): dopo un blackout la stampa riprende da dove si era interrotta.

### Stampa da SD card
La UI permette di stampare dalla SD card inserita nella mainboard: puoi lanciare la stampa dal
display, senza PC.

### Z-offset: due cose diverse
- Nella **build Plus** (`LKPRO_ZOFFSET_UI`) il valore impostato dal display è l'**home offset**
  (`M206 Z`): uno shift del sistema di coordinate, salvato in EEPROM. Dopo l'homing l'ugello
  tocca il piano correttamente e il valore **resta valido tra un riavvio e l'altro**.
- Nella **build Stock** il display fa uno spostamento **fisico** dell'asse Z: valido solo fino al
  prossimo homing.
- In entrambi i casi: **in Cura l'offset Z resta a 0**, altrimenti compensi due volte.
- Cura emette Z assoluto: se sposti il Z a metà stampa, il prossimo movimento assoluto riparte
  dalla posizione comandata → usa il display per l'offset iniziale, non durante la stampa.

## 9. Cose da NON fare

- **Niente `M92`**: gli E-steps sono già corretti nel firmware (92.6).
- **Niente `G29`**: non c'è la sonda.
- **Non superare 250 mm/s** e **3000 mm/s²**: oltre, il firmware li limita da solo (e il display
  mostra "Reduced accuracy").
- **Non lasciare acceso `lk4pro-monitor.py` (o OctoPrint) mentre stai stampando da Cura**: la porta
  seriale è esclusiva e i due programmi si pestano i piedi.
- Non usare `G29`/`M421` (mesh): non c'è una sonda.

## 10. Verifica dopo la configurazione

```
M115     -> versione del firmware (deve dire V47)
M503     -> verifica gli step: deve dire M92 X80.00 Y80.00 Z400.00 E92.60
M206     -> leggibile/scrivibile: qui vedi l'home offset Z
```

Se `M503` non corrisponde, la stampante sta usando un'altra configurazione: rifai
Factory Reset dal display (*Settings → EEPROM*).
