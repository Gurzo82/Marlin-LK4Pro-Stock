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
 * jgbrown54 - Added status messages for Save and Restore EEPROM
 */

#include "../../../inc/MarlinConfigPre.h"

#if DGUS_LCD_UI_RELOADED

#include "DGUSScreenHandler.h"

#include "DGUSDisplay.h"
#include "definition/DGUS_ScreenAddrList.h"
#include "definition/DGUS_ScreenSetup.h"

#include "../../../gcode/queue.h"
#include "../../../module/printcounter.h"

uint8_t DGUSScreenHandler::debug_count = 0;

#if HAS_MEDIA
  ExtUI::FileList DGUSScreenHandler::filelist;
  uint16_t DGUSScreenHandler::filelist_offset = 0;
  int16_t DGUSScreenHandler::filelist_selected = -1;
#endif

DGUS_Data::StepSize DGUSScreenHandler::offset_steps = DGUS_Data::StepSize::MMP1;
DGUS_Data::StepSize DGUSScreenHandler::move_steps = DGUS_Data::StepSize::MM10;

#if HAS_LEVELING
  uint16_t DGUSScreenHandler::probing_icons[] = { 0, 0 };
  float DGUSScreenHandler::probing_values[GRID_MAX_POINTS_X][GRID_MAX_POINTS_Y];
  #else
  float DGUSScreenHandler::baby_offset = 0;
#endif

DGUS_Data::Extruder DGUSScreenHandler::filament_extruder = DGUS_Data::Extruder::CURRENT;
uint16_t DGUSScreenHandler::filament_length = DGUS_DEFAULT_FILAMENT_LEN;

char DGUSScreenHandler::gcode[] = "";

DGUS_Data::Heater DGUSScreenHandler::pid_heater = DGUS_Data::Heater::H0;
celsius_t DGUSScreenHandler::pid_temp = DGUS_PLA_TEMP_HOTEND;
uint8_t DGUSScreenHandler::pid_cycles = 5;

bool DGUSScreenHandler::settings_ready = false;
bool DGUSScreenHandler::booted = false;

DGUS_ScreenID DGUSScreenHandler::current_screenID = DGUS_ScreenID::BOOT;
DGUS_ScreenID DGUSScreenHandler::new_screenID = DGUS_ScreenID::BOOT;
bool DGUSScreenHandler::full_update = false;

DGUS_ScreenID DGUSScreenHandler::wait_return_screenID = DGUS_ScreenID::HOME;
bool DGUSScreenHandler::wait_continue = false;

bool DGUSScreenHandler::print_aborted = false;  // tx3m

bool DGUSScreenHandler::leveling_active = false;

millis_t DGUSScreenHandler::status_expire = 0;
millis_t DGUSScreenHandler::eeprom_save = 0;

void DGUSScreenHandler::init() {
  dgus.init();
  if (booted)
    triggerFullUpdate(); // Reinit LCD hardware then refresh current screen
  else
    moveToScreen(DGUS_ScreenID::BOOT, true);
}

void DGUSScreenHandler::ready() {
  dgus.playSound(1);
}

