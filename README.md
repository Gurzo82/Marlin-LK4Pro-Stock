# Marlin firmware per Longer LK4 Pro **stock** (senza BLTouch)

> Fork di **[tx3m/Marlin-LK4Pro](https://github.com/tx3m/Marlin-LK4Pro)** (a sua volta fork di
> [MarlinFirmware/Marlin](https://github.com/MarlinFirmware/Marlin), ramo `bugfix-2.1.x`).
> Tutte le modifiche rispetto all'upstream sono elencate in **[docs/MODIFICHE-FIRMWARE.md](docs/MODIFICHE-FIRMWARE.md)**.

[![Licenza](https://img.shields.io/badge/licenza-GPL--3.0-blue.svg)](LICENSE)
![Firmware](https://img.shields.io/badge/firmware-Marlin%202.1.x-green.svg)
![Schermo](https://img.shields.io/badge/UI-DWIN%20Reloaded-blue.svg)

---

## Il problema

Il firmware [tx3m/Marlin-LK4Pro](https://github.com/tx3m/Marlin-LK4Pro) è ottimo, ma è tarato su una
LK4 Pro **con BLTouch** (l'autore ha montato la sonda). Sulla **LK4 Pro di serie, senza sonda**, quel
firmware:

| Sintomo | Causa |
|---|---|
| `Error:!! STOP called because of BLTouch error` → la stampante si blocca all'avvio | il pin del finecorsa Z viene letto come sonda BLTouch, con polarità invertita |
| L'homing Z non trova mai il finecorsa | idem: la "sonda" non risponde |
| Il menu **Leveling** / **Assisted** non fa nulla | nel ramo "senza sonda" (mai compilato da nessuno) c'erano bug |
| L'estrusore si muove in modo strano | configurato come `TMC2208`, mentre la macchina stock ha un **A4988** |
| Estrazione e velocità fuori rispetto a com'erano in fabbrica | i parametri di movimento erano quelli della stampante dell'autore |

In più: **su questa mainboard `avrdude` non riesce a flashare** (vedi [Nota sul bootloader](#nota-sul-bootloader)).

Questo repository rende il firmware **utilizzabile al 100% su una LK4 Pro stock**, con i parametri
motori identici a quelli di fabbrica e tre strumenti nuovi.

---

## Le due varianti del firmware

| Variante | File | Include |
|---|---|---|
| **Stock** | `LK4Pro-Stock-V41.hex` | correzioni essenziali per la LK4 Pro **di serie**: BLTouch disabilitato, driver estrusore **A4988**, steps/feedrate **identici al factory**, fix del codice DGUS (menu Leveling funzionante, "Auto level" bloccato con messaggio) |
| **Plus** | `LK4Pro-Stock-Plus-V41.hex` | tutto Stock **+** Z-offset regolabile dal display **+** log diagnostico su SD card (`M9000`) |

La differenza è una riga di configurazione:

```ini
[env:LK4Pro_Plus]
extends       = env:LK4Pro
build_flags   = ${common_avr8.build_flags} -DLKPRO_ZOFFSET_UI -DSD_DIAG_LOG
```

> Se non sei sicuro: **"Plus"** è la versione più completa, "Stock" è la più sobria.

---

## Avvio rapido

**1.** Scarica questo repository (o clona):
```bash
git clone https://github.com/Gurzo82/Marlin-LK4Pro-Stock.git
```

**2.** Collega la stampante al PC con il cavo **micro-USB** e accendila.

> ⚠️ **Isola il pin 5V/VCC** del cavo USB con nastro adesivo (istruzione ufficiale Longer): la mainboard
> è alimentata dalla stampante, l'USB non deve alimentarla.

**3.** Lancia lo script di aggiornamento:

| Sistema | Comando |
|---|---|
| Windows | doppio click su `scripts\update.bat` (sceglie la variante) |
| Linux / macOS | `bash scripts/update.sh` (sceglie la variante) |

Lo script **scarica l'ultimo firmware già compilato** dalle Release, lo carica sulla stampante e
verifica l'installazione. Non serve PlatformIO né Arduino IDE.

Requisiti: **Python 3.8+** e `pyserial` (`pip install pyserial`).

Se preferisci caricare un file specifico:
```bash
python scripts/flash-lk4pro.py firmware/LK4Pro-Stock-Plus-V41.hex
python scripts/flash-lk4pro.py --port COM5            # porta specifica
```

---

## Firmware dello **schermo** (DWIN)

Il nuovo firmware del display va installato **separatamente** e **insieme** a questo firmware
(schermo e firmware devono essere abbinati).

1. micro SD **≤ 32 GB**, formattata **FAT32** con **4096 byte** di unità di allocazione
   (`format X: /FS:FAT32 /Q /A:4096`)
2. Copia la cartella **`DWIN_SET`** (in `dwin/DWIN_SET.zip`) nella **radice** della SD
3. Stampante **spenta** → inserisci la SD nello slot **dietro il display** → accendi
4. Attendi **"SD CARD PROCESS... END!"** → spegni, togli la SD, riaccendi

> I display con versione "TL5", "DWJT" o "DWJTB" **non** sono compatibili.

---

## Cosa cambia rispetto all'upstream

| Area | Upstream | Qui |
|---|---|---|
| BLTouch | attivo (blocca la stampante stock) | **disabilitato** → homing Z meccanico sul pin 35 |
| Driver estrusore | `TMC2208_STANDALONE` | **`A4988`** (come la macchina stock) |
| Steps/mm | `80 80 400 **96**` | `80 80 400 **92.6**` (identico al firmware factory Longer) |
| Max feedrate | `500 500 20 60` | **`300 300 5 25`** (identico al factory) |
| Menu Leveling | non funziona | funziona: *Assisted* (guida manuale 5 punti), *Z offset* reale, *Auto level* bloccato con messaggio |
| Offset Z | solo con sonda (inerto) | **regolabile dal display** (home offset `M206 Z`, salvato in EEPROM) |
| Log diagnostico | — | **`M9000`** → CSV su SD (posizione, feedrate, temperature) |
| Dashboard PC | — | **`scripts/lk4pro-monitor.py`** (live + CSV) |
| Flash | avrdude (non funziona qui) | **`scripts/flash-lk4pro.py`** (protocollo STK500v2 diretto) |

Dettaglio completo: **[docs/MODIFICHE-FIRMWARE.md](docs/MODIFICHE-FIRMWARE.md)**.

### Funzionalità nuove in dettaglio

**Z-offset dal display** — su una macchina senza sonda l'offset sonda (`M851`) è inerte: qui il firmware
usa l'**home offset `M206 Z`**, che viene salvato in EEPROM e applicato a ogni homing.
Procedura: *Home* → foglio di carta sul letto → *Move → Z* a passi di 0,1 mm fino al contatto →
*Leveling → Z offset* → imposta il valore → *Home* di nuovo.

**Log su SD card** (`M9000 S1` / `M9000` / `M9000 S0`) — scrive `LK4LOG<n>.csv` con
`t_ms, X, Y, Z, feedrate, T_hotend, T_bed` a 4 Hz. Si rifiuta di partire durante una stampa
(scrivere sulla SD in stampa può corromperla).

**Dashboard PC** (`scripts/lk4pro-monitor.py`) — posizione, feedrate, temperature e ultime
notifiche in tempo reale; `--csv` salva i campioni con timestamp.
La porta seriale è esclusiva: non usarlo insieme a OctoPrint/Cura.

---

## ⚠️ Peculiarità della mainboard: il cavo USB "stordisce" il display

Su questa mainboard (LGT KIT V1.0) la linea **DTR** del CH340 è collegata anche al **RX del display DWIN**.
Se il DTR resta attivo (è il caso dopo un reset via seriale, quindi dopo **qualsiasi** flash con
`avrdude` o con `flash-lk4pro.py`), il display **non riceve più i comandi** di Marlin e resta fermo
sulla pagina di boot (logo Marlin), anche se il resto funziona: Marlin parte, la stampante è
utilizzabile da PC, i touch dal display *eventuali* arrivano (lo screen→Marlin è libero), ma
**Marlin→display non funziona**.

**Soluzione**: stacca (e se necessario riattacca) il cavo USB dopo il flash.
`flash-lk4pro.py` rilascia già DTR/RTS alla fine ✅, ma per sicurezza conviene staccare il cavo.

Sintomo riconoscibile: `M115` risponde e la stampante è viva, ma il display non cambia pagina.

## Nota sul bootloader
La mainboard usa il bootloader Arduino **stk500v2** standard, ma questo ascolta il primo pacchetto
STK500 per **soltanto ~20-40 ms** dopo il reset: dopo, salta nell'applicazione. `avrdude` aspetta
~300 ms prima di parlare, riceve quindi il banner di Marlin (`start`) e fallisce con
`not in sync: resp=0x73` (`0x73` = 's' di "start").

`scripts/flash-lk4pro.py` parla direttamente il protocollo STK500v2: entra nel bootloader e invia
subito `SIGN_ON`, poi scrive le pagine da 256 byte (`SPM_PAGESIZE` dell'ATmega2560).
È lo stesso protocollo usato da Cura/Longer Slicer, solo sincronizzato sul tempo giusto.

> Nota sul protocollo STK500v2, se vuoi implementarlo tu:
> il checksum è `MESSAGE_START ^ seq ^ len ^ TOKEN ^ body` (**incluso 0x1B**), il body del messaggio
> **inizia dal byte del comando** e la risposta **echoa il comando**: lo status è `body[1]`, non `body[0]`.

---

## Compilare dal sorgente

```bash
# 1. toolchain
pip install platformio
pio pkg install -g -t tool-avrdude

# 2. build
pio run -e LK4Pro            # build
pio run -e LK4Pro_SilentE    # build con estrusore TMC2208 silenzioso
```

Il numero di versione (`V<N>`) viene calcolato automaticamente da git. L'archivio delle build sta in
`.pio/firmware/`.

Il CI (`.github/workflows/build.yml`) compila il firmware a ogni push e pubblica il `.hex` nelle Release.

---

## Struttura del repository

```
.
├── Marlin/                 sorgente Marlin (fork di MarlinFirmware/Marlin)
├── buildroot/  ini/  platformio.ini
├── scripts/
│   ├── flash-lk4pro.py     flash via protocollo STK500v2 (standalone)
│   ├── update.bat/.sh      scarica l'ultimo firmware dalla Release e lo carica
│   ├── lk4pro-monitor.py   dashboard diagnostica
│   └── build.ps1/.sh       build da sorgente
├── firmware/               firmware precompilato
├── dwin/                   firmware dello schermo (DWIN_SET.zip)
└── docs/                   MODIFICHE-FIRMWARE.md, DESIGN.md
```

---

## Crediti

- **[tx3m/Marlin-LK4Pro](https://github.com/tx3m/Marlin-LK4Pro)** — il fork da cui questo nasce
  (UI DWIN ridisegnata, adattamento alla LK4 Pro)
- **[Guizz27/Marlin---Longer-LKx-Pro](https://github.com/Guizz27/Marlin---Longer-LKx-Pro)** e
  **[Guizz27/DWIN_LK4Pro](https://github.com/Guizz27/DWIN_LK4Pro)** — port originale e UI
- **[jgbrown54](https://github.com/jgbrown54)** — aggiornamento a Marlin 2.1.2.1 e redesign dei menu
- **[MarlinFirmware/Marlin](https://github.com/MarlinFirmware/Marlin)** — Marlin `bugfix-2.1.x`
- **Longer3D** — hardware, firmware stock e materiale originale (GPL-3.0)

## Licenza

GPL-3.0, come Marlin e come il fork di partenza. Vedi [LICENSE](LICENSE).
I file distribuiti con questa licenza devono mantenere la stessa licenza e la stessa fonte.
