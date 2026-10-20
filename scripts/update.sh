#!/usr/bin/env bash
# ===========================================================================
#  Aggiornamento firmware Longer LK4 Pro (Marlin) - Linux / macOS
#
#  Scarica l'ultimo firmware gia' compilato dalla Release di GitHub e lo carica
#  sulla stampante. Non servono PlatformIO ne' Arduino IDE.
#
#  Uso:  ./update.sh            (menu' con le due varianti)
#        ./update.sh stock
#        ./update.sh plus
# ===========================================================================
set -euo pipefail

REPO="Gurzo82/Marlin-LK4Pro-Stock"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(dirname "$SCRIPT_DIR")"

VARIANT="${1:-}"
ASSET=""

echo
echo "=========================================================="
echo "  Longer LK4 Pro - aggiornamento firmware"
echo "=========================================================="
echo

# ---- scelta della variante ---------------------------------------------------
if [ -z "$VARIANT" ]; then
  echo "  [1] Stock  - correzioni essenziali per la LK4 Pro di serie"
  echo "                BLTouch disabilitato, driver E A4988, parametri factory"
  echo "  [2] Plus   - Stock + Z-offset dal display + log diagnostico su SD"
  echo
  read -r -p "  Variante [1]: " SCELTA
else
  SCELTA="$VARIANT"
fi

case "$SCELTA" in
  1|stock) SCELTA=stock; ASSET="LK4Pro-Stock-V41.hex" ;;
  2|plus)  SCELTA=plus;  ASSET="LK4Pro-Stock-Plus-V41.hex" ;;
  *) echo "SCELTA NON VALIDA. Usa: $0 stock  oppure  $0 plus"; exit 1 ;;
esac
echo
echo "  Variante: $SCELTA  ($ASSET)"

# ---- python -----------------------------------------------------------------
PY=""
command -v python3 >/dev/null 2>&1 && PY="python3"
command -v python  >/dev/null 2>&1 && [ -z "$PY" ] && PY="python"
if [ -z "$PY" ]; then
  echo "ERRORE: Python 3 non trovato (python3)."
  exit 1
fi

if ! $PY -c "import serial" >/dev/null 2>&1; then
  echo "  Installo pyserial..."
  $PY -m pip install --user pyserial
fi

# ---- firmware: locale o download --------------------------------------------
HEX="$ROOT/firmware/$ASSET"
if [ ! -f "$HEX" ]; then
  echo "  Scarico $ASSET dalla Release di GitHub..."
  URL=$($PY - "$REPO" "$ASSET" <<'PYEOF'
import json, sys, urllib.request
repo, asset = sys.argv[1], sys.argv[2]
req = urllib.request.Request("https://api.github.com/repos/%s/releases/latest" % repo,
                            headers={"User-Agent": "lk4pro-updater"})
d = json.load(urllib.request.urlopen(req))
m = [a for a in d["assets"] if a["name"].endswith(asset)]
if not m:
    sys.exit("asset non trovato")
print(m[0]["browser_download_url"])
PYEOF
)
  $PY -c "import urllib.request,sys; urllib.request.urlretrieve('$URL','$HEX')"
else
  echo "  Trovato in locale: $ASSET"
fi

# ---- flash ------------------------------------------------------------------
echo
$PY "$SCRIPT_DIR/flash-lk4pro.py" "$HEX"
RC=$?

echo
if [ "$RC" -eq 0 ]; then
  echo "=========================================================="
  echo "  AGGIORNAMENTO COMPLETATO"
  echo "=========================================================="
  echo "  Ora, sul display:"
  echo "    1. Home (homing)"
  echo "    2. Settings -> EEPROM -> Factory Reset"
  echo "    3. Se usi OctoPrint: baudrate 250000"
fi
exit $RC