void DGUSScreenHandler::loop() {
  if (!settings_ready || current_screenID == DGUS_ScreenID::KILL)
    return;

  const millis_t ms = ExtUI::safe_millis();
  static millis_t next_event_ms = 0;

  if (new_screenID != DGUS_ScreenID::BOOT) {
    const DGUS_ScreenID screenID = new_screenID;
    new_screenID = DGUS_ScreenID::BOOT;
    if (current_screenID == screenID)
      triggerFullUpdate();
    else
      moveToScreen(screenID);
    return;
  }

  if (!booted && ELAPSED(ms, 3000)) {
    booted = true;

    dgus.readVersions();

    if (current_screenID == DGUS_ScreenID::BOOT)
      moveToScreen(DGUS_ScreenID::HOME);

    return;
  }

  if (ELAPSED(ms, next_event_ms) || full_update) {
    next_event_ms = ms + DGUS_UPDATE_INTERVAL_MS;

    if (!sendScreenVPData(current_screenID, full_update))
      DEBUG_ECHOLNPGM("sendScreenVPData failed");

    return;
  }

  if (current_screenID == DGUS_ScreenID::WAIT
      && ((wait_continue && !marlin.wait_for_user) || (!wait_continue && isPrinterIdle()))
  ) {
    moveToScreen(wait_return_screenID, true);
    return;
  }

  #if HAS_MESH
    if (current_screenID == DGUS_ScreenID::LEVELING_PROBING && isPrinterIdle()) {
      dgus.playSound(3);

      setStatusMessage(ExtUI::getLevelingIsValid() ? GET_TEXT_F(DGUS_MSG_PROBING_SUCCESS) : GET_TEXT_F(DGUS_MSG_PROBING_FAILED));

      moveToScreen(DGUS_ScreenID::LEVELING_AUTOMATIC);
      return;
    }
  #endif

  if (status_expire > 0 && ELAPSED(ms, status_expire)) {
    setStatusMessage(FPSTR(NUL_STR), 0);
    return;
  }

  if (eeprom_save > 0 && ELAPSED(ms, eeprom_save) && isPrinterIdle()) {
    eeprom_save = 0;
    queue.enqueue_now(F("M500"));
    return;
  }

  dgus.loop();
}

void DGUSScreenHandler::printerKilled(FSTR_P const error, FSTR_P const component) {
  setMessageLine(error, 1);
  setMessageLine(component, 2);
  setMessageLine_P(NUL_STR, 3);
  setMessageLine(GET_TEXT_F(MSG_PLEASE_RESET), 4);

  dgus.playSound(3, 1, 200);

  moveToScreen(DGUS_ScreenID::KILL, true);
}

void DGUSScreenHandler::userConfirmRequired(const char * const msg) {
  // tx3m - While a paused print is parked, stay on PRINT_STATUS. Its Resume button continues.
  if (isParkedForResume()) {
    setStatusMessage(msg, 0);
    dgus.playSound(3);
    return;
  }

  setMessageLine_P(NUL_STR, 1);
  setMessageLine(msg, 2);
  setMessageLine_P(NUL_STR, 3);
  setMessageLine_P(NUL_STR, 4);

  dgus.playSound(3);

  showWaitScreen(current_screenID, true);
}

void DGUSScreenHandler::settingsReset() {
  dgus.setVolume(DGUS_DEFAULT_VOLUME);

  DGUSDisplay::displayDimmed = false;  // tx3m
  dgus.setBrightness(DGUS_DEFAULT_BRIGHTNESS);

  if (!settings_ready) {
    settings_ready = true;
    ready();
  }

  setStatusMessage(GET_TEXT_F(DGUS_MSG_RESET_EEPROM));
}

void DGUSScreenHandler::storeSettings(char *buff) {
  eeprom_data_t data;

  static_assert(sizeof(data) <= ExtUI::eeprom_data_size, "sizeof(eeprom_data_t) > eeprom_data_size.");

  data.initialized = true;
  data.volume = dgus.getVolume();
  data.brightness = dgus.getBrightness();

  data.abl_okay = false;
  #if HAS_MESH
    data.abl_okay = (ExtUI::getLevelingActive() && ExtUI::getLevelingIsValid());
  #endif

  memcpy(buff, &data, sizeof(data));
}

