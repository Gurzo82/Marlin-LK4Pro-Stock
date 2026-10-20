@echo off
REM ===========================================================================
REM  Aggiornamento firmware Longer LK4 Pro (Marlin) - Windows
REM
REM  Scarica l'ultimo firmware gia' compilato dalla Release di GitHub e lo
REM  carica sulla stampante. Non servono PlatformIO ne' Arduino IDE.
REM
REM  Uso:  update.bat            (menu' con le due varianti)
REM        update.bat stock     (solo la variante "stock")
REM        update.bat plus      (solo la variante "plus")
REM ===========================================================================
setlocal enabledelayedexpansion
chcp 65001 >nul

set "REPO=Gurzo82/Marlin-LK4Pro-Stock"
set "SCRIPT_DIR=%~dp0"
set "ROOT=%SCRIPT_DIR%.."
set "VARIANT=%~1"
set "ASSET="

if not exist "%SCRIPT_DIR%flash-lk4pro.py" (
  echo ERRORE: flash-lk4pro.py non trovato in "%SCRIPT_DIR%"
  pause
  exit /b 1
)

echo.
echo ==========================================================
echo   Longer LK4 Pro - aggiornamento firmware
echo ==========================================================
echo.

REM ---- scelta della variante -------------------------------------------------
if "%VARIANT%"=="" (
  echo   [1] Stock  - correzioni essenziali per la LK4 Pro di serie
  echo                 BLTouch disabilitato, driver E A4988, parametri factory
  echo   [2] Plus   - Stock + Z-offset dal display + log diagnostico su SD
  echo.
  set /p "SCEGLI=  Variante [1]: "
) else (
  set "SCEGLI=%VARIANT%"
)

if /i "%SCEGLI%"=="1" set "SCEGLI=stock"
if /i "%SCEGLI%"=="2" set "SCEGLI=plus"
if /i "%SCEGLI%"=="stock" set "ASSET=LK4Pro-Stock-V41.hex"
if /i "%SCEGLI%"=="plus" set "ASSET=LK4Pro-Stock-Plus-V41.hex"

if "%ASSET%"=="" (
  echo.
  echo SCELTA NON VALIDA. Usa: update.bat stock  oppure  update.bat plus
  pause
  exit /b 1
)
echo.
echo   Variante: %SCEGLI%  (%ASSET%)

REM ---- python ---------------------------------------------------------------
set "PY=python"
where python >nul 2>&1 || (
  echo.
  echo ERRORE: Python 3 non trovato. Installalo da https://www.python.org/downloads/
  pause
  exit /b 1
)

REM ---- pyserial -------------------------------------------------------------
%PY% -c "import serial" >nul 2>&1
if errorlevel 1 (
  echo   Installo pyserial...
  %PY% -m pip install --user pyserial
)

REM ---- firmware: locale o download dalla Release ------------------------------
set "HEX=%ROOT%\firmware\%ASSET%"
if exist "%HEX%" (
  echo   Trovato gia' in locale: %ASSET%
) else (
  echo   Scarico %ASSET% dalla Release di GitHub...
  %PY% "%SCRIPT_DIR%download_firmware.py" %REPO% %ASSET% "%HEX%"
  if errorlevel 1 (
    echo.
    echo ERRORE: download non riuscito. Scarica il .hex a mano dalle Release e usa:
    echo         python flash-lk4pro.py percorso\del\file.hex
    pause
    exit /b 1
  )
)

REM ---- flash -----------------------------------------------------------------
echo.
%PY% "%SCRIPT_DIR%flash-lk4pro.py" "%HEX%"
set "RC=%ERRORLEVEL%"

echo.
if "%RC%"=="0" (
  echo ==========================================================
  echo   AGGIORNAMENTO COMPLETATO
  echo ==========================================================
  echo   Ora, sul display:
  echo     1. Home - oming
  echo     2. Settings -^> EEPROM -^> Factory Reset
  echo     3. Se usi OctoPrint: baudrate 250000
) else (
  echo AGGIORNAMENTO NON COMPLETATO ^(codice %RC%^)
  echo - La stampante era accesa e il cavo USB collegato?
  echo - Riprova: python flash-lk4pro.py "%HEX%"
)
echo.
pause
exit /b %RC%
