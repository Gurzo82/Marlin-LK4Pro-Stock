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
 * jgbrown54 - Added Advanced Settings
 */
#pragma once

#include "DGUSDisplay.h"
#include "definition/DGUS_VP.h"

namespace DGUSTxHandler {

  #if HAS_MEDIA
    void setFileControlState(int16_t, bool);
    void fileType(DGUS_VP &);
    void fileName(DGUS_VP &);
    void scrollIcons(DGUS_VP &);
    void selectedFileName(DGUS_VP &);
  #endif

  void zPosition(DGUS_VP &);
  void elapsed(DGUS_VP &);
  void percent(DGUS_VP &);
  void statusIcons(DGUS_VP &);

  void flowrate(DGUS_VP &);

  void tempMax(DGUS_VP &);

  void stepperStatus(DGUS_VP &);

  void stepIcons(DGUS_VP &);

  void ablDisableIcon(DGUS_VP &);
  void ablGrid(DGUS_VP &);
  void ablGridColor(DGUS_VP& vp);  // tx3m

  void filamentIcons(DGUS_VP &);

  void blTouch(DGUS_VP &);

  void pidIcons(DGUS_VP &);
  void pidKp(DGUS_VP &);
  void pidKi(DGUS_VP &);
  void pidKd(DGUS_VP &);

  void buildVolume(DGUS_VP &);
  void totalPrints(DGUS_VP &);
  void finishedPrints(DGUS_VP &);
  void printTime(DGUS_VP &);
  void longestPrint(DGUS_VP &);
  void filamentUsed(DGUS_VP &);

  void waitIcons(DGUS_VP &);

  void fanSpeed(DGUS_VP &);
  void feedrateMMS(DGUS_VP& vp);  // tx3m
  void volume(DGUS_VP &);

  void brightness(DGUS_VP &);

  // Motion settings tx3m
  void stepsPerMmX(DGUS_VP& vp);
  void stepsPerMmY(DGUS_VP& vp);
  void stepsPerMmZ(DGUS_VP& vp);
  void stepsPerMmE(DGUS_VP& vp);
  void jerkStepsMmX(DGUS_VP& vp);
  void jerkStepsMmY(DGUS_VP& vp);
  void jerkStepsMmZ(DGUS_VP& vp);
  void jerkStepsMmE(DGUS_VP& vp);
  void junctionDeviation(DGUS_VP& vp);
  void linearAdvance(DGUS_VP& vp);
  void accelerationX(DGUS_VP& vp);
  void accelerationY(DGUS_VP& vp);
  void accelerationZ(DGUS_VP& vp);
  void accelerationE(DGUS_VP& vp);
  void accelerationPrint(DGUS_VP& vp);
  void accelerationRetract(DGUS_VP& vp);
  void accelerationTravel(DGUS_VP& vp);
  void maxFeedRateX(DGUS_VP& vp);
  void maxFeedRateY(DGUS_VP& vp);
  void maxFeedRateZ(DGUS_VP& vp);
  void maxFeedRateE(DGUS_VP& vp);
  void minPrintFeedRate(DGUS_VP& vp);
  void minTravelFeedRate(DGUS_VP& vp);

  void extraToString(DGUS_VP &);
  void extraPGMToString(DGUS_VP &);

  template<typename T>
  void extraToInteger(DGUS_VP &vp) {
    if (!vp.size || !vp.extra) return;
    switch (vp.size) {
      default: return;
      case 1: {
        const uint8_t data = uint8_t(*(T*)vp.extra);
        dgus.write(uint16_t(vp.addr), data);
        break;
      }
      case 2: {
        const uint16_t data = uint16_t(*(T*)vp.extra);
        dgus.write(uint16_t(vp.addr), Swap16(data));
        break;
      }
      case 4: {
        const uint32_t data = uint32_t(*(T*)vp.extra);
        dgus.write(uint16_t(vp.addr), dgus.swapBytes(data));
        break;
      }
    }
  }

  template<typename T, uint8_t decimals>
  void extraToFixedPoint(DGUS_VP &vp) {
    if (!vp.size || !vp.extra) return;
    switch (vp.size) {
      default: return;
      case 1: {
        const uint8_t data = dgus.toFixedPoint<T, uint8_t, decimals>(*(T*)vp.extra);
        dgus.write(uint16_t(vp.addr), data);
        break;
      }
      case 2: {
        const uint16_t data = dgus.toFixedPoint<T, uint16_t, decimals>(*(T*)vp.extra);
        dgus.write(uint16_t(vp.addr), Swap16(data));
        break;
      }
      case 4: {
        const uint32_t data = dgus.toFixedPoint<T, uint32_t, decimals>(*(T*)vp.extra);
        dgus.write(uint16_t(vp.addr), dgus.swapBytes(data));
        break;
      }
    }
  }

}
