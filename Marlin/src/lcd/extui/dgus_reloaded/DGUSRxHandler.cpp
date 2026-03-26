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
 * jgbrown54
 *   Added support for enable/disable Runout Sensor
 *   Added support for Saving and Restoring EEPROM
 *   Added Advanced Settings
 */

#include "../../../inc/MarlinConfigPre.h"

#if DGUS_LCD_UI_RELOADED

#include "DGUSRxHandler.h"

#include "DGUSScreenHandler.h"
#include "config/DGUS_ScreenID.h"

#include "../ui_api.h"
#include "../../../core/language.h"
#include "../../../module/temperature.h"
#include "../../../module/printcounter.h"
#include "../../../module/stepper.h"
#include "../../../gcode/queue.h"
#if ENABLED(ADVANCED_PAUSE_FEATURE)
  #include "../../../feature/pause.h"
#endif
#if ENABLED(POWER_LOSS_RECOVERY)
  #include "../../../feature/powerloss.h"
#endif

void DGUSRxHandler::screenChange(DGUS_VP &vp, void *data_ptr) {
  const DGUS_ScreenID screenID = (DGUS_ScreenID)((uint8_t*)data_ptr)[1];

  if (vp.addr == DGUS_Addr::SCREENCHANGE_SD) {
    #if HAS_MEDIA
      IF_DISABLED(HAS_SD_DETECT, card.mount());

      if (!ExtUI::isMediaMounted()) {
        screen.setStatusMessage(GET_TEXT_F(MSG_NO_MEDIA));
        return;
      }

      card.cdroot();
    #else
      screen.setStatusMessage(GET_TEXT_F(MSG_NO_MEDIA));
      return;
    #endif
  }

  if (vp.addr == DGUS_Addr::SCREENCHANGE_Idle
      && (ExtUI::isPrinting() || ExtUI::isPrintingPaused())) {
    screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_NOT_WHILE_PRINTING));
    return;
  }

  if (vp.addr == DGUS_Addr::SCREENCHANGE_Printing
      && (!ExtUI::isPrinting() && !ExtUI::isPrintingPaused())) {
    screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_NOT_WHILE_IDLE));
    return;
  }

#if !HAS_LEVELING
  // Skip Leveling menu if no automatic leveling
  if (screen == DGUS_ScreenID::LEVELING_MENU) {
	if (dgus.getCurrentScreen() == DGUS_ScreenID::SETTINGS_MENU) {
	  screenID = DGUS_ScreenID::LEVELING_MANUAL;
	}
	else {
	  screenID = DGUS_ScreenID::SETTINGS_MENU;
	}
  }
#endif

  screen.triggerScreenChange(screenID);
}

#if HAS_MEDIA
  void DGUSRxHandler::scroll(DGUS_VP &vp, void *data_ptr) {
    UNUSED(vp);

    const DGUS_Data::Scroll scroll = (DGUS_Data::Scroll)((uint8_t*)data_ptr)[1];

    switch (scroll) {
      case DGUS_Data::Scroll::GO_BACK:
        if (screen.filelist.isAtRootDir()) {
          return;
        }

        screen.filelist_offset = 0;
        screen.filelist_selected = -1;
        screen.filelist.upDir();
        break;
      case DGUS_Data::Scroll::UP:
        if (screen.filelist_offset < 1) {
          return;
        }

        --screen.filelist_offset;
        break;
      case DGUS_Data::Scroll::DOWN:
        if (screen.filelist_offset + 1 + DGUS_FILE_COUNT > screen.filelist.count()) {
          return;
        }

        ++screen.filelist_offset;
        break;
    }

    screen.triggerFullUpdate();
  }

  void DGUSRxHandler::selectFile(DGUS_VP &vp, void *data_ptr) {
    UNUSED(vp);

    const uint8_t index = ((uint8_t*)data_ptr)[1];

    if (!screen.filelist.seek(screen.filelist_offset + index)) {
      return;
    }

    if (screen.filelist.isDir()) {
      screen.filelist_offset = 0;
      screen.filelist_selected = -1;
      screen.filelist.changeDir(screen.filelist.filename());
    }
    else {
      screen.filelist_selected = screen.filelist_offset + index;
    }

    screen.triggerFullUpdate();
  }

  void DGUSRxHandler::printFile(DGUS_VP &vp, void *data_ptr) {
    UNUSED(vp);
    UNUSED(data_ptr);

    if (screen.filelist_selected < 0) {
      screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_NO_FILE_SELECTED));
      return;
    }

    if (!screen.filelist.seek(screen.filelist_selected)
        || screen.filelist.isDir()) {
      return;
    }

    if (!screen.isPrinterIdle()) {
      screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_BUSY));
      return;
    }

    ExtUI::printFile(screen.filelist.shortFilename());
    screen.triggerScreenChange(DGUS_ScreenID::PRINT_STATUS);
  }
#endif // HAS_MEDIA

void DGUSRxHandler::printAbort(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  // tx3m
  if (DGUSDisplay::displayDimmed) {
	DGUSDisplay::displayDimmed = false;
    dgus.setBrightness(DGUSDisplay::getBrightness()); // unDim Backlight
  }

  const DGUS_Data::Popup result = (DGUS_Data::Popup)((uint8_t*)data_ptr)[1];

  if (result != DGUS_Data::Popup::CONFIRMED) {
    return;
  }

  if (!ExtUI::isPrinting() && !ExtUI::isPrintingPaused()) {
    screen.triggerFullUpdate();
    return;
  }

  ExtUI::stopPrint();
}

