#!/usr/bin/env python3
"""
flash-lk4pro.py - Carica un firmware .hex sulla Longer LK4 Pro (mainboard LGT KIT V1.0, ATmega2560).

Perche' non usare avrdude?
  Il bootloader Arduino stk500v2 di questa mainboard ascolta il primo pacchetto STK500
  per soli ~20-40 ms dopo il reset; avrdude aspetta ~300 ms prima di parlare, riceve
  quindi il banner di Marlin ("start") e fallisce con "not in sync: resp=0x73".
  Qui si entra nel bootloader e si invia subito il pacchetto SIGN_ON, poi si scrivono
  le pagine da 256 byte con il protocollo STK500v2.

Uso:
    python flash-lk4pro.py                       # prende l'hex piu' recente in firmware/
    python flash-lk4pro.py firmware/LK4Pro.hex   # file specifico
    python flash-lk4pro.py file.hex --port COM5  # porta specifica
    python flash-lk4pro.py file.hex --no-verify  # salta la verifica finale

Requisiti: pip install pyserial
"""

import argparse
import glob
import os
import re
import sys
import time

try:
    import serial
except ImportError:
    print("Manca pyserial. Installalo con:  pip install pyserial", file=sys.stderr)
    sys.exit(1)

# ---------------------------------------------------------------- protocollo
MESSAGE_START = 0x1B
TOKEN = 0x0E
CMD_SIGN_ON = 0x01
CMD_LOAD_ADDRESS = 0x06
CMD_PROGRAM_FLASH_ISP = 0x13
CMD_LEAVE_PROGMODE_ISP = 0x11
STATUS_OK = 0x00
PAGE_SIZE = 256            # SPM_PAGESIZE dell'ATmega2560
BAUD_BOOTLOADER = 115200   # velocita' del bootloader
BAUD_MARLIN = 250000       # velocita' di Marlin

# ---------------------------------------------------------------------------


def read_intel_hex(path):
    """Legge un .hex Intel e restituisce (dati, indirizzo_massimo)."""
    data = bytearray()
    base = 0
    for raw in open(path, "r", encoding="ascii", errors="ignore"):
        line = raw.strip()
        if not line or not line.startswith(":"):
            continue
        b = bytes.fromhex(line[1:])
        count, addr, rtype = b[0], (b[1] << 8) | b[2], b[3]
        if rtype == 0:
            while len(data) < base + addr:
                data.append(0xFF)
            data[base + addr:base + addr + count] = b[4:4 + count]
        elif rtype == 2:
            base = ((b[4] << 8) | b[5]) << 4
        elif rtype == 4:
            base = ((b[4] << 8) | b[5]) << 16
    return bytes(data)


def stk_msg(seq, body):
    """Costruisce un messaggio STK500v2: START seq len_hi len_lo TOKEN body... checksum.
    Il checksum e' XOR di tutto (MESSAGE_START incluso)."""
    m = bytearray([MESSAGE_START, seq])
    m += bytes([(len(body) >> 8) & 0xFF, len(body) & 0xFF, TOKEN])
    m += bytes(body)
    ck = 0
    for x in m:
        ck ^= x
    m.append(ck)
    return bytes(m)


def find_frames(buf):
    """Estrae i frame STK500 completi dal buffer. Ritorna (frames, resto)."""
    frames = []
    i = 0
    while True:
        i = buf.find(MESSAGE_START, i)
        if i < 0:
            return frames, b""
        if len(buf) < i + 5:
            return frames, buf[i:]
        length = (buf[i + 2] << 8) | buf[i + 3]
        total = 6 + length
        if len(buf) < i + total:
            return frames, buf[i:]
        frame = buf[i:i + total]
        ck = 0
        for x in frame[:-1]:
            ck ^= x
        if ck == frame[-1]:
            frames.append((frame[1], frame[5:5 + length]))
            i += total
        else:
            i += 1
    return frames, b""


class Bootloader:
    def __init__(self, port, baud=BAUD_BOOTLOADER):
        self.ser = serial.Serial(port, baud, timeout=0.05, write_timeout=1.0)
        self.buf = b""
        self.seq = 1

    def close(self):
        try:
            self.ser.close()
        except Exception:
            pass

    def _next_seq(self):
        self.seq = self.seq % 255 + 1
        return self.seq

    def send(self, cmd, payload=b""):
        body = bytes([cmd]) + payload
        self.ser.write(stk_msg(self._next_seq(), body))

    def read_frames(self, timeout=0.5):
        end = time.time() + timeout
        got = []
        while time.time() < end:
            try:
                chunk = self.ser.read(self.ser.in_waiting or 1)
                if chunk:
                    self.buf += chunk
            except Exception:
                pass
            frames, self.buf = find_frames(self.buf)
            if frames:
                got += frames
                # appena arriva la risposta attesa basta: non aspettare il timeout
                end = min(end, time.time() + 0.05)
            else:
                time.sleep(0.005)
        return got

    def enter(self, tries=40):
        """Reset + handshake SIGN_ON immediato."""
        for attempt in range(tries):
            self.ser.dtr = True
            self.ser.rts = False
            time.sleep(0.35)
            self.ser.reset_input_buffer()
            self.ser.dtr = False          # rilascio del reset: entra il bootloader
            # burst immediato: i byte devono arrivare entro ~40 ms
            burst = b"".join(stk_msg((i % 255) + 1, bytes([CMD_SIGN_ON])) for i in range(12))
            self.ser.write(burst)
            frames = self.read_frames(0.4)
            for seq, body in frames:
                # body = [comando echoato, status, ...] ; "AVRISP_2" parte da body[3]
                if b"AVRISP" in body:
                    return True
            time.sleep(0.05)
        return False


