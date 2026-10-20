# Design — Funzionalità A (Z-offset) e D (diagnostica)

Data: 20/10/2026 · Progetto: firmware Marlin tx3m per Longer LK4 Pro stock (senza BLTouch)

## 1. Riepilogo di comprensione

- **Cosa costruiamo**: (A) uno **Z-offset funzionante dal display** su una macchina senza sonda;
  (D1) un **dashboard diagnostico sul PC**; (D2) un **logger su file SD** a stampante ferma.
- **Perché**: la pagina "Z offset" dello schermo è oggi **inerte** (scrive l'offset sonda `M851`,
  inesistente su questa macchina) → l'offset Z non è correggibile senza ricompilare; e non esiste
  alcuno strumento per osservare il comportamento reale della macchina.
- **Vincoli**: ATmega2560 (8 KB RAM, ~2 KB liberi, ~97 KB flash liberi), Marlin 2.1.x su fork tx3m,
  UI DGUS Reloaded su UART1, TMC2208 in modalità standalone (niente UART → niente StallGuard né
  input shaping), nessuna sonda, A4988 su estrusore.
- **Non obiettivi**: non modificare lo schermo (la C resta per un secondo tempo), non abbandonare
  Marlin, non aggiungere hardware.

## 2. Assunzioni

1. La SD sulla mainboard non viene usata per stampare durante il logging → il logger si attiva solo
   a stampante ferma.
2. Lo Z-offset è un'impostazione **permanente** della macchina (salva in EEPROM con `M500`),
   non un aggiustamento temporaneo.
3. Il dashboard (D1) non può girare insieme a OctoPrint (porta seriale esclusiva) → uso previsto:
   sessioni di diagnosi dedicate.
4. Il segno dell'offset è determinato dal test carta; se è invertito si nega il valore.
5. Il logger scrive ~4 righe/s: adeguato per curve di movimento, non per analisi vibrazioni
   ad alta frequenza (servirebbe un accelerometro → progetto separato).

## 3. Decision Log

| # | Decisione | Alternative scartate | Motivo |
|---|---|---|---|
| 1 | Z-offset = `motion.home_offset.z` (M206) | `M851` con sonda virtuale; spostare `Z_MIN_POS` | Standard Marlin, persistito in EEPROM, applicato a ogni G28 (`HAS_HOME_OFFSET`), nessuna modifica al layout EEPROM |
| 2 | "Leveling" riapre il **menu** (pag. 10) | redirect diretto alla pagina manuale | Con il redirect la pagina Z-offset (11) diventava irraggiungibile; il menu contiene già i 3 bottoni giusti |
| 3 | "Auto level" bloccata con messaggio | lasciarla fallire in silenzio | L'utente capisce subito che serve una sonda |
| 4 | D2: log **su SD, solo a stampante ferma** | log via seriale durante la stampa | Scrivere sulla SD durante una stampa può corromperla (vincolo Marlin) |
| 5 | D1 in **Python** (già installato sul PC) | PowerShell | Dashboard più ricca, CSV comodo, multipiattaforma |
| 6 | Comando custom `M9000` (S0/S1) | 3 comandi separati | Un solo file, meno codice |
| 7 | Ordine di lavoro A → D2 → D1 | tutto insieme | Ogni passo verificato da solo (build + flash + test reale) |

## 4. Design — A (Z-offset dal display)

### 4.1 Comportamento delle pagine

| Pagina | Pulsante | Comportamento |
|---|---|---|
| 9 SETTINGS | "Leveling" | apre il **menu Leveling** (pag. 10) |
| 10 LEVELING_MENU | "Assisted" | → pag. 12 (guida manuale 5 punti) ✅ |
| 10 | "Z offset" | → pag. 11 ✅ **diventa funzionante** |
| 10 | "Auto level" | → **bloccata**: messaggio "sonda non presente" (pag. 13 non esiste per noi) |
| 10 | "Return" | → torna a Impostazioni ✅ |
| 11 LEVELING_OFFSET | input / step up / step down / passo | agiscono su `motion.home_offset.z` |
| 11 | "Home Z" | homing Z (già esistente) |

### 4.2 Implementazione

- `DGUSRxHandler::zOffset` → scrive `motion.home_offset.z` (2 decimali, come il formato del VP)
- `DGUSRxHandler::zOffsetStep` → somma/sottrae il passo a `motion.home_offset.z`
- `DGUSRxHandler::zOffsetSetStep` → imposta la dimensione del passo (0.01 / 0.1 / 1 mm)
- TX di `LEVEL_OFFSET_Current` (VP `0x3106`) → mostra `motion.home_offset.z`
- Persistenza: il salvataggio avviene con `M500` (pagina 28 "EEPROM → Save"), che già scrive
  `motion.home_offset` (`settings.cpp: EEPROM_WRITE(motion.home_offset)`) ✅
- Blocco auto-leveling: in `DGUSRxHandler::screenChange`, se il target è `LEVELING_AUTOMATIC`
  (o `LEVELING_PROBING`) → messaggio di stato e nessun cambio pagina

## 5. Design — D2 (logger su SD)

- Nuovo modulo `src/feature/sd_log.{h,cpp}`
- G-code `M9000 S1` = avvia, `M9000 S0` = ferma, `M9000` = stato
- Scrive `LK4LOG_<n>.csv` nella root della SD: `t_ms,X,Y,Z,E,feedrate,T_hotend,T_bed,stato`
- Frequenza ~4 Hz (250 ms), flush periodico, chiusura del file su `M9000 S0`
- **Blocco di sicurezza**: si rifiuta l'avvio se la stampante non è in idle (mai scritture in stampa)
- File incrementali (`LK4LOG1.csv`, `LK4LOG2.csv`, …) per non sovrascrivere

## 6. Design — D1 (dashboard PC)

- `lk4pro-monitor.py`: apre COM3 @250000, interroga M114/M105/M503 e ascolta le notifiche `//action:`
- Dashboard live: posizione, feedrate, temperature, assi in movimento, ultimo errore
- `--csv file.csv`: salva i campioni con timestamp
- `--once`: snapshot singolo
- Documentato: la porta seriale è esclusiva → non usare insieme a OctoPrint

## 7. Strategia di test

Ogni passo:
1. build (PlatformIO) → verifica assenza errori
2. flash (script STK500v2) → verifica `M115` con la versione nuova
3. test reale sulla stampante via seriale + osservazione dello schermo

| Passo | Test |
|---|---|
| A | impostare Z-offset dal display → `M503`/`M206` riflette il valore → riavvio → valore persistito → il movimento Z post-homing è corretto |
| D2 | `M9000 S1` → file CSV sulla SD con campioni → `M9000 S0` → file chiuso e leggibile |
| D1 | dashboard mostra posizione e temperature coerenti con M114/M105 |