void DGUSRxHandler::printPause(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  // tx3m
  if (DGUSDisplay::displayDimmed) {
	DGUSDisplay::displayDimmed = false;
    dgus.setBrightness(DGUSDisplay::getBrightness()); // unDim Backlight
  }

  const DGUS_Data::Popup result = (DGUS_Data::Popup)((uint8_t*)data_ptr)[1];

  if (result != DGUS_Data::Popup::CONFIRMED) {
    return;
  }

  if (!ExtUI::isPrinting()) {
    screen.triggerFullUpdate();
    return;
  }

  ExtUI::pausePrint();
}

void DGUSRxHandler::printResume(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  // tx3m
  if (DGUSDisplay::displayDimmed) {
	DGUSDisplay::displayDimmed = false;
    dgus.setBrightness(DGUSDisplay::getBrightness()); // unDim Backlight
  }

  const DGUS_Data::Popup result = (DGUS_Data::Popup)((uint8_t*)data_ptr)[1];

  if (result != DGUS_Data::Popup::CONFIRMED) {
    return;
  }

  if (!ExtUI::isPrintingPaused()) {
    screen.triggerFullUpdate();
    return;
  }

  if (!screen.isPrinterIdle()) {
    screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_BUSY));
    return;
  }

  ExtUI::resumePrint();
}

// tx3m - Used on a previous Longer FW version (with a single pause/resume button). Most likely won't be needed or used anymore, but keep for reference
void DGUSRxHandler::printPauseResume(DGUS_VP& vp, void* data_ptr) {
  UNUSED(vp);
  UNUSED(data_ptr);
  
  if (ExtUI::isPrinting()) {
    // confirm pause
    screen.triggerScreenChange(DGUS_ScreenID::PAUSE_CONFIRM);
  }
  else {
    // confirm resume
    screen.triggerScreenChange(DGUS_ScreenID::RESUME_CONFIRM);
  }
}

void DGUSRxHandler::feedrate(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  const int16_t feedrate = BE16_P(data_ptr);

  ExtUI::setFeedrate_percent(feedrate);

  screen.triggerFullUpdate();
}

void DGUSRxHandler::flowrate(DGUS_VP &vp, void *data_ptr) {
  const int16_t flowrate = BE16_P(data_ptr);

  switch (vp.addr) {
    default: return;
    case DGUS_Addr::ADJUST_SetFlowrate_CUR:
      ExtUI::setFlow_percent(flowrate, TERN(HAS_MULTI_EXTRUDER, ExtUI::getActiveTool(), ExtUI::E0));
      break;
    #if HAS_MULTI_EXTRUDER
      case DGUS_Addr::ADJUST_SetFlowrate_E0: ExtUI::setFlow_percent(flowrate, ExtUI::E0); break;
      case DGUS_Addr::ADJUST_SetFlowrate_E1: ExtUI::setFlow_percent(flowrate, ExtUI::E1); break;
    #endif
  }

  screen.triggerFullUpdate();
}

void DGUSRxHandler::babystepSet(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  const int16_t data = BE16_P(data_ptr);
  const float offset = dgus.fromFixedPoint<int16_t, float, 2>(data);

  const int16_t steps = ExtUI::mmToWholeSteps(offset - ExtUI::getZOffset_mm(), ExtUI::Z);

  ExtUI::smartAdjustAxis_steps(steps, ExtUI::Z, true);

  screen.triggerEEPROMSave();
  screen.triggerFullUpdate();
}

void DGUSRxHandler::babystep(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  const DGUS_Data::Adjust adjust = (DGUS_Data::Adjust)((uint8_t*)data_ptr)[1];
  int16_t steps;

  switch (adjust) {
    default: return;
    case DGUS_Data::Adjust::INCREMENT: steps = ExtUI::mmToWholeSteps(DGUS_PRINT_BABYSTEP, ExtUI::Z); break;
    case DGUS_Data::Adjust::DECREMENT: steps = ExtUI::mmToWholeSteps(-DGUS_PRINT_BABYSTEP, ExtUI::Z); break;
  }

  ExtUI::smartAdjustAxis_steps(steps, ExtUI::Z, true);
  screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_BABYSTEP_CHANGED));  // tx3m

  screen.triggerEEPROMSave();
  screen.triggerFullUpdate();
}

void DGUSRxHandler::tempPreset(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  const DGUS_Data::TempPreset preset = (DGUS_Data::TempPreset)((uint8_t*)data_ptr)[1];

  switch (preset) {
    case DGUS_Data::TempPreset::PLA:
      #if HOTENDS < 2
        ExtUI::setTargetTemp_celsius(DGUS_PLA_TEMP_HOTEND, ExtUI::H0);
      #else
        ExtUI::setTargetTemp_celsius(DGUS_PLA_TEMP_HOTEND, ExtUI::getActiveTool());
      #endif
      ExtUI::setTargetTemp_celsius(DGUS_PLA_TEMP_BED, ExtUI::BED);
      break;
    case DGUS_Data::TempPreset::ABS:
      #if HOTENDS < 2
        ExtUI::setTargetTemp_celsius(DGUS_ABS_TEMP_HOTEND, ExtUI::H0);
      #else
        ExtUI::setTargetTemp_celsius(DGUS_ABS_TEMP_HOTEND, ExtUI::getActiveTool());
      #endif
      ExtUI::setTargetTemp_celsius(DGUS_ABS_TEMP_BED, ExtUI::BED);
      break;
    case DGUS_Data::TempPreset::PETG:
      #if HOTENDS < 2
        ExtUI::setTargetTemp_celsius(DGUS_PETG_TEMP_HOTEND, ExtUI::H0);
      #else
        ExtUI::setTargetTemp_celsius(DGUS_PETG_TEMP_HOTEND, ExtUI::getActiveTool());
      #endif
      ExtUI::setTargetTemp_celsius(DGUS_PETG_TEMP_BED, ExtUI::BED);
      break;
  }

  screen.triggerFullUpdate();
}