void DGUSScreenHandler::loadSettings(const char *buff) {
  eeprom_data_t data;

  static_assert(sizeof(data) <= ExtUI::eeprom_data_size, "sizeof(eeprom_data_t) > eeprom_data_size.");

  memcpy(&data, buff, sizeof(data));

  dgus.setVolume(data.initialized ? data.volume : DGUS_DEFAULT_VOLUME);

  DGUSDisplay::displayDimmed = false;  // tx3m
  // dgus.setBrightness(data.initialized ? data.brightness : DGUS_DEFAULT_BRIGHTNESS);
  dgus.setBrightness(DGUSDisplay::getBrightness());

  if (data.initialized) {
	leveling_active = false;
	#if HAS_MESH
    leveling_active = (data.abl_okay && ExtUI::getLevelingIsValid());
    ExtUI::setLevelingActive(leveling_active);
	#endif
  }

  // tx3m - Al boot con un EEPROM valido si passa da loadSettings() e non da
  // settingsReset(): senza questo settings_ready resta false, il loop della UI
  // non parte e il display resta fermo sulla pagina di boot (logo Marlin).
  if (!settings_ready) {
	settings_ready = true;
	ready();
  }
}

void DGUSScreenHandler::configurationStoreWritten(bool success) {
  if (!success)
    setStatusMessage(GET_TEXT_F(DGUS_MSG_WRITE_EEPROM_FAILED));
  else
    setStatusMessage(GET_TEXT_F(DGUS_MSG_EEPROM_SAVED));
}

void DGUSScreenHandler::configurationStoreRead(bool success) {
  if (!success) {
    setStatusMessage(GET_TEXT_F(DGUS_MSG_READ_EEPROM_FAILED));
  }
  else if (!settings_ready) {
    settings_ready = true;
    ready();
    setStatusMessage(GET_TEXT_F(DGUS_MSG_EEPROM_LOADED));
  }
}

void DGUSScreenHandler::playTone(const uint16_t frequency, const uint16_t duration/*=0*/) {
  if (WITHIN(frequency, 1, 255)) {
    if (WITHIN(duration, 1, 255))
      dgus.playSound((uint8_t)frequency, (uint8_t)duration);
    else
      dgus.playSound((uint8_t)frequency);
  }
}

void DGUSScreenHandler::meshUpdate(const int8_t xpos, const int8_t ypos) {
#if HAS_LEVELING
  if (current_screenID != DGUS_ScreenID::LEVELING_PROBING) {
    if (current_screenID == DGUS_ScreenID::LEVELING_AUTOMATIC)
      triggerFullUpdate();
    return;
  }

  uint8_t point = ypos * GRID_MAX_POINTS_X + xpos;
  probing_icons[point < 16 ? 0 : 1] |= (1U << (point % 16));

  if (xpos >= GRID_MAX_POINTS_X - 1 && ypos >= GRID_MAX_POINTS_Y - 1 && !ExtUI::getLevelingIsValid())
    probing_icons[0] = probing_icons[1] = 0;

  triggerFullUpdate();
#endif
}

// tx3m added a version that takes also the zval as a param
void DGUSScreenHandler::meshUpdate(const int8_t xpos, const int8_t ypos, const float zval) {
#if HAS_LEVELING
  if (current_screenID != DGUS_ScreenID::LEVELING_PROBING) {
    if (current_screenID == DGUS_ScreenID::LEVELING_AUTOMATIC)
      triggerFullUpdate();
    return;
  }

  probing_values[xpos][ypos] = zval;
  uint8_t point = ypos * GRID_MAX_POINTS_X + xpos;
  probing_icons[point < 16 ? 0 : 1] |= (1U << (point % 16));

  if (xpos >= GRID_MAX_POINTS_X - 1 && ypos >= GRID_MAX_POINTS_Y - 1 && !ExtUI::getLevelingIsValid())
    probing_icons[0] = probing_icons[1] = 0;

  triggerFullUpdate();
#endif
}

void DGUSScreenHandler::printTimerStarted() {
  print_aborted = false;  // tx3m
  if (print_job_timer.isPaused()) setStatusMessage(FPSTR(NUL_STR), 0);  // tx3m - Resuming: clear "Print Paused"
  triggerScreenChange(DGUS_ScreenID::PRINT_STATUS);
  #if !HAS_LEVELING  
    baby_offset = 0;
  #endif
}

