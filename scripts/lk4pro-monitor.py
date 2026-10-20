#!/usr/bin/env python3
"""
lk4pro-monitor.py - Dashboard diagnostica per la Longer LK4 Pro (Marlin, 250000 baud).

Mostra in tempo reale: posizione, feedrate, temperature, assi in movimento,
ultime notifiche/errori. Con --csv salva i campioni con timestamp.

Uso:
    python lk4pro-monitor.py                  # dashboard live
    python lk4pro-monitor.py --port COM5      # porta specifica
    python lk4pro-monitor.py --csv log.csv    # salva anche un CSV
    python lk4pro-monitor.py --once           # un unico snapshot e esce

NOTA: la porta seriale e' esclusiva -> non usare insieme a OctoPrint/Cura.
Requisito: pip install pyserial
"""

import argparse
import csv
import os
import re
import sys
import time

try:
    import serial
except ImportError:
    print("Manca pyserial. Installalo con:  pip install pyserial", file=sys.stderr)
    sys.exit(1)

BAUD = 250000
POLL = 0.4          # secondi fra due interrogazioni
CSV_FIELDS = ["t", "x", "y", "z", "e", "feedrate", "hotend", "bed", "note"]


def parse_position(text):
    m = re.search(r"X:\s*(-?[\d.]+)\s*Y:\s*(-?[\d.]+)\s*Z:\s*(-?[\d.]+)\s*E:\s*(-?[\d.]+)", text)
    if not m:
        return None
    return tuple(float(v) for v in m.groups())


def parse_feedrate(text):
    m = re.search(r"Feedrate:\s*([\d.]+)", text)
    return float(m.group(1)) if m else None


def parse_temps(text):
    m = re.search(r"T:\s*(-?[\d.]+)\s*/\s*(-?[\d.]+)\s*B:\s*(-?[\d.]+)\s*/\s*(-?[\d.]+)", text)
    if not m:
        return None
    return float(m.group(1)), float(m.group(3))


def clear_screen():
    sys.stdout.write("\x1b[2J\x1b[H")


def main():
    ap = argparse.ArgumentParser(description="Dashboard diagnostica Longer LK4 Pro")
    ap.add_argument("--port", default="COM3", help="porta seriale (default COM3)")
    ap.add_argument("--baud", type=int, default=BAUD, help="baudrate (default 250000)")
    ap.add_argument("--csv", help="salva i campioni in questo file CSV")
    ap.add_argument("--once", action="store_true", help="un solo snapshot ed esce")
    args = ap.parse_args()

    try:
        ser = serial.Serial(args.port, args.baud, timeout=0.2)
    except Exception as e:
        print(f"Impossibile aprire {args.port}: {e}", file=sys.stderr)
        return 1

    csv_file = None
    writer = None
    if args.csv:
        csv_file = open(args.csv, "w", newline="", encoding="utf-8")
        writer = csv.writer(csv_file)
        writer.writerow(CSV_FIELDS)

    pos = feedrate = None
    hotend = bed = None
    notes = []
    t0 = time.time()

    try:
        while True:
            try:
                ser.reset_input_buffer()
                ser.write(b"M114\nM105\n")
                ser.flush()
            except Exception:
                pass

            raw = ""
            deadline = time.time() + 0.9
            while time.time() < deadline:
                try:
                    chunk = ser.read(ser.in_waiting or 1)
                    if chunk:
                        raw += chunk.decode("ascii", "ignore")
                except Exception:
                    pass
                if "ok" in raw and raw.count("ok") >= 2:
                    break

            p = parse_position(raw)
            if p:
                pos = p
            f = parse_feedrate(raw)
            if f is not None:
                feedrate = f
            t = parse_temps(raw)
            if t:
                hotend, bed = t

            for line in raw.splitlines():
                line = line.strip()
                if line.startswith("//action:") or line.startswith("Error:") or line.startswith("echo:Unknown"):
                    if line and line not in notes:
                        notes.append(line)
                        notes = notes[-6:]

            if args.once:
                print("=== Snapshot Longer LK4 Pro ===")
                print(f"Posizione   : {pos}")
                print(f"Feedrate    : {feedrate} mm/s")
                print(f"Hotend / Bed: {hotend} / {bed} °C")
                for n in notes:
                    print(f"  {n}")
                break

            if writer and pos:
                writer.writerow([round(time.time() - t0, 2), *pos,
                                 feedrate if feedrate is not None else "",
                                 hotend if hotend is not None else "",
                                 bed if bed is not None else "",
                                 " | ".join(notes[-1:])])
                csv_file.flush()

            clear_screen()
            print("  Longer LK4 Pro - diagnostica (Marlin, DGUS)          Ctrl+C per usciere")
            print("  " + "-" * 62)
            print(f"  Porta        : {args.port} @ {args.baud}")
            if pos:
                print(f"  Posizione    : X {pos[0]:8.2f}   Y {pos[1]:8.2f}   Z {pos[2]:8.2f}   E {pos[3]:8.2f}")
            else:
                print("  Posizione    : (in attesa)")
            print(f"  Feedrate     : {feedrate} mm/s" if feedrate is not None else "  Feedrate     : -")
            if hotend is not None:
                print(f"  Temperature  : hotend {hotend:6.1f} °C    bed {bed:6.1f} °C")
            else:
                print("  Temperature  : -")
            print("  " + "-" * 62)
            if notes:
                for n in notes:
                    print(f"  {n}")
            print(f"\n  t = {time.time() - t0:6.1f} s" + (f"   |  CSV: {args.csv}" if args.csv else ""))
            time.sleep(POLL)

    except KeyboardInterrupt:
        print("\nInterrotto.")
    finally:
        if csv_file:
            csv_file.close()
        try:
            ser.close()
        except Exception:
            pass
    return 0


if __name__ == "__main__":
    sys.exit(main())