void DGUSRxHandler::tempTarget(DGUS_VP &vp, void *data_ptr) {
  const int16_t temp = BE16_P(data_ptr);

  switch (vp.addr) {
    default: return;
    case DGUS_Addr::TEMP_SetTarget_Bed:  ExtUI::setTargetTemp_celsius(temp, ExtUI::BED); break;
    case DGUS_Addr::TEMP_SetTarget_H0:   ExtUI::setTargetTemp_celsius(temp, ExtUI::H0); break;
    #if HAS_MULTI_HOTEND
      case DGUS_Addr::TEMP_SetTarget_H1: ExtUI::setTargetTemp_celsius(temp, ExtUI::H1); break;
    #endif
  }

  screen.triggerFullUpdate();
}

void DGUSRxHandler::tempCool(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  const DGUS_Data::Heater heater = (DGUS_Data::Heater)BE16_P(data_ptr);

  switch (heater) {
    default: return;
    case DGUS_Data::Heater::ALL:
      ExtUI::setTargetTemp_celsius(0, ExtUI::BED);
      ExtUI::setTargetTemp_celsius(0, ExtUI::H0);
      #if HAS_MULTI_HOTEND
        ExtUI::setTargetTemp_celsius(0, ExtUI::H1);
      #endif
      break;
    case DGUS_Data::Heater::BED:  ExtUI::setTargetTemp_celsius(0, ExtUI::BED); break;
    case DGUS_Data::Heater::H0:   ExtUI::setTargetTemp_celsius(0, ExtUI::H0); break;
    #if HAS_MULTI_HOTEND
      case DGUS_Data::Heater::H1: ExtUI::setTargetTemp_celsius(0, ExtUI::H1); break;
    #endif
  }

  screen.setStatusMessage(GET_TEXT_F(MSG_COOLING));

  screen.triggerFullUpdate();
}

void DGUSRxHandler::steppers(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  const DGUS_Data::Control control = (DGUS_Data::Control)((uint8_t*)data_ptr)[1];

  switch (control) {
    case DGUS_Data::Control::ENABLE:
      stepper.enable_all_steppers();
      break;
    case DGUS_Data::Control::DISABLE:
      stepper.disable_all_steppers();
      break;
  }

  screen.triggerFullUpdate();
}

// jgbrown54 - Added Runout Control tx3m
void DGUSRxHandler::runoutControl(DGUS_VP& vp, void* data_ptr) {
  UNUSED(vp);

  const DGUS_Data::Control control = (DGUS_Data::Control)((uint8_t*)data_ptr)[1];

  switch (control) {
  case DGUS_Data::Control::ENABLE:
    ExtUI::setFilamentRunoutEnabled(true);
    break;
  case DGUS_Data::Control::DISABLE:
    ExtUI::setFilamentRunoutEnabled(false);
    break;
  }

  screen.triggerFullUpdate();
}


void DGUSRxHandler::zOffset(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  if (TERN0(NO_MOTION_BEFORE_HOMING, !ExtUI::isAxisPositionKnown(ExtUI::Z))) {
    screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_HOMING_REQUIRED));
    return;
  }

  if (!screen.isPrinterIdle()) {
    screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_NO_Z_OFFSET_PRINTING));
    return;
  }

  const int16_t data = BE16_P(data_ptr);
  const float offset = dgus.fromFixedPoint<int16_t, float, 2>(data);

  const int16_t steps = ExtUI::mmToWholeSteps(offset - ExtUI::getZOffset_mm(), ExtUI::Z);

  ExtUI::smartAdjustAxis_steps(steps, ExtUI::Z, true);

  screen.triggerEEPROMSave();
  screen.triggerFullUpdate();
}

void DGUSRxHandler::zOffsetStep(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  if (TERN0(NO_MOTION_BEFORE_HOMING, !ExtUI::isAxisPositionKnown(ExtUI::Z))) {
    screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_HOMING_REQUIRED));
    return;
  }

  if (!screen.isPrinterIdle()) {
    screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_BUSY));
    return;
  }

  const DGUS_Data::Adjust adjust = (DGUS_Data::Adjust)((uint8_t*)data_ptr)[1];
  int16_t steps;

  switch (screen.offset_steps) {
    default: return;
    case DGUS_Data::StepSize::MMP1:
      steps = ExtUI::mmToWholeSteps((adjust == DGUS_Data::Adjust::INCREMENT ? 0.1f : -0.1f), ExtUI::Z);
      break;
    case DGUS_Data::StepSize::MMP01:
      steps = ExtUI::mmToWholeSteps((adjust == DGUS_Data::Adjust::INCREMENT ? 0.01f : -0.01f), ExtUI::Z);
      break;
  }

  // tx3m this does not work as expected so adjust only the Z offset and then do a relative move
  // ExtUI::smartAdjustAxis_steps(steps, ExtUI::Z, true);

  const float movement = (adjust == DGUS_Data::Adjust::INCREMENT ? 0.1f : -0.1f);
  const float new_offset = ExtUI::getZOffset_mm() + movement;

  // Update stored offset
  ExtUI::setZOffset_mm(new_offset);

  // Apply matching physical move (relative Z)
  char cmd[32];
  char float_str[10];
  dtostrf(movement, 1, 3, float_str);
  snprintf(cmd, sizeof(cmd), "G91\nG0 Z%s F600\nG90", float_str);
  queue.inject(cmd);

  screen.triggerEEPROMSave();
  screen.triggerFullUpdate();
}

