@echo off
setlocal

set "SERVICE_DIR=%~dp0Services\tts_service"
set "PYTHON_EXE=python"
set "VENV_PYTHON=%SERVICE_DIR%\.venv\Scripts\python.exe"
set "REQUIREMENTS=%SERVICE_DIR%\requirements.txt"

if not exist "%SERVICE_DIR%" (
  echo LocalTTS service directory was not found:
  echo %SERVICE_DIR%
  pause
  exit /b 1
)

if not exist "%REQUIREMENTS%" (
  echo requirements.txt was not found:
  echo %REQUIREMENTS%
  pause
  exit /b 1
)

if not exist "%VENV_PYTHON%" (
  echo Creating Python virtual environment...
  "%PYTHON_EXE%" -m venv "%SERVICE_DIR%\.venv"
  if errorlevel 1 (
    echo Failed to create virtual environment. Please confirm Python is installed and available as "python".
    pause
    exit /b %errorlevel%
  )
)

echo Upgrading pip...
"%VENV_PYTHON%" -m pip install --upgrade pip
if errorlevel 1 (
  echo Failed to upgrade pip.
  pause
  exit /b %errorlevel%
)

echo Installing LocalTTS service dependencies...
"%VENV_PYTHON%" -m pip install -r "%REQUIREMENTS%"
if errorlevel 1 (
  echo Failed to install LocalTTS service dependencies.
  pause
  exit /b %errorlevel%
)

echo.
echo LocalTTS service environment is ready:
echo %VENV_PYTHON%
pause
