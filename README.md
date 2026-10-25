<p align="center"><img src="buildroot/share/pixmaps/logo/marlin-outrun-nf-500.png" height="200" alt="Marlin logo" /></p>

<h1 align="center">Marlin for the Longer LK4 Pro (DWIN touch screen)</h1>

<p align="center">
  <a href="/LICENSE"><img alt="GPL-V3.0 License" src="https://img.shields.io/github/license/marlinfirmware/marlin.svg"></a>
</p>

This is [Marlin](https://github.com/MarlinFirmware/Marlin) for the **Longer LK4 Pro** with its original LGT
mainboard and DWIN touch screen. Longer's stock firmware is based on a much older Marlin. This one tracks Marlin's
current `bugfix-2.1.x` development branch, and comes with a redesigned touch screen firmware driven by Marlin's
DGUS Reloaded screen driver.

It builds on years of community work: Guizz27's original LK4/LK5 Pro port and screen design, jgbrown54's update to
Marlin 2.1.2.1 and screen redesign, and this fork's port to current Marlin (see [Credits](#credits)).

> **Use at your own risk.** Flashing firmware can leave a printer unusable until it is flashed again.
> The configuration contains values for the author's own printer (see [Configuration](#configuration)), so check them
> before printing.

The firmware version (`V<N>`) is set automatically from git when building (see [Building](#building)).

## Branches

| Branch | Contents |
| --- | --- |
| `lk4pro` | The LK4 Pro firmware (default branch). |
| `bugfix-2.1.x` | Unmodified copy of official Marlin, used to bring in upstream updates. |

## Requirements

- Longer LK4 Pro with the LGT mainboard. An LK5 Pro option exists in `Configuration.h` but is untested.
- Pick the firmware file that matches your extruder driver: `LK4Pro` for the stock driver, `LK4Pro-SilentE` if you
  replaced it with a silent TMC2208.
- The DWIN touch screen with the **matching screen firmware**: [tx3m/DWIN_LK4Pro](https://github.com/tx3m/DWIN_LK4Pro).
  The screen and printer firmware must be updated together.
- Screens labelled "TL5", "DWJT" or "DWJTB" in their About screen can't be updated with this screen firmware.
- Optional: BLTouch (`LKPro_BLTOUCH`) for automatic bed leveling.

> **LK4 Pro without BLTouch:** this configuration is written for a printer *with* the probe, and the DGUS
> touch screen's Leveling menu needs it (bed mesh + Z offset babystepping). On a **stock LK4 Pro, which has
> no probe**, the screen firmware still works, but the Leveling menu has to be used for manual leveling and
> "Auto level" can't do anything. A configuration adapted to the stock printer (probe disabled, factory
> axis steps and feedrates, manual leveling) is available in
> [Gurzo82/Marlin-LK4Pro-Stock](https://github.com/Gurzo82/Marlin-LK4Pro-Stock), together with prebuilt
> firmware and a one-command updater.

## Compared with the stock firmware

**Newer Marlin features enabled in this configuration:**

- Linear Advance (pressure advance), adjustable from the screen.
- Bilinear auto bed leveling with BLTouch, Z safe homing, and Z offset babystepping while printing.
- Park on pause with host prompts (OctoPrint), filament runout sensor, power-loss recovery.
- Arc moves (`G2`/`G3`), bed PID, print statistics, automatic temperature reports to the host.
- Input shaping is supported by this Marlin version, but not enabled in this configuration.

**Touch screen firmware:** a full redesign of Longer's original screens, started by Guizz27 and extended by
jgbrown54 and in this fork. See the screen firmware repository for screenshots.

## Touch screen features

- Print from SD card with progress, elapsed time, Z height, speed and fan display.
- **Pause / Resume / Abort** that work with Marlin's park-on-pause:
  - Pause parks the nozzle (back left) and shows the **Resume** button.
  - Abort while paused stops the print properly: heaters off, no power-loss prompt afterwards, and the screen shows
    "Print Aborted" with the elapsed time.
  - A pause from OctoPrint is shown on the screen (needs the scripts below).
- Auto bed leveling with the measured values shown on the grid (5×5 with BLTouch, otherwise 3×3), Z offset
  (0.1 / 0.01 mm) and babystepping while printing.
- Advanced settings: steps/mm, jerk, junction deviation, linear advance, accelerations and maximum speeds.
- EEPROM save / restore / factory reset, filament runout sensor on/off, PID tuning, filament load/unload,
  power-loss recovery, screen brightness and volume.

## Building

1. Install [Visual Studio Code](https://code.visualstudio.com/) with the
   [PlatformIO](https://platformio.org/) extension.
2. Clone this repository and open it in VS Code:
   ```
   git clone https://github.com/tx3m/Marlin-LK4Pro.git
   ```
3. Check the settings in `Marlin/Configuration.h` (see below).
4. Build with the PlatformIO **Build** button or `pio run`. This builds two firmware files:

   | Environment | Extruder driver | File, e.g. |
   | --- | --- | --- |
   | `LK4Pro` | Stock A4988 | `.pio/build/LK4Pro/bugfix-2.1.x_V29_LK4Pro.hex` |
   | `LK4Pro_SilentE` | Replaced with a silent TMC2208 | `.pio/build/LK4Pro_SilentE/bugfix-2.1.x_V29_LK4Pro-SilentE.hex` |

   Use `pio run -e LK4Pro` or `pio run -e LK4Pro_SilentE` to build only one.
   A copy of every build is also kept in `.pio/firmware/`, so older versions stay available for comparison.

The version number comes from git: the newest `v<N>` tag plus the number of commits on this branch since then.
Merging official Marlin counts as one commit. A build with uncommitted changes gets `-dev` added (e.g. `V29-dev`).
Without git, for example from a downloaded ZIP, the `LKPro_FW_VERSION` fallback in `Configuration.h` is used.
The setup is in `ini/lk4pro.ini` and `buildroot/share/PlatformIO/scripts/lk4pro_version.py`.

## Flashing

- **Printer:** flash `firmware.hex` over USB, for example with PlatformIO's **Upload** or OctoPrint's Firmware Updater
  plugin. Do a factory reset from the screen afterwards (Settings → EEPROM), as new versions can change the stored
  settings.
- **Screen:** copy the `DWIN_SET` folder of the matching screen firmware to the root of a FAT32 SD card
  (32 GB or smaller, 4096-byte allocation unit), insert it into the screen's own card slot and power on. Full
  instructions come with the screen firmware.

## Configuration

The options at the top of `Marlin/Configuration.h` select the printer:

| Option | Meaning |
| --- | --- |
| `LGT_MAC` | LK4/LK5 Pro family with the LGT mainboard. Keep enabled. |
| `LK4_Pro` / `LK5_Pro` | Printer model. `LK4_Pro` is the default. |
| `LKPro_BLTOUCH` | Enable if a BLTouch is fitted. |
| `LKPro_SILENT_E_DRIVER` | Enable if the extruder's A4988 driver was replaced with a silent TMC2208 (set automatically by `LK4Pro_SilentE`). If the extruder then runs backwards, set `INVERT_E0_DIR` to `false` or turn the motor plug around. |
| `LKPro_FW_VERSION` | Version text shown on the screen. |

Values tuned for the author's printer that you should check on yours:

- `DEFAULT_AXIS_STEPS_PER_UNIT` is `{ 79.5, 80, 405, 145 }` (stock LK4 Pro: `{ 80, 80, 400, 96 }`).
- `BAUDRATE` is `250000`, so set your host (e.g. OctoPrint) to 250000 baud.

`Marlin/Config-export.h` lists all active settings in one short file, which is handy for comparing configurations.

## OctoPrint

When a print is streamed from OctoPrint, pausing in OctoPrint only runs OctoPrint's own scripts. These scripts park
the nozzle like the screen does, and tell the printer about the pause so the screen shows it. They assume the slicer
uses relative extrusion (`M83`).

Add them under **OctoPrint Settings → GCODE Scripts**:

**After print job is paused**
```
{% if pause_position.x is not none %}
G91                 ; relative moves
G1 E-5 F2400        ; retract
G90                 ; absolute XYZ (this also sets E to absolute)
M83                 ; E back to relative, like the slicer
G27                 ; park like the screen: back left, Z at least 20 mm
M76                 ; pause the print timer: the screen shows "Print Paused" and Resume
{% endif %}
```

**Before print job is resumed**
```
{% if pause_position.x is not none %}
G90                 ; absolute XYZ
M83                 ; relative E, like the slicer
G1 E10 F300         ; at the park position: undo the retract and purge 5 mm
G1 E-1 F2400        ; small retract for the move back
G1 X{{ pause_position.x }} Y{{ pause_position.y }} F6000
G1 Z{{ pause_position.z }} F600
G1 E1 F2400         ; undo the small retract
{% if pause_position.f is not none %}G1 F{{ pause_position.f }}{% endif %}
M75                 ; resume the print timer
{% endif %}
```

**After print job is cancelled**
```
G91                 ; relative moves
G1 E-2 F2400        ; retract while the nozzle is still hot
G1 Z10 F600         ; lift 10 mm
G90                 ; absolute XYZ
M84                 ; disable motors
{% snippet 'disable_hotends' %}
{% snippet 'disable_bed' %}
M106 S0             ; fan off
```

## Credits

| Who | Contribution |
| --- | --- |
| [MarlinFirmware](https://github.com/MarlinFirmware/Marlin) | Marlin, including the DGUS Reloaded screen driver. |
| [Guizz27](https://github.com/Guizz27) | The original LK4/LK5 Pro port of Marlin 2 ([Marlin---Longer-LKx-Pro](https://github.com/Guizz27/Marlin---Longer-LKx-Pro)) and the screen firmware design ([DWIN_LK4Pro](https://github.com/Guizz27/DWIN_LK4Pro)), replacing Longer's original screens. Includes the advanced settings, EEPROM, runout control, PID and G-code entry screens. |
| [jgbrown54](https://github.com/jgbrown54) | Update to Marlin 2.1.2.1 ("JGB2.2B"). Reworked the screen firmware to match Marlin's DGUS Reloaded layout: separate Pause / Resume / Abort buttons, new bed leveling grid, redesigned home, print status, move, filament, advanced settings and runout screens, a new G-code keyboard ([DWIN_LK4Pro #22](https://github.com/Guizz27/DWIN_LK4Pro/issues/22)), status messages on more screens, and EEPROM save / restore messages. |
| [tx3m](https://github.com/tx3m) | Port to current Marlin `bugfix-2.1.x`: screen address fixes, pause / resume / abort rework, OctoPrint pause integration, bed leveling view and Z offset fixes. |

## Bugs and questions

Please use this repository's [Issues](https://github.com/tx3m/Marlin-LK4Pro/issues) for problems with this firmware,
not Marlin's issue tracker. Fixes that apply to all printers are sent to official Marlin as separate pull requests.

## License

Marlin is published under the [GPL v3 license](/LICENSE), and so is this fork. You may use, modify and share it, but
any firmware you distribute must come with its source code. Pointing people to this repository satisfies that.