void DGUSRxHandler::zOffsetSetStep(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  const DGUS_Data::StepSize size = (DGUS_Data::StepSize)((uint8_t*)data_ptr)[1];

  screen.offset_steps = size;

  screen.triggerFullUpdate();
}

void DGUSRxHandler::moveToPoint(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  if (!ExtUI::isPositionKnown()) {
    screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_HOMING_REQUIRED));
    return;
  }

  if (!screen.isPrinterIdle()) {
    screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_BUSY));
    return;
  }

  const uint8_t point = ((uint8_t*)data_ptr)[1];
  constexpr float lfrb[4] = BED_TRAMMING_INSET_LFRB;
  float x, y;

  switch (point) {
    default: return;
    case 1:
      x = DGUS_LEVEL_CENTER_X;
      y = DGUS_LEVEL_CENTER_Y;
      break;
    case 2:
      x = X_MIN_BED + lfrb[0];
      y = Y_MIN_BED + lfrb[1];
      break;
    case 3:
      x = X_MAX_BED - lfrb[2];
      y = Y_MIN_BED + lfrb[1];
      break;
    case 4:
      x = X_MAX_BED - lfrb[2];
      y = Y_MAX_BED - lfrb[3];
      break;
    case 5:
      x = X_MIN_BED + lfrb[0];
      y = Y_MAX_BED - lfrb[3];
      break;
  }

  if (BED_TRAMMING_Z_HOP)
    ExtUI::setAxisPosition_mm(ExtUI::getAxisPosition_mm(ExtUI::Z) + (BED_TRAMMING_Z_HOP), ExtUI::Z);
  ExtUI::setAxisPosition_mm(x, ExtUI::X);
  ExtUI::setAxisPosition_mm(y, ExtUI::Y);
  ExtUI::setAxisPosition_mm((Z_MIN_POS) + (BED_TRAMMING_HEIGHT), ExtUI::Z);
}

void DGUSRxHandler::probe(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);
  UNUSED(data_ptr);

  #if ENABLED(MESH_BED_LEVELING)
    screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_ABL_REQUIRED));
    return;
  #endif

  if (!ExtUI::isPositionKnown()) {
    screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_HOMING_REQUIRED));
    return;
  }

  if (!screen.isPrinterIdle()) {
    screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_BUSY));
    return;
  }

  screen.triggerScreenChange(DGUS_ScreenID::LEVELING_PROBING);

  #if ENABLED(AUTO_BED_LEVELING_UBL)
    queue.enqueue_now(F("G29P1\nG29P3\nG29P5C"));
  #else
    queue.enqueue_now(F("G29"));
  #endif
  queue.enqueue_now(F("M500"));
}

void DGUSRxHandler::disableABL(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);
  UNUSED(data_ptr);

  if (!screen.isPrinterIdle()) {
    screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_BUSY));
    return;
  }

#if HAS_LEVELING
  ExtUI::setLevelingActive(false);
#endif

  screen.triggerEEPROMSave();
  screen.triggerFullUpdate();
}

void DGUSRxHandler::filamentSelect(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  const DGUS_Data::Extruder extruder = (DGUS_Data::Extruder)BE16_P(data_ptr);

  switch (extruder) {
    default: return;
    case DGUS_Data::Extruder::CURRENT:
    case DGUS_Data::Extruder::E0:
    E_TERN_(case DGUS_Data::Extruder::E1:)
      screen.filament_extruder = extruder;
      break;
  }

  screen.triggerFullUpdate();
}

void DGUSRxHandler::filamentLength(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  const uint16_t length = BE16_P(data_ptr);

  screen.filament_length = constrain(length, 0, EXTRUDE_MAXLENGTH);

  screen.triggerFullUpdate();
}

void DGUSRxHandler::filamentMove(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  if (!screen.isPrinterIdle()) {
    screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_BUSY));
    return;
  }

  ExtUI::extruder_t extruder;

  switch (screen.filament_extruder) {
    default: return;
    case DGUS_Data::Extruder::CURRENT:
      #if HAS_MULTI_EXTRUDER
        extruder = ExtUI::getActiveTool();
        break;
      #endif
    case DGUS_Data::Extruder::E0:
      extruder = ExtUI::E0;
      break;
    #if HAS_MULTI_EXTRUDER
      case DGUS_Data::Extruder::E1:
        extruder = ExtUI::E1;
        break;
    #endif
  }

  if (ExtUI::getActualTemp_celsius(extruder) < (float)EXTRUDE_MINTEMP) {
    screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_TEMP_TOO_LOW));
    return;
  }

  const DGUS_Data::FilamentMove move = (DGUS_Data::FilamentMove)((uint8_t*)data_ptr)[1];

  switch (move) {
    case DGUS_Data::FilamentMove::RETRACT:
      UI_DECREMENT_BY(AxisPosition_mm, (float)screen.filament_length, extruder);
      break;
    case DGUS_Data::FilamentMove::EXTRUDE:
      UI_INCREMENT_BY(AxisPosition_mm, (float)screen.filament_length, extruder);
      break;
    // tx3m
    case DGUS_Data::FilamentMove::UNLOAD:
      // UI_DECREMENT_BY(AxisPosition_mm, (float)screen.filament_length, extruder);
      screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_NOT_IMPLEMENTED));
      break;
    // tx3m
    case DGUS_Data::FilamentMove::LOAD:
      //UI_INCREMENT_BY(AxisPosition_mm, (float)screen.filament_length, extruder);
      screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_NOT_IMPLEMENTED));
      break;
  }
}

