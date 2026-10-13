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
 *   Changed EEPROM_Reset to EEPROM_Control
 *   Added Advanced Settings
 *   Added Print Status Data
 */

#include "../../../../inc/MarlinConfigPre.h"

#if DGUS_LCD_UI_RELOADED

#include "DGUS_VPList.h"

#include "../config/DGUS_Addr.h"
#include "../DGUSScreenHandler.h"
#include "../DGUSRxHandler.h"
#include "../DGUSTxHandler.h"

#include "../../ui_api.h"
#include "../../../../module/probe.h"
#include "../../../../module/motion.h"
#include "../../../../module/temperature.h"

const char DGUS_MACHINENAME[] PROGMEM = MACHINE_NAME;
// tx3m - The screen's version field shows 16 characters (DGUS_VERSION_LEN), e.g. "2.1.x tx3mV37".
// SHORT_BUILD_VERSION ("bugfix-2.1.x") is too long for it, so only the Marlin series is shown.
const char DGUS_MARLINVERSION[] PROGMEM = "2.1.x " LKPro_FW_VERSION;

#define VP_HELPER(ADDR, SIZE, FLAGS, EXTRA, RXHANDLER, TXHANDLER) \
  { .addr = ADDR, \
  .size = SIZE, \
  .flags = FLAGS, \
  .extra = EXTRA, \
  .rx_handler = RXHANDLER, \
  .tx_handler = TXHANDLER }

#define VP_HELPER_WORD(ADDR, FLAGS, EXTRA, RXHANDLER, TXHANDLER) \
  VP_HELPER(ADDR, 2, FLAGS, EXTRA, RXHANDLER, TXHANDLER)

#define VP_HELPER_DWORD(ADDR, FLAGS, EXTRA, RXHANDLER, TXHANDLER) \
  VP_HELPER(ADDR, 4, FLAGS, EXTRA, RXHANDLER, TXHANDLER)

#define VP_HELPER_RX(ADDR, RXHANDLER) \
  VP_HELPER_WORD(ADDR, VPFLAG_NONE, nullptr, RXHANDLER, nullptr)

#define VP_HELPER_RX_NODATA(ADDR, RXHANDLER) \
  VP_HELPER(ADDR, 0, VPFLAG_NONE, nullptr, RXHANDLER, nullptr)

#define VP_HELPER_TX(ADDR, TXHANDLER) \
  VP_HELPER_WORD(ADDR, VPFLAG_NONE, nullptr, nullptr, TXHANDLER)

#define VP_HELPER_TX_SIZE(ADDR, SIZE, TXHANDLER) \
  VP_HELPER(ADDR, SIZE, VPFLAG_NONE, nullptr, nullptr, TXHANDLER)

#define VP_HELPER_TX_EXTRA(ADDR, EXTRA, TXHANDLER) \
  VP_HELPER_WORD(ADDR, VPFLAG_NONE, EXTRA, nullptr, TXHANDLER)

#define VP_HELPER_TX_AUTO(ADDR, EXTRA, TXHANDLER) \
  VP_HELPER_WORD(ADDR, VPFLAG_AUTOUPLOAD, EXTRA, nullptr, TXHANDLER)

// tx3m
#define RX(HANDLER) &DGUSRxHandler::HANDLER
#define TX(HANDLER) &DGUSTxHandler::HANDLER

#define VP_HELPER_TX_RX(ADDR, HANDLER) \
  VP_HELPER_WORD(ADDR, VPFLAG_NONE, nullptr, RX(HANDLER), TX(HANDLER))

#define VP_HELPER_TX_RX_SIZE(ADDR, SIZE, HANDLER) \
  VP_HELPER(ADDR, SIZE, VPFLAG_NONE, nullptr, RX(HANDLER), TX(HANDLER))