void DGUSScreenHandler::printTimerPaused() {
  // tx3m - Also shows a pause from the host (M76 in its pause script)
  setStatusMessage(GET_TEXT_F(MSG_PRINT_PAUSED), 0);
  dgus.playSound(3);
  triggerFullUpdate();
}

void DGUSScreenHandler::printTimerStopped() {
  if (current_screenID != DGUS_ScreenID::PRINT_STATUS && current_screenID != DGUS_ScreenID::PRINT_ADJUST)
    return;

  dgus.playSound(3);

  triggerScreenChange(DGUS_ScreenID::PRINT_FINISHED);
}

// tx3m
void DGUSScreenHandler::printFinished() {
}

void DGUSScreenHandler::filamentRunout(const ExtUI::extruder_t extruder) {
  char buffer[21];

  snprintf_P(buffer, sizeof(buffer), GET_TEXT(DGUS_MSG_FILAMENT_RUNOUT), extruder);

  setStatusMessage(buffer);

  dgus.playSound(3);
}

#if HAS_MEDIA

  void DGUSScreenHandler::sdCardInserted() {
    if (current_screenID == DGUS_ScreenID::HOME)
      triggerScreenChange(DGUS_ScreenID::PRINT);
  }

  void DGUSScreenHandler::sdCardRemoved() {
    if (current_screenID == DGUS_ScreenID::PRINT)
      triggerScreenChange(DGUS_ScreenID::HOME);
  }

  void DGUSScreenHandler::sdCardError() {
    setStatusMessage(GET_TEXT_F(MSG_MEDIA_READ_ERROR));
    if (current_screenID == DGUS_ScreenID::PRINT)
      triggerScreenChange(DGUS_ScreenID::HOME);
  }

#endif // HAS_MEDIA

#if ENABLED(POWER_LOSS_RECOVERY)
  void DGUSScreenHandler::powerLossResume() {
    moveToScreen(DGUS_ScreenID::POWERLOSS, true);
  }
#endif

#if HAS_PID_HEATING

  void DGUSScreenHandler::pidTuning(const ExtUI::pidresult_t rst) {
    switch (rst) {
      case ExtUI::PID_STARTED:
      case ExtUI::PID_BED_STARTED:
      case ExtUI::PID_CHAMBER_STARTED:
        setStatusMessage(GET_TEXT_F(MSG_PID_AUTOTUNE));
        break;
      case ExtUI::PID_BAD_HEATER_ID:
        setStatusMessage(GET_TEXT_F(MSG_PID_BAD_HEATER_ID));
        break;
      case ExtUI::PID_TEMP_TOO_HIGH:
        setStatusMessage(GET_TEXT_F(MSG_PID_TEMP_TOO_HIGH));
        break;
      case ExtUI::PID_TUNING_TIMEOUT:
        setStatusMessage(GET_TEXT_F(MSG_PID_TIMEOUT));
        break;
      case ExtUI::PID_DONE:
        setStatusMessage(GET_TEXT_F(MSG_PID_AUTOTUNE_DONE));
        break;
      default:
        return;
    }

    dgus.playSound(3);
  }

#endif // HAS_PID_HEATING