// tx3m - Added Filament Load/Unload which is the same as Move but without the temp check
void DGUSRxHandler::filamentLoadUnload(DGUS_VP& vp, void* data_ptr) {
  filamentMove(vp, data_ptr);
}

void DGUSRxHandler::home(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  if (!screen.isPrinterIdle()) {
    screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_BUSY));
    return;
  }

  DGUS_Data::Axis axis = (DGUS_Data::Axis)((uint8_t*)data_ptr)[1];

  screen.showWaitScreen(GET_TEXT_F(DGUS_MSG_HOMING), screen.getCurrentScreen());

  switch (axis) {
    case DGUS_Data::Axis::X_Y_Z:
      queue.enqueue_now(F("G28XYZ"));
      break;
    case DGUS_Data::Axis::X_Y:
      queue.enqueue_now(F("G28XY"));
      break;
    case DGUS_Data::Axis::Z:
      queue.enqueue_now(F("G28Z"));
      break;
  }
}

void DGUSRxHandler::move(DGUS_VP &vp, void *data_ptr) {
  const int16_t data = BE16_P(data_ptr);
  const float position = dgus.fromFixedPoint<int16_t, float, 1>(data);
  ExtUI::axis_t axis;

  switch (vp.addr) {
    default: return;
    case DGUS_Addr::MOVE_SetX: axis = ExtUI::X; break;
    case DGUS_Addr::MOVE_SetY: axis = ExtUI::Y; break;
    case DGUS_Addr::MOVE_SetZ: axis = ExtUI::Z; break;
  }

  if (TERN0(NO_MOTION_BEFORE_HOMING, !ExtUI::isAxisPositionKnown(axis))) {
    screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_HOMING_REQUIRED));
    return;
  }

  ExtUI::setAxisPosition_mm(position, axis);

  screen.triggerFullUpdate();
}

void DGUSRxHandler::moveStep(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  float offset;
  switch (screen.move_steps) {
    default: return;
    case DGUS_Data::StepSize::MM10: offset = 10.0f; break;
    case DGUS_Data::StepSize::MM1: offset = 1.0f; break;
    case DGUS_Data::StepSize::MMP1: offset = 0.1f; break;
  }

  const DGUS_Data::MoveDirection direction = (DGUS_Data::MoveDirection)((uint8_t*)data_ptr)[1];
  ExtUI::axis_t axis;

  switch (direction) {
    default: return;
    case DGUS_Data::MoveDirection::XM: offset *= -1;
    case DGUS_Data::MoveDirection::XP: axis = ExtUI::X; break;
    case DGUS_Data::MoveDirection::YM: offset *= -1;
    case DGUS_Data::MoveDirection::YP: axis = ExtUI::Y; break;
    case DGUS_Data::MoveDirection::ZM: offset *= -1;
    case DGUS_Data::MoveDirection::ZP: axis = ExtUI::Z; break;
  }

  if (TERN0(NO_MOTION_BEFORE_HOMING, !ExtUI::isAxisPositionKnown(axis))) {
    screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_HOMING_REQUIRED));
    return;
  }

  UI_INCREMENT_BY(AxisPosition_mm, offset, axis);

  screen.triggerFullUpdate();
}

void DGUSRxHandler::moveSetStep(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  const DGUS_Data::StepSize size = (DGUS_Data::StepSize)((uint8_t*)data_ptr)[1];
  screen.move_steps = size;

  screen.triggerFullUpdate();
}

void DGUSRxHandler::gcodeClear(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);
  UNUSED(data_ptr);

  ZERO(screen.gcode);

  screen.triggerFullUpdate();
}

void DGUSRxHandler::gcodeExecute(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);
  UNUSED(data_ptr);

  if (!strlen(screen.gcode)) return;

  if (!screen.isPrinterIdle()) {
    screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_BUSY));
    return;
  }

  screen.showWaitScreen(GET_TEXT_F(DGUS_MSG_EXECUTING_COMMAND), DGUS_ScreenID::GCODE);

  queue.enqueue_one_now(screen.gcode);
}

// jgbrown54 - Added EEPROM Save and Restore
// tx3m: replaced by resetEEPROM with multiple actions
// void DGUSRxHandler::resetEEPROM(DGUS_VP &vp, void *data_ptr) {
//   UNUSED(vp);

//   const DGUS_Data::Popup result = (DGUS_Data::Popup)((uint8_t*)data_ptr)[1];

//   if (result != DGUS_Data::Popup::CONFIRMED) return;

//   if (!screen.isPrinterIdle()) {
//     screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_BUSY));
//     return;
//   }

//   queue.enqueue_now(F("M502"));
//   queue.enqueue_now(F("M500"));
// }

// tx3m
void DGUSRxHandler::controlEEPROM(DGUS_VP& vp, void* data_ptr) {
  UNUSED(vp);

  const DGUS_Data::EepromAction action = (DGUS_Data::EepromAction)((uint8_t*)data_ptr)[1];

  if (!screen.isPrinterIdle()) {
    screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_BUSY));
    return;
  }

  switch (action) {
  case DGUS_Data::EepromAction::RESET:
    settings.reset();
    queue.enqueue_now(F("M502"));
    queue.enqueue_now(F("M500"));
    break;

  case DGUS_Data::EepromAction::LOAD:
    settings.load();
    break;

  case DGUS_Data::EepromAction::SAVE:
    settings.save();
    break;
  }
  screen.triggerScreenChange(DGUS_ScreenID::SETTINGS_MENU2);
}