const struct DGUS_VP vp_list[] PROGMEM = {

  // READ-ONLY VARIABLES

  VP_HELPER_RX(DGUS_Addr::SCREENCHANGE,          &DGUSRxHandler::screenChange),
  VP_HELPER_RX(DGUS_Addr::SCREENCHANGE_SD,       &DGUSRxHandler::screenChange),
  VP_HELPER_RX(DGUS_Addr::SCREENCHANGE_Idle,     &DGUSRxHandler::screenChange),
  VP_HELPER_RX(DGUS_Addr::SCREENCHANGE_Printing, &DGUSRxHandler::screenChange),

  #if HAS_MEDIA
    VP_HELPER_RX(DGUS_Addr::SD_SelectFile,   &DGUSRxHandler::selectFile),
    VP_HELPER_RX(DGUS_Addr::SD_Scroll,       &DGUSRxHandler::scroll),
    VP_HELPER_RX_NODATA(DGUS_Addr::SD_Print, &DGUSRxHandler::printFile),
  #endif

  VP_HELPER_RX(DGUS_Addr::STATUS_Abort,  &DGUSRxHandler::printAbort),
  VP_HELPER_RX(DGUS_Addr::STATUS_Pause,  &DGUSRxHandler::printPause),
  VP_HELPER_RX(DGUS_Addr::STATUS_Resume, &DGUSRxHandler::printResume),
  VP_HELPER_RX(DGUS_Addr::STATUS_PrintPause, &DGUSRxHandler::printPauseResume),  // tx3m

  VP_HELPER_RX(DGUS_Addr::ADJUST_SetFeedrate,     &DGUSRxHandler::feedrate),
  VP_HELPER_RX(DGUS_Addr::ADJUST_SetFlowrate_CUR, &DGUSRxHandler::flowrate),
  #if HAS_MULTI_EXTRUDER
    VP_HELPER_RX(DGUS_Addr::ADJUST_SetFlowrate_E0,  &DGUSRxHandler::flowrate),
    VP_HELPER_RX(DGUS_Addr::ADJUST_SetFlowrate_E1,  &DGUSRxHandler::flowrate),
  #endif
  VP_HELPER_RX(DGUS_Addr::ADJUST_SetBabystep,     &DGUSRxHandler::babystepSet),
  VP_HELPER_RX(DGUS_Addr::ADJUST_Babystep,        &DGUSRxHandler::babystep),

  VP_HELPER_RX(DGUS_Addr::TEMP_Preset,        &DGUSRxHandler::tempPreset),
  VP_HELPER_RX(DGUS_Addr::TEMP_SetTarget_Bed, &DGUSRxHandler::tempTarget),
  VP_HELPER_RX(DGUS_Addr::TEMP_SetTarget_H0,  &DGUSRxHandler::tempTarget),
  #if HAS_MULTI_HOTEND
    VP_HELPER_RX(DGUS_Addr::TEMP_SetTarget_H1,  &DGUSRxHandler::tempTarget),
  #endif
  VP_HELPER_RX(DGUS_Addr::TEMP_Cool,          &DGUSRxHandler::tempCool),

  VP_HELPER_RX(DGUS_Addr::STEPPER_Control, &DGUSRxHandler::steppers),

  //jgbrown54 - Added Runout Control
  VP_HELPER_RX(DGUS_Addr::RUNOUT_Control, &DGUSRxHandler::runoutControl),  //tx3m

  VP_HELPER_RX(DGUS_Addr::LEVEL_OFFSET_Set,     &DGUSRxHandler::zOffset),
  VP_HELPER_RX(DGUS_Addr::LEVEL_OFFSET_Step,    &DGUSRxHandler::zOffsetStep),
  VP_HELPER_RX(DGUS_Addr::LEVEL_OFFSET_SetStep, &DGUSRxHandler::zOffsetSetStep),

  VP_HELPER_RX(DGUS_Addr::LEVEL_MANUAL_Point, &DGUSRxHandler::moveToPoint),

  VP_HELPER_RX_NODATA(DGUS_Addr::LEVEL_AUTO_Probe,   &DGUSRxHandler::probe),
  VP_HELPER_RX_NODATA(DGUS_Addr::LEVEL_AUTO_Disable, &DGUSRxHandler::disableABL),

  VP_HELPER_RX(DGUS_Addr::FILAMENT_Select,    &DGUSRxHandler::filamentSelect),
  VP_HELPER_RX(DGUS_Addr::FILAMENT_SetLength, &DGUSRxHandler::filamentLength),
  VP_HELPER_RX(DGUS_Addr::FILAMENT_Move,      &DGUSRxHandler::filamentMove),

  // tx3m
  VP_HELPER_RX(DGUS_Addr::FILAMENT_Load_Unload, &DGUSRxHandler::filamentLoadUnload),

  VP_HELPER_RX(DGUS_Addr::MOVE_Home,    &DGUSRxHandler::home),
  VP_HELPER_RX(DGUS_Addr::MOVE_SetX,    &DGUSRxHandler::move),
  VP_HELPER_RX(DGUS_Addr::MOVE_SetY,    &DGUSRxHandler::move),
  VP_HELPER_RX(DGUS_Addr::MOVE_SetZ,    &DGUSRxHandler::move),
  VP_HELPER_RX(DGUS_Addr::MOVE_Step,    &DGUSRxHandler::moveStep),
  VP_HELPER_RX(DGUS_Addr::MOVE_SetStep, &DGUSRxHandler::moveSetStep),

  VP_HELPER_RX_NODATA(DGUS_Addr::GCODE_Clear,   &DGUSRxHandler::gcodeClear),
  VP_HELPER_RX_NODATA(DGUS_Addr::GCODE_Execute, &DGUSRxHandler::gcodeExecute),

  //jgbrown54 - Changed EEPROM_Reset to EEPROM_Control
  //VP_HELPER_RX(DGUS_Addr::EEPROM_Reset, &DGUSRxHandler::ResetEEPROM),
  VP_HELPER_RX(DGUS_Addr::EEPROM_Control, &DGUSRxHandler::controlEEPROM),  // tx3m

  VP_HELPER_RX(DGUS_Addr::SETTINGS2_Extra, &DGUSRxHandler::settingsExtra),

  VP_HELPER_RX(DGUS_Addr::PID_Select,     &DGUSRxHandler::pidSelect),
  VP_HELPER_RX(DGUS_Addr::PID_SetTemp,    &DGUSRxHandler::pidSetTemp),
  VP_HELPER_RX_NODATA(DGUS_Addr::PID_Run, &DGUSRxHandler::pidRun),

  #if ENABLED(POWER_LOSS_RECOVERY)
    VP_HELPER_RX(DGUS_Addr::POWERLOSS_Abort,  &DGUSRxHandler::powerLossAbort),
    VP_HELPER_RX(DGUS_Addr::POWERLOSS_Resume, &DGUSRxHandler::powerLossResume),
  #endif

  VP_HELPER_RX(DGUS_Addr::WAIT_Abort, &DGUSRxHandler::waitAbort),
  VP_HELPER_RX_NODATA(DGUS_Addr::WAIT_Continue, &DGUSRxHandler::waitContinue),

  VP_HELPER_RX_NODATA(DGUS_Addr::DIM_Backlight, &DGUSRxHandler::dimBacklight),  // tx3m

  // WRITE-ONLY VARIABLES

  #if HAS_MEDIA
    VP_HELPER_TX(DGUS_Addr::SD_Type, &DGUSTxHandler::fileType),
    VP_HELPER_TX_SIZE(DGUS_Addr::SD_FileName0,
                      DGUS_FILENAME_LEN,
                      &DGUSTxHandler::fileName),
    VP_HELPER_TX_SIZE(DGUS_Addr::SD_FileName1,
                      DGUS_FILENAME_LEN,
                      &DGUSTxHandler::fileName),
    VP_HELPER_TX_SIZE(DGUS_Addr::SD_FileName2,
                      DGUS_FILENAME_LEN,
                      &DGUSTxHandler::fileName),
    VP_HELPER_TX_SIZE(DGUS_Addr::SD_FileName3,
                      DGUS_FILENAME_LEN,
                      &DGUSTxHandler::fileName),
    VP_HELPER_TX_SIZE(DGUS_Addr::SD_FileName4,
                      DGUS_FILENAME_LEN,
                      &DGUSTxHandler::fileName),
    VP_HELPER_TX(DGUS_Addr::SD_ScrollIcons, &DGUSTxHandler::scrollIcons),
    VP_HELPER_TX_SIZE(DGUS_Addr::SD_SelectedFileName,
                      DGUS_FILENAME_LEN,
                      &DGUSTxHandler::selectedFileName),
  #endif

  VP_HELPER_TX_AUTO(DGUS_Addr::STATUS_PositionZ,
                    nullptr,
                    &DGUSTxHandler::zPosition),
  VP_HELPER(DGUS_Addr::STATUS_Elapsed,
            DGUS_ELAPSED_LEN,
            VPFLAG_AUTOUPLOAD,
            nullptr,
            nullptr,
            &DGUSTxHandler::elapsed),
  VP_HELPER_TX_AUTO(DGUS_Addr::STATUS_Percent,
                    nullptr,
                    &DGUSTxHandler::percent),
  VP_HELPER_TX(DGUS_Addr::STATUS_Icons, &DGUSTxHandler::statusIcons),

  VP_HELPER_TX_AUTO(DGUS_Addr::ADJUST_Feedrate,
                    &motion.feedrate_percentage,
                    &DGUSTxHandler::extraToInteger<int16_t>),
  VP_HELPER_TX_AUTO(DGUS_Addr::ADJUST_Flowrate_CUR,
                    nullptr,
                    &DGUSTxHandler::flowrate),
  #if HAS_MULTI_EXTRUDER
    VP_HELPER_TX_AUTO(DGUS_Addr::ADJUST_Flowrate_E0,
                      nullptr,
                      &DGUSTxHandler::flowrate),
    VP_HELPER_TX_AUTO(DGUS_Addr::ADJUST_Flowrate_E1,
                      nullptr,
                      &DGUSTxHandler::flowrate),
  #endif

  VP_HELPER_TX_AUTO(DGUS_Addr::TEMP_Current_Bed,
                    &thermalManager.temp_bed.celsius,
                    (&DGUSTxHandler::extraToFixedPoint<float, 0>)),  // tx3m
  VP_HELPER_TX_AUTO(DGUS_Addr::TEMP_Target_Bed,
                    &thermalManager.temp_bed.target,
                    &DGUSTxHandler::extraToInteger<int16_t>),
  VP_HELPER_TX(DGUS_Addr::TEMP_Max_Bed, &DGUSTxHandler::tempMax),
  VP_HELPER_TX_AUTO(DGUS_Addr::TEMP_Current_H0,
                    &thermalManager.temp_hotend[ExtUI::heater_t::H0].celsius,
                    (&DGUSTxHandler::extraToFixedPoint<float, 0>)),  // tx3m
  VP_HELPER_TX_AUTO(DGUS_Addr::TEMP_Target_H0,
                    &thermalManager.temp_hotend[ExtUI::heater_t::H0].target,
                    &DGUSTxHandler::extraToInteger<int16_t>),
  VP_HELPER_TX(DGUS_Addr::TEMP_Max_H0, &DGUSTxHandler::tempMax),
  #if HAS_MULTI_HOTEND
    VP_HELPER_TX_AUTO(DGUS_Addr::TEMP_Current_H1,
                      &thermalManager.temp_hotend[ExtUI::heater_t::H1].celsius,
                     (&DGUSTxHandler::extraToFixedPoint<float, 1>)),
    VP_HELPER_TX_AUTO(DGUS_Addr::TEMP_Target_H1,
                      &thermalManager.temp_hotend[ExtUI::heater_t::H1].target,
                      &DGUSTxHandler::extraToInteger<int16_t>),
    VP_HELPER_TX(DGUS_Addr::TEMP_Max_H1, &DGUSTxHandler::tempMax),
  #endif

  VP_HELPER_TX_AUTO(DGUS_Addr::STEPPER_Status,
                    nullptr,
                    &DGUSTxHandler::stepperStatus),

#if HAS_LEVELING
  VP_HELPER_TX_AUTO(DGUS_Addr::LEVEL_OFFSET_Current,
                    &probe.offset.z,
                    (&DGUSTxHandler::extraToFixedPoint<float, 2>)),
  VP_HELPER_TX_EXTRA(DGUS_Addr::LEVEL_OFFSET_StepIcons,
                     &DGUSScreenHandler::offset_steps,
                     &DGUSTxHandler::stepIcons),

  VP_HELPER_TX_AUTO(DGUS_Addr::LEVEL_AUTO_DisableIcon,
                    nullptr,
                    &DGUSTxHandler::ablDisableIcon),
  VP_HELPER_TX(DGUS_Addr::LEVEL_AUTO_Grid, &DGUSTxHandler::ablGrid),

  // VP_HELPER(DGUS_Addr::LEVEL_AUTO_Grid, sizeof(int16_t) * DGUS_LEVEL_GRID_SIZE, VPFLAG_NONE, nullptr, nullptr, &DGUSTxHandler::ablGrid),
  VP_HELPER(DGUS_Addr::SP_LEVEL_AUTO_Grid, 2, VPFLAG_NONE, nullptr, nullptr, &DGUSTxHandler::ablGridColor),
#else
    VP_HELPER_TX_AUTO(DGUS_Addr::LEVEL_OFFSET_Current,
                        &DGUSScreenHandler::baby_offset,
                        (&DGUSTxHandler::extraToFixedPoint<float, 2>)),
#endif

  VP_HELPER_TX_EXTRA(DGUS_Addr::LEVEL_PROBING_Icons1,
                     &DGUSScreenHandler::probing_icons[0],
                     &DGUSTxHandler::extraToInteger<uint16_t>),
  VP_HELPER_TX_EXTRA(DGUS_Addr::LEVEL_PROBING_Icons2,
                     &DGUSScreenHandler::probing_icons[1],
                     &DGUSTxHandler::extraToInteger<uint16_t>),

  VP_HELPER_TX(DGUS_Addr::FILAMENT_ExtruderIcons, &DGUSTxHandler::filamentIcons),
  VP_HELPER_TX_EXTRA(DGUS_Addr::FILAMENT_Length,
                     &DGUSScreenHandler::filament_length,
                     &DGUSTxHandler::extraToInteger<uint16_t>),

  VP_HELPER_TX_AUTO(DGUS_Addr::MOVE_CurrentX,
                    &motion.position.x,
                    (&DGUSTxHandler::extraToFixedPoint<float, 1>)),
  VP_HELPER_TX_AUTO(DGUS_Addr::MOVE_CurrentY,
                    &motion.position.y,
                    (&DGUSTxHandler::extraToFixedPoint<float, 1>)),
  VP_HELPER_TX_AUTO(DGUS_Addr::MOVE_CurrentZ,
                    &motion.position.z,
                    (&DGUSTxHandler::extraToFixedPoint<float, 1>)),
  VP_HELPER_TX_EXTRA(DGUS_Addr::MOVE_StepIcons,
                     &DGUSScreenHandler::move_steps,
                     &DGUSTxHandler::stepIcons),

  VP_HELPER_TX(DGUS_Addr::SETTINGS2_BLTouch, &DGUSTxHandler::blTouch),

  VP_HELPER_TX(DGUS_Addr::PID_HeaterIcons, &DGUSTxHandler::pidIcons),
  VP_HELPER_TX_EXTRA(DGUS_Addr::PID_Temp,
                     &DGUSScreenHandler::pid_temp,
                     &DGUSTxHandler::extraToInteger<uint16_t>),
  VP_HELPER_DWORD(DGUS_Addr::PID_Kp,
                  VPFLAG_AUTOUPLOAD,
                  nullptr,
                  nullptr,
                  &DGUSTxHandler::pidKp),
  VP_HELPER_DWORD(DGUS_Addr::PID_Ki,
                  VPFLAG_AUTOUPLOAD,
                  nullptr,
                  nullptr,
                  &DGUSTxHandler::pidKi),
  VP_HELPER_DWORD(DGUS_Addr::PID_Kd,
                  VPFLAG_AUTOUPLOAD,
                  nullptr,
                  nullptr,
                  &DGUSTxHandler::pidKd),

  VP_HELPER(DGUS_Addr::INFOS_Machine,
            DGUS_MACHINE_LEN,
            VPFLAG_NONE,
            (void*)DGUS_MACHINENAME,
            nullptr,
            &DGUSTxHandler::extraPGMToString),
  VP_HELPER_TX_SIZE(DGUS_Addr::INFOS_BuildVolume,
                    DGUS_BUILDVOLUME_LEN,
                    &DGUSTxHandler::buildVolume),
  VP_HELPER(DGUS_Addr::INFOS_Version,
            DGUS_VERSION_LEN,
            VPFLAG_NONE,
            (void*)DGUS_MARLINVERSION,
            nullptr,
            &DGUSTxHandler::extraPGMToString),
  VP_HELPER_TX(DGUS_Addr::INFOS_TotalPrints, &DGUSTxHandler::totalPrints),
  VP_HELPER_TX(DGUS_Addr::INFOS_FinishedPrints, &DGUSTxHandler::finishedPrints),
  VP_HELPER_TX_SIZE(DGUS_Addr::INFOS_PrintTime,
                    DGUS_PRINTTIME_LEN,
                    &DGUSTxHandler::printTime),
  VP_HELPER_TX_SIZE(DGUS_Addr::INFOS_LongestPrint,
                    DGUS_LONGESTPRINT_LEN,
                    &DGUSTxHandler::longestPrint),
  VP_HELPER_TX_SIZE(DGUS_Addr::INFOS_FilamentUsed,
                    DGUS_FILAMENTUSED_LEN,
                    &DGUSTxHandler::filamentUsed),

  VP_HELPER_TX(DGUS_Addr::WAIT_Icons, &DGUSTxHandler::waitIcons),
  // jgbrown54 tx3m
  VP_HELPER_TX_AUTO(DGUS_Addr::FAN0_Speed_CUR, nullptr, &DGUSTxHandler::fanSpeed),
  VP_HELPER_TX_AUTO(DGUS_Addr::STATUS_Feedrate_MMS, nullptr, &DGUSTxHandler::feedrateMMS),
  VP_HELPER_TX_AUTO(DGUS_Addr::STATUS_Pause_Resume_Icon, nullptr, &DGUSTxHandler::statusIcons),
  // tx3m

  // READ-WRITE VARIABLES

  VP_HELPER(DGUS_Addr::FAN0_Speed,
            2,
            VPFLAG_AUTOUPLOAD,
            nullptr,
            &DGUSRxHandler::fanSpeed,
            &DGUSTxHandler::fanSpeed),

  VP_HELPER(DGUS_Addr::GCODE_Data,
            DGUS_GCODE_LEN,
            VPFLAG_RXSTRING,
            (void*)DGUSScreenHandler::gcode,
            &DGUSRxHandler::stringToExtra,
            &DGUSTxHandler::extraToString),

  VP_HELPER(DGUS_Addr::PID_Cycles,
            2,
            VPFLAG_NONE,
            &DGUSScreenHandler::pid_cycles,
            &DGUSRxHandler::integerToExtra<uint8_t>,
            &DGUSTxHandler::extraToInteger<uint8_t>),

  VP_HELPER(DGUS_Addr::VOLUME_Level,
            2,
            VPFLAG_NONE,
            nullptr,
            &DGUSRxHandler::volume,
            &DGUSTxHandler::volume),

  VP_HELPER(DGUS_Addr::BRIGHTNESS_Level,
            2,
            VPFLAG_NONE,
            nullptr,
            &DGUSRxHandler::brightness,
            &DGUSTxHandler::brightness),

  // jgbrown54 - Added Advanced Settings tx3m
  VP_HELPER_TX_RX_SIZE(DGUS_Addr::X_Steps_mm, 4, stepsPerMmX),
  VP_HELPER_TX_RX_SIZE(DGUS_Addr::Y_Steps_mm, 4, stepsPerMmY),
  VP_HELPER_TX_RX_SIZE(DGUS_Addr::Z_Steps_mm, 4, stepsPerMmZ),
  VP_HELPER_TX_RX_SIZE(DGUS_Addr::E_Steps_mm, 4, stepsPerMmE),
  VP_HELPER_TX_RX(DGUS_Addr::X_Jerk_Steps_mm, jerkStepsMmX),
  VP_HELPER_TX_RX(DGUS_Addr::Y_Jerk_Steps_mm, jerkStepsMmY),
  VP_HELPER_TX_RX(DGUS_Addr::Z_Jerk_Steps_mm, jerkStepsMmZ),
  VP_HELPER_TX_RX(DGUS_Addr::E_Jerk_Steps_mm, jerkStepsMmE),
  VP_HELPER_TX_RX(DGUS_Addr::JunctionDeviation, junctionDeviation),
  VP_HELPER_TX_RX(DGUS_Addr::Linear_Advance, linearAdvance),
  VP_HELPER_TX_RX(DGUS_Addr::X_Acceleration, accelerationX),
  VP_HELPER_TX_RX(DGUS_Addr::Y_Acceleration, accelerationY),
  VP_HELPER_TX_RX(DGUS_Addr::Z_Acceleration, accelerationZ),
  VP_HELPER_TX_RX(DGUS_Addr::E_Acceleration, accelerationE),
  VP_HELPER_TX_RX(DGUS_Addr::Print_Acceleration, accelerationPrint),
  VP_HELPER_TX_RX(DGUS_Addr::Retract_Acceleration, accelerationRetract),
  VP_HELPER_TX_RX(DGUS_Addr::Travel_Acceleration, accelerationTravel),
  VP_HELPER_TX_RX(DGUS_Addr::X_Max_Speed, maxFeedRateX),
  VP_HELPER_TX_RX(DGUS_Addr::Y_Max_Speed, maxFeedRateY),
  VP_HELPER_TX_RX(DGUS_Addr::Z_Max_Speed, maxFeedRateZ),
  VP_HELPER_TX_RX(DGUS_Addr::E_Max_Speed, maxFeedRateE),
  VP_HELPER_TX_RX(DGUS_Addr::Min_Speed, minPrintFeedRate),
  VP_HELPER_TX_RX(DGUS_Addr::Min_Travel_Speed, minTravelFeedRate),

  // SPECIAL CASES

  VP_HELPER_TX(DGUS_Addr::STATUS_Percent_Complete, &DGUSTxHandler::percent),
  VP_HELPER_RX_NODATA(DGUS_Addr::INFOS_Debug, &DGUSRxHandler::debug),

  VP_HELPER((DGUS_Addr)0, 0, VPFLAG_NONE, nullptr, nullptr, nullptr)

};

#endif // DGUS_LCD_UI_RELOADED
