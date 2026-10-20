/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "../gcode.h"
#include "../../module/endstops.h"

#if defined(__AVR__)
  #include "../../HAL/AVR/fastio.h"   // READ_PIN
#endif

/**
 * M119: Output endstop states to serial output
 */
void GcodeSuite::M119() {

  endstops.report_states();

  // Debug: stato elettrico grezzo dei pin dei finecorsa (0 = LOW, 1 = HIGH).
  // Serve a verificare la polarita' reale dei finecorsa di questa stampante.
  SERIAL_ECHOPGM("DEBUG");
  #if PIN_EXISTS(X_STOP)
    SERIAL_ECHOPGM(" X(", (int)X_STOP_PIN, ")=", (int)READ(X_STOP_PIN));
  #endif
  #if PIN_EXISTS(Y_MIN)
    SERIAL_ECHOPGM(" Y_MIN(", (int)Y_MIN_PIN, ")=", (int)READ(Y_MIN_PIN));
  #endif
  #if PIN_EXISTS(Y_MAX)
    SERIAL_ECHOPGM(" Y_MAX(", (int)Y_MAX_PIN, ")=", (int)READ(Y_MAX_PIN));
  #endif
  #if PIN_EXISTS(Z_MIN)
    SERIAL_ECHOPGM(" Z_MIN(", (int)Z_MIN_PIN, ")=", (int)READ(Z_MIN_PIN));
  #endif
  #if PIN_EXISTS(Z_MAX)
    SERIAL_ECHOPGM(" Z_MAX(", (int)Z_MAX_PIN, ")=", (int)READ(Z_MAX_PIN));
  #endif
  SERIAL_EOL();

}
