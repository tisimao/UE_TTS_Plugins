@echo off
setlocal EnableExtensions

set "PROJECT_ROOT=%~dp0"
set "UPROJECT=%PROJECT_ROOT%UE56_TTSHost\UE56_TTSHost.uproject"
set "SERVICE_SRC=%PROJECT_ROOT%Services\tts_service"
set "ARCHIVE_DIR=%PROJECT_ROOT%Dist\UE56_TTSHost_Win64"
set "WINDOWS_DIR=%ARCHIVE_DIR%\Windows"
set "SERVICE_DST=%WINDOWS_DIR%\Services\tts_service"
set "UE_ROOT=C:\Program Files\Epic Games\UE_5.6"
set "RUN_UAT=%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat"
set "CONFIG=Shipping"

rem UE 5.6 ships C# helper projects that can trip NuGet audit warnings.
set "NuGetAudit=false"
set "NuGetAuditLevel=critical"
set "WarningsNotAsErrors=NU1901;NU1902;NU1903;NU1904"

if not exist "%RUN_UAT%" (
  echo RunUAT.bat was not found:
  echo %RUN_UAT%
  echo.
  echo Edit UE_ROOT in Package_UE56_TTSHost_Windows.bat if UE 5.6 is installed elsewhere.
  pause
  exit /b 1
)

if not exist "%UPROJECT%" (
  echo Project file was not found:
  echo %UPROJECT%
  pause
  exit /b 1
)

if not exist "%SERVICE_SRC%\run_server.py" (
  echo LocalTTS service was not found:
  echo %SERVICE_SRC%
  pause
  exit /b 1
)

echo Packaging UE56_TTSHost for Win64 %CONFIG%...
call "%RUN_UAT%" BuildCookRun ^
  -project="%UPROJECT%" ^
  -noP4 ^
  -platform=Win64 ^
  -clientconfig=%CONFIG% ^
  -build ^
  -cook ^
  -stage ^
  -pak ^
  -archive ^
  -archivedirectory="%ARCHIVE_DIR%"

if errorlevel 1 (
  echo.
  echo UE packaging failed. Check the first real UAT error above.
  pause
  exit /b %errorlevel%
)

echo.
echo Copying LocalTTS service runtime...
if not exist "%WINDOWS_DIR%" (
  echo Packaged Windows directory was not found:
  echo %WINDOWS_DIR%
  pause
  exit /b 1
)

robocopy "%SERVICE_SRC%" "%SERVICE_DST%" /MIR /XD cache logs __pycache__ .pytest_cache /XF *.pyc
if errorlevel 8 (
  echo.
  echo Failed to copy LocalTTS service runtime.
  pause
  exit /b 1
)

set "HF_OMNIVOICE_SRC=%USERPROFILE%\.cache\huggingface\hub\models--k2-fsa--OmniVoice"
set "HF_OMNIVOICE_DST=%SERVICE_DST%\models\huggingface\models--k2-fsa--OmniVoice"

if exist "%HF_OMNIVOICE_SRC%" (
  echo.
  echo Copying bundled OmniVoice model cache...
  robocopy "%HF_OMNIVOICE_SRC%" "%HF_OMNIVOICE_DST%" /MIR
  if errorlevel 8 (
    echo.
    echo Failed to copy OmniVoice model cache.
    pause
    exit /b 1
  )
) else (
  echo.
  echo Warning: OmniVoice cache was not found:
  echo %HF_OMNIVOICE_SRC%
  echo The packaged app may download the model on first run or fail offline.
)

echo.
echo Package is ready:
echo %WINDOWS_DIR%
echo.
echo Zip the entire Windows folder for delivery.
pause
