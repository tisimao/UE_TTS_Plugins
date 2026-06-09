@echo off
setlocal

set "PROJECT_DIR=%~dp0UE56_TTSHost"
set "UPROJECT=%PROJECT_DIR%\UE56_TTSHost.uproject"
set "UE_BUILD=C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\Build.bat"

rem UE 5.6 ships C# helper projects that reference Magick.NET 14.7.0.
rem Newer NuGet audits can turn that engine-side warning into a build error.
set "NuGetAudit=false"
set "NuGetAuditLevel=critical"
set "WarningsNotAsErrors=NU1901;NU1902;NU1903;NU1904"

if not exist "%UE_BUILD%" (
  echo UE 5.6 Build.bat was not found:
  echo %UE_BUILD%
  echo.
  echo Please edit Build_UE56_TTSHost.bat and set UE_BUILD to your UE 5.6 install path.
  pause
  exit /b 1
)

if not exist "%UPROJECT%" (
  echo Project file was not found:
  echo %UPROJECT%
  pause
  exit /b 1
)

echo Building UE56_TTSHostEditor...
call "%UE_BUILD%" UE56_TTSHostEditor Win64 Development -Project="%UPROJECT%" -WaitMutex -NoHotReloadFromIDE

if errorlevel 1 (
  echo.
  echo Build failed. Please send the first real error line above to Codex.
  pause
  exit /b %errorlevel%
)

echo.
echo Build succeeded.
pause
