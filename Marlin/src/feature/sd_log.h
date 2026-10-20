/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2023 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Log diagnostico su file SD (solo a stampante ferma).
 *
 * Uso:
 *   M9000 S1   avvia il logging (scrive LK4LOG<n>.csv nella root della SD)
 *   M9000 S0   ferma il logging e chiude il file
 *   M9000      stato del logger
 *
 * Scrivere sulla SD durante una stampa puo' corromperla: per questo il logger
 * si rifiuta di partire se la stampante non e' in idle.
 */
#pragma once

namespace sd_log {

  void start();     // avvia il logging
  void stop();      // ferma e chiude il file
  void status();    // stato
  void tick();      // campionamento (chiamato dal loop principale)

}