void DGUSRxHandler::settingsExtra(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  const DGUS_Data::Extra extra = (DGUS_Data::Extra)((uint8_t*)data_ptr)[1];

  switch (extra) {
    default: return;
    case DGUS_Data::Extra::BUTTON1:
      #if ENABLED(BLTOUCH)
        if (!screen.isPrinterIdle()) {
          screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_BUSY));
          return;
        }

        queue.enqueue_now(F(DGUS_RESET_BLTOUCH));
      #else
        screen.triggerScreenChange(DGUS_ScreenID::INFOS);
      #endif
      break;
    #if ENABLED(BLTOUCH)
      case DGUS_Data::Extra::BUTTON2:
        screen.triggerScreenChange(DGUS_ScreenID::INFOS);
        break;
    #endif
  }
}

void DGUSRxHandler::pidSelect(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  const DGUS_Data::Heater heater = (DGUS_Data::Heater)BE16_P(data_ptr);

  switch (heater) {
    default: return;
    case DGUS_Data::Heater::BED:
      screen.pid_temp = DGUS_PLA_TEMP_BED;
      screen.pid_heater = heater;
      break;
    case DGUS_Data::Heater::H0:
    #if HAS_MULTI_HOTEND
      case DGUS_Data::Heater::H1:
    #endif
      screen.pid_temp = DGUS_PLA_TEMP_HOTEND;
      screen.pid_heater = heater;
      break;
  }

  screen.pid_cycles = 5;

  screen.triggerFullUpdate();
}

void DGUSRxHandler::pidSetTemp(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  if (!screen.isPrinterIdle()) {
    screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_BUSY));
    return;
  }

  celsius_t temp = BE16_P(data_ptr);

  switch (screen.pid_heater) {
    default: return;
    #if HAS_HEATED_BED
      case DGUS_Data::Heater::BED:
        LIMIT(temp, celsius_t(BED_MINTEMP), celsius_t(BED_MAX_TARGET));
        break;
    #endif
    #if HAS_HOTEND
      case DGUS_Data::Heater::H0:
        LIMIT(temp, celsius_t(HEATER_0_MINTEMP), thermalManager.hotend_max_target(0));
        break;
    #endif
    #if HAS_MULTI_HOTEND
      case DGUS_Data::Heater::H1:
        LIMIT(temp, celsius_t(HEATER_1_MINTEMP), thermalManager.hotend_max_target(0));
        break;
    #endif
  }

  screen.pid_temp = temp;

  screen.triggerFullUpdate();
}

void DGUSRxHandler::pidRun(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);
  UNUSED(data_ptr);

  if (!screen.isPrinterIdle()) {
    screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_BUSY));
    return;
  }

  heater_id_t heater;
  uint8_t cycles = constrain(screen.pid_cycles, 3, 10);

  switch (screen.pid_heater) {
    default: return;
    case DGUS_Data::Heater::BED:
      #if ENABLED(PIDTEMPBED)
        heater = H_BED;
        break;
      #else
        screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_BED_PID_DISABLED));
        return;
      #endif
    case DGUS_Data::Heater::H0:
      #if ENABLED(PIDTEMP)
        heater = H_E0;
        break;
      #else
        screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_PID_DISABLED));
        return;
      #endif
    #if HAS_MULTI_HOTEND
      case DGUS_Data::Heater::H1:
        #if ENABLED(PIDTEMP)
          heater = H_E1;
          break;
        #else
          screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_PID_DISABLED));
          return;
        #endif
    #endif
  }

  char buffer[24];
  snprintf_P(buffer, sizeof(buffer), PSTR("M303C%dE%dS%dU1"), cycles, heater, screen.pid_temp);

  screen.showWaitScreen(GET_TEXT_F(DGUS_MSG_PID_AUTOTUNING), DGUS_ScreenID::PID);

  queue.enqueue_one_now(buffer);
  queue.enqueue_now(F("M500"));
}

#if ENABLED(POWER_LOSS_RECOVERY)
  void DGUSRxHandler::powerLossAbort(DGUS_VP &vp, void *data_ptr) {
    UNUSED(vp);

    const DGUS_Data::Popup result = (DGUS_Data::Popup)((uint8_t*)data_ptr)[1];

    if (result != DGUS_Data::Popup::CONFIRMED) {
      return;
    }

    if (!screen.isPrinterIdle()) {
      screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_BUSY));
      return;
    }

    screen.triggerScreenChange(DGUS_ScreenID::HOME);

    queue.enqueue_now(F("M1000C"));
  }

  void DGUSRxHandler::powerLossResume(DGUS_VP &vp, void *data_ptr) {
    UNUSED(vp);

    const DGUS_Data::Popup result = (DGUS_Data::Popup)((uint8_t*)data_ptr)[1];

    if (result != DGUS_Data::Popup::CONFIRMED) {
      return;
    }

    if (!screen.isPrinterIdle()) {
      screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_BUSY));
      return;
    }

    if (!recovery.valid()) {
      screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_INVALID_RECOVERY_DATA));
      return;
    }

    screen.triggerScreenChange(DGUS_ScreenID::PRINT_STATUS);

    queue.enqueue_now(F("M1000"));
  }
#endif // POWER_LOSS_RECOVERY

void DGUSRxHandler::waitAbort(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  const DGUS_Data::Popup result = (DGUS_Data::Popup)((uint8_t*)data_ptr)[1];

  if (result != DGUS_Data::Popup::CONFIRMED) {
    return;
  }

  if (!ExtUI::isPrintingPaused()) {
    screen.triggerFullUpdate();
    return;
  }

  ExtUI::stopPrint();

  screen.triggerFullUpdate();
}

