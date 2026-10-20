# Divulgazione: cosa e' stato pubblicato

Tutti i link sono stati creati con l'account GitHub **Gurzo82**.

## Pubblicato

| Canale | Link | Contenuto |
| --- | --- | --- |
| Issue - port Marlin di riferimento | https://github.com/Guizz27/Marlin---Longer-LKx-Pro/issues/31 | Adattamento alla LK4 Pro stock (senza BLTouch): BLTouch disabilitato, ramo "senza sonda" della UI DGUS, parametri di fabbrica, peculiarita' del display, avrdude non sincronizza |
| Discussione - repo dello schermo | https://github.com/Guizz27/DWIN_LK4Pro/discussions/27 | Display fermo sul logo dopo il flash (DTR) + ramo senza sonda della UI DGUS |
| Pull Request - upstream | https://github.com/tx3m/Marlin-LK4Pro/pull/1 | Guard `#if HAS_LEVELING` in `meshUpdate()` (no-op verificato: `.text` 150476 byte e `meshUpdate` a `0x16aea`/size `0x72` identici) + note in README per LK4 Pro stock e troubleshooting display |
| Issue - tutorial di riferimento | https://github.com/Baddflash/LK4-Pro-Firmware-Tutorial/issues/1 | `avrdude` non sincronizza + rimedio "stacca/riattacca il cavo USB" + link alla repo |
| Issue - firmware Marlin 2.x ufficiale Longer | https://github.com/LONGER3D/Marlin2.0-lgt/issues/21 | Firmware stock gia' compilato + gli stessi due problemi frequenti |

## Da pubblicare (serve login dell'utente)

| Canale | Testo | Note |
| --- | --- | --- |
| Reddit r/Longer3d | `3-reddit.md` | Il browser non ha una sessione Reddit attiva: occorre fare "Accedi" e poi si puo' pubblicare |
| Gruppo Facebook "Longer 3D LK5 Pro Mods and Tweaks" | `4-facebook.md` | https://www.facebook.com/groups/718449532977111/ - idem, serve login |

## Rettifiche fatte dopo la pubblicazione

Nella prima stesura dell'issue #31 era indicato che `settings_ready` non venisse impostato al boot
con EEPROM valido. **Non e' vero**: `configurationStoreRead()` lo imposta gia' normalmente, quindi la
UI DGUS si avvia. Il sintomo reale (display fermo sul logo) e' quello della linea USB/DTR, e la
causa e' stata descritta come "osservazione" con causa **probabile** (non certa).

Stessa rettifica nel corpo della discussione #27, nel README e nel CHANGELOG di questo repository.
