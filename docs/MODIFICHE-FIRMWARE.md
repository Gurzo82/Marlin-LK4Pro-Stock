# Modifiche applicate al fork Marlin-LK4Pro (tx3m)

Adattamento del firmware `tx3m/Marlin-LK4Pro` (ramo `lk4pro`, base Marlin `bugfix-2.1.x`)
per una **Longer LK4 Pro stock**: mainboard LGT KIT V1.0 (ATmega2560), **senza BLTouch**,
estrusore con driver **A4988**, X/Y/Z con TMC2208 standalone.

Tutte le modifiche sono nel fork locale in `src/Marlin-LK4Pro/` (git clone completo,
pronto per `git merge origin/bugfix-2.1.x` per gli aggiornamenti upstream).

---

## 1. Configurazione (`Marlin/Configuration.h`)

| Riga | Modifica | Motivo |
|---|---|---|
| 76 | `#define LKPro_BLTOUCH` → commentato | La macchina non ha il BLTouch: attivarlo faceva leggere il finecorsa Z (pin 35) come sonda, con polarità invertita → `STOP called because of BLTouch error` → stampante in stato *killed* |
| 76 (dopo) | aggiunto `#define DGUS_LCD_UI_RELOADED_NO_PROBE` | Opt-in per la configurazione senza sonda (vedi §2) |
| 1330-1347 | `Z_MIN_ENDSTOP_HIT_STATE LOW` ( ramo senza BLTouch, già presente nel fork) | Con BLTouch la polarità è HIGH: la sonda BLTouch è collegata allo stesso pin 35 ma con logica invertita |
| 193 | `E0_DRIVER_TYPE` : `TMC2208_STANDALONE` → **`A4988`** | Lo stock LK4 Pro ha un A4988 per l'estrusore; il fork lo aveva cambiato perché l'autore ha montato un TMC2208 silenzioso |
| 1981 | `INVERT_Z_DIR true` | **Valore ORIGINALE del fork** (durante il lavoro era stato cambiato in `false` per errore, su un test non valido fatto a motore scollegato) |
| 88 | `DEBUG_DGUSLCD` commentato | Era attivo per la diagnosi: flooda la seriale e rovina OctoPrint/Cura |

## 2. Requisiti della UI DGUS Reloaded (`Marlin/src/inc/SanityCheck.h`)

