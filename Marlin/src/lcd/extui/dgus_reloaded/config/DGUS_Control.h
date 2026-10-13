/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2021 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
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
#pragma once

/**
 * tx3m - Touch control numbers for the LK4 Pro screen (tx3m/DWIN_LK4Pro).
 * The display counts all touch areas of a page from 0, in the order of DWIN_SET/13_touch.bin
 * (confirmed with the DGUS debug log on the file list). These differ from the official
 * DGUS Reloaded screen, so they must follow the screen firmware.
 */
enum class DGUS_Control : uint8_t {

  // PRINT
  FILE0       = 2, // RETURN_KEY_CODE
  FILE1       = 3, // RETURN_KEY_CODE
  FILE2       = 4, // RETURN_KEY_CODE
  FILE3       = 5, // RETURN_KEY_CODE
  FILE4       = 6, // RETURN_KEY_CODE
  GO_BACK     = 7, // RETURN_KEY_CODE
  SCROLL_UP   = 8, // RETURN_KEY_CODE
  SCROLL_DOWN = 0, // RETURN_KEY_CODE

  // PRINT_STATUS
  PAUSE       = 4, // POPUP_WINDOW
  RESUME      = 5, // POPUP_WINDOW

  // LEVELING_AUTOMATIC
  DISABLE     = 5, // RETURN_KEY_CODE (not on this screen, see ablDisableIcon())

  // SETTINGS_MENU2
  EXTRA2      = 6, // RETURN_KEY_CODE (not on this screen)

  // WAIT
  ABORT       = 0, // POPUP_WINDOW
  CONTINUE    = 1  // RETURN_KEY_CODE

};