void DGUSRxHandler::waitContinue(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);
  UNUSED(data_ptr);

  ExtUI::setUserConfirmed();

  screen.triggerFullUpdate();
}

void DGUSRxHandler::fanSpeed(DGUS_VP &vp, void *data_ptr) {
  uint8_t speed = ((uint8_t*)data_ptr)[1];
  switch (vp.addr) {
    default: return;
    case DGUS_Addr::FAN0_Speed:
      ExtUI::setTargetFan_percent(speed, ExtUI::FAN0);
      break;
  }
}

void DGUSRxHandler::volume(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  uint8_t volume = ((uint8_t*)data_ptr)[1];
  dgus.setVolume(volume);

  screen.triggerEEPROMSave();
}

void DGUSRxHandler::brightness(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  uint8_t brightness = ((uint8_t*)data_ptr)[1];
  DGUSDisplay::displayDimmed = false;
  dgus.setBrightness(brightness);

  screen.triggerEEPROMSave();
}

// tx3m
void DGUSRxHandler::dimBacklight(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);

  if (DGUSDisplay::displayDimmed) {
	DGUSDisplay::displayDimmed = false;
    dgus.setBrightness(DGUSDisplay::getBrightness()); // unDim Backlight
  }
  else {
	DGUSDisplay::displayDimmed = true;
    dgus.setBrightness(1); // Dim backlight
  }
}

// GCTODO rework  tx3m
void DGUSRxHandler::stepsPerMmX(DGUS_VP& vp, void* data_ptr) {
  const int32_t data = dgus.swapBytes(*(int32_t*)data_ptr);
  const float value = dgus.fromFixedPoint<int32_t, float, 2>(data);
  ExtUI::setAxisSteps_per_mm(value, ExtUI::axis_t::X);
}

void DGUSRxHandler::stepsPerMmY(DGUS_VP& vp, void* data_ptr) {
  const int32_t data = dgus.swapBytes(*(int32_t*)data_ptr);
  const float value = dgus.fromFixedPoint<int32_t, float, 2>(data);
  ExtUI::setAxisSteps_per_mm(value, ExtUI::axis_t::Y);
  // Auto save??
  settings.save();
  //  queue.enqueue_now_P(DGUS_CMD_EEPROM_SAVE);
}

void DGUSRxHandler::stepsPerMmZ(DGUS_VP& vp, void* data_ptr) {
  const int32_t data = dgus.swapBytes(*(int32_t*)data_ptr);
  const float value = dgus.fromFixedPoint<int32_t, float, 2>(data);
  ExtUI::setAxisSteps_per_mm(value, ExtUI::axis_t::Z);
}

void DGUSRxHandler::stepsPerMmE(DGUS_VP& vp, void* data_ptr) {
  const int32_t data = dgus.swapBytes(*(int32_t*)data_ptr);
  const float value = dgus.fromFixedPoint<int32_t, float, 2>(data);
  ExtUI::setAxisSteps_per_mm(value, ExtUI::extruder_t::E0);
}

void DGUSRxHandler::jerkStepsMmX(DGUS_VP& vp, void* data_ptr) {
#if ENABLED(CLASSIC_JERK)
  const int16_t data = Swap16(*(int16_t*)data_ptr);
  const float value = dgus.fromFixedPoint<int16_t, float, 1>(data);
  ExtUI::setAxisMaxJerk_mm_s(value, ExtUI::axis_t::X);
#else
#endif
}

void DGUSRxHandler::jerkStepsMmY(DGUS_VP& vp, void* data_ptr) {
#if ENABLED(CLASSIC_JERK)
  const int16_t data = Swap16(*(int16_t*)data_ptr);
  const float value = dgus.fromFixedPoint<int16_t, float, 1>(data);
  ExtUI::setAxisMaxJerk_mm_s(value, ExtUI::axis_t::Y);
#else
#endif
}

void DGUSRxHandler::jerkStepsMmZ(DGUS_VP& vp, void* data_ptr) {
#if ENABLED(CLASSIC_JERK)
  const int16_t data = Swap16(*(int16_t*)data_ptr);
  const float value = dgus.fromFixedPoint<int16_t, float, 1>(data);
  ExtUI::setAxisMaxJerk_mm_s(value, ExtUI::axis_t::Z);
#else
#endif
}

void DGUSRxHandler::jerkStepsMmE(DGUS_VP& vp, void* data_ptr) {
#if ENABLED(CLASSIC_JERK)
  const int16_t data = Swap16(*(int16_t*)data_ptr);
  const float value = dgus.fromFixedPoint<int16_t, float, 1>(data);
  ExtUI::setAxisMaxJerk_mm_s(value, ExtUI::extruder_t::E0);
#else
#endif
}

void DGUSRxHandler::junctionDeviation(DGUS_VP& vp, void* data_ptr) {
#if HAS_JUNCTION_DEVIATION
  const int16_t data = Swap16(*(int16_t*)data_ptr);
  const float value = dgus.fromFixedPoint<int16_t, float, 3>(data);
  ExtUI::setJunctionDeviation_mm(value);
#else
  screen.setStatusMessage(GET_TEXT_F(DGUS_MSG_FEATURE_NOT_ENABLED));
#endif
}