// Pause message - equivalent of ui.pause_show_message() tx3m
void DGUSScreenHandler::showPauseMessage(PauseMessage message, PauseMode mode) {
  PGM_P msg = NUL_STR;
  PGM_P header = NUL_STR;
  bool waitForInput = false;

  TERN_(ADVANCED_PAUSE_FEATURE, ExtUI::pauseModeStatus = message);  // tx3m

  if (mode != PAUSE_MODE_SAME) {
    pause_mode = mode;
  }

  switch (pause_mode) {
    case PAUSE_MODE_CHANGE_FILAMENT: header = GET_TEXT(MSG_FILAMENT_CHANGE_HEADER); break;
    case PAUSE_MODE_LOAD_FILAMENT: header = GET_TEXT(MSG_FILAMENT_CHANGE_HEADER_LOAD); break;
    case PAUSE_MODE_UNLOAD_FILAMENT: header = GET_TEXT(MSG_FILAMENT_CHANGE_HEADER_UNLOAD); break;
    default: header = GET_TEXT(MSG_FILAMENT_CHANGE_HEADER_PAUSE); break;
  }

  switch (message) {
    case PAUSE_MESSAGE_PARKING: msg = GET_TEXT(MSG_PAUSE_PRINT_PARKING); break;
    case PAUSE_MESSAGE_CHANGING: msg = GET_TEXT(MSG_FILAMENT_CHANGE_INIT); break;
    case PAUSE_MESSAGE_UNLOAD: msg = GET_TEXT(MSG_FILAMENT_CHANGE_UNLOAD); break;   
    case PAUSE_MESSAGE_WAITING:
      // Parked print pause: wait on PRINT_STATUS, where the Resume button continues
      if (isParkedForResume()) {
        showPrintStatus();
        return;
      }
      msg = GET_TEXT(MSG_ADVANCED_PAUSE_WAITING); waitForInput = true; break;
    case PAUSE_MESSAGE_INSERT: msg = GET_TEXT(MSG_FILAMENT_CHANGE_INSERT); waitForInput = true; break;
    case PAUSE_MESSAGE_LOAD: msg = GET_TEXT(MSG_FILAMENT_CHANGE_LOAD); break;
    case PAUSE_MESSAGE_PURGE:
      #if ENABLED(ADVANCED_PAUSE_CONTINUOUS_PURGE)
        msg = GET_TEXT(MSG_FILAMENT_CHANGE_CONT_PURGE);
      #else
        msg = GET_TEXT(MSG_FILAMENT_CHANGE_PURGE);
      #endif
      break;
    case PAUSE_MESSAGE_RESUME: msg = GET_TEXT(MSG_FILAMENT_CHANGE_RESUME); break;
    case PAUSE_MESSAGE_HEAT: msg = GET_TEXT(MSG_FILAMENT_CHANGE_HEAT); waitForInput = true; break;
    case PAUSE_MESSAGE_HEATING: msg = GET_TEXT(MSG_FILAMENT_CHANGE_HEATING); break;
    case PAUSE_MESSAGE_OPTION: pause_menu_response = PAUSE_RESPONSE_RESUME_PRINT; // continue without asking user
      break;
    case PAUSE_MESSAGE_STATUS: break;
    default: break;
  }

  if (msg == NUL_STR) {
    showPrintStatus();
    return;
  }

  PGM_P const msg2 = msg;
  PGM_P const msg3 = msg2 + strlen_P(msg2) + 1;
  PGM_P const msg4 = msg3 + strlen_P(msg3) + 1;

  setMessageLine_P(header, 1);
  setMessageLine_P(msg2, 2);
  setMessageLine_P(msg3, 3);
  setMessageLine_P(msg4, 4);
  showWaitScreen(DGUS_ScreenID::PRINT_STATUS, waitForInput);
}

// tx3m - Paused print parked by M125 and waiting for the user to resume
bool DGUSScreenHandler::isParkedForResume() {
  #if ENABLED(ADVANCED_PAUSE_FEATURE)
    return did_pause_print && pause_mode == PAUSE_MODE_PAUSE_PRINT && ExtUI::pauseModeStatus == PAUSE_MESSAGE_WAITING;
  #else
    return false;
  #endif
}

// tx3m - Print paused by the screen, an SD pause or the host (parked, or print timer paused).
// Unlike ExtUI::isPrintingPaused(), an SD file that is only open (e.g. left by OctoPrint) doesn't count.
bool DGUSScreenHandler::isPrintPaused() {
  return ExtUI::isPrinting() && (TERN0(ADVANCED_PAUSE_FEATURE, did_pause_print) || print_job_timer.isPaused());
}

