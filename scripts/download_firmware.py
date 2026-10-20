#!/usr/bin/env python3
"""
download_firmware.py - Scarica l'ultimo firmware dalla Release di GitHub.

Individua l'asset più recente che inizia con il prefisso indicato (es. "LK4Pro-Stock-Plus-")
e lo scarica nel percorso di destinazione.

Uso:
    python download_firmware.py <repo> <prefisso_asset> [percorso_di_destinazione]
"""
import json
import os
import re
import sys
import urllib.request

REPO_DEFAULT = "Gurzo82/Marlin-LK4Pro-Stock"


def version_key(name):
    """Estrae il numero di versione dal nome (es. LK4Pro-Stock-V43.hex -> 43)."""
    m = re.search(r"-V(\d+)", name)
    return int(m.group(1)) if m else -1


def main():
    repo = sys.argv[1] if len(sys.argv) > 1 else REPO_DEFAULT
    prefix = sys.argv[2] if len(sys.argv) > 2 else ""
    dest = sys.argv[3] if len(sys.argv) > 3 else ""

    if not prefix:
        print("manca il prefisso dell'asset", file=sys.stderr)
        return 1

    url = f"https://api.github.com/repos/{repo}/releases/latest"
    req = urllib.request.Request(url, headers={"User-Agent": "lk4pro-updater"})
    with urllib.request.urlopen(req, timeout=30) as r:
        data = json.load(r)

    found = [a for a in data.get("assets", []) if a["name"].startswith(prefix) and a["name"].endswith(".hex")]
    if not found:
        print(f"nessun asset che inizia con '{prefix}' nella release {data.get('tag_name')}", file=sys.stderr)
        return 1

    # il piu' recente
    asset = max(found, key=lambda a: (version_key(a["name"]), a["name"]))
    link = asset["browser_download_url"]
    print(f"asset: {asset['name']}")
    print(link)

    if dest:
        os.makedirs(os.path.dirname(os.path.abspath(dest)), exist_ok=True)
        urllib.request.urlretrieve(link, dest)
        print(f"scaricato in {dest}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