def flash(path, port, do_verify=True):
    data = read_intel_hex(path)
    if not data:
        print("File .hex vuoto o non leggibile:", path, file=sys.stderr)
        return 2
    pages = (len(data) + PAGE_SIZE - 1) // PAGE_SIZE
    print(f"  File    : {os.path.basename(path)}  ({len(data)} byte, {pages} pagine)")

    bl = Bootloader(port)
    try:
        print("  Ingresso nel bootloader...")
        if not bl.enter():
            print("  ERRORE: il bootloader non risponde.", file=sys.stderr)
            print("  - La stampante e' accesa? il cavo USB e' collegato?", file=sys.stderr)
            print("  - La porta e' corretta? (prova --port COMx)", file=sys.stderr)
            return 3
        print("  Bootloader raggiunto (AVRISP_2)")

        t0 = time.time()
        for p in range(pages):
            addr = p * PAGE_SIZE
            word = addr >> 1
            chunk = data[addr:addr + PAGE_SIZE]
            chunk = chunk + b"\xFF" * (PAGE_SIZE - len(chunk))

            # LOAD_ADDRESS
            bl.send(CMD_LOAD_ADDRESS, bytes([(word >> 24) & 0xFF, (word >> 16) & 0xFF,
                                             (word >> 8) & 0xFF, word & 0xFF]))
            frames = bl.read_frames(0.6)
            ok = any(len(b) >= 2 and b[1] == STATUS_OK for _, b in frames)

            # PROGRAM_FLASH_ISP
            payload = bytes([(PAGE_SIZE >> 8) & 0xFF, PAGE_SIZE & 0xFF,
                             0x03, 0x00, 0x00, 0x00, 0x46, 0x00, 0x00]) + chunk
            bl.send(CMD_PROGRAM_FLASH_ISP, payload)
            frames = bl.read_frames(0.6)
            ok = any(len(b) >= 2 and b[1] == STATUS_OK for _, b in frames)
            if not ok:
                print(f"\n  ERRORE in scrittura pagina {p}", file=sys.stderr)
                return 4

            if p % 64 == 0 or p == pages - 1:
                pct = int((p + 1) * 100 / pages)
                print(f"    pagine {p + 1}/{pages} ({pct}%)")

        # esci e riavvia
        bl.send(CMD_LEAVE_PROGMODE_ISP)
        time.sleep(0.4)
        print(f"  Scritte {pages} pagine in {int(time.time() - t0)}s")
    finally:
        bl.close()

    if do_verify:
        time.sleep(3)
        try:
            ser = serial.Serial(port, BAUD_MARLIN, timeout=0.3)
            time.sleep(1.5)
            ser.reset_input_buffer()
            ser.write(b"M115\n")
            time.sleep(1.0)
            txt = ""
            end = time.time() + 3
            while time.time() < end:
                chunk = ser.read(ser.in_waiting or 1)
                if chunk:
                    txt += chunk.decode("ascii", "ignore")
                if "FIRMWARE_NAME" in txt:
                    break
            ser.close()
            m = re.search(r"FIRMWARE_NAME:[^\r\n]*", txt)
            if m:
                print("  Nuovo firmware attivo:")
                print("   ", m.group(0))
            else:
                print("  (verifica non riuscita: il firmware dovrebbe comunque essere in esecuzione)")
        except Exception as e:
            print("  Verifica non riuscita:", e)
    return 0


def main():
    ap = argparse.ArgumentParser(description="Flash del firmware Marlin sulla Longer LK4 Pro")
    ap.add_argument("hexfile", nargs="?", help="file .hex (default: il piu' recente in firmware/)")
    ap.add_argument("--port", default=None, help="porta seriale (default: rilevata)")
    ap.add_argument("--no-verify", action="store_true", help="non verificare dopo il flash")
    args = ap.parse_args()

    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(here)

    hexfile = args.hexfile
    if not hexfile:
        candidates = glob.glob(os.path.join(root, "firmware", "*.hex"))
        if not candidates:
            print("Nessun file .hex trovato in firmware/. Scaricalo dalle Release.", file=sys.stderr)
            return 1
        hexfile = max(candidates, key=os.path.getmtime)
    if not os.path.isfile(hexfile):
        print("File non trovato:", hexfile, file=sys.stderr)
        return 1

    port = args.port
    if not port:
        try:
            ports = [p.device for p in serial.tools.list_ports.comports()]
        except Exception:
            ports = []
        ch340 = [p for p in ports if "CH340" in p.upper() or "CH341" in p.upper() or "USB-SERIAL" in p.upper()]
        if ch340:
            port = ch340[0]
        elif len(ports) == 1:
            port = ports[0]
        else:
            print(f"Porte trovate: {ports}\nSpecifica con --port COMx", file=sys.stderr)
            return 1
    print(f"  Porta   : {port}")

    print("\n  Longer LK4 Pro - flash del firmware\n")
    return flash(hexfile, port, not args.no_verify)


if __name__ == "__main__":
    sys.exit(main())