// tx3m - Show "Print Aborted" and the elapsed time on PRINT_FINISHED
void DGUSScreenHandler::printAborted() {
  setStatusMessage(GET_TEXT_F(MSG_PRINT_ABORTED), 0);
  print_aborted = true;
  triggerScreenChange(DGUS_ScreenID::PRINT_FINISHED);
}

// tx3m - Status messages from Marlin. Keep "Print Aborted" until the user leaves PRINT_FINISHED.
void DGUSScreenHandler::statusChanged(const char * const msg) {
  if (print_aborted) return;
  setStatusMessage(msg);
}

// tx3m - Switch to PRINT_STATUS right away, leaving the WAIT screen without confirming anything
void DGUSScreenHandler::showPrintStatus() {
  new_screenID = DGUS_ScreenID::BOOT; // Drop a pending switch to WAIT
  wait_continue = false;
  moveToScreen(DGUS_ScreenID::PRINT_STATUS, true);
}

void DGUSScreenHandler::setMessageLine(const char * const msg, const uint8_t line) {
  switch (line) {
    default: return;
    case 1:
      dgus.writeString((uint16_t)DGUS_Addr::MESSAGE_Line1, msg, DGUS_LINE_LEN, true, true);
      break;
    case 2:
      dgus.writeString((uint16_t)DGUS_Addr::MESSAGE_Line2, msg, DGUS_LINE_LEN, true, true);
      break;
    case 3:
      dgus.writeString((uint16_t)DGUS_Addr::MESSAGE_Line3, msg, DGUS_LINE_LEN, true, true);
      break;
    case 4:
      dgus.writeString((uint16_t)DGUS_Addr::MESSAGE_Line4, msg, DGUS_LINE_LEN, true, true);
      break;
  }
}

void DGUSScreenHandler::setMessageLine_P(PGM_P const msg, const uint8_t line) {
  switch (line) {
    default: return;
    case 1:
      dgus.writeStringPGM((uint16_t)DGUS_Addr::MESSAGE_Line1, msg, DGUS_LINE_LEN, true, true);
      break;
    case 2:
      dgus.writeStringPGM((uint16_t)DGUS_Addr::MESSAGE_Line2, msg, DGUS_LINE_LEN, true, true);
      break;
    case 3:
      dgus.writeStringPGM((uint16_t)DGUS_Addr::MESSAGE_Line3, msg, DGUS_LINE_LEN, true, true);
      break;
    case 4:
      dgus.writeStringPGM((uint16_t)DGUS_Addr::MESSAGE_Line4, msg, DGUS_LINE_LEN, true, true);
      break;
  }
}

void DGUSScreenHandler::setStatusMessage(const char* msg, const millis_t duration) {
  dgus.writeString((uint16_t)DGUS_Addr::MESSAGE_Status, msg, DGUS_STATUS_LEN, false, true);
  status_expire = (duration > 0 ? ExtUI::safe_millis() + duration : 0);
}

void DGUSScreenHandler::setStatusMessage(FSTR_P const fmsg, const millis_t duration) {
  dgus.writeString((uint16_t)DGUS_Addr::MESSAGE_Status, fmsg, DGUS_STATUS_LEN, false, true);
  status_expire = (duration > 0 ? ExtUI::safe_millis() + duration : 0);
}

void DGUSScreenHandler::showWaitScreen(const DGUS_ScreenID return_screenID, const bool has_continue/*=false*/) {
  if (return_screenID != DGUS_ScreenID::WAIT)
    wait_return_screenID = return_screenID;

  wait_continue = has_continue;

  triggerScreenChange(DGUS_ScreenID::WAIT);
}

void DGUSScreenHandler::showWaitScreen(FSTR_P const msg, const DGUS_ScreenID return_screenID, const bool has_continue/*=false*/) {
  setMessageLine_P(NUL_STR, 1);
  setMessageLine(msg, 2);
  setMessageLine_P(NUL_STR, 3);
  setMessageLine_P(NUL_STR, 4);
  showWaitScreen(return_screenID, has_continue);
}

DGUS_ScreenID DGUSScreenHandler::getCurrentScreen() {
  return current_screenID;
}

