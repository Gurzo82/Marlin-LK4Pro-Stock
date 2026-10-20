/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2023 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * M9000 - Log diagnostico su SD card
 *   M9000 S1   avvia il logging
 *   M9000 S0   ferma il logging
 *   M9000      stato
 */

#include "../../inc/MarlinConfig.h"

#if ENABLED(SD_DIAG_LOG)

#include "../../feature/sd_log.h"
#include "../gcode.h"

void GcodeSuite::M9000() {
  if (parser.seenval('S')) {
    if (parser.value_int() > 0)
      sd_log::start();
    else
      sd_log::stop();
  }
  else
    sd_log::status();
}

#endif // SD_DIAG_LOG
