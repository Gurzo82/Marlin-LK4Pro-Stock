#!/usr/bin/env bash
# Build del firmware Longer LK4 Pro (variante: stock | plus | silentE)
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

VARIANT="${1:-stock}"
case "$VARIANT" in
  stock)   ENV=LK4Pro ;;
  plus)    ENV=LK4Pro_Plus ;;
  silentE) ENV=LK4Pro_SilentE ;;
  *) echo "Uso: $0 [stock|plus|silentE]"; exit 1 ;;
esac

command -v pio >/dev/null 2>&1 || { echo "Serve PlatformIO: pip install platformio"; exit 1; }
pio pkg install -g -t tool-avrdude

echo "Build di $ENV..."
pio run -e "$ENV"
echo
echo "Firmware in .pio/firmware/ :"
ls -l .pio/firmware/
