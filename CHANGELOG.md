# Changelog

Tutte le varianti del firmware per la Longer LK4 Pro stock sono elencate in
[docs/MODIFICHE-FIRMWARE.md](docs/MODIFICHE-FIRMWARE.md).

## [1.0.1] - 20/10/2026

### Correzione

- **La UI del display si avviava solo con un EEPROM non valido**: dopo il Factory Reset
  (che rende l'EEPROM valido) il boot passava da `onLoadSettings` → `loadSettings()`, che non
  impostava `settings_ready` → il display restava **fermo sul logo Marlin** senza avviarsi.
  Ora `settings_ready` viene impostato anche in quel percorso.
  (Bug presente nel fork di partenza, non introdotto dalle modifiche di questo repository.)

### Altro

- `update.bat` / `update.sh` scaricano l'asset **più recente per prefisso**
  (es. `LK4Pro-Stock-Plus-`): non vanno più aggiornati a ogni release.

## [1.0.0] - 20/10/2026

Prima release pubblica del fork "stock" (basato su
[tx3m/Marlin-LK4Pro](https://github.com/tx3m/Marlin-LK4Pro), commit `ad9321a`).

### Variante `LK4Pro-Stock`

Correzioni essenziali per usare il firmware su una **LK4 Pro di serie (senza BLTouch)**:

- **BLTouch disabilitato**: il fork di partenza è tarato su una macchina con sonda; su una
  macchina stock il pin del finecorsa Z veniva letto come sonda BLTouch (polarità invertita) →
  `STOP called because of BLTouch error` → stampante bloccata all'avvio, touch apparentemente morto.
- **Homing Z meccanico** sul finecorsa (pin 35, `LOW` = premuto).
- **Driver estrusore `A4988`** (la macchina stock ha un A4988, il fork era configurato per un
  TMC2208 montato dall'autore).
- **Parametri di movimento identici al firmware factory Longer**: steps `80 80 400 92.6`,
  max feedrate `300 300 5 25` (il fork usava `96` e `500 500 20 60`).
- **Codice DGUS per la configurazione senza sonda**: il ramo `#if !HAS_LEVELING` del fork non
  era mai stato compilato (bug di compilazione e logica di reindirizzamento errata):
  - il menu **Leveling** ora si apre; *Assisted* porta alla guida manuale a 5 punti
  - *Auto level* viene bloccata con il messaggio "Nessuna sonda collegata" invece di fallire in silenzio
  - corrette le guardie `#if HAS_LEVELING` mancanti in 7 file del driver DGUS
- Rimosso il messaggio `#warning` spurio su `Config-export.h`.

### Variante `LK4Pro-Stock-Plus`

Tutto Stock **+**:

- **Z-offset regolabile dal display** (`LKPRO_ZOFFSET_UI`): su una macchina senza sonda l'offset
  sonda (`M851`) è inerte, quindi il firmware usa l'**home offset `M206 Z`** (salvato in EEPROM,
  applicato a ogni homing).
- **Log diagnostico su SD card** (`SD_DIAG_LOG`, comando `M9000`): scrive `LK4LOG<n>.csv` con
  posizione, feedrate e temperature a 4 Hz; si rifiuta di partire durante una stampa.

### Strumenti

- `scripts/flash-lk4pro.py` — flash standalone via protocollo **STK500v2** (`avrdude` non
  sincronizza su questa mainboard: il bootloader ascolta il primo pacchetto per 20-40 ms).
- `scripts/update.bat` / `update.sh` — scarica l'ultimo firmware dalla Release e lo carica.
- `scripts/lk4pro-monitor.py` — dashboard diagnostica per PC.
- CI GitHub Actions: build automatico delle varianti e pubblicazione nelle Release.