void DGUSScreenHandler::triggerScreenChange(const DGUS_ScreenID screenID) {
  new_screenID = screenID;
}

void DGUSScreenHandler::triggerFullUpdate() {
  full_update = true;
}

void DGUSScreenHandler::triggerEEPROMSave() {
  eeprom_save = ExtUI::safe_millis() + 500;
}

bool DGUSScreenHandler::isPrinterIdle() {
  return (!ExtUI::commandsInQueue() && !ExtUI::isMoving());
}

const DGUS_Addr* DGUSScreenHandler::findScreenAddrList(const DGUS_ScreenID screenID) {
  DGUS_ScreenAddrList list;
  const DGUS_ScreenAddrList *map = screen_addr_list_map;

  do {
    memcpy_P(&list, map, sizeof(*map));
    if (!list.addr_list) break;
    if (list.screenID == screenID) return list.addr_list;
  } while (++map);

  return nullptr;
}

bool DGUSScreenHandler::callScreenSetup(const DGUS_ScreenID screenID) {
  DGUS_ScreenSetup setup;
  const DGUS_ScreenSetup *list = screen_setup_list;

  do {
    memcpy_P(&setup, list, sizeof(*list));
    if (!setup.setup_fn) break;
    if (setup.screenID == screenID) {
      return setup.setup_fn();
    }
  } while (++list);

  return true;
}

void DGUSScreenHandler::moveToScreen(const DGUS_ScreenID screenID, bool abort_wait) {

  if (DGUSDisplay::displayDimmed) {
	DGUSDisplay::displayDimmed = false;
    dgus.setBrightness(DGUSDisplay::getBrightness()); // unDim Backlight
  }

  if (current_screenID == DGUS_ScreenID::KILL) return;

  if (current_screenID == DGUS_ScreenID::WAIT) {
    if (screenID != DGUS_ScreenID::WAIT)
      wait_return_screenID = screenID;

    if (!abort_wait) return;

    if (wait_continue && marlin.wait_for_user)
      ExtUI::setUserConfirmed();
  }

  if (!callScreenSetup(screenID)) return;

  if (!sendScreenVPData(screenID, true)) return;

  DEBUG_ECHOLNPGM("From screen ", (uint16_t)current_screenID, " to screen ", (uint16_t)screenID);  // tx3m

  // tx3m - The aborted print is done with once the user leaves PRINT_FINISHED
  if (print_aborted && screenID != DGUS_ScreenID::PRINT_FINISHED
      && screenID != DGUS_ScreenID::PRINT_STATUS && screenID != DGUS_ScreenID::WAIT)
    print_aborted = false;

  current_screenID = screenID;
  dgus.switchScreen(current_screenID);
}

bool DGUSScreenHandler::sendScreenVPData(const DGUS_ScreenID screenID, bool complete_update) {
  if (complete_update) full_update = false;

  const DGUS_Addr *list = findScreenAddrList(screenID);

  while (true) {
    if (!list) return true; // Nothing left to send

    const uint16_t addr = pgm_read_word(list++);
    if (!addr) return true; // Nothing left to send

    DGUS_VP vp;
    if (!populateVP((DGUS_Addr)addr, &vp)) continue; // Invalid VP
    if (!vp.tx_handler) continue; // Nothing to send
    if (!complete_update && !(vp.flags & VPFLAG_AUTOUPLOAD)) continue; // Unnecessary VP

    uint8_t expected_tx = 6 + vp.size; // 6 bytes header + payload.
    const millis_t try_until = ExtUI::safe_millis() + 1000;

    while (expected_tx > dgus.getFreeTxBuffer()) {
      if (ELAPSED(ExtUI::safe_millis(), try_until)) return false; // Stop trying after 1 second

      dgus.flushTx(); // Flush the TX buffer
      delay(50);
    }

    vp.tx_handler(vp);
  }
}

#endif // DGUS_LCD_UI_RELOADED
