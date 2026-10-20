#!/usr/bin/env python3
"""
download_firmware.py - Scarica l'ultimo firmware dalla Release di GitHub.

Uso:
    python download_firmware.py <repo> <nome_asset> [percorso_di_destinazione]

Stampa sempre l'URL dell'asset su stdout (utile per il debug).
"""
import json
import os
import sys
import urllib.request

REPO_DEFAULT = "Gurzo82/Marlin-LK4Pro-Stock"


def main():
    repo = sys.argv[1] if len(sys.argv) > 1 else REPO_DEFAULT
    asset = sys.argv[2] if len(sys.argv) > 2 else ""
    dest = sys.argv[3] if len(sys.argv) > 3 else ""

    if not asset:
        print("manca il nome dell'asset", file=sys.stderr)
        return 1

    url = f"https://api.github.com/repos/{repo}/releases/latest"
    req = urllib.request.Request(url, headers={"User-Agent": "lk4pro-updater"})
    with urllib.request.urlopen(req, timeout=30) as r:
        data = json.load(r)

    found = [a for a in data.get("assets", []) if a["name"].endswith(asset)]
    if not found:
        print(f"asset non trovato nella release: {asset}", file=sys.stderr)
        return 1

    link = found[0]["browser_download_url"]
    print(link)

    if dest:
        os.makedirs(os.path.dirname(os.path.abspath(dest)), exist_ok=True)
        urllib.request.urlretrieve(link, dest)
        print(f"scaricato in {dest}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
