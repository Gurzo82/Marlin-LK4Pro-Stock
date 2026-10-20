/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2023 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Log diagnostico su file SD. Vedi sd_log.h per l'uso.
 */

#include "../inc/MarlinConfig.h"

#if ENABLED(SD_DIAG_LOG)

#include "../gcode/gcode.h"
#include "../module/motion.h"
#include "../module/printcounter.h"
#include "../module/temperature.h"
#include "../sd/cardreader.h"

namespace sd_log {

  static bool logging = false;
  static uint16_t samples = 0;
  static char filename[16] = "";

  static constexpr uint16_t SAMPLE_INTERVAL_MS = 250;   // 4 Hz
  static constexpr uint8_t  SYNC_EVERY = 8;             // flush sulla SD ogni 2 s

  void start() {
    if (logging) {
      SERIAL_ECHOLNPGM("SD log: gia' attivo");
      return;
    }

    // Scrivere sulla SD durante una stampa e' pericoloso: ci si rifiuta.
    if (card.isPrinting() || card.isPaused() || print_job_timer.isRunning()) {
      SERIAL_ECHOLNPGM("SD log: impossibile, la stampante non e' in idle");
      return;
    }

    if (!card.isMounted()) {
      SERIAL_ECHOLNPGM("SD log: nessuna SD montata");
      return;
    }

    // Trova il primo nome libero: LK4LOG1.csv, LK4LOG2.csv, ...
    bool opened = false;
    for (uint8_t i = 1; i < 100; i++) {
      snprintf(filename, sizeof(filename), "LK4LOG%u.csv", i);
      if (card.openDiagLogFile(filename)) { opened = true; break; }
    }
    if (!opened) {
      SERIAL_ECHOLNPGM("SD log: impossibile aprire il file");
      return;
    }

    // Intestazione CSV
    card.write((void *)"t_ms,X,Y,Z,feedrate,T_hotend,T_bed\n", 38);

    logging = true;
    samples = 0;
    SERIAL_ECHOLNPGM("SD log: avviato su ", filename);
  }

  void stop() {
    if (!logging) {
      SERIAL_ECHOLNPGM("SD log: non attivo");
      return;
    }
    logging = false;
    card.closeDiagLogFile();
    SERIAL_ECHOLNPGM("SD log: fermato, ", samples, " campioni in ", filename);
  }

  void status() {
    if (!logging) {
      SERIAL_ECHOLNPGM("SD log: inattivo");
      return;
    }
    SERIAL_ECHOLNPGM("SD log: attivo, ", samples, " campioni, file ", filename);
  }

  void tick() {
    if (!logging) return;

    static uint32_t last = 0;
    const uint32_t now = millis();
    if (now - last < SAMPLE_INTERVAL_MS) return;
    last = now;

    char line[80];
    const int len = snprintf(line, sizeof(line), "%lu,%.2f,%.2f,%.2f,%.1f,%.1f,%.1f\n",
                             (unsigned long)now,
                             (double)motion.position.x, (double)motion.position.y, (double)motion.position.z,
                             (double)motion.feedrate_mm_s,
                             (double)thermalManager.temp_hotend[0].celsius,
                             (double)thermalManager.temp_bed.celsius);
    if (len > 0)
      card.write((void *)line, (uint16_t)len);

    if (++samples % SYNC_EVERY == 0)
      card.syncDiagLogFile();
  }

}

#endif // SD_DIAG_LOG