void DGUSRxHandler::linearAdvance(DGUS_VP& vp, void* data_ptr) {
#if ENABLED(LIN_ADVANCE)
  const int16_t data = Swap16(*(int16_t*)data_ptr);
  const float value = dgus.fromFixedPoint<int16_t, float, 2>(data);
  ExtUI::setLinearAdvance_mm_mm_s(value, ExtUI::extruder_t::E0);
#else
  dgus_screen_handler.SetStatusMessage(GET_TEXT_F(DGUS_MSG_FEATURE_NOT_ENABLED));
#endif
}

void DGUSRxHandler::accelerationX(DGUS_VP& vp, void* data_ptr) {
  const int16_t data = Swap16(*(int16_t*)data_ptr);
  const float value = dgus.fromFixedPoint<int16_t, float, 0>(data);
  ExtUI::setAxisMaxAcceleration_mm_s2(value, ExtUI::axis_t::X);
}

void DGUSRxHandler::accelerationY(DGUS_VP& vp, void* data_ptr) {
  const int16_t data = Swap16(*(int16_t*)data_ptr);
  const float value = dgus.fromFixedPoint<int16_t, float, 0>(data);
  ExtUI::setAxisMaxAcceleration_mm_s2(value, ExtUI::axis_t::Y);
}

void DGUSRxHandler::accelerationZ(DGUS_VP& vp, void* data_ptr) {
  const int16_t data = Swap16(*(int16_t*)data_ptr);
  const float value = dgus.fromFixedPoint<int16_t, float, 0>(data);
  ExtUI::setAxisMaxAcceleration_mm_s2(value, ExtUI::axis_t::Z);
}

void DGUSRxHandler::accelerationE(DGUS_VP& vp, void* data_ptr) {
  const int16_t data = Swap16(*(int16_t*)data_ptr);
  const float value = dgus.fromFixedPoint<int16_t, float, 0>(data);
  ExtUI::setAxisMaxAcceleration_mm_s2(value, ExtUI::extruder_t::E0);
}

void DGUSRxHandler::accelerationPrint(DGUS_VP& vp, void* data_ptr) {
  const int16_t data = Swap16(*(int16_t*)data_ptr);
  const float value = dgus.fromFixedPoint<int16_t, float, 0>(data);
  ExtUI::setPrintingAcceleration_mm_s2(value);
}

void DGUSRxHandler::accelerationRetract(DGUS_VP& vp, void* data_ptr) {
  const int16_t data = Swap16(*(int16_t*)data_ptr);
  const float value = dgus.fromFixedPoint<int16_t, float, 0>(data);
  ExtUI::setRetractAcceleration_mm_s2(value);
}

void DGUSRxHandler::accelerationTravel(DGUS_VP& vp, void* data_ptr) {
  const int16_t data = Swap16(*(int16_t*)data_ptr);
  const float value = dgus.fromFixedPoint<int16_t, float, 0>(data);
  ExtUI::setTravelAcceleration_mm_s2(value);
}

void DGUSRxHandler::maxFeedRateX(DGUS_VP& vp, void* data_ptr) {
  const int16_t data = Swap16(*(int16_t*)data_ptr);
  const feedRate_t value = dgus.fromFixedPoint<int16_t, feedRate_t, 0>(data);
  ExtUI::setAxisMaxFeedrate_mm_s(value, ExtUI::axis_t::X);
}

void DGUSRxHandler::maxFeedRateY(DGUS_VP& vp, void* data_ptr) {
  const int16_t data = Swap16(*(int16_t*)data_ptr);
  const feedRate_t value = dgus.fromFixedPoint<int16_t, feedRate_t, 0>(data);
  ExtUI::setAxisMaxFeedrate_mm_s(value, ExtUI::axis_t::Y);
}

void DGUSRxHandler::maxFeedRateZ(DGUS_VP& vp, void* data_ptr) {
  const int16_t data = Swap16(*(int16_t*)data_ptr);
  const feedRate_t value = dgus.fromFixedPoint<int16_t, feedRate_t, 0>(data);
  ExtUI::setAxisMaxFeedrate_mm_s(value, ExtUI::axis_t::Z);
}

void DGUSRxHandler::maxFeedRateE(DGUS_VP& vp, void* data_ptr) {
  const int16_t data = Swap16(*(int16_t*)data_ptr);
  const feedRate_t value = dgus.fromFixedPoint<int16_t, feedRate_t, 0>(data);
  ExtUI::setAxisMaxFeedrate_mm_s(value, ExtUI::extruder_t::E0);
}

void DGUSRxHandler::minPrintFeedRate(DGUS_VP& vp, void* data_ptr) {
  const int16_t data = Swap16(*(int16_t*)data_ptr);
  const feedRate_t value = dgus.fromFixedPoint<int16_t, feedRate_t, 1>(data);
  ExtUI::setMinFeedrate_mm_s(value);
}

void DGUSRxHandler::minTravelFeedRate(DGUS_VP& vp, void* data_ptr) {
  const int16_t data = Swap16(*(int16_t*)data_ptr);
  const feedRate_t value = dgus.fromFixedPoint<int16_t, feedRate_t, 1>(data);
  ExtUI::setMinTravelFeedrate_mm_s(value);
}

void DGUSRxHandler::debug(DGUS_VP &vp, void *data_ptr) {
  UNUSED(vp);
  UNUSED(data_ptr);

  ++screen.debug_count;

  if (screen.debug_count >= 10) {
    screen.triggerScreenChange(DGUS_ScreenID::DEBUG);
  }
}

void DGUSRxHandler::stringToExtra(DGUS_VP &vp, void *data_ptr) {
  if (!vp.size || !vp.extra) return;
  memcpy(vp.extra, data_ptr, vp.size);
}

#endif // DGUS_LCD_UI_RELOADED