La UI DGUS Reloaded di Marlin *a monte* richiede **sonda + mesh + `BABYSTEP_ZPROBE_OFFSET`**
(che Marlin stesso rimuove automaticamente quando non c'è una sonda: `Conditionals-4-adv.h:120`).
Su una macchina senza sonda i tre check impedivano la compilazione.

Modifiche (tutte protette da `ENABLED(DGUS_LCD_UI_RELOADED_NO_PROBE)`):

```c
#elif !HAS_BED_PROBE && !ENABLED(DGUS_LCD_UI_RELOADED_NO_PROBE)
  #error "DGUS_LCD_UI RELOADED requires a bed probe."
#elif !HAS_MESH && !ENABLED(DGUS_LCD_UI_RELOADED_NO_PROBE)
  #error "DGUS_LCD_UI RELOADED requires mesh leveling."
...
#elif DISABLED(BABYSTEP_ZPROBE_OFFSET) && !ENABLED(DGUS_LCD_UI_RELOADED_NO_PROBE)
  #error "DGUS_LCD_UI RELOADED requires BABYSTEP_ZPROBE_OFFSET."
```

## 3. Codice DGUS per la configurazione senza sonda

Il fork prevedeva già un ramo `#if !HAS_LEVELING` ("Skip Leveling menu if no automatic
leveling") ma **non era mai stato compilato** (l'unica configurazione testata ha il BLTouch)
e conteneva errori. Correzioni:

| File | Modifica |
|---|---|
| `DGUSRxHandler.cpp` | `screen ==` → `screen.getCurrentScreen()` (non esiste un `operator==` tra `DGUSScreenHandler` e `DGUS_ScreenID`; l'API corretta è `DGUSScreenHandler::getCurrentScreen()`) |
| `DGUSRxHandler.cpp` | `const DGUS_ScreenID screenID` → variabile mutabile: il codice deve poter redirigere al livellamento manuale |
| `DGUS_VPList.cpp` | i due VP delle icone di probing (`LEVEL_PROBING_Icons1/2`) spostati dentro `#if HAS_LEVELING` (usano `probing_icons`, che esiste solo con il leveling) |
| `DGUSTxHandler.cpp` | `ablGrid()` e `ablGridColor()` protetti con `#if HAS_LEVELING` (usano `GRID_MAX_POINTS_X` e `probing_icons`) |
| `DGUSSetupHandler.cpp` | `levelingProbing()` protetto con `#if HAS_LEVELING` |
| `DGUSScreenHandler.cpp` | entrambe le versioni di `meshUpdate()` protette con `#if HAS_LEVELING` |
| `dgus_reloaded_extui.cpp` | callback `onMeshUpdate(..., probe_state_t)` protetto con `#if HAS_BED_PROBE` (il tipo `probe_state_t` esiste solo con una sonda) |

## 4. Diagnostica (`Marlin/src/gcode/host/M119.cpp`)

Aggiunta la riga di stato elettrico grezzo dei pin dei finecorsa (utilissima per
verificare polarità e cablaggi su una mainboard non documentata):

```
M119 ->  z_min: open
          DEBUG X(3)=1 Y_MIN(14)=1 Y_MAX(15)=1 Z_MIN(35)=0 Z_MAX(37)=1
```

- `READ(pin)` restituisce lo stato logico grezzo (0/1) del pin
- include aggiunto: `#include "../../HAL/AVR/fastio.h"`

## 5. Allineamento con la LK4 Pro stock (confronto M503 firmware stock vs fork)

I valori sono stati confrontati con il **firmware stock Longer** (`FW_LK4_PRO_0_3_4.hex`, Marlin 1.1.9)
letto via `M503` sulla stessa macchina.

| Parametro | Stock Longer | Fork tx3m (originale) | Configurazione adottata |
|---|---|---|---|
| Steps/mm | `X80 Y80 Z400 E92.60` | `X80 Y80 Z400 E96` | **E = 92.60** (factory) |
| Max feedrate | `X300 Y300 Z5 E25` | `X500 Y500 Z20 E60` | **300 / 300 / 5 / 25** (factory) |
| Max acceleration | `1000 1000 100 3000` | identico | invariato |
| Accel (P/R/T) | `3000 / 3000 / 3000` | identico | invariato |
| PID hotend | `P22.20 I1.08 D114.00` | `P30.69 I2.76 D85.49` | **invariato**: sono i default di Marlin 1.1.9 vs 2.1, non una taratura di macchina (c'è il PID auto-tune a schermo) |
| PID letto | assente | presente | **invariato** (miglior stabilità termica) |
| Impostazioni filamento | disabilitate | attive (lunghezza, carico/scarico) | **invariato** (funzionalità nuova) |
| Driver estrusore | A4988 | `TMC2208_STANDALONE` | **A4988** (stock) |
| Driver X/Y/Z | TMC2208 standalone | TMC2208 standalone | invariato |

## 6. Audit firmware ↔ interfaccia DWIN

Confronto automatico fra le due mappe (repo `tx3m/DWIN_LK4Pro`: `TouchConfig.xls` /
`DisplayConfig.xls` ↔ `DGUS_Addr.h` / `DGUS_ScreenID.h` del firmware):

| Controllo | Esito |
|---|---|
| Pulsanti dello schermo senza handler nel firmware | **0** (su 73 VP toccati) |
| VP "special" `0x40xx`/`0x50xx` (pagine avanzate, volume, luminosità, fan) | **tutti presenti** |
| Pagine dello schermo senza ID corrispondente nel firmware | **0** (i 200/202/204/212/214/216/255 sono i popup DWIN, gestiti a parte) |
| Pagine del firmware assenti dallo schermo | 3: `TEMP_MANUAL`, `VOLUME`, `BRIGHTNESS` → spostate nella pagina 24 dal redesign tx3m |
| Comandi del firmware non attivabili dallo schermo | solo `H1`/`E1` (1 estrusore/1 riscaldatore) e le funzioni BLTouch: non applicabili |

**Schermo e firmware sono quindi perfettamente sincronizzati.**

## 7. Script di flash (`scripts/lk4pro-flash.ps1`)

`avrdude` **non funziona** su questa mainboard: il bootloader Arduino stk500v2 ascolta il
primo pacchetto STK500 per soli **~20-40 ms** dopo il reset, poi salta nell'applicazione.
`avrdude` aspetta ~300 ms → legge il banner di Marlin → `not in sync: resp=0x73`
(`0x73` = 's' di "start").

`lk4pro-flash.ps1` parla **direttamente il protocollo STK500v2**:

1. reset via DTR (identico ad avrdude)
2. `SIGN_ON` (0x01) inviato subito → handshake con il bootloader
3. `LOAD_ADDRESS` (0x06) + `PROGRAM_FLASH_ISP` (0x13) pagina da 256 byte (SPM_PAGESIZE)
4. verifica opzionale con `READ_FLASH_ISP` (0x14)
5. `LEAVE_PROGMODE_ISP` (0x11) → riavvio

Note implementative (protocollo STK500v2):
- il checksum è `MESSAGE_START ^ seq ^ len ^ TOKEN ^ body` (**incluso 0x1B**)
- il body del messaggio **inizia dal byte del comando**
- la risposta del bootloader **echoa il comando** nel body: lo status è `body[1]`, non `body[0]`
- il bootloader cancella le pagine in sequenza: le pagine vanno scritte **in ordine crescente**

## 8. Configurazione risultante (M503)

| Parametro | Valore |
|---|---|
| Steps/mm | X80 Y80 Z400 **E92.60** (valori factory) |
| Max feedrate | X300 Y300 Z5 E25 (valori factory) |
| Max acceleration | X1000 Y1000 Z100 E3000 |
| Baudrate | 250000 (Marlin) / 115200 (bootloader) |
| Driver | X/Y/Z TMC2208 standalone, **E A4988** (stock) |
| Sonda | **nessuna** -> homing Z meccanico (pin 35, LOW = premuto) |
| Auto bed leveling | non disponibile (serve la sonda) -> **usare il livellamento manuale 3x3** |
| PID hotend | P30.69 I2.76 D85.49 (default Marlin 2.1), tarabile dal menu PID |
| PID letto | attivo (P174.73 I34.40 D591.63) |
